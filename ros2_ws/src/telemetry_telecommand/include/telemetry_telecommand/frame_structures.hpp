#ifndef TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
#define TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

namespace telemetry_telecommand
{

// 固定帧头，用于在串口字节流中识别帧边界。
constexpr std::array<uint8_t, 2> FRAME_HEADER{0xEB, 0x90};
// 帧总长 = 帧头 2 字节 + 数据区 61 字节 + CRC8 1 字节。
constexpr std::size_t FRAME_LENGTH = 64;
constexpr std::size_t FRAME_CRC8_LENGTH = 1;
constexpr std::size_t FRAME_PAYLOAD_LENGTH =
  FRAME_LENGTH - FRAME_HEADER.size() - FRAME_CRC8_LENGTH;
// 预留足够空间，在重同步时搜索连续两个帧头。
constexpr std::size_t SEARCH_BUFFER_SIZE = FRAME_LENGTH * 2 + 1;

struct Crc8Config
{
  uint8_t polynomial;
  uint8_t initial_value;
  uint8_t xor_out;
  bool reflect_input;
  bool reflect_output;
};

constexpr Crc8Config CRC8_STANDARD{0x07, 0x00, 0x00, false, false};
constexpr Crc8Config CRC8_MAXIM{0x31, 0x00, 0x00, true, true};
constexpr Crc8Config CRC8_SAE_J1850{0x1D, 0xFF, 0xFF, false, false};

inline uint8_t reflect8(uint8_t value)
{
  uint8_t result = 0;
  for (int i = 0; i < 8; ++i) {
    result = static_cast<uint8_t>((result << 1) | (value & 0x01U));
    value = static_cast<uint8_t>(value >> 1);
  }
  return result;
}

inline uint8_t compute_crc8(
  const uint8_t * data,
  std::size_t length,
  const Crc8Config & config)
{
  uint8_t crc = config.initial_value;
  for (std::size_t i = 0; i < length; ++i) {
    const uint8_t current_byte = config.reflect_input ? reflect8(data[i]) : data[i];
    crc = static_cast<uint8_t>(crc ^ current_byte);

    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x80U) != 0U) {
        crc = static_cast<uint8_t>((crc << 1) ^ config.polynomial);
      } else {
        crc = static_cast<uint8_t>(crc << 1);
      }
    }
  }

  if (config.reflect_output) {
    crc = reflect8(crc);
  }

  return static_cast<uint8_t>(crc ^ config.xor_out);
}

struct SerialFrame
{
  // 串口线上实际传输的完整原始字节。
  std::array<uint8_t, FRAME_LENGTH> data{};

  // 有效帧必须以固定两字节帧头开头。
  bool has_valid_header() const
  {
    return data[0] == FRAME_HEADER[0] && data[1] == FRAME_HEADER[1];
  }

  // CRC8 覆盖帧头和数据区，最后 1 字节保存发送端写入的校验值。
  uint8_t calculate_crc8(const Crc8Config & config) const
  {
    return compute_crc8(data.data(), FRAME_LENGTH - FRAME_CRC8_LENGTH, config);
  }

  uint8_t encoded_crc8() const
  {
    return data[FRAME_LENGTH - FRAME_CRC8_LENGTH];
  }

  void set_crc8(uint8_t crc)
  {
    data[FRAME_LENGTH - FRAME_CRC8_LENGTH] = crc;
  }

  bool validate_crc8(const Crc8Config & config) const
  {
    return calculate_crc8(config) == encoded_crc8();
  }
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
