cmake_minimum_required(VERSION 3.8)
project(interfaces)

# 为接口包启用常见编译告警，便于尽早发现配置问题。
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找消息生成所需的 ROS 2 构建依赖。
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/SyncedFrame.msg"
)

# 导出运行时依赖，供其他包使用生成后的消息类型。
ament_export_dependencies(rosidl_default_runtime)
ament_package()
