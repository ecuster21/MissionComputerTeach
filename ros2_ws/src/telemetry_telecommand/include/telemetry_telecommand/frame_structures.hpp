#ifndef TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
#define TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace telemetry_telecommand
{

// 固定帧头，用于在串口字节流中识别帧边界。
constexpr std::array<uint8_t, 2> FRAME_HEADER{0xEB, 0x90};
constexpr std::size_t FRAME_CRC8_LENGTH = 1;
constexpr std::size_t FRAME_TYPE_OFFSET = FRAME_HEADER.size();
constexpr std::size_t FRAME_SOURCE_ID_OFFSET = FRAME_TYPE_OFFSET + 1;
constexpr std::size_t FRAME_DESTINATION_ID_OFFSET = FRAME_SOURCE_ID_OFFSET + 1;
constexpr std::size_t FRAME_ROUTE_FIELDS_LENGTH = 3;
constexpr std::size_t FC_TM_FRAME_LENGTH = 64;
constexpr std::size_t TC_FRAME_LENGTH = 32;

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

inline std::size_t payload_length_for(std::size_t frame_length)
{
  return frame_length - FRAME_HEADER.size() - FRAME_CRC8_LENGTH;
}

inline std::size_t search_buffer_size_for(std::size_t frame_length)
{
  return frame_length * 2 + 1;
}

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
  std::vector<uint8_t> data{};
};

inline bool has_valid_header(const uint8_t * data, std::size_t frame_length)
{
  return frame_length >= FRAME_HEADER.size() &&
         data[0] == FRAME_HEADER[0] &&
         data[1] == FRAME_HEADER[1];
}

inline uint8_t frame_type(const uint8_t * data)
{
  return data[FRAME_TYPE_OFFSET];
}

inline uint8_t source_id(const uint8_t * data)
{
  return data[FRAME_SOURCE_ID_OFFSET];
}

inline uint8_t destination_id(const uint8_t * data)
{
  return data[FRAME_DESTINATION_ID_OFFSET];
}

inline uint8_t calculate_crc8(
  const uint8_t * data,
  std::size_t frame_length,
  const Crc8Config & config)
{
  return compute_crc8(data, frame_length - FRAME_CRC8_LENGTH, config);
}

inline uint8_t encoded_crc8(const uint8_t * data, std::size_t frame_length)
{
  return data[frame_length - FRAME_CRC8_LENGTH];
}

inline bool validate_crc8(
  const uint8_t * data,
  std::size_t frame_length,
  const Crc8Config & config)
{
  return calculate_crc8(data, frame_length, config) == encoded_crc8(data, frame_length);
}

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__FRAME_STRUCTURES_HPP_
