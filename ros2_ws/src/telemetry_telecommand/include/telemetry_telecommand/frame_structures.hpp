#ifndef TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
#define TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

namespace telemetry_telecommand
{

// 固定帧头，用于在串口字节流中识别帧边界。
constexpr std::array<uint8_t, 2> FRAME_HEADER{0xEB, 0x90};
// 帧总长 = 帧头 2 字节 + 数据区 60 字节 + CRC16 2 字节。
constexpr std::size_t FRAME_LENGTH = 64;
constexpr std::size_t FRAME_CRC16_LENGTH = 2;
constexpr std::size_t FRAME_PAYLOAD_LENGTH =
  FRAME_LENGTH - FRAME_HEADER.size() - FRAME_CRC16_LENGTH;
// 预留足够空间，在重同步时搜索连续两个帧头。
constexpr std::size_t SEARCH_BUFFER_SIZE = FRAME_LENGTH * 2 + 1;

struct Crc16Config
{
  uint16_t polynomial;
  uint16_t initial_value;
  uint16_t xor_out;
  bool reflect_input;
  bool reflect_output;
};

constexpr Crc16Config CRC16_CCITT_FALSE{0x1021, 0xFFFF, 0x0000, false, false};
constexpr Crc16Config CRC16_MODBUS{0x8005, 0xFFFF, 0x0000, true, true};
constexpr Crc16Config CRC16_IBM{0x8005, 0x0000, 0x0000, true, true};
constexpr Crc16Config CRC16_X25{0x1021, 0xFFFF, 0xFFFF, true, true};

inline uint8_t reflect8(uint8_t value)
{
  uint8_t result = 0;
  for (int i = 0; i < 8; ++i) {
    result = static_cast<uint8_t>((result << 1) | (value & 0x01U));
    value = static_cast<uint8_t>(value >> 1);
  }
  return result;
}

inline uint16_t reflect16(uint16_t value)
{
  uint16_t result = 0;
  for (int i = 0; i < 16; ++i) {
    result = static_cast<uint16_t>((result << 1) | (value & 0x0001U));
    value = static_cast<uint16_t>(value >> 1);
  }
  return result;
}

inline uint16_t compute_crc16(
  const uint8_t * data,
  std::size_t length,
  const Crc16Config & config)
{
  uint16_t crc = config.initial_value;
  for (std::size_t i = 0; i < length; ++i) {
    const uint8_t current_byte = config.reflect_input ? reflect8(data[i]) : data[i];
    crc = static_cast<uint16_t>(crc ^ (static_cast<uint16_t>(current_byte) << 8));

    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000U) != 0U) {
        crc = static_cast<uint16_t>((crc << 1) ^ config.polynomial);
      } else {
        crc = static_cast<uint16_t>(crc << 1);
      }
    }
  }

  if (config.reflect_output) {
    crc = reflect16(crc);
  }

  return static_cast<uint16_t>(crc ^ config.xor_out);
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

  // CRC16 覆盖帧头和数据区，最后两字节保存发送端写入的校验值。
  uint16_t calculate_crc16(const Crc16Config & config) const
  {
    return compute_crc16(data.data(), FRAME_LENGTH - FRAME_CRC16_LENGTH, config);
  }

  uint16_t encoded_crc16(bool big_endian = true) const
  {
    const std::size_t crc_index = FRAME_LENGTH - FRAME_CRC16_LENGTH;
    if (big_endian) {
      return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[crc_index]) << 8) |
        static_cast<uint16_t>(data[crc_index + 1]));
    }

    return static_cast<uint16_t>(
      (static_cast<uint16_t>(data[crc_index + 1]) << 8) |
      static_cast<uint16_t>(data[crc_index]));
  }

  void set_crc16(uint16_t crc, bool big_endian = true)
  {
    const std::size_t crc_index = FRAME_LENGTH - FRAME_CRC16_LENGTH;
    if (big_endian) {
      data[crc_index] = static_cast<uint8_t>((crc >> 8) & 0xFFU);
      data[crc_index + 1] = static_cast<uint8_t>(crc & 0xFFU);
      return;
    }

    data[crc_index] = static_cast<uint8_t>(crc & 0xFFU);
    data[crc_index + 1] = static_cast<uint8_t>((crc >> 8) & 0xFFU);
  }

  bool validate_crc16(const Crc16Config & config, bool big_endian = true) const
  {
    return calculate_crc16(config) == encoded_crc16(big_endian);
  }
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
