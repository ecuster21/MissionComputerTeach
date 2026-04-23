# 帧同步算法图解

这个目录把当前公共接收类 `FixedFrameSerialReceiver` 中的接收、同步、校验和重同步流程拆成了几张独立的 Mermaid 图。

对应源码：

- [fixed_frame_serial_receiver.cpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp:1)
- [fixed_frame_serial_receiver.hpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/fixed_frame_serial_receiver.hpp:1)
- [frame_structures.hpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp:1)

## 当前实现背景

项目里有 3 个接收节点：

- `fc_tm_serial_recv`：默认 `64` 字节帧
- `c_tc_serial_recv`：默认 `32` 字节帧
- `l_tc_serial_recv`：默认 `32` 字节帧

它们都复用同一个 `FixedFrameSerialReceiver`，只是默认参数不同。

帧布局为 `EB 90 + frame_type + source_id + destination_id + data + CRC8`。同步和 CRC8 校验通过后，接收节点还会按 `destination_ids` 和 `handled_frame_types` 参数过滤，通过后才发布。

## 文件说明

- [01_总流程.md](/home/zkxt/MissionComputer/docs/frame_sync/01_总流程.md)
  接收线程从启动到发布帧的完整流程
- [02_寻找同步点.md](/home/zkxt/MissionComputer/docs/frame_sync/02_寻找同步点.md)
  `try_find_sync()` 如何在当前有效缓冲区里寻找可信同步点
- [03_已同步后处理.md](/home/zkxt/MissionComputer/docs/frame_sync/03_已同步后处理.md)
  已同步后如何在“缓冲区切帧模式”和“固定 frame_length 读帧模式”之间切换
- [04_校验失败重同步.md](/home/zkxt/MissionComputer/docs/frame_sync/04_校验失败重同步.md)
  校验失败后当前实现如何恢复同步
- [05_字节流示例.md](/home/zkxt/MissionComputer/docs/frame_sync/05_字节流示例.md)
  用简化字节流说明为什么双帧头判断更稳健
