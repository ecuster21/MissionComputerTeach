#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // L_TC 通道：默认 /dev/ttyS4，固定 32 字节帧，发布到 l_tc_synced_frame。
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "l_tc_serial_recv",
    "/dev/ttyS4",
    telemetry_telecommand::TC_FRAME_LENGTH,
    "l_tc_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
