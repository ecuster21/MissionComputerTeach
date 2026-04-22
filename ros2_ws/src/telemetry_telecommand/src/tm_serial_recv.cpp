#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "tm_serial_recv",
    "/dev/ttyS7",
    telemetry_telecommand::TM_FRAME_LENGTH,
    "tm_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
