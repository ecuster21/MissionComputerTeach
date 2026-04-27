#include "telemetry_telecommand/can_frame_publisher.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // C 链路 CAN 提取节点：订阅 c_tc_synced_frame，发布 c_can_frame。
  auto publisher = std::make_shared<telemetry_telecommand::CanFramePublisher>(
    "c_can_pub",
    "c_tc_synced_frame",
    "c_can_frame");
  rclcpp::spin(publisher);
  rclcpp::shutdown();
  return 0;
}
