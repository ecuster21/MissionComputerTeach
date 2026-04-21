#ifndef TELEMETRY_TELECOMMAND__SERIAL_RECEIVER_HPP_
#define TELEMETRY_TELECOMMAND__SERIAL_RECEIVER_HPP_

#include <atomic>
#include <string>
#include <thread>

#include <interfaces/msg/synced_frame.hpp>
#include <rclcpp/rclcpp.hpp>

#include "telemetry_telecommand/frame_structures.hpp"
#include "telemetry_telecommand/posix_serial_port.hpp"

namespace telemetry_telecommand
{

// ROS 2 串口接收节点，负责读串口、重同步并发布完整帧。
class SerialReceiver : public rclcpp::Node
{
public:
  SerialReceiver();
  ~SerialReceiver() override;

private:
  // 按当前节点参数打开或重新打开串口。
  bool initialize_serial();
  void close_serial();
  // 后台读取循环，持续维持对帧边界的同步。
  void receive_data();
  // 将校验通过的原始帧连同本地时间戳一起发布出去。
  void publish_frame(const SerialFrame & frame);

  // 串口配置和 ROS 发布器状态。
  PosixSerialPort serial_port_;
  std::string port_name_;
  uint32_t baud_rate_;
  uint32_t timeout_ms_;
  std::string crc16_variant_name_;
  Crc16Config crc16_config_{CRC16_CCITT_FALSE};
  bool crc16_big_endian_{true};
  rclcpp::Publisher<interfaces::msg::SyncedFrame>::SharedPtr frame_publisher_;
  // 停止标记和专用接收线程，避免串口阻塞影响 ROS spin。
  std::atomic<bool> stop_requested_{false};
  std::thread receive_thread_;
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__SERIAL_RECEIVER_HPP_
