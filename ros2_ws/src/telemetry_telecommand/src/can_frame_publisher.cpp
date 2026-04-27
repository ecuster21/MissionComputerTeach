#include "telemetry_telecommand/can_frame_publisher.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include <rcl_interfaces/msg/parameter_descriptor.hpp>

#include "telemetry_telecommand/frame_structures.hpp"

namespace telemetry_telecommand
{
namespace
{

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

uint16_t decode_can_id_big_endian(const uint8_t high_byte, const uint8_t low_byte)
{
  return static_cast<uint16_t>((static_cast<uint16_t>(high_byte) << 8U) | low_byte);
}

}  // namespace

CanFramePublisher::CanFramePublisher(
  const std::string & node_name,
  const std::string & default_input_topic,
  const std::string & default_output_topic)
: Node(node_name)
{
  const auto input_topic =
    this->declare_parameter<std::string>("input_topic", default_input_topic);
  const auto output_topic =
    this->declare_parameter<std::string>("output_topic", default_output_topic);

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

  destination_ids_ = parse_uint8_list_parameter(
    this->get_parameter("destination_ids"),
    10,
    "destination_ids");
  handled_frame_types_ = parse_uint8_list_parameter(
    this->get_parameter("handled_frame_types"),
    16,
    "handled_frame_types");

  can_publisher_ = this->create_publisher<interfaces::msg::CanFrame>(output_topic, 10);
  frame_subscriber_ = this->create_subscription<interfaces::msg::SyncedFrame>(
    input_topic,
    rclcpp::QoS(100),
    [this](const interfaces::msg::SyncedFrame::SharedPtr msg) {
      handle_serial_frame(msg);
    });

  parameter_callback_handle_ = this->add_on_set_parameters_callback(
    [this](const std::vector<rclcpp::Parameter> & parameters) {
      return handle_parameter_update(parameters);
    });

  RCLCPP_INFO(
    this->get_logger(),
    "CAN publisher started. input=%s output=%s destination_ids=%s handled_frame_types=%s",
    input_topic.c_str(),
    output_topic.c_str(),
    format_byte_list(destination_ids_).c_str(),
    format_byte_list(handled_frame_types_).c_str());
}

void CanFramePublisher::handle_serial_frame(const interfaces::msg::SyncedFrame::SharedPtr msg)
{
  const auto & frame = msg->frame_data;
  const auto required_size =
    FRAME_DATA_OFFSET + CAN_PACKETS_PER_SERIAL_FRAME * CAN_PACKET_LENGTH +
    FRAME_PROTOCOL_TIMESTAMP_LENGTH + FRAME_CRC8_LENGTH;

  if (frame.size() < required_size) {
    RCLCPP_WARN(
      this->get_logger(),
      "Ignoring short serial frame: size=%zu required>=%zu",
      frame.size(),
      required_size);
    return;
  }

  if (!should_process_frame(frame)) {
    return;
  }

  for (std::size_t packet_index = 0; packet_index < CAN_PACKETS_PER_SERIAL_FRAME; ++packet_index) {
    publish_can_packet(frame, packet_index);
  }
}

bool CanFramePublisher::should_process_frame(const std::vector<uint8_t> & frame) const
{
  const auto type = frame_type(frame.data());
  const auto destination = destination_id(frame.data());

  std::lock_guard<std::mutex> lock(filter_mutex_);
  const bool destination_matches =
    destination_ids_.empty() || contains_byte(destination_ids_, destination);
  const bool type_matches =
    handled_frame_types_.empty() || contains_byte(handled_frame_types_, type);

  if (!destination_matches || !type_matches) {
    RCLCPP_DEBUG(
      this->get_logger(),
      "Ignoring CAN source frame: frame_type=0x%02X destination_id=0x%02X "
      "destination_ids=%s handled_frame_types=%s",
      type,
      destination,
      format_byte_list(destination_ids_).c_str(),
      format_byte_list(handled_frame_types_).c_str());
    return false;
  }

  return true;
}

void CanFramePublisher::publish_can_packet(
  const std::vector<uint8_t> & frame,
  std::size_t packet_index)
{
  const auto packet_offset = can_packet_offset(packet_index);
  const auto can_id = decode_can_id_big_endian(
    frame[packet_offset],
    frame[packet_offset + 1]);

  interfaces::msg::CanFrame msg;
  msg.id = static_cast<int16_t>(can_id);
  std::copy_n(
    frame.begin() + static_cast<std::vector<uint8_t>::difference_type>(
      packet_offset + CAN_ID_LENGTH),
    CAN_DATA_LENGTH,
    msg.data.begin());

  can_publisher_->publish(msg);

  RCLCPP_DEBUG(
    this->get_logger(),
    "Published CAN packet index=%zu can_id=0x%04X msg_id=0x%04X",
    packet_index,
    can_id,
    static_cast<unsigned int>(static_cast<uint16_t>(msg.id)));
}

rcl_interfaces::msg::SetParametersResult CanFramePublisher::handle_parameter_update(
  const std::vector<rclcpp::Parameter> & parameters)
{
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
    "Updated CAN filter: destination_ids=%s handled_frame_types=%s",
    format_byte_list(next_destination_ids).c_str(),
    format_byte_list(next_handled_frame_types).c_str());

  return result;
}

}  // namespace telemetry_telecommand
