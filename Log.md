# Log

用于记录阶段性进展。每次收尾都追加一条记录，并写清楚：

- 这次做了什么
- 改了哪些文件
- 下一步做什么

## 2026-04-21

### 这次做了什么

- 统一整理了项目入口文档，补充了项目结构、运行方法、文档维护约定和相关文档入口
- 新建了 `Log.md` 与 `Memory.md`，建立后续协作记录规则
- 在 `docs/` 下新增文档总览和一份可直接复用的 Codex 项目启动提示词

### 改了哪些文件

- `README.md`
- `Log.md`
- `Memory.md`
- `docs/README.md`
- `docs/codex_start_prompt.md`

### 下一步做什么

- 根据实际开发目标继续补充专题文档到 `docs/`
- 开始功能开发时使用 `docs/codex_start_prompt.md` 作为基础提示词
- 随着项目推进持续更新 `README.md`、`Log.md` 和必要的说明文档

## 2026-04-23

### 这次做了什么

- 将 64 字节接收节点从 `tm_serial_recv` 统一重命名为 `fc_tm_serial_recv`
- 将对应默认话题从 `tm_synced_frame` 改为 `fc_tm_synced_frame`
- 将 launch 参数、可视化节点名、默认 topic 和文档说明同步到 `fc_tm_*` 命名

### 改了哪些文件

- `README.md`
- `docs/codex_start_prompt.md`
- `docs/frame_sync/README.md`
- `docs/frame_sync/01_总流程.md`
- `ros2_ws/src/telemetry_telecommand/CMakeLists.txt`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp`
- `ros2_ws/src/telemetry_telecommand/launch/multi_serial_visualizers.launch.py`
- `ros2_ws/src/telemetry_telecommand/src/fc_tm_serial_recv.cpp`
- `ros2_ws/src/telemetry_telecommand/src/frame_visualizer.cpp`

### 下一步做什么

- 重新构建并运行 launch，确认 `fc_tm_serial_recv` 和 `fc_tm_frame_visualizer` 正常启动
- 若外部脚本仍使用旧参数 `tm_port` 或旧话题 `tm_synced_frame`，同步更新为 `fc_tm_port` 和 `fc_tm_synced_frame`
