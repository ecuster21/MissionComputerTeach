# 帧同步算法图解

这个目录把 `serial_receiver.cpp` 中的帧同步流程拆成了几张独立的 Mermaid 图，避免单张图过大看不清。

对应源码：

- [serial_receiver.cpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/src/serial_receiver.cpp:1)
- [frame_structures.hpp](/home/zkxt/MissionComputer/ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp:1)

## 文件说明

- [01_总流程.md](/home/zkxt/MissionComputer/docs/frame_sync/01_总流程.md)
  接收线程从启动到发布帧的完整流程
- [02_寻找同步点.md](/home/zkxt/MissionComputer/docs/frame_sync/02_寻找同步点.md)
  `try_find_sync()` 的详细判断逻辑
- [03_已同步后处理.md](/home/zkxt/MissionComputer/docs/frame_sync/03_已同步后处理.md)
  已经对齐后如何取帧、校验并发布
- [04_校验失败重同步.md](/home/zkxt/MissionComputer/docs/frame_sync/04_校验失败重同步.md)
  帧校验失败时如何恢复同步

## 查看建议

如果你在本机看图，推荐下面两种方式：

1. 用 VS Code 打开这些 `.md` 文件，然后使用 Markdown Preview
2. 直接打开每个文件里的 Mermaid 代码块，复制到 Mermaid Live Editor 中查看

如果你希望我继续帮你导出成 `svg` 或 `png`，我也可以继续做；当前环境里只是还没有现成的 Mermaid 导出工具。
