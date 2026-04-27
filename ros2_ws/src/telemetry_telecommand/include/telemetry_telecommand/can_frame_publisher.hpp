#ifndef TELEMETRY_TELECOMMAND__CAN_FRAME_PUBLISHER_HPP_
#define TELEMETRY_TELECOMMAND__CAN_FRAME_PUBLISHER_HPP_

#include <mutex>
#include <string>
#include <vector>

#include <interfaces/msg/can_frame.hpp>
#include <interfaces/msg/synced_frame.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <rclcpp/rclcpp.hpp>

namespace telemetry_telecommand
{

// 从已经同步和 CRC 校验通过的串口完整帧中提取两包 CAN 数据并发布。
class CanFramePublisher : public rclcpp::Node
{
public:
  CanFramePublisher(
    const std::string & node_name,
    const std::string & default_input_topic,
    const std::string & default_output_topic);

private:
  void handle_serial_frame(const interfaces::msg::SyncedFrame::SharedPtr msg);
  bool should_process_frame(const std::vector<uint8_t> & frame) const;
  void publish_can_packet(const std::vector<uint8_t> & frame, std::size_t packet_index);
  rcl_interfaces::msg::SetParametersResult handle_parameter_update(
    const std::vector<rclcpp::Parameter> & parameters);

  rclcpp::Subscription<interfaces::msg::SyncedFrame>::SharedPtr frame_subscriber_;
  rclcpp::Publisher<interfaces::msg::CanFrame>::SharedPtr can_publisher_;
  std::vector<uint8_t> destination_ids_;
  std::vector<uint8_t> handled_frame_types_;
  mutable std::mutex filter_mutex_;
  rclcpp::Node::OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
};

}  // namespace telemetry_telecommand

#endif  // TELEMETRY_TELECOMMAND__CAN_FRAME_PUBLISHER_HPP_
