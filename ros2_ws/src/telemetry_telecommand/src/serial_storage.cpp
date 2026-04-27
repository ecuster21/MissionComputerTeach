#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <interfaces/msg/synced_frame.hpp>
#include <rclcpp/rclcpp.hpp>

// 将 3 路已经通过同步、CRC 和业务过滤的串口帧合并存储到单个文件。
// 节点只写 SyncedFrame.frame_data 原始字节，不额外写入话题名或 ROS 时间戳。
class SerialStorage : public rclcpp::Node
{
public:
  SerialStorage()
  : Node("serial_storage")
  {
    storage_file_ = this->declare_parameter<std::string>("storage_file", "serial_storage.bin");
    const auto fc_tm_topic =
      this->declare_parameter<std::string>("fc_tm_topic", "fc_tm_synced_frame");
    const auto c_tc_topic =
      this->declare_parameter<std::string>("c_tc_topic", "c_tc_synced_frame");
    const auto l_tc_topic =
      this->declare_parameter<std::string>("l_tc_topic", "l_tc_synced_frame");
    const auto truncate_file = this->declare_parameter<bool>("truncate_file", true);

    open_storage_file(truncate_file);

    subscriptions_.push_back(create_frame_subscription(fc_tm_topic));
    subscriptions_.push_back(create_frame_subscription(c_tc_topic));
    subscriptions_.push_back(create_frame_subscription(l_tc_topic));

    writer_thread_ = std::thread(&SerialStorage::writer_loop, this);

    RCLCPP_INFO(
      this->get_logger(),
      "Serial storage started. file=%s buffer_size=%zu topics=[%s, %s, %s]",
      storage_file_.c_str(),
      kBufferSize,
      fc_tm_topic.c_str(),
      c_tc_topic.c_str(),
      l_tc_topic.c_str());
  }

  ~SerialStorage() override
  {
    stop_and_flush();
  }

private:
  // 双缓存的单块大小固定为 4KB：一个缓冲接收，另一个可交给写盘线程。
  static constexpr std::size_t kBufferSize = 4096;
  static constexpr std::size_t kBufferCount = 2;

  // Empty 可被接收线程选为 Active；Ready 等待写盘；Writing 正在被写盘线程使用。
  enum class BufferState
  {
    Empty,
    Active,
    Ready,
    Writing,
  };

  rclcpp::Subscription<interfaces::msg::SyncedFrame>::SharedPtr create_frame_subscription(
    const std::string & topic_name)
  {
    // 三路话题共用同一个回调，按照 ROS 回调实际到达顺序进入缓存。
    return this->create_subscription<interfaces::msg::SyncedFrame>(
      topic_name,
      rclcpp::QoS(100),
      [this, topic_name](const interfaces::msg::SyncedFrame::SharedPtr msg) {
        handle_frame(topic_name, *msg);
      });
  }

  void open_storage_file(bool truncate_file)
  {
    // storage_file 可以带目录；目录不存在时自动创建，方便直接落到外接盘路径。
    const std::filesystem::path storage_path(storage_file_);
    const auto parent_path = storage_path.parent_path();
    if (!parent_path.empty()) {
      std::filesystem::create_directories(parent_path);
    }

    auto mode = std::ios::binary | std::ios::out;
    mode |= truncate_file ? std::ios::trunc : std::ios::app;
    output_file_.open(storage_file_, mode);
    if (!output_file_.is_open()) {
      throw std::runtime_error("Unable to open storage file: " + storage_file_);
    }
  }

  void handle_frame(
    const std::string & topic_name,
    const interfaces::msg::SyncedFrame & msg)
  {
    // 空消息不参与统计和写盘，避免生成无意义的 0 字节写入。
    if (msg.frame_data.empty()) {
      return;
    }

    append_bytes(msg.frame_data.data(), msg.frame_data.size());
    frames_received_.fetch_add(1, std::memory_order_relaxed);
    bytes_received_.fetch_add(msg.frame_data.size(), std::memory_order_relaxed);

    RCLCPP_DEBUG(
      this->get_logger(),
      "Buffered frame from %s: frame_size=%zu timestamp_ns=%ld",
      topic_name.c_str(),
      msg.frame_data.size(),
      static_cast<long>(msg.timestamp_ns));
  }

  void append_bytes(const uint8_t * data, std::size_t size)
  {
    // 单帧可能跨越 4KB 边界，因此按剩余空间分段拷贝到当前 Active 缓冲区。
    std::size_t offset = 0;
    while (offset < size && rclcpp::ok()) {
      std::unique_lock<std::mutex> lock(buffer_mutex_);
      // 两个缓冲区都在等待/执行写盘时，接收回调会短暂阻塞等待空缓冲。
      empty_buffer_cv_.wait(lock, [this]() {
        return stop_requested_.load() || active_buffer_index_ >= 0;
      });

      if (stop_requested_.load()) {
        return;
      }

      const auto buffer_index = static_cast<std::size_t>(active_buffer_index_);
      const auto available = kBufferSize - buffer_sizes_[buffer_index];
      if (available == 0) {
        queue_active_buffer_locked();
        continue;
      }

      const auto copy_size = std::min(available, size - offset);
      std::memcpy(
        buffers_[buffer_index].data() + buffer_sizes_[buffer_index],
        data + offset,
        copy_size);

      buffer_sizes_[buffer_index] += copy_size;
      offset += copy_size;

      if (buffer_sizes_[buffer_index] == kBufferSize) {
        queue_active_buffer_locked();
      }
    }
  }

