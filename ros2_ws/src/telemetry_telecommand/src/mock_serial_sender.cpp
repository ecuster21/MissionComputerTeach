#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include "telemetry_telecommand/frame_structures.hpp"
#include "telemetry_telecommand/posix_serial_port.hpp"

int main(int argc, char * argv[])
{
  // 直接从命令行读取串口参数，便于独立测试。
  const std::string port_name = argc > 1 ? argv[1] : "/dev/ttyUSB0";
  const uint32_t baud_rate = argc > 2 ? static_cast<uint32_t>(std::stoul(argv[2])) : 115200U;

  telemetry_telecommand::PosixSerialPort serial_port;
  serial_port.set_port(port_name);
  serial_port.set_baud_rate(baud_rate);
  serial_port.set_timeout_ms(1000);

  try {
    serial_port.open();
  } catch (const std::exception & ex) {
    std::cerr << "Failed to open " << port_name << ": " << ex.what() << std::endl;
    return 1;
  }

  // 数据区字节在多帧之间持续递增：00、01、...、FF、00...
  uint8_t next_payload_byte = 0x00;

  while (true) {
    telemetry_telecommand::SerialFrame frame;
    frame.data[0] = telemetry_telecommand::FRAME_HEADER[0];
    frame.data[1] = telemetry_telecommand::FRAME_HEADER[1];

    // 填充位于帧头和 CRC 之间的 61 字节数据区。
    for (std::size_t i = 2; i < telemetry_telecommand::FRAME_LENGTH - 1; ++i) {
      frame.data[i] = next_payload_byte;
      next_payload_byte = static_cast<uint8_t>(next_payload_byte + 1);
    }

    // 最后 1 字节写入前 63 字节的校验和。
    frame.data[telemetry_telecommand::FRAME_LENGTH - 1] = frame.calculate_checksum();

    try {
      serial_port.write(frame.data.data(), frame.data.size());
    } catch (const std::exception & ex) {
      std::cerr << "Write failed on " << port_name << ": " << ex.what() << std::endl;
      return 1;
    }

    // 以 100Hz 的频率发送，方便稳定联调。
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}
