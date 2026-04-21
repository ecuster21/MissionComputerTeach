#include "telemetry_telecommand/posix_serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace telemetry_telecommand
{
namespace
{

// 将用户输入的波特率数值转换成 termios 所需的系统常量。
speed_t map_baud_rate(uint32_t baud_rate)
{
  switch (baud_rate) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 921600:
      return B921600;
    default:
      throw std::runtime_error("Unsupported baud rate: " + std::to_string(baud_rate));
  }
}

// 在抛出底层串口异常时保留 errno 对应的错误信息。
std::string last_error_message(const std::string & prefix)
{
  return prefix + ": " + std::strerror(errno);
}

}  // 匿名命名空间

PosixSerialPort::~PosixSerialPort()
{
  close();
}

void PosixSerialPort::set_port(const std::string & port_name)
{
  port_name_ = port_name;
}

void PosixSerialPort::set_baud_rate(uint32_t baud_rate)
{
  baud_rate_ = baud_rate;
}

void PosixSerialPort::set_timeout_ms(uint32_t timeout_ms)
{
  timeout_ms_ = timeout_ms;
}

bool PosixSerialPort::is_open() const
{
  return fd_ >= 0;
}

void PosixSerialPort::open()
{
  if (is_open()) {
    return;
  }

  // 以原始串口设备方式打开 tty，而不是作为控制终端。
  fd_ = ::open(port_name_.c_str(), O_RDWR | O_NOCTTY);
  if (fd_ < 0) {
    throw std::runtime_error(last_error_message("Unable to open serial port " + port_name_));
  }

  try {
    // 打开后立即配置波特率、超时和 8N1 等参数。
    configure_port();
  } catch (...) {
    close();
    throw;
  }
}

void PosixSerialPort::close()
{
  if (!is_open()) {
    return;
  }

  ::close(fd_);
  fd_ = -1;
}

std::size_t PosixSerialPort::read(uint8_t * buffer, std::size_t size) const
{
  if (!is_open()) {
    throw std::runtime_error("Serial port is not open");
  }

  const auto result = ::read(fd_, buffer, size);
  if (result < 0) {
    throw std::runtime_error(last_error_message("Serial read failed"));
  }

  return static_cast<std::size_t>(result);
}

std::size_t PosixSerialPort::write(const uint8_t * buffer, std::size_t size) const
{
  if (!is_open()) {
    throw std::runtime_error("Serial port is not open");
  }

  std::size_t total_written = 0;
  // 循环写入，直到整帧数据全部送入设备。
  while (total_written < size) {
    const auto result = ::write(fd_, buffer + total_written, size - total_written);
    if (result < 0) {
      throw std::runtime_error(last_error_message("Serial write failed"));
    }
    total_written += static_cast<std::size_t>(result);
  }

  return total_written;
}

void PosixSerialPort::configure_port() const
{
  termios tty{};
  if (tcgetattr(fd_, &tty) != 0) {
    throw std::runtime_error(last_error_message("tcgetattr failed"));
  }

  // 使用原始模式，避免内核修改遥测字节流内容。
  cfmakeraw(&tty);
  tty.c_cflag |= (CLOCAL | CREAD);
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cc[VMIN] = 0;
  // termios 的超时单位是十分之一秒。
  tty.c_cc[VTIME] = static_cast<cc_t>((timeout_ms_ + 99U) / 100U);

  const auto speed = map_baud_rate(baud_rate_);
  if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
    throw std::runtime_error(last_error_message("Failed to set baud rate"));
  }

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    throw std::runtime_error(last_error_message("tcsetattr failed"));
  }

  if (tcflush(fd_, TCIOFLUSH) != 0) {
    throw std::runtime_error(last_error_message("tcflush failed"));
  }
}

}  // telemetry_telecommand 命名空间
