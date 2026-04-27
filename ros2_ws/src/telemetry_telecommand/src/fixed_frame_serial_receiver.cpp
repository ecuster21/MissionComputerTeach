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
#include <string>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>

namespace telemetry_telecommand
{
namespace
{

void consume_bytes(
  std::vector<uint8_t> & buffer,
  std::size_t & bytes_in_buffer,
  std::size_t count)
{
  // 保留未消费的尾部字节，便于下一轮继续拼接串口流。
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
  // 同时确认当前位置和下一帧位置都是 EB90，降低数据区误含 EB90 时的误同步概率。
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
  // 串口 read 可能按任意分片返回，这里持续补读直到凑满一帧。
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
  // 参数允许用户写 crc-8 / crc8 / CRC8 等近似形式，内部统一成小写下划线。
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

std::string trim_copy(const std::string & text)
{
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }

  const auto last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

bool starts_with_hex_prefix(const std::string & text)
{
  return text.size() > 2 &&
         text[0] == '0' &&
         (text[1] == 'x' || text[1] == 'X');
}

bool contains_hex_letter(const std::string & text)
{
  return std::any_of(
    text.begin(),
    text.end(),
    [](unsigned char ch) {
      return (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
    });
}

uint8_t parse_uint8_token(
  const std::string & token,
  int default_base,
  const std::string & parameter_name)
{
  auto number_text = trim_copy(token);
  if (number_text.empty()) {
    throw std::invalid_argument("Empty item in parameter " + parameter_name);
  }

  int base = default_base;
  if (starts_with_hex_prefix(number_text)) {
    base = 16;
    number_text = number_text.substr(2);
  } else if (contains_hex_letter(number_text)) {
    base = 16;
  }

  std::size_t parsed_chars = 0;
  unsigned long value = 0;
  try {
    value = std::stoul(number_text, &parsed_chars, base);
  } catch (const std::exception &) {
    throw std::invalid_argument(
      "Invalid byte value '" + token + "' in parameter " + parameter_name);
  }

  if (parsed_chars != number_text.size() || value > 0xFFUL) {
    throw std::invalid_argument(
      "Invalid byte value '" + token + "' in parameter " + parameter_name);
  }

  return static_cast<uint8_t>(value);
}

std::vector<uint8_t> parse_uint8_list(
  const std::string & parameter_text,
  int default_base,
  const std::string & parameter_name)
{
  // 字符串参数兼容 "1,2,0x24"、"[1, 2, 36]" 和 "0C 0D 10 11" 等写法。
  std::string normalized = parameter_text;
  for (auto & ch : normalized) {
    if (ch == ',' || ch == ';' || ch == '[' || ch == ']' || ch == '\'' || ch == '"') {
      ch = ' ';
    }
  }

  std::vector<uint8_t> values;
  std::istringstream stream(normalized);
  std::string token;
  while (stream >> token) {
    const auto value = parse_uint8_token(token, default_base, parameter_name);
    if (std::find(values.begin(), values.end(), value) == values.end()) {
      values.push_back(value);
    }
  }

  return values;
}

uint8_t checked_int64_to_uint8(int64_t value, const std::string & parameter_name)
{
  if (value < 0 || value > 0xFF) {
    throw std::invalid_argument(
      "Invalid byte value '" + std::to_string(value) + "' in parameter " + parameter_name);
  }

  return static_cast<uint8_t>(value);
}

std::vector<uint8_t> parse_uint8_list_parameter(
  const rclcpp::Parameter & parameter,
  int default_base,
  const std::string & parameter_name)
{
  // launch/ros2 param 可能把同一个配置解析成字符串、整数或整数数组，这里统一收敛为字节列表。
  std::vector<uint8_t> values;

  switch (parameter.get_type()) {
    case rclcpp::ParameterType::PARAMETER_NOT_SET:
      return values;
    case rclcpp::ParameterType::PARAMETER_STRING:
      return parse_uint8_list(parameter.as_string(), default_base, parameter_name);
    case rclcpp::ParameterType::PARAMETER_INTEGER:
      values.push_back(checked_int64_to_uint8(parameter.as_int(), parameter_name));
      return values;
    case rclcpp::ParameterType::PARAMETER_INTEGER_ARRAY:
      for (const auto value : parameter.as_integer_array()) {
        const auto byte_value = checked_int64_to_uint8(value, parameter_name);
        if (std::find(values.begin(), values.end(), byte_value) == values.end()) {
          values.push_back(byte_value);
        }
      }
      return values;
    default:
      throw std::invalid_argument(
        parameter_name + " must be a string, integer, or integer array");
  }
}

std::string format_byte_list(const std::vector<uint8_t> & values)
{
  if (values.empty()) {
    return "any";
  }

  std::ostringstream stream;
  stream << std::uppercase << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      stream << ',';
    }
    stream << "0x" << std::setw(2) << static_cast<int>(values[i]);
  }
  return stream.str();
}

bool contains_byte(const std::vector<uint8_t> & values, uint8_t value)
{
  return std::find(values.begin(), values.end(), value) != values.end();
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
  // 同时打印几种常用 CRC8 计算结果，便于现场快速判断是不是 CRC 变体配错。
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
  rcl_interfaces::msg::ParameterDescriptor flexible_byte_list_descriptor;
  flexible_byte_list_descriptor.dynamic_typing = true;
  flexible_byte_list_descriptor.description =
    "Byte list as string, integer, or integer array.";
  this->declare_parameter(
    "destination_ids",
    rclcpp::ParameterValue(""),
    flexible_byte_list_descriptor);
  this->declare_parameter(
    "handled_frame_types",
    rclcpp::ParameterValue("0C,0D,10,11"),
    flexible_byte_list_descriptor);
  const auto topic_name = this->declare_parameter<std::string>("topic", default_topic);

  // 当前协议要求 CRC 前至少有协议时间戳，避免配置出过短帧导致越界解析。
  if (frame_length_ < minimum_frame_length()) {
    throw std::invalid_argument(
      "frame_length must contain header + frame_type + source_id + destination_id + "
      "protocol_timestamp + CRC8");
  }
  search_buffer_size_ = search_buffer_size_for(frame_length_);
  destination_ids_ = parse_uint8_list_parameter(
    this->get_parameter("destination_ids"),
    10,
    "destination_ids");
  handled_frame_types_ = parse_uint8_list_parameter(
    this->get_parameter("handled_frame_types"),
    16,
    "handled_frame_types");

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
  parameter_callback_handle_ = this->add_on_set_parameters_callback(
    [this](const std::vector<rclcpp::Parameter> & parameters) {
      return handle_parameter_update(parameters);
    });

  RCLCPP_INFO(
    this->get_logger(),
    "Expecting frames on %s: frame_length=%zu payload=%zu data=%zu CRC8(%s).",
    port_name_.c_str(),
    frame_length_,
    payload_length_for(frame_length_),
    data_area_length_for(frame_length_),
    crc8_variant_name_.c_str());
  RCLCPP_INFO(
    this->get_logger(),
    "Frame filter: destination_ids=%s handled_frame_types=%s.",
    format_byte_list(destination_ids_).c_str(),
    format_byte_list(handled_frame_types_).c_str());

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
      // 支持串口设备热插拔或启动顺序晚于节点启动的情况。
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
          // 搜索窗口已满仍未同步时，丢弃最旧的一帧长度，继续等待新数据进入窗口。
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

        // 找到可信同步点后，移除同步点之前的噪声字节。
        consume_bytes(buffer, bytes_in_buffer, offset);
        is_synced = true;
        RCLCPP_INFO(this->get_logger(), "Frame synchronization established.");
      }

