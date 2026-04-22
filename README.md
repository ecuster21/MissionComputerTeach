# MissionComputer

基于 ROS 2 Jazzy 的串口接收工程。当前项目包含 3 个串口接收节点、1 个可视化节点，以及 1 个用于整套联调的一键启动文件：

- `tm_serial_recv`：默认接收 `/dev/ttyS7`，固定 `64` 字节帧
- `c_tc_serial_recv`：默认接收 `/dev/ttyS3`，固定 `32` 字节帧
- `l_tc_serial_recv`：默认接收 `/dev/ttyS4`，固定 `32` 字节帧
- `frame_visualizer`：订阅接收到的完整帧并以十六进制打印
- `multi_serial_visualizers.launch.py`：同时启动 3 个串口接收节点和 3 个对应的可视化节点

这 3 个接收节点共用同一套串口同步与 CRC8 校验逻辑，只是默认串口、默认帧长和默认发布话题不同。

## 环境要求

- Ubuntu
- ROS 2 Jazzy
- `colcon`
- 可访问的真实串口设备，例如 `/dev/ttyS7`、`/dev/ttyS3`、`/dev/ttyS4`

## 构建

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select interfaces telemetry_telecommand
source install/setup.bash
```

## 当前协议

所有接收节点都按下面这套协议做同步与校验：

- 帧头固定：`0xEB 0x90`
- 校验类型：`CRC8`
- 支持的 CRC8 变体：
  - `crc8`
  - `maxim`
  - `sae_j1850`

不同节点的默认帧长：

- `tm_serial_recv`：`64` 字节
- `c_tc_serial_recv`：`32` 字节
- `l_tc_serial_recv`：`32` 字节

因此：

- `tm_serial_recv` 的默认帧结构是 `EB 90 + 61字节数据 + 1字节CRC8`
- `c_tc_serial_recv` 和 `l_tc_serial_recv` 的默认帧结构是 `EB 90 + 29字节数据 + 1字节CRC8`

## 运行

推荐直接使用 launch 文件一次启动 3 路串口接收和 3 路显示。

### 一键启动

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch telemetry_telecommand multi_serial_visualizers.launch.py
```

这个 launch 会同时启动：

- `tm_serial_recv`，默认读取 `/dev/ttyS7`
- `c_tc_serial_recv`，默认读取 `/dev/ttyS3`
- `l_tc_serial_recv`，默认读取 `/dev/ttyS4`
- `tm_frame_visualizer`，订阅 `tm_synced_frame`
- `c_tc_frame_visualizer`，订阅 `c_tc_synced_frame`
- `l_tc_frame_visualizer`，订阅 `l_tc_synced_frame`

如果需要改串口或公共参数，可以直接覆盖 launch 参数：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch telemetry_telecommand multi_serial_visualizers.launch.py \
  tm_port:=/dev/ttyS7 \
  c_tc_port:=/dev/ttyS3 \
  l_tc_port:=/dev/ttyS4 \
  baud_rate:=115200 \
  timeout_ms:=100 \
  crc8_variant:=crc8
```

### 手动分别启动

终端 1：启动真实硬件发送端或上位机程序，确保目标串口已经持续发数。

终端 2：按需启动接收节点。

`tm_serial_recv`

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand tm_serial_recv
```

`c_tc_serial_recv`

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand c_tc_serial_recv
```

`l_tc_serial_recv`

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand l_tc_serial_recv
```

终端 3：启动可视化节点，并指定要看的话题。

例如查看 `tm_serial_recv`：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=tm_synced_frame
```

如果串口权限不足：

```bash
sudo usermod -aG dialout $USER
```

重新登录后再试。

## 默认参数

`tm_serial_recv`

- `port`：`/dev/ttyS7`
- `frame_length`：`64`
- `crc8_variant`：`crc8`
- `topic`：`tm_synced_frame`

`c_tc_serial_recv`

- `port`：`/dev/ttyS3`
- `frame_length`：`32`
- `crc8_variant`：`crc8`
- `topic`：`c_tc_synced_frame`

`l_tc_serial_recv`

- `port`：`/dev/ttyS4`
- `frame_length`：`32`
- `crc8_variant`：`crc8`
- `topic`：`l_tc_synced_frame`

3 个接收节点都还支持：

- `baud_rate`：默认 `115200`
- `timeout_ms`：默认 `100`

`frame_visualizer` 支持：

- `topic`：订阅话题名，默认 `tm_synced_frame`

`multi_serial_visualizers.launch.py` 支持：

- `tm_port`：默认 `/dev/ttyS7`
- `c_tc_port`：默认 `/dev/ttyS3`
- `l_tc_port`：默认 `/dev/ttyS4`
- `baud_rate`：默认 `115200`
- `timeout_ms`：默认 `100`
- `crc8_variant`：默认 `crc8`

## 发布消息

消息类型：`interfaces/msg/SyncedFrame`

- `frame_data`：完整原始帧，长度由对应节点决定
- `timestamp_ns`：本地接收时间戳，单位纳秒

`frame_data` 已改成变长数组，因此可以同时承载 `64` 字节和 `32` 字节帧。

## 常见问题

如果日志显示：

```text
Frame synchronization established.
Frame validation failed.
```

通常表示：

- 帧头和帧长已经对上了
- 但 `crc8_variant` 配置不对，或者发送端数据内容本身有误

当前接收节点会在校验失败时打印：

- 当前使用的 CRC8 配置
- 收到的 CRC8
- 多种 CRC8 计算结果
- 整帧十六进制内容

可以根据这条日志快速判断应该切换到哪一种 `crc8_variant`。

## 实现说明

- 串口底层使用仓库内置的 POSIX `termios` 封装
- 3 个接收节点都复用了同一个 `FixedFrameSerialReceiver`
- 接收线程采用“搜索双帧头建立同步 -> 固定帧长取帧 -> 校验失败重同步”的方式工作
