#ifndef TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_
#define TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <interfaces/msg/synced_frame.hpp>
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <rclcpp/rclcpp.hpp>

#include "telemetry_telecommand/frame_structures.hpp"
#include "telemetry_telecommand/posix_serial_port.hpp"

namespace telemetry_telecommand
{

// 固定帧串口接收节点的公共实现。
// 具体的 fc_tm/c_tc/l_tc 节点只提供默认串口、帧长和发布话题。
class FixedFrameSerialReceiver : public rclcpp::Node
{
public:
  FixedFrameSerialReceiver(
    const std::string & node_name,
    const std::string & default_port,
    std::size_t default_frame_length,
    const std::string & default_topic);
  ~FixedFrameSerialReceiver() override;

private:
  // 串口可能在节点启动时还未就绪，因此接收线程会反复调用该方法重连。
  bool initialize_serial();
  void close_serial();
  void receive_data();
  // CRC 通过后再做业务过滤：目的 ID 命中本机号，帧类型属于待处理集合。
  bool should_process_frame(const std::vector<uint8_t> & frame) const;
  void publish_frame(const std::vector<uint8_t> & frame);
  // 支持运行时调整目的 ID 和帧类型过滤表，便于联调中增减机号或协议类型。
  rcl_interfaces::msg::SetParametersResult handle_parameter_update(
    const std::vector<rclcpp::Parameter> & parameters);

  PosixSerialPort serial_port_;
  std::string port_name_;
  uint32_t baud_rate_;
  uint32_t timeout_ms_;
  std::size_t frame_length_;
  std::size_t search_buffer_size_;
  std::string crc8_variant_name_;
  Crc8Config crc8_config_{CRC8_STANDARD};
  // 空列表表示不过滤该字段；非空时只允许列表中的字节值通过。
  std::vector<uint8_t> destination_ids_;
  std::vector<uint8_t> handled_frame_types_;
  // 接收线程读取过滤表，参数回调写入过滤表，两边用同一把锁保护。
  mutable std::mutex filter_mutex_;
  rclcpp::Publisher<interfaces::msg::SyncedFrame>::SharedPtr frame_publisher_;
  rclcpp::Node::OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
  std::atomic<bool> stop_requested_{false};
  std::thread receive_thread_;
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_
