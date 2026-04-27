#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // 飞控遥测通道：默认 /dev/ttyS7，固定 64 字节帧，发布到 fc_tm_synced_frame。
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "fc_tm_serial_recv",
    "/dev/ttyS7",
    telemetry_telecommand::FC_TM_FRAME_LENGTH,
    "fc_tm_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
