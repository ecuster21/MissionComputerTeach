#include "telemetry_telecommand/can_frame_publisher.hpp"

#include <memory>

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  // L 链路 CAN 提取节点：订阅 l_tc_synced_frame，发布 l_can_frame。
  auto publisher = std::make_shared<telemetry_telecommand::CanFramePublisher>(
    "l_can_pub",
    "l_tc_synced_frame",
    "l_can_frame");
  rclcpp::spin(publisher);
  rclcpp::shutdown();
  return 0;
}
