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

## 2026-04-23

### 这次做了什么

- 明确帧头后 3 字节为帧类型、源 ID、目的 ID
- 在同步和 CRC8 校验通过后新增目的 ID 与帧类型过滤
- 新增可配置参数 `destination_ids` 和 `handled_frame_types`，支持启动时配置和运行时参数更新，参数值可写字符串、单个整数或整数数组
- 默认处理帧类型为 `0C,0D,10,11`，目的 ID 默认空表示联调阶段接收任意目的 ID

### 改了哪些文件

- `README.md`
- `Memory.md`
- `Log.md`
- `docs/codex_start_prompt.md`
- `docs/frame_sync/README.md`
- `docs/frame_sync/01_总流程.md`
- `docs/frame_sync/03_已同步后处理.md`
- `docs/frame_sync/04_校验失败重同步.md`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/fixed_frame_serial_receiver.hpp`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp`
- `ros2_ws/src/telemetry_telecommand/launch/multi_serial_visualizers.launch.py`
- `ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp`

### 下一步做什么

- 实机运行时传入本机机号，例如 `destination_ids:=1`
- 后续新增需要处理的帧类型时，只更新 `handled_frame_types` 参数

## 2026-04-23

### 这次做了什么

- 在 CRC8 前新增 1 字节协议时间戳字段
- 保持固定总帧长 `64` 和 `32` 不变，数据区相应缩短为 `57` 字节和 `25` 字节
- 明确 CRC8 计算范围包含协议时间戳，不包含最后 CRC8 字节

### 改了哪些文件

