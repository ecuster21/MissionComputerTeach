#ifndef TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
#define TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

namespace telemetry_telecommand
{

// 固定帧头，用于在串口字节流中识别帧边界。
constexpr std::array<uint8_t, 2> FRAME_HEADER{0xEB, 0x90};
// 帧总长 = 帧头 2 字节 + 数据区 61 字节 + CRC 1 字节。
constexpr std::size_t FRAME_LENGTH = 64;
// 预留足够空间，在重同步时搜索连续两个帧头。
constexpr std::size_t SEARCH_BUFFER_SIZE = FRAME_LENGTH * 2 + 1;

struct SerialFrame
{
  // 串口线上实际传输的完整原始字节。
  std::array<uint8_t, FRAME_LENGTH> data{};

  // 有效帧必须以固定两字节帧头开头。
  bool has_valid_header() const
  {
    return data[0] == FRAME_HEADER[0] && data[1] == FRAME_HEADER[1];
  }

  // 这里的 CRC 定义为前 63 字节的 8 位累加和。
  uint8_t calculate_checksum() const
  {
    uint8_t sum = 0;
    for (std::size_t i = 0; i < FRAME_LENGTH - 1; ++i) {
      sum = static_cast<uint8_t>(sum + data[i]);
    }
    return sum;
  }

  // 帧的最后 1 字节保存发送端写入的校验值。
  bool validate_checksum() const
  {
    return calculate_checksum() == data[FRAME_LENGTH - 1];
  }
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
