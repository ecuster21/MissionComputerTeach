# 帧同步算法图解

这个目录把当前 `serial_receiver.cpp` 中的接收、同步、校验和重同步流程拆成了几张独立的 Mermaid 图。

对应源码：

- [serial_receiver.cpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/src/serial_receiver.cpp:1)
- [frame_structures.hpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp:1)

## 当前协议背景

当前协议定义在 `frame_structures.hpp`：

- 帧头 `EB 90`
- 帧总长 `64`
- 数据区 `60` 字节
- CRC16 `2` 字节

## 文件说明

- [01_总流程.md](/home/zkxt/MissionComputer/docs/frame_sync/01_总流程.md)
  接收线程从启动到发布帧的完整流程
- [02_寻找同步点.md](/home/zkxt/MissionComputer/docs/frame_sync/02_寻找同步点.md)
  `try_find_sync()` 如何在当前有效缓冲区里寻找可信同步点
- [03_已同步后处理.md](/home/zkxt/MissionComputer/docs/frame_sync/03_已同步后处理.md)
  已同步后如何在“缓冲区切帧模式”和“固定 64 字节读帧模式”之间切换
- [04_校验失败重同步.md](/home/zkxt/MissionComputer/docs/frame_sync/04_校验失败重同步.md)
  校验失败后当前实现如何恢复同步
- [05_字节流示例.md](/home/zkxt/MissionComputer/docs/frame_sync/05_字节流示例.md)
  用简化字节流说明为什么双帧头判断更稳健

## 查看建议

如果你在本机看图，推荐下面两种方式：

1. 用 VS Code 打开这些 `.md` 文件，然后使用 Markdown Preview
2. 把 Mermaid 代码块复制到 Mermaid Live Editor 中查看
