#include "rclcpp/rclcpp.hpp"
#include "interfaces/msg/telemetry_frame.hpp"
#include <libserial/SerialPort.h>
#include <vector>
#include <cstdint>
#include <chrono>
#include <random>

// 帧头定义
constexpr uint16_t FRAME_HEADER = 0xEB90;
// 帧长度
constexpr size_t FRAME_LENGTH = 64;
// 数据部分长度
constexpr size_t DATA_LENGTH = 60;

class SerialSimulatorNode : public rclcpp::Node {
public:
    SerialSimulatorNode() : Node("serial_simulator") {
        serial_port_ = this->declare_parameter<std::string>("serial_port", "/dev/ttyS7");

        // 配置串口
        configure_serial();
        
        // 设置定时器
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&SerialSimulatorNode::send_simulated_data, this));
    }

private:
    void configure_serial() {
        try {
            serial_.Open(serial_port_);
            serial_.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
            serial_.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
            serial_.SetParity(LibSerial::Parity::PARITY_NONE);
            serial_.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
            serial_.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "无法打开串口: %s", e.what());
            return;
        }

        RCLCPP_INFO(this->get_logger(), "已打开串口: %s", serial_port_.c_str());
    }

    void send_simulated_data() {
        if (!serial_.IsOpen()) {
            return;
        }

        // 生成模拟数据
        std::vector<uint8_t> frame(FRAME_LENGTH);
        
        // 帧头
        frame[0] = (FRAME_HEADER >> 8) & 0xFF;
        frame[1] = FRAME_HEADER & 0xFF;
        
        // 随机数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < DATA_LENGTH; ++i) {
            frame[2 + i] = dis(gen);
        }
        
        // 计算校验和
        uint8_t checksum = 0;
        for (size_t i = 0; i < FRAME_LENGTH - 1; ++i) {
            checksum += frame[i];
        }
        frame[FRAME_LENGTH - 1] = checksum;
        
        // 发送数据
        try {
            serial_.Write(frame);
            serial_.DrainWriteBuffer();
            RCLCPP_INFO(this->get_logger(), "发送模拟数据帧");
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "发送数据错误: %s", e.what());
        }
    }

    rclcpp::TimerBase::SharedPtr timer_;
    std::string serial_port_;
    LibSerial::SerialPort serial_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<SerialSimulatorNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
