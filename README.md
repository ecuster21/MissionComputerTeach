# MissionComputer

`MissionComputer` 是一个基于 ROS 2 Jazzy 的串口接收工程，用于从真实串口读取固定长度二进制帧，完成帧同步与 CRC16 校验，并将有效帧发布为 ROS 2 消息供下游使用。

当前项目只保留两个可执行程序：

- `serial_receiver`：从真实串口读取字节流，完成帧同步、CRC 校验并发布 ROS 2 消息
- `frame_visualizer`：订阅接收结果并以十六进制打印完整帧

项目内已经不再包含模拟发送器、虚拟串口脚本或串口仿真节点。

## 项目结构

- `ros2_ws/`：ROS 2 工作区
- `ros2_ws/src/interfaces`：自定义消息定义
- `ros2_ws/src/telemetry_telecommand`：串口接收与可视化节点实现
- `docs/`：专题说明文档、流程图、提示词等资料
- `Log.md`：每次收尾的工作记录
- `Memory.md`：长期有效规则与固定工作流

## 环境要求

- Ubuntu
- ROS 2 Jazzy
- `colcon`
- 可访问的真实串口设备，例如 `/dev/ttyS7`

## 构建

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select interfaces telemetry_telecommand
source install/setup.bash
```

## 运行

终端 1：启动真实硬件发送端或你的上位机程序，确保它已经持续向目标串口发送协议帧。

终端 2：启动接收节点。

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand serial_receiver --ros-args -p port:=/dev/ttyS7 -p baud_rate:=115200 -p timeout_ms:=100 -p crc16_variant:=ibm -p crc16_big_endian:=true -p topic:=synced_frame
```

终端 3：启动可视化节点。

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=synced_frame
```

如果串口权限不足：

```bash
sudo usermod -aG dialout $USER
```

重新登录后再试。

## 当前协议

当前代码里的帧定义如下：

- 帧头：`0xEB 0x90`
- 总长度：`64` 字节
- 数据区：`60` 字节
- CRC：`2` 字节

即：

```text
EB 90 + 60字节数据 + 2字节CRC16
```

接收节点当前支持这些 CRC16 变体：

- `ccitt_false`
- `modbus`
- `ibm`
- `x25`

当前硬件联调示例使用：

- `crc16_variant:=ibm`
- `crc16_big_endian:=true`

## 参数

`serial_receiver` 支持：

- `port`：串口路径，默认 `/dev/ttyS7`
- `baud_rate`：波特率，默认 `115200`
- `timeout_ms`：读超时，默认 `100`
- `crc16_variant`：CRC16 变体，默认 `ccitt_false`
- `crc16_big_endian`：CRC 字节序，默认 `true`
- `topic`：发布话题，默认 `synced_frame`

`frame_visualizer` 支持：

- `topic`：订阅话题，默认 `synced_frame`

## 发布消息

- 话题默认名：`/synced_frame`
- 消息类型：`interfaces/msg/SyncedFrame`
- `frame_data`：原始 `64` 字节完整帧
- `timestamp_ns`：本地接收时间戳，单位纳秒

## 文档维护约定

- `README.md`：维护项目入口信息、结构概览、运行方式和关键说明
- `Log.md`：每次收尾都追加记录，写清楚“做了什么 / 改了哪些文件 / 下一步做什么”
- `Memory.md`：只记录长期有效内容，例如用户偏好、命名规范、固定工作流
- `docs/`：存放专题说明、设计记录、流程图、提示词和补充文档

如果项目结构、运行方式、串口协议、调试流程发生变化，应同步更新这些文档。

## 相关文档

- [docs/README.md](/home/zkxt/MissionComputer/docs/README.md)
- [docs/codex_start_prompt.md](/home/zkxt/MissionComputer/docs/codex_start_prompt.md)
- [docs/frame_sync/README.md](/home/zkxt/MissionComputer/docs/frame_sync/README.md)
- [Log.md](/home/zkxt/MissionComputer/Log.md)
- [Memory.md](/home/zkxt/MissionComputer/Memory.md)

## 常见问题

如果日志显示：

```text
Frame synchronization established.
Frame validation failed.
```

通常表示：

- 帧头和帧长已经对上了
- 但 `crc16_variant` 或 `crc16_big_endian` 配置不对

当前接收节点会在校验失败时打印：

- 当前使用的 CRC 配置
- 收到的 CRC 大端/小端解释值
- 多种 CRC16 计算结果
- 原始 `64` 字节帧内容

可以根据这条日志快速判断应该切换到哪一种 CRC 参数。

## 实现说明

- 串口底层使用仓库内置的 POSIX `termios` 封装
- 接收线程采用“搜索双帧头建立同步 -> 固定帧长取帧 -> 校验失败重同步”的方式工作
- 校验通过的完整帧会发布到 ROS 2 话题
