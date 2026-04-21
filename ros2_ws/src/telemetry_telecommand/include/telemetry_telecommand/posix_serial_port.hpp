#ifndef TELEMETRY_TELECOMMAND__POSIX_SERIAL_PORT_HPP_
#define TELEMETRY_TELECOMMAND__POSIX_SERIAL_PORT_HPP_

#include <cstddef>
#include <cstdint>
#include <string>

namespace telemetry_telecommand
{

// 轻量级 POSIX 串口封装，避免依赖额外第三方串口库。
class PosixSerialPort
{
public:
  PosixSerialPort() = default;
  ~PosixSerialPort();

  // 在 open() 前设置串口参数。
  void set_port(const std::string & port_name);
  void set_baud_rate(uint32_t baud_rate);
  void set_timeout_ms(uint32_t timeout_ms);

  // 基于文件描述符风格的基本生命周期接口。
  bool is_open() const;
  void open();
  void close();

  // 基于 termios 超时配置的阻塞式读写接口。
  std::size_t read(uint8_t * buffer, std::size_t size) const;
  std::size_t write(const uint8_t * buffer, std::size_t size) const;

private:
  // 已打开 tty 设备对应的 Linux 文件描述符。
  int fd_{-1};
  std::string port_name_{"/dev/ttyUSB0"};
  uint32_t baud_rate_{115200};
  uint32_t timeout_ms_{100};

  // 应用原始模式、波特率、8N1 和超时等串口配置。
  void configure_port() const;
};

}  // telemetry_telecommand 命名空间

#endif  // TELEMETRY_TELECOMMAND__POSIX_SERIAL_PORT_HPP_
