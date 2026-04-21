# MissionComputer

基于 ROS 2 Jazzy 的遥测遥控串口处理示例工程，包含：

- `interfaces`：自定义消息 `SyncedFrame`
- `telemetry_telecommand`：串口接收、帧同步、模拟发送、帧可视化

当前仓库已经按需求文档落地为完整工作区 `ros2_ws`，并在本地完成过一次 `colcon build` 验证。

## 目录结构

```text
MissionComputer/
├── README.md
├── ros2_ws
│   ├── src
│   │   ├── interfaces
│   │   └── telemetry_telecommand
│   └── colcon.meta
└── 一、项目结构设计.md
```

## 环境要求

- Ubuntu + ROS 2 Jazzy
- `colcon`
- 可访问的串口设备，例如 `/dev/ttyUSB0`

可选工具：

- `socat`
  用于创建虚拟串口对，方便无实体设备时联调

## 构建

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select interfaces telemetry_telecommand
```

为了让 VS Code 或 Cursor 的 `Ctrl + 左键` 正常跳转到定义，建议按当前仓库自带的 `colcon.meta` 重新构建一次。该配置已经打开 `compile_commands.json` 导出，供 C/C++ 语言服务读取。

构建完成后：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
```

如果你使用的是 VS Code / Cursor，首次打开工作区后建议安装工作区推荐插件：

- `ms-vscode.cpptools`
- `ms-vscode.cmake-tools`
- `ms-iot.vscode-ros`

安装后执行一次上面的构建命令，再重新加载编辑器窗口，`Ctrl + 左键` 一般就会恢复。

## 真实串口运行

终端 1，运行模拟发送器或你自己的上位机发送端：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand mock_serial_sender /dev/ttyS7 115200
```

终端 2，启动串口接收节点：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand serial_receiver --ros-args -p port:=/dev/ttyS7 -p baud_rate:=115200 -p timeout_ms:=100 -p topic:=synced_frame
```

终端 3，启动可视化节点：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=synced_frame
```

如果串口权限不足，可执行：

```bash
sudo usermod -aG dialout $USER
```

然后重新登录会话。

## 虚拟串口联调

如果手头没有实体串口，推荐先使用 `socat` 创建一对互联的伪终端。

终端 1，创建虚拟串口对：

```bash
cd /home/zkxt/MissionComputer
./tools/create_virtual_serial_pair.sh
```

默认会创建：

- `/tmp/ttyMCU`
- `/tmp/ttyHOST`

终端 2，向其中一端发送模拟帧：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand mock_serial_sender /tmp/ttyMCU 115200
```

终端 3，启动串口接收节点：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand serial_receiver --ros-args -p port:=/tmp/ttyHOST -p baud_rate:=115200 -p timeout_ms:=100 -p topic:=synced_frame
```

终端 4，启动可视化节点：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=synced_frame
```

## 数据帧说明

- 帧长固定为 `64` 字节
- 帧头固定为 `0xEB 0x90`
- 最后 1 字节为校验和
- 中间 `61` 字节数据内容由发送端按 `00` 到 `FF` 循环递增填充
- 校验和算法为前 `63` 字节的 8 位累加和

ROS 2 话题：

- 话题名默认 `synced_frame`
- 消息类型 `interfaces/msg/SyncedFrame`

## 运行参数

`serial_receiver` 支持以下参数：

- `port`：串口设备路径，默认 `/dev/ttyUSB0`
- `baud_rate`：波特率，默认 `115200`
- `timeout_ms`：串口读超时，默认 `100`
- `topic`：发布话题名，默认 `synced_frame`

`frame_visualizer` 支持以下参数：

- `topic`：订阅话题名，默认 `synced_frame`

启动方式：

- `ros2 run telemetry_telecommand serial_receiver --ros-args -p port:=/dev/ttyUSB0 -p baud_rate:=115200 -p timeout_ms:=100 -p topic:=synced_frame`
- `ros2 run telemetry_telecommand frame_visualizer --ros-args -p topic:=synced_frame`

## 实现说明

项目中的串口访问使用仓库内置的 POSIX `termios` 实现，而不是第三方 `serial` CMake 包。这样做的原因是当前环境里没有可直接被 `find_package(serial)` 找到的开发包，但整体功能与需求文档保持一致。

## 常用命令

查看当前已注册的话题：

```bash
ros2 topic list
```

查看消息内容：

```bash
ros2 topic echo /synced_frame
```

## 后续可扩展项

- 增加单元测试和集成测试
- 增加 CRC/更严格的协议字段解析
- 接入真实遥测遥控协议字段定义
- 增加录包与回放能力
