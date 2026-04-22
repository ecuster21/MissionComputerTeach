#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "c_tc_serial_recv",
    "/dev/ttyS3",
    telemetry_telecommand::TC_FRAME_LENGTH,
    "c_tc_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
