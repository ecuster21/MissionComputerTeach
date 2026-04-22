#ifndef TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_
#define TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <interfaces/msg/synced_frame.hpp>
#include <rclcpp/rclcpp.hpp>

#include "telemetry_telecommand/frame_structures.hpp"
#include "telemetry_telecommand/posix_serial_port.hpp"

namespace telemetry_telecommand
{

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
  bool initialize_serial();
  void close_serial();
  void receive_data();
  void publish_frame(const std::vector<uint8_t> & frame);

  PosixSerialPort serial_port_;
  std::string port_name_;
  uint32_t baud_rate_;
  uint32_t timeout_ms_;
  std::size_t frame_length_;
  std::size_t search_buffer_size_;
  std::string crc8_variant_name_;
  Crc8Config crc8_config_{CRC8_STANDARD};
  rclcpp::Publisher<interfaces::msg::SyncedFrame>::SharedPtr frame_publisher_;
  std::atomic<bool> stop_requested_{false};
  std::thread receive_thread_;
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FIXED_FRAME_SERIAL_RECEIVER_HPP_
