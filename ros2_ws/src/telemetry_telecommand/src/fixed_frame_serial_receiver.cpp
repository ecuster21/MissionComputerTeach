#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace telemetry_telecommand
{
namespace
{

void consume_bytes(
  std::vector<uint8_t> & buffer,
  std::size_t & bytes_in_buffer,
  std::size_t count)
{
  if (count >= bytes_in_buffer) {
    bytes_in_buffer = 0;
    return;
  }

  std::memmove(buffer.data(), buffer.data() + count, bytes_in_buffer - count);
  bytes_in_buffer -= count;
}

bool try_find_sync(
  const std::vector<uint8_t> & buffer,
  std::size_t bytes_in_buffer,
  std::size_t frame_length,
  std::size_t & offset)
{
  if (bytes_in_buffer < frame_length + FRAME_HEADER.size()) {
    offset = 0;
    return false;
  }

  for (std::size_t i = 0; i + frame_length + 1 < bytes_in_buffer; ++i) {
    if (
      buffer[i] == FRAME_HEADER[0] &&
      buffer[i + 1] == FRAME_HEADER[1] &&
      buffer[i + frame_length] == FRAME_HEADER[0] &&
      buffer[i + frame_length + 1] == FRAME_HEADER[1]
    ) {
      offset = i;
      return true;
    }
  }

  offset = 0;
  return false;
}

bool read_exact(
  PosixSerialPort & serial_port,
  uint8_t * buffer,
  std::size_t size,
  const std::atomic<bool> & stop_requested)
{
  std::size_t total_read = 0;
  while (total_read < size && rclcpp::ok() && !stop_requested.load()) {
    const auto bytes_read = serial_port.read(buffer + total_read, size - total_read);
    if (bytes_read == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    total_read += bytes_read;
  }

  return total_read == size;
}

std::string normalize_crc8_variant_name(std::string variant_name)
{
  std::transform(
    variant_name.begin(),
    variant_name.end(),
    variant_name.begin(),
    [](unsigned char ch) {
      if (ch == '-') {
        return '_';
      }
      return static_cast<char>(std::tolower(ch));
    });
  return variant_name;
}

const Crc8Config * try_get_crc8_config(const std::string & variant_name)
{
  const auto normalized_name = normalize_crc8_variant_name(variant_name);
  if (normalized_name == "crc8" || normalized_name == "standard") {
    return &CRC8_STANDARD;
  }
  if (normalized_name == "maxim" || normalized_name == "dallas") {
    return &CRC8_MAXIM;
  }
  if (normalized_name == "sae_j1850" || normalized_name == "j1850") {
    return &CRC8_SAE_J1850;
  }
  return nullptr;
}

std::string format_frame_hex(const std::vector<uint8_t> & frame)
{
  std::ostringstream stream;
  stream << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < frame.size(); ++i) {
    if (i != 0) {
      stream << ' ';
    }
    stream << std::setw(2) << static_cast<int>(frame[i]);
  }
  return stream.str();
}

void log_validation_failure_details(
  const rclcpp::Logger & logger,
  const std::vector<uint8_t> & frame,
  std::size_t frame_length,
  const std::string & configured_variant)
{
  const auto received_crc = encoded_crc8(frame.data(), frame_length);
  const auto crc_standard = calculate_crc8(frame.data(), frame_length, CRC8_STANDARD);
  const auto crc_maxim = calculate_crc8(frame.data(), frame_length, CRC8_MAXIM);
  const auto crc_j1850 = calculate_crc8(frame.data(), frame_length, CRC8_SAE_J1850);

  RCLCPP_WARN(
    logger,
    "Frame validation failed. configured=%s frame_length=%zu recv_crc8=0x%02X "
    "calc_crc8=0x%02X calc_maxim=0x%02X calc_sae_j1850=0x%02X frame=[%s]",
    configured_variant.c_str(),
    frame_length,
    received_crc,
    crc_standard,
    crc_maxim,
    crc_j1850,
    format_frame_hex(frame).c_str());
}

}  // 匿名命名空间

FixedFrameSerialReceiver::FixedFrameSerialReceiver(
  const std::string & node_name,
  const std::string & default_port,
  std::size_t default_frame_length,
  const std::string & default_topic)
: Node(node_name)
{
  port_name_ = this->declare_parameter<std::string>("port", default_port);
  baud_rate_ = static_cast<uint32_t>(this->declare_parameter<int>("baud_rate", 115200));
  timeout_ms_ = static_cast<uint32_t>(this->declare_parameter<int>("timeout_ms", 100));
  frame_length_ = static_cast<std::size_t>(
    this->declare_parameter<int>("frame_length", static_cast<int>(default_frame_length)));
  const auto requested_crc8_variant =
    this->declare_parameter<std::string>("crc8_variant", "crc8");
  const auto topic_name = this->declare_parameter<std::string>("topic", default_topic);

  if (frame_length_ <= FRAME_HEADER.size() + FRAME_CRC8_LENGTH) {
    throw std::invalid_argument("frame_length must be greater than header + CRC8 length");
  }
  search_buffer_size_ = search_buffer_size_for(frame_length_);

  const auto * crc8_config = try_get_crc8_config(requested_crc8_variant);
  if (crc8_config == nullptr) {
    RCLCPP_WARN(
      this->get_logger(),
      "Unsupported crc8_variant '%s', defaulting to crc8.",
      requested_crc8_variant.c_str());
    crc8_variant_name_ = "crc8";
    crc8_config_ = CRC8_STANDARD;
  } else {
    crc8_variant_name_ = normalize_crc8_variant_name(requested_crc8_variant);
    crc8_config_ = *crc8_config;
  }

  frame_publisher_ = this->create_publisher<interfaces::msg::SyncedFrame>(topic_name, 10);

  RCLCPP_INFO(
    this->get_logger(),
    "Expecting frames on %s: frame_length=%zu payload=%zu CRC8(%s).",
    port_name_.c_str(),
    frame_length_,
    payload_length_for(frame_length_),
    crc8_variant_name_.c_str());

  if (!initialize_serial()) {
    RCLCPP_WARN(
      this->get_logger(),
      "Serial port %s is not available yet, the receiver thread will retry.",
      port_name_.c_str());
  }

  receive_thread_ = std::thread(&FixedFrameSerialReceiver::receive_data, this);
}

FixedFrameSerialReceiver::~FixedFrameSerialReceiver()
{
  stop_requested_.store(true);
  close_serial();
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

bool FixedFrameSerialReceiver::initialize_serial()
{
  try {
    serial_port_.set_port(port_name_);
    serial_port_.set_baud_rate(baud_rate_);
    serial_port_.set_timeout_ms(timeout_ms_);
    if (!serial_port_.is_open()) {
      serial_port_.open();
    }

    RCLCPP_INFO(
      this->get_logger(),
      "Connected to serial port %s at %u baud.",
      port_name_.c_str(),
      baud_rate_);
    return true;
  } catch (const std::exception & ex) {
    RCLCPP_WARN(
      this->get_logger(),
      "Failed to open serial port %s: %s",
      port_name_.c_str(),
      ex.what());
    close_serial();
    return false;
  }
}

void FixedFrameSerialReceiver::close_serial()
{
  try {
    if (serial_port_.is_open()) {
      serial_port_.close();
    }
  } catch (const std::exception & ex) {
    RCLCPP_WARN(
      this->get_logger(),
      "Error while closing serial port %s: %s",
      port_name_.c_str(),
      ex.what());
  }
}

void FixedFrameSerialReceiver::receive_data()
{
  std::vector<uint8_t> buffer(search_buffer_size_, 0U);
  std::size_t bytes_in_buffer = 0;
  bool is_synced = false;

  while (rclcpp::ok() && !stop_requested_.load()) {
    if (!serial_port_.is_open()) {
      if (!initialize_serial()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }

      bytes_in_buffer = 0;
      is_synced = false;
    }

    try {
      if (!is_synced) {
        if (bytes_in_buffer == search_buffer_size_) {
          consume_bytes(buffer, bytes_in_buffer, frame_length_);
        }

        const auto bytes_read =
          serial_port_.read(buffer.data() + bytes_in_buffer, search_buffer_size_ - bytes_in_buffer);
        if (bytes_read == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        bytes_in_buffer += bytes_read;

        std::size_t offset = 0;
        if (!try_find_sync(buffer, bytes_in_buffer, frame_length_, offset)) {
          continue;
        }

        consume_bytes(buffer, bytes_in_buffer, offset);
        is_synced = true;
        RCLCPP_INFO(this->get_logger(), "Frame synchronization established.");
      }

      while (is_synced && rclcpp::ok() && !stop_requested_.load()) {
        if (bytes_in_buffer == 0) {
          std::vector<uint8_t> frame(frame_length_, 0U);
          if (!read_exact(serial_port_, frame.data(), frame_length_, stop_requested_)) {
            return;
          }

          if (
            has_valid_header(frame.data(), frame_length_) &&
            validate_crc8(frame.data(), frame_length_, crc8_config_))
          {
            publish_frame(frame);
            continue;
          }

          log_validation_failure_details(
            this->get_logger(),
            frame,
            frame_length_,
            crc8_variant_name_);
          std::copy(frame.begin(), frame.end(), buffer.begin());
          bytes_in_buffer = frame_length_;
          is_synced = false;
          break;
        }

        if (bytes_in_buffer < frame_length_) {
          const auto bytes_read =
            serial_port_.read(buffer.data() + bytes_in_buffer, frame_length_ - bytes_in_buffer);
          if (bytes_read == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            break;
          }
          bytes_in_buffer += bytes_read;
          if (bytes_in_buffer < frame_length_) {
            break;
          }
        }

        std::vector<uint8_t> frame(frame_length_, 0U);
        std::copy_n(buffer.data(), frame_length_, frame.begin());

        if (
          has_valid_header(frame.data(), frame_length_) &&
          validate_crc8(frame.data(), frame_length_, crc8_config_))
        {
          publish_frame(frame);
          consume_bytes(buffer, bytes_in_buffer, frame_length_);
          continue;
        }

        log_validation_failure_details(
          this->get_logger(),
          frame,
          frame_length_,
          crc8_variant_name_);
        consume_bytes(buffer, bytes_in_buffer, 1);

        std::size_t offset = 0;
        if (try_find_sync(buffer, bytes_in_buffer, frame_length_, offset)) {
          consume_bytes(buffer, bytes_in_buffer, offset);
          continue;
        }

        is_synced = false;
        break;
      }
    } catch (const std::exception & ex) {
      RCLCPP_WARN(
        this->get_logger(),
        "Serial receiver encountered an error: %s",
        ex.what());
      close_serial();
      bytes_in_buffer = 0;
      is_synced = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }
}

void FixedFrameSerialReceiver::publish_frame(const std::vector<uint8_t> & frame)
{
  interfaces::msg::SyncedFrame msg;
  msg.frame_data = frame;
  msg.timestamp_ns = this->get_clock()->now().nanoseconds();
  frame_publisher_->publish(msg);
}

}  // telemetry_telecommand 命名空间
