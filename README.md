# MissionComputer

基于 ROS 2 Jazzy 的串口接收工程。当前项目包含 3 个串口接收节点、1 个可视化节点、1 个存储节点，以及 1 个用于整套联调的一键启动文件：

- `fc_tm_serial_recv`：默认接收 `/dev/ttyS7`，对应 PC 端 `COM1`，固定 `64` 字节帧
- `c_tc_serial_recv`：默认接收 `/dev/ttyS3`，对应 PC 端 `COM6`，固定 `32` 字节帧
- `l_tc_serial_recv`：默认接收 `/dev/ttyS4`，对应 PC 端 `COM7`，固定 `32` 字节帧
- `frame_visualizer`：订阅接收到的完整帧并以十六进制打印
- `serial_storage`：订阅 3 路过滤后的完整帧，使用两个 4KB 缓冲区按到达顺序写入同一个存储文件
- `multi_serial_visualizers.launch.py`：同时启动 3 个串口接收节点、3 个对应的可视化节点和 1 个存储节点

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

## 文档

- [docs/README.md](/home/zkxt/MissionComputer/docs/README.md)：文档目录总览
- [docs/flows/README.md](/home/zkxt/MissionComputer/docs/flows/README.md)：运行链路、接收同步和双缓存存储流程图
- [docs/prompts/README.md](/home/zkxt/MissionComputer/docs/prompts/README.md)：项目协作提示词

## 当前协议

所有接收节点都按下面这套协议做同步与校验：

- 帧头固定：`0xEB 0x90`
- 帧头后 3 个字节依次是：`frame_type`、`source_id`、`destination_id`
- CRC8 前 1 个字节是协议时间戳
- 校验类型：`CRC8`
- 支持的 CRC8 变体：
  - `crc8`
  - `maxim`
  - `sae_j1850`

完整帧布局：

```text
EB 90 + 1字节帧类型 + 1字节源ID + 1字节目的ID + 数据区 + 1字节时间戳 + 1字节CRC8
```

CRC8 计算范围是从帧头 `EB 90` 到 1 字节时间戳为止，不包含最后的 CRC8 字节。

同步和 CRC8 校验通过后，接收节点会继续做业务过滤：

- `destination_id` 必须命中 `destination_ids`
- `frame_type` 必须命中 `handled_frame_types`
- `destination_ids` 为空时表示接收任意目的 ID，适合尚未配置本机号的联调阶段
- `handled_frame_types` 为空时表示接收任意帧类型

不同节点的默认帧长：

- `fc_tm_serial_recv`：`64` 字节
- `c_tc_serial_recv`：`32` 字节
- `l_tc_serial_recv`：`32` 字节

因此默认结构是：

- `fc_tm_serial_recv`：`EB 90 + 帧类型1字节 + 源ID1字节 + 目的ID1字节 + 数据57字节 + 时间戳1字节 + CRC8 1字节`
- `c_tc_serial_recv` 和 `l_tc_serial_recv`：`EB 90 + 帧类型1字节 + 源ID1字节 + 目的ID1字节 + 数据25字节 + 时间戳1字节 + CRC8 1字节`

## 运行

推荐直接使用 launch 文件一次启动 3 路串口接收、3 路显示和 1 路存储。

### 一键启动

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch telemetry_telecommand multi_serial_visualizers.launch.py
```

这个 launch 会同时启动：

- `fc_tm_serial_recv`，默认读取 `/dev/ttyS7`
- `c_tc_serial_recv`，默认读取 `/dev/ttyS3`
- `l_tc_serial_recv`，默认读取 `/dev/ttyS4`
- `fc_tm_frame_visualizer`，订阅 `fc_tm_synced_frame`
- `c_tc_frame_visualizer`，订阅 `c_tc_synced_frame`
- `l_tc_frame_visualizer`，订阅 `l_tc_synced_frame`
- `serial_storage`，订阅上述 3 个话题并写入单个存储文件

如果需要改串口或公共参数，可以直接覆盖 launch 参数：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch telemetry_telecommand multi_serial_visualizers.launch.py \
  fc_tm_port:=/dev/ttyS7 \
  c_tc_port:=/dev/ttyS3 \
  l_tc_port:=/dev/ttyS4 \
  baud_rate:=115200 \
  timeout_ms:=100 \
  crc8_variant:=crc8 \
  destination_ids:=1 \
  handled_frame_types:=0C,0D,10,11 \
  storage_file:=serial_storage.bin
```