      while (is_synced && rclcpp::ok() && !stop_requested_.load()) {
        if (bytes_in_buffer == 0) {
          // 已同步且没有遗留缓冲时，直接按固定帧长从串口精确读取一帧。
          std::vector<uint8_t> frame(frame_length_, 0U);
          if (!read_exact(serial_port_, frame.data(), frame_length_, stop_requested_)) {
            return;
          }

          if (
            has_valid_header(frame.data(), frame_length_) &&
            validate_crc8(frame.data(), frame_length_, crc8_config_))
          {
            // 有效帧可能发给其他机号或属于暂不处理的类型，此时保持同步但不发布。
            if (should_process_frame(frame)) {
              publish_frame(frame);
            }
            continue;
          }

          log_validation_failure_details(
            this->get_logger(),
            frame,
            frame_length_,
            crc8_variant_name_);
          // 失败帧复制回搜索缓冲，下一轮从其中重新寻找同步点。
          std::copy(frame.begin(), frame.end(), buffer.begin());
          bytes_in_buffer = frame_length_;
          is_synced = false;
          break;
        }

        if (bytes_in_buffer < frame_length_) {
          // 缓冲区里已有部分同步数据时，继续补到至少一帧再切帧。
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
          // 从用户态缓冲区切出的有效帧同样要先经过业务过滤。
          if (should_process_frame(frame)) {
            publish_frame(frame);
          }
          consume_bytes(buffer, bytes_in_buffer, frame_length_);
          continue;
        }

        log_validation_failure_details(
          this->get_logger(),
          frame,
          frame_length_,
          crc8_variant_name_);
        // 缓冲模式下校验失败时只滑动 1 字节，避免错过紧随其后的真实帧头。
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

bool FixedFrameSerialReceiver::should_process_frame(const std::vector<uint8_t> & frame) const
{
  // 过滤依据来自协议头部固定位置，不解析数据区内容。
  const auto type = frame_type(frame.data());
  const auto source = source_id(frame.data());
  const auto destination = destination_id(frame.data());
  const auto timestamp = protocol_timestamp(frame.data(), frame.size());

  std::lock_guard<std::mutex> lock(filter_mutex_);
  const bool destination_matches =
    destination_ids_.empty() || contains_byte(destination_ids_, destination);
  const bool type_matches =
    handled_frame_types_.empty() || contains_byte(handled_frame_types_, type);

  if (!destination_matches || !type_matches) {
    RCLCPP_DEBUG(
      this->get_logger(),
      "Ignoring frame: type=0x%02X source_id=0x%02X destination_id=0x%02X "
      "timestamp=0x%02X destination_ids=%s handled_frame_types=%s.",
      type,
      source,
      destination,
      timestamp,
      format_byte_list(destination_ids_).c_str(),
      format_byte_list(handled_frame_types_).c_str());
    return false;
  }

  return true;
}

void FixedFrameSerialReceiver::publish_frame(const std::vector<uint8_t> & frame)
{
  interfaces::msg::SyncedFrame msg;
  msg.frame_data = frame;
  msg.timestamp_ns = this->get_clock()->now().nanoseconds();
  frame_publisher_->publish(msg);
}

rcl_interfaces::msg::SetParametersResult FixedFrameSerialReceiver::handle_parameter_update(
  const std::vector<rclcpp::Parameter> & parameters)
{
  // 先在临时变量里完成解析，只有全部参数合法时才一次性替换当前过滤表。
  std::vector<uint8_t> next_destination_ids;
  std::vector<uint8_t> next_handled_frame_types;

  {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    next_destination_ids = destination_ids_;
    next_handled_frame_types = handled_frame_types_;
  }

  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  bool filter_parameter_changed = false;

  try {
    for (const auto & parameter : parameters) {
      if (parameter.get_name() == "destination_ids") {
        filter_parameter_changed = true;
        next_destination_ids =
          parse_uint8_list_parameter(parameter, 10, "destination_ids");
      } else if (parameter.get_name() == "handled_frame_types") {
        filter_parameter_changed = true;
        next_handled_frame_types =
          parse_uint8_list_parameter(parameter, 16, "handled_frame_types");
      }
    }
  } catch (const std::exception & ex) {
    result.successful = false;
    result.reason = ex.what();
    return result;
  }

  if (!filter_parameter_changed) {
    return result;
  }

  {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    destination_ids_ = next_destination_ids;
    handled_frame_types_ = next_handled_frame_types;
  }

  RCLCPP_INFO(
    this->get_logger(),
    "Updated frame filter: destination_ids=%s handled_frame_types=%s.",
    format_byte_list(next_destination_ids).c_str(),
    format_byte_list(next_handled_frame_types).c_str());

  return result;
}

}  // telemetry_telecommand 命名空间