- `README.md`
- `Memory.md`
- `Log.md`
- `docs/codex_start_prompt.md`
- `docs/frame_sync/README.md`
- `docs/frame_sync/05_字节流示例.md`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp`
- `ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp`

### 下一步做什么

- 使用新的 PC 端测试脚本发送含 1 字节协议时间戳的帧
- 实机观察可视化输出，确认 CRC 校验和目的 ID、帧类型过滤都正常

## 2026-04-23

### 这次做了什么

- 新增 `serial_storage` 存储节点，订阅 `fc_tm_synced_frame`、`c_tc_synced_frame`、`l_tc_synced_frame`
- 使用两个 `4096` 字节缓冲区按消息到达顺序缓存 `frame_data`
- 缓冲区满后交给后台线程写入同一个存储文件，节点退出时刷新剩余数据
- 将 `serial_storage` 接入 CMake 构建和 `multi_serial_visualizers.launch.py`

### 改了哪些文件

- `README.md`
- `Memory.md`
- `Log.md`
- `docs/codex_start_prompt.md`
- `ros2_ws/src/telemetry_telecommand/CMakeLists.txt`
- `ros2_ws/src/telemetry_telecommand/launch/multi_serial_visualizers.launch.py`
- `ros2_ws/src/telemetry_telecommand/src/serial_storage.cpp`

### 下一步做什么

- 实机启动 launch，确认生成 `serial_storage.bin`
- 根据实际落盘路径需要调整 `storage_file` 参数

## 2026-04-27

### 这次做了什么

- 给项目核心源码补充中文注释，说明协议字段、同步策略、过滤逻辑、串口配置和双缓存存储流程
- 给 `fc_tm/c_tc/l_tc` 三个入口节点补充默认通道职责说明
- 给 `frame_visualizer` 和 `serial_storage` 补充节点行为边界说明

### 改了哪些文件

- `Log.md`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/fixed_frame_serial_receiver.hpp`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp`
- `ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp`
- `ros2_ws/src/telemetry_telecommand/src/serial_storage.cpp`
- `ros2_ws/src/telemetry_telecommand/src/posix_serial_port.cpp`
- `ros2_ws/src/telemetry_telecommand/src/frame_visualizer.cpp`
- `ros2_ws/src/telemetry_telecommand/src/fc_tm_serial_recv.cpp`
- `ros2_ws/src/telemetry_telecommand/src/c_tc_serial_recv.cpp`
- `ros2_ws/src/telemetry_telecommand/src/l_tc_serial_recv.cpp`

### 下一步做什么

- 后续协议字段或存储格式变化时，继续同步更新相邻注释和 README

## 2026-04-27

### 这次做了什么

- 整理 `docs/` 目录，将流程图统一移动到 `docs/flows/`
- 将原 `docs/frame_sync/` 下 5 张过细流程图合并为 3 张：整体链路、固定帧接收、双缓存存储
- 将 Codex 启动提示词移动到 `docs/prompts/`
- 删除旧的 `docs/frame_sync/` 文档文件，减少重复维护点

### 改了哪些文件

- `README.md`
- `Memory.md`
- `Log.md`
- `docs/README.md`
- `docs/flows/README.md`
- `docs/flows/serial_pipeline.md`
- `docs/flows/receiver_sync.md`
- `docs/flows/storage_double_buffer.md`
- `docs/prompts/README.md`
- `docs/prompts/codex_start_prompt.md`
- `docs/frame_sync/README.md`
- `docs/frame_sync/01_总流程.md`
- `docs/frame_sync/02_寻找同步点.md`
- `docs/frame_sync/03_已同步后处理.md`
- `docs/frame_sync/04_校验失败重同步.md`
- `docs/frame_sync/05_字节流示例.md`

### 下一步做什么

- 后续新增文档时，优先判断是否能归入 `flows/` 或 `prompts/`，避免过早拆分目录

## 2026-04-27

### 这次做了什么

- 将串口接收层调整为同步和 CRC8 通过后只按目的 ID 过滤，符合本机号就发布完整原始帧
- 新增 `interfaces/msg/CanFrame.msg`
- 新增 `c_can_pub` 和 `l_can_pub`，订阅 C/L 链路完整帧，按目的 ID 与可配置帧类型过滤后提取两包 CAN 数据并发布
- 将 CAN 提取节点接入 CMake 和 `multi_serial_visualizers.launch.py`
- 同步更新 README、Memory 和 docs 流程说明
- 完成构建、接口、launch 参数和 `c_can_pub` 话题级冒烟测试

### 改了哪些文件

- `README.md`
- `Memory.md`
- `Log.md`
- `docs/README.md`
- `docs/flows/README.md`
- `docs/flows/serial_pipeline.md`
- `docs/flows/receiver_sync.md`
- `docs/flows/storage_double_buffer.md`
- `docs/prompts/codex_start_prompt.md`
- `ros2_ws/src/interfaces/CMakeLists.txt`
- `ros2_ws/src/interfaces/package.xml`
- `ros2_ws/src/interfaces/msg/CanFrame.msg`
- `ros2_ws/src/telemetry_telecommand/CMakeLists.txt`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/can_frame_publisher.hpp`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/fixed_frame_serial_receiver.hpp`
- `ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp`
- `ros2_ws/src/telemetry_telecommand/launch/multi_serial_visualizers.launch.py`
- `ros2_ws/src/telemetry_telecommand/package.xml`
- `ros2_ws/src/telemetry_telecommand/src/c_can_pub.cpp`
- `ros2_ws/src/telemetry_telecommand/src/can_frame_publisher.cpp`
- `ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp`
- `ros2_ws/src/telemetry_telecommand/src/l_can_pub.cpp`

### 下一步做什么

- 联调时根据本机号设置 `destination_ids`，根据业务帧类型维护 `handled_frame_types`
- 如果后续 CANID 需要按无符号 `0..65535` 显示，建议把 `CanFrame.msg` 的 `id` 从 `int16` 调整为 `uint16`

## 2026-04-29

### 这次做了什么

- 将 `CanFrame.msg` 的 `id` 从 `int8` 改为 `int16`，用于承载 2 字节 CANID
- 将 `c_can_pub` 和 `l_can_pub` 的 CANID 发布逻辑改为发布完整 2 字节 CANID，不再只发布低 8 位
- 复核 `serial_storage`，确认它仍订阅 `fc_tm_synced_frame`、`c_tc_synced_frame`、`l_tc_synced_frame`，只写 `SyncedFrame.frame_data` 原始帧字节
- 将存储节点注释中的过滤描述修正为目的 ID 过滤
- 完成构建、接口检查和 `c_can_pub` 冒烟测试，确认 CANID `0x0123` 发布为 `291`

### 改了哪些文件

- `README.md`
- `Log.md`
- `ros2_ws/src/interfaces/msg/CanFrame.msg`
- `ros2_ws/src/telemetry_telecommand/src/can_frame_publisher.cpp`
- `ros2_ws/src/telemetry_telecommand/src/serial_storage.cpp`

### 下一步做什么

- 联调时继续确认实际 CANID 字节序；当前按高字节在前解析