### 手动分别启动

终端 1：启动真实硬件发送端或上位机程序，确保目标串口已经持续发数。

终端 2：按需启动接收节点。

`fc_tm_serial_recv`

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand fc_tm_serial_recv
```

如果要在手动启动时指定本机号和帧类型：

```bash
ros2 run telemetry_telecommand fc_tm_serial_recv --ros-args \
  -p destination_ids:=1 \
  -p handled_frame_types:=0C,0D,10,11
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

例如查看 `fc_tm_serial_recv`：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=fc_tm_synced_frame
```

如果只启动存储节点：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand serial_storage --ros-args \
  -p storage_file:=serial_storage.bin
```

如果串口权限不足：

```bash
sudo usermod -aG dialout $USER
```

重新登录后再试。

## 默认参数

`fc_tm_serial_recv`

- `port`：`/dev/ttyS7`
- `frame_length`：`64`
- `crc8_variant`：`crc8`
- `topic`：`fc_tm_synced_frame`

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
- `destination_ids`：目的 ID 过滤列表，默认空字符串，表示任意目的 ID
- `handled_frame_types`：需要处理的帧类型列表，默认 `0C,0D,10,11`

这两个过滤参数支持字符串、单个整数或整数数组。字符串写法中，`destination_ids` 默认按十进制解析，支持用 `0x` 写十六进制；`handled_frame_types` 默认按十六进制解析。多个值可以用逗号、空格或分号分隔，例如 `destination_ids:=1,2,0x24`、`handled_frame_types:=0C,0D,10,11`。整数数组写法可以用 `destination_ids:=[1,2,36]`。注意帧类型 `0D` 的第一位是数字 `0`。

`frame_visualizer` 支持：

- `topic`：订阅话题名，默认 `fc_tm_synced_frame`

`serial_storage` 支持：

- `storage_file`：单个输出文件路径，默认 `serial_storage.bin`
- `truncate_file`：启动时是否清空旧文件，默认 `true`
- `fc_tm_topic`：默认 `fc_tm_synced_frame`
- `c_tc_topic`：默认 `c_tc_synced_frame`
- `l_tc_topic`：默认 `l_tc_synced_frame`

`serial_storage` 只存储 `SyncedFrame.frame_data` 原始帧字节，不额外写入 ROS 时间戳或话题名。3 路数据谁先到就先进入双缓存，没有固定先后顺序。内部有两个 `4096` 字节缓冲区，当前缓冲区满后交给后台线程写盘，另一个缓冲区继续接收；节点退出时会把未满 4KB 的剩余数据也刷新到同一个文件。

`multi_serial_visualizers.launch.py` 支持：

- `fc_tm_port`：默认 `/dev/ttyS7`
- `c_tc_port`：默认 `/dev/ttyS3`
- `l_tc_port`：默认 `/dev/ttyS4`
- `baud_rate`：默认 `115200`
- `timeout_ms`：默认 `100`
- `crc8_variant`：默认 `crc8`
- `destination_ids`：默认空字符串
- `handled_frame_types`：默认 `0C,0D,10,11`
- `storage_file`：默认 `serial_storage.bin`
- `truncate_storage_file`：默认 `true`

## 发布消息

消息类型：`interfaces/msg/SyncedFrame`

- `frame_data`：完整原始帧，长度由对应节点决定
- `timestamp_ns`：本地接收时间戳，单位纳秒，不是串口帧内的 1 字节协议时间戳

`frame_data` 已改成变长数组，因此可以同时承载 `64` 字节和 `32` 字节帧。完整原始帧中包含 CRC 前的 1 字节协议时间戳。只有通过目的 ID 和帧类型过滤的帧会被发布。

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
- 存储节点 `serial_storage` 采用双 4KB 缓冲区和后台写盘线程，将 3 路过滤后的 `frame_data` 合并写入单个文件



对话在Add serial CRC frame sender中