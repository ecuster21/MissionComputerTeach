#include "telemetry_telecommand/fixed_frame_serial_receiver.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto receiver = std::make_shared<telemetry_telecommand::FixedFrameSerialReceiver>(
    "fc_tm_serial_recv",
    "/dev/ttyS7",
    telemetry_telecommand::FC_TM_FRAME_LENGTH,
    "fc_tm_synced_frame");
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