  void queue_active_buffer_locked()
  {
    // 调用者必须持有 buffer_mutex_；该函数只切换状态，不直接写盘。
    if (active_buffer_index_ < 0) {
      return;
    }

    const auto buffer_index = static_cast<std::size_t>(active_buffer_index_);
    if (buffer_sizes_[buffer_index] == 0) {
      return;
    }

    buffer_states_[buffer_index] = BufferState::Ready;
    ready_buffers_.push_back(buffer_index);

    // 如果另一个缓冲区可用，立即切过去继续接收；否则等待写盘线程释放。
    active_buffer_index_ = find_empty_buffer_locked();
    if (active_buffer_index_ >= 0) {
      buffer_states_[static_cast<std::size_t>(active_buffer_index_)] = BufferState::Active;
    }

    ready_buffer_cv_.notify_one();
  }

  int find_empty_buffer_locked() const
  {
    for (std::size_t i = 0; i < kBufferCount; ++i) {
      if (buffer_states_[i] == BufferState::Empty) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  void writer_loop()
  {
    while (true) {
      std::size_t buffer_index = 0;
      std::size_t write_size = 0;

      {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        // 写盘线程只消费 Ready 队列；停止时也会处理析构阶段压入的尾包。
        ready_buffer_cv_.wait(lock, [this]() {
          return stop_requested_.load() || !ready_buffers_.empty();
        });

        if (ready_buffers_.empty() && stop_requested_.load()) {
          break;
        }

        buffer_index = ready_buffers_.front();
        ready_buffers_.pop_front();
        buffer_states_[buffer_index] = BufferState::Writing;
        write_size = buffer_sizes_[buffer_index];
      }

      if (write_size > 0) {
        // 只写有效长度，最后一次 flush 可能不足 4KB。
        output_file_.write(
          reinterpret_cast<const char *>(buffers_[buffer_index].data()),
          static_cast<std::streamsize>(write_size));
        output_file_.flush();

        if (!output_file_) {
          RCLCPP_ERROR(
            this->get_logger(),
            "Failed to write %zu bytes to %s",
            write_size,
            storage_file_.c_str());
        } else {
          chunks_written_.fetch_add(1, std::memory_order_relaxed);
          bytes_written_.fetch_add(write_size, std::memory_order_relaxed);
        }
      }

      {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        // 写盘完成后把缓冲区归还给接收侧；如果接收侧正在等待，立即接管为 Active。
        buffer_sizes_[buffer_index] = 0;
        buffer_states_[buffer_index] = BufferState::Empty;
        if (!stop_requested_.load() && active_buffer_index_ < 0) {
          active_buffer_index_ = static_cast<int>(buffer_index);
          buffer_states_[buffer_index] = BufferState::Active;
        }
      }
      empty_buffer_cv_.notify_all();
    }
  }

  void stop_and_flush()
  {
    // 析构和异常退出路径可能重复触发，compare_exchange 保证只执行一次收尾。
    bool expected = false;
    if (!stop_requested_.compare_exchange_strong(expected, true)) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(buffer_mutex_);
      if (active_buffer_index_ >= 0) {
        const auto buffer_index = static_cast<std::size_t>(active_buffer_index_);
        // 节点退出时，不满 4KB 的当前缓冲也要落盘，避免丢失最后几帧。
        if (buffer_sizes_[buffer_index] > 0) {
          buffer_states_[buffer_index] = BufferState::Ready;
          ready_buffers_.push_back(buffer_index);
        } else {
          buffer_states_[buffer_index] = BufferState::Empty;
        }
        active_buffer_index_ = -1;
      }
    }

    ready_buffer_cv_.notify_all();
    empty_buffer_cv_.notify_all();

    if (writer_thread_.joinable()) {
      writer_thread_.join();
    }

    if (output_file_.is_open()) {
      output_file_.flush();
      output_file_.close();
    }

    RCLCPP_INFO(
      this->get_logger(),
      "Serial storage stopped. frames=%lu received_bytes=%lu written_bytes=%lu chunks=%lu file=%s",
      static_cast<unsigned long>(frames_received_.load()),
      static_cast<unsigned long>(bytes_received_.load()),
      static_cast<unsigned long>(bytes_written_.load()),
      static_cast<unsigned long>(chunks_written_.load()),
      storage_file_.c_str());
  }

  std::string storage_file_;
  std::ofstream output_file_;

  // buffers_ 保存原始二进制帧流；buffer_sizes_ 记录每块缓冲当前有效字节数。
  std::array<std::array<uint8_t, kBufferSize>, kBufferCount> buffers_{};
  std::array<std::size_t, kBufferCount> buffer_sizes_{};
  std::array<BufferState, kBufferCount> buffer_states_{
    BufferState::Active,
    BufferState::Empty,
  };
  int active_buffer_index_{0};
  // 保持 Ready 缓冲的先后顺序，确保写盘顺序和接收入队顺序一致。
  std::deque<std::size_t> ready_buffers_;

  std::mutex buffer_mutex_;
  std::condition_variable ready_buffer_cv_;
  std::condition_variable empty_buffer_cv_;
  std::atomic<bool> stop_requested_{false};
  std::thread writer_thread_;

  std::vector<rclcpp::Subscription<interfaces::msg::SyncedFrame>::SharedPtr> subscriptions_;
  std::atomic<uint64_t> frames_received_{0};
  std::atomic<uint64_t> bytes_received_{0};
  std::atomic<uint64_t> bytes_written_{0};
  std::atomic<uint64_t> chunks_written_{0};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto storage_node = std::make_shared<SerialStorage>();
  rclcpp::spin(storage_node);
  // 在 rclcpp::shutdown() 前释放节点，确保析构里能使用 logger 打印最终统计。
  storage_node.reset();
  rclcpp::shutdown();
  return 0;
}
