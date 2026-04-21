#include "telemetry_telecommand/serial_receiver.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <exception>
#include <memory>

namespace telemetry_telecommand
{
namespace
{

// 丢弃已消费的数据，同时保持剩余未读数据在缓冲区中连续。
void consume_bytes(
  // 传入整个缓冲区和当前有效数据长度，以及要丢弃的字节数。
  std::array<uint8_t, SEARCH_BUFFER_SIZE> & buffer,
  std::size_t & bytes_in_buffer,
  std::size_t count)
{
  // 如果要丢弃的字节数超过当前缓冲区中的有效数据，就直接清空缓冲区。
  if (count >= bytes_in_buffer) {
    bytes_in_buffer = 0;
    return;
  }
//std::memmove(目标地址, 源地址, 字节数)
  std::memmove(buffer.data(), buffer.data() + count, bytes_in_buffer - count);
  bytes_in_buffer -= count;
}

// 搜索相隔一个完整帧长的两个连续帧头。
// 与只匹配单个帧头相比，这样能让初始同步更稳健。
bool try_find_sync(
  std::array<uint8_t, SEARCH_BUFFER_SIZE> & buffer,
  std::size_t & bytes_in_buffer)
{
  if (bytes_in_buffer < FRAME_LENGTH + FRAME_HEADER.size()) {
    return false;
  }

  for (std::size_t i = 0; i + FRAME_LENGTH + 1 < bytes_in_buffer; ++i) {
    if (
      buffer[i] == FRAME_HEADER[0] &&
      buffer[i + 1] == FRAME_HEADER[1] &&
      buffer[i + FRAME_LENGTH] == FRAME_HEADER[0] &&
      buffer[i + FRAME_LENGTH + 1] == FRAME_HEADER[1])
    {
      consume_bytes(buffer, bytes_in_buffer, i);
      return true;
    }
  }

  // 仅保留仍有可能包含下一帧起始位置的尾部数据。
  const std::size_t keep_bytes = std::min(bytes_in_buffer, FRAME_LENGTH + 1);
  if (keep_bytes < bytes_in_buffer) {
    std::memmove(
      buffer.data(),
      buffer.data() + (bytes_in_buffer - keep_bytes),
      keep_bytes);
    bytes_in_buffer = keep_bytes;
  }

  return false;
}

}  // 匿名命名空间

SerialReceiver::SerialReceiver()
: Node("serial_receiver")
{
  // 通过 ROS 参数配置串口设备，避免每次修改都重新编译。
  port_name_ = this->declare_parameter<std::string>("port", "/dev/ttyUSB0");
  baud_rate_ = static_cast<uint32_t>(this->declare_parameter<int>("baud_rate", 115200));
  timeout_ms_ = static_cast<uint32_t>(this->declare_parameter<int>("timeout_ms", 100));
  const auto topic_name = this->declare_parameter<std::string>("topic", "synced_frame");

  // 发布校验通过的完整帧，供可视化等下游节点使用。
  frame_publisher_ = this->create_publisher<interfaces::msg::SyncedFrame>(topic_name, 10);

  if (!initialize_serial()) {
    RCLCPP_WARN(
      this->get_logger(),
      "Serial port %s is not available yet, the receiver thread will retry.",
      port_name_.c_str());
  }

  receive_thread_ = std::thread(&SerialReceiver::receive_data, this);
}

SerialReceiver::~SerialReceiver()
{
  // 先通知线程退出，再关闭串口，最后等待线程结束。
  stop_requested_.store(true);
  close_serial();
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

bool SerialReceiver::initialize_serial()
{
  try {
    // 每次重连都重新下发配置，便于处理插拔后的串口恢复。
    serial_port_.set_port(port_name_);
    serial_port_.set_baud_rate(baud_rate_);
    serial_port_.set_timeout_ms(timeout_ms_);
    if (!serial_port_.is_open()) {
      serial_port_.open();
    }

    RCLCPP_INFO(
      this->get_logger(),
      "Connected to serial port %s at %u baud.",
      port_name_.c_str(),
      baud_rate_);
    return true;
  } catch (const std::exception & ex) {
    RCLCPP_WARN(
      this->get_logger(),
      "Failed to open serial port %s: %s",
      port_name_.c_str(),
      ex.what());
    close_serial();
    return false;
  }
}

void SerialReceiver::close_serial()
{
  try {
    if (serial_port_.is_open()) {
      serial_port_.close();
    }
  } catch (const std::exception & ex) {
    RCLCPP_WARN(
      this->get_logger(),
      "Error while closing serial port %s: %s",
      port_name_.c_str(),
      ex.what());
  }
}

void SerialReceiver::receive_data()
{
  // 缓冲区既要能用于搜索帧边界，也要能暂存不完整的尾部数据。
  std::array<uint8_t, SEARCH_BUFFER_SIZE> buffer{};
  std::size_t bytes_in_buffer = 0;
  bool is_synced = false;

  while (rclcpp::ok() && !stop_requested_.load()) {
    if (!serial_port_.is_open()) {
      // 串口暂不可用时持续重试，而不是直接退出节点。
      if (!initialize_serial()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }
      bytes_in_buffer = 0;
      is_synced = false;
    }
    // 计算剩余缓冲区容量，避免越界写入。如果意外满了，就丢弃最旧的数据以腾出空间。
    const auto read_capacity = SEARCH_BUFFER_SIZE - bytes_in_buffer;
    if (read_capacity == 0) {
      // 如果缓冲区意外写满，就丢弃最旧的一帧长度数据。
      consume_bytes(buffer, bytes_in_buffer, FRAME_LENGTH);
    }

    std::size_t bytes_read = 0;
    try {
      // 从串口读取数据直接追加到缓冲区末尾，保持连续性以便搜索帧边界。buffer.data()拿到缓冲区起始地址，bytes_in_buffer是当前有效数据长度，read_capacity是剩余空间大小。
      bytes_read = serial_port_.read(buffer.data() + bytes_in_buffer, SEARCH_BUFFER_SIZE - bytes_in_buffer);
    } catch (const std::exception & ex) {
      RCLCPP_WARN(
        this->get_logger(),
        "Serial read failed on %s: %s",
        port_name_.c_str(),
        ex.what());
      close_serial();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    if (bytes_read == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    bytes_in_buffer += bytes_read;

    if (!is_synced) {
      // 在同步建立前一直停留在捕获模式，直到确认两个对齐帧头。
      is_synced = try_find_sync(buffer, bytes_in_buffer);
      if (is_synced) {
        RCLCPP_INFO(this->get_logger(), "Frame synchronization established.");
      } else {
        continue;
      }
    }

    while (is_synced && bytes_in_buffer >= FRAME_LENGTH) {
      SerialFrame frame;
      //std::copy_n(源起点, 个数, 目标起点);
      // 从缓冲区开头复制一个完整帧长度的数据到 SerialFrame 结构中，准备进行帧头和校验验证。
      std::copy_n(buffer.data(), FRAME_LENGTH, frame.data.begin());

      if (frame.has_valid_header() && frame.validate_checksum()) {
        // 校验通过后立刻发布，并从缓冲区中移除这一帧。
        publish_frame(frame);
        consume_bytes(buffer, bytes_in_buffer, FRAME_LENGTH);
        continue;
      }

      // 若数据损坏，则右移一个字节并重新搜索同步点。
      RCLCPP_WARN(
        this->get_logger(),
        "Frame validation failed, attempting to resynchronize.");
      consume_bytes(buffer, bytes_in_buffer, 1);
      is_synced = try_find_sync(buffer, bytes_in_buffer);
    }
  }
}

void SerialReceiver::publish_frame(const SerialFrame & frame)
{
  interfaces::msg::SyncedFrame msg;
  // 发布原始帧字节，并附带本地接收时间戳。
  std::copy(frame.data.begin(), frame.data.end(), msg.frame_data.begin());
  msg.timestamp_ns = this->get_clock()->now().nanoseconds();
  frame_publisher_->publish(msg);
}

}  // telemetry_telecommand 命名空间

int main(int argc, char * argv[])
{
  // 标准 ROS 2 节点启动与退出流程。
  rclcpp::init(argc, argv);
  auto receiver = std::make_shared<telemetry_telecommand::SerialReceiver>();
  rclcpp::spin(receiver);
  rclcpp::shutdown();
  return 0;
}
