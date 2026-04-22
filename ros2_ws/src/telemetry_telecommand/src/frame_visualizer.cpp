#include <functional>
#include <iomanip>
#include <sstream>

#include <interfaces/msg/synced_frame.hpp>
#include <rclcpp/rclcpp.hpp>

// 调试节点：将收到的每一帧按格式化十六进制输出到终端。
class FrameVisualizer : public rclcpp::Node
{
public:
  FrameVisualizer()
  : Node("frame_visualizer")
  {
    // 允许通过命令行参数覆盖订阅话题名。
    const auto topic_name = this->declare_parameter<std::string>("topic", "tm_synced_frame");
    frame_subscriber_ = this->create_subscription<interfaces::msg::SyncedFrame>(
      topic_name,
      10,
      std::bind(&FrameVisualizer::frame_callback, this, std::placeholders::_1));
  }

private:
  void frame_callback(const interfaces::msg::SyncedFrame::SharedPtr msg) const
  {
    // 每 16 字节换一行，便于在终端中观察长帧内容。
    constexpr std::size_t bytes_per_row = 16;

    std::ostringstream output;
    output << "Received frame at " << msg->timestamp_ns << " ns";

    for (std::size_t i = 0; i < msg->frame_data.size(); ++i) {
      if (i % bytes_per_row == 0) {
        output << '\n' << std::setw(4) << std::setfill(' ') << i << ": ";
      }
      output
        << std::hex
        << std::setw(2)
        << std::setfill('0')
        << static_cast<int>(msg->frame_data[i])
        << ' ';
    }

    RCLCPP_INFO(this->get_logger(), "%s", output.str().c_str());
  }

  rclcpp::Subscription<interfaces::msg::SyncedFrame>::SharedPtr frame_subscriber_;
};

int main(int argc, char * argv[])
{
  // 标准 ROS 2 节点启动与退出流程。
  rclcpp::init(argc, argv);
  auto visualizer = std::make_shared<FrameVisualizer>();
  rclcpp::spin(visualizer);
  rclcpp::shutdown();
  return 0;
}
