#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // C_TC 通道：默认 /dev/ttyS3，固定 32 字节帧，发布到 c_tc_synced_frame。
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "c_tc_serial_recv",
    "/dev/ttyS3",
    telemetry_telecommand::TC_FRAME_LENGTH,
    "c_tc_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
