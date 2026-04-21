cmake_minimum_required(VERSION 3.8)
project(telemetry_telecommand)

# 为本包内所有目标统一开启常见编译告警。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 节点实现使用了 C++17 特性。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找核心 ROS 2 依赖和自定义消息包。
find_package(ament_cmake REQUIRED)
find_package(interfaces REQUIRED)
find_package(rclcpp REQUIRED)

# 发送端和接收端共用的串口实现库。
add_library(serial_support
  src/posix_serial_port.cpp
)
target_include_directories(serial_support
  PUBLIC
    include
)

add_executable(serial_receiver
  src/serial_receiver.cpp
)
target_include_directories(serial_receiver
  PRIVATE
    include
)
ament_target_dependencies(serial_receiver
  interfaces
  rclcpp
)
target_link_libraries(serial_receiver serial_support)

# 持续向串口发送协议帧的测试发送程序。
add_executable(mock_serial_sender
  src/mock_serial_sender.cpp
)
target_include_directories(mock_serial_sender
  PRIVATE
    include
)
target_link_libraries(mock_serial_sender serial_support)

# 以十六进制形式打印接收帧的调试工具。
add_executable(frame_visualizer
  src/frame_visualizer.cpp
)
target_include_directories(frame_visualizer
  PRIVATE
    include
)
ament_target_dependencies(frame_visualizer
  interfaces
  rclcpp
)

install(TARGETS
  serial_support
  serial_receiver
  mock_serial_sender
  frame_visualizer
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

# 导出依赖，便于其他包复用本包能力。
ament_export_dependencies(interfaces rclcpp)
ament_package()
