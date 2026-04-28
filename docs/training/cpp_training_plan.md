# C++ 项目接管训练手册

这份训练手册面向当前 `MissionComputer` 项目。目标不是把 C++ 语法从头背一遍，而是训练到能够自己写出这个项目里的同类代码。

当前真实情况：

- 有些 C++ 代码能看懂，但空白文件里写不出来。
- 当前项目主要由 Codex 根据需求生成，不是自己从零写成。
- 容易停留在“我看得懂”的状态，但缺少“我能主动生成代码”的能力。

因此训练重点是：

```text
少看完整答案
多写小版本
多编译
多复盘
多让 Codex review 自己写的代码
```

## 项目能力地图

当前项目可以拆成 6 块能力：

```text
1. C++ 基础表达能力
   vector / array / string / 函数 / class / const / 引用 / 指针 / 命名空间

2. 二进制协议能力
   uint8_t / 十六进制 / 固定帧 / 字节偏移 / CRC8 / 帧头同步

3. ROS2 节点能力
   rclcpp::Node / 参数 / publisher / subscription / msg / launch / CMakeLists

4. Linux 串口能力
   open / read / write / close / termios / 波特率 / read 不保证一次读满

5. 工程组织能力
   头文件 / 源文件 / 库 / 可执行节点 / CMake / include 路径

6. 并发和存储能力
   thread / mutex / condition_variable / atomic / 双缓存 / 文件二进制写入
```

对应项目文件：

```text
协议与 CRC：
ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp

串口封装：
ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/posix_serial_port.hpp
ros2_ws/src/telemetry_telecommand/src/posix_serial_port.cpp

固定帧接收节点：
ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/fixed_frame_serial_receiver.hpp
ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp

可视化订阅节点：
ros2_ws/src/telemetry_telecommand/src/frame_visualizer.cpp

双缓存存储节点：
ros2_ws/src/telemetry_telecommand/src/serial_storage.cpp

编译配置：
ros2_ws/src/telemetry_telecommand/CMakeLists.txt
```

## 训练总原则

1. 每个任务先自己写。第一版写得丑、写错、编译不过都可以。
2. 卡住超过 15 分钟就求助，但必须带上代码、报错和你自己的判断。
3. 求助时先要提示，不要直接要完整答案。
4. 每个模块按三遍训练：看懂、遮住重写、独立改造。
5. 每天都要产生可运行代码，哪怕只有 20 行。
6. 不追求一次写对，追求“知道哪里错、怎么查、怎么改”。
7. 当前项目文件可以作为参考，但训练代码尽量先写在独立练习文件里。

推荐和 Codex 协作的提问格式：

```text
训练第 X 周第 Y 天

目标：
我写的代码：
编译命令：
报错或运行结果：
我认为问题可能是：

请先给提示，不要直接给完整答案。
```

让 Codex review 的格式：

```text
这是我的第 N 版实现。
请按当前项目风格 review：
1. 先指出 bug
2. 再指出不清晰的地方
3. 最后只给下一步改法
先不要直接替我重写完整代码
```

## 每日固定流程

每天建议 90 到 150 分钟。时间少时也可以只做前 3 步。

```text
1. 5 分钟：写下今天目标
2. 10 分钟：读项目里对应的小片段
3. 45 分钟：关掉原文件，自己写简化版
4. 20 分钟：编译、运行、修第一个错误
5. 15 分钟：对比项目原实现
6. 10 分钟：写训练日志
```

训练日志模板：

```text
日期：
训练任务：
今天写出的代码：
编译是否通过：
运行是否符合预期：
卡住点：
我最后怎么解决：
明天继续：
```

## 阶段验收规则

不要用“我感觉懂了”作为通过标准。每个阶段至少满足这些条件：

```text
能从空白文件写出核心代码
能说出每个函数的输入、输出和副作用
能自己编译运行
能定位一个常见错误
能做一个小改造
```

例如 `has_valid_header` 不是“看得懂”就算会，而是要能做到：

```text
1. 不看原文件自己写出来
2. 能解释为什么要检查 frame_length
3. 能写一个正确帧测试
4. 能写一个错误帧测试
5. 能处理空 vector 或长度不足的情况
```

## 第 0 周：准备训练环境

目标：保证后续训练不会卡在环境、编译命令和文件位置上。

### 第 0 天任务

确认 ROS2 项目能构建：

```bash
cd /home/zkxt/MissionComputer/ros2_ws
source /opt/ros/jazzy/setup.bash
colcon build --packages-select interfaces telemetry_telecommand
source install/setup.bash
```

确认普通 C++ 练习能单独编译：

```bash
mkdir -p /home/zkxt/MissionComputer/training/cpp_basics
cd /home/zkxt/MissionComputer/training/cpp_basics
```

创建训练目录建议：

```text
training/
  cpp_basics/
  frame_protocol/
  stream_parser/
  ros2_nodes/
  serial_receiver/
  storage/
  logs/
```

注意：训练目录可以放练习代码，不影响正式 ROS2 包。等代码成熟后，再迁移到 `ros2_ws/src/...`。

### 第 0 周验收

你需要能做到：

```text
知道在哪里写练习代码
知道怎么编译一个普通 C++ 文件
知道怎么构建 ROS2 工作区
知道怎么把报错完整贴给 Codex
```

## 第 1 周：C++ 小函数生成能力

目标：从“看懂函数”训练到“能写出函数”。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp
```

本周重点：

```text
uint8_t
std::size_t
std::vector
std::array
constexpr
函数返回值
const 参数
数组下标
边界检查
```

### 第 1 天：写出最小帧结构

任务：

```text
1. 新建 training/cpp_basics/day01_frame_fields.cpp
2. 创建 std::vector<uint8_t> frame(32, 0)
3. 填入：
   frame[0] = 0xEB
   frame[1] = 0x90
   frame[2] = 0x0C
   frame[3] = 0x01
   frame[4] = 0x02
4. 打印 frame_type/source_id/destination_id
5. 输出使用十六进制
```

限制：

```text
不要用 ROS2
不要复制项目代码
不要写 CRC
```

验收：

```text
程序能编译
能打印 0c、01、02 或等价十六进制输出
能解释为什么 frame_type 在下标 2
```

卡住时问 Codex：

```text
我在第 1 周第 1 天，uint8_t 打印出来不是十六进制。
这是我的代码和输出，请先提示我应该检查哪几个点。
```

### 第 2 天：写字段访问函数

任务：

```text
1. 写 uint8_t frame_type(const std::vector<uint8_t>& frame)
2. 写 uint8_t source_id(const std::vector<uint8_t>& frame)
3. 写 uint8_t destination_id(const std::vector<uint8_t>& frame)
4. main 里调用这 3 个函数
```

进阶：

```text
把偏移量定义成 constexpr std::size_t
例如 FRAME_TYPE_OFFSET = 2
```

验收：

```text
函数里不直接写神秘数字，优先用常量名
main 函数不直接访问 frame[2]、frame[3]、frame[4]
能说出 const std::vector<uint8_t>& 的作用
```

### 第 3 天：写帧头检查

任务：

```text
1. 写 bool has_valid_header(const std::vector<uint8_t>& frame)
2. 长度小于 2 时返回 false
3. frame[0] == 0xEB 且 frame[1] == 0x90 时返回 true
4. 写 3 个测试用例：
   正确帧头
   错误帧头
   空 vector
```

验收：

```text
不会因为空 vector 崩溃
能解释为什么先检查 size
能自己构造失败样例
```

### 第 4 天：写长度计算函数

任务：

```text
1. 写 minimum_frame_length()
2. 写 data_area_length_for(frame_length)
3. 写 protocol_timestamp_offset_for(frame_length)
4. 用 32 字节帧和 64 字节帧各打印一次结果
```

提示：

```text
固定布局：
EB 90 + frame_type + source_id + destination_id + data + timestamp + crc8
```

验收：

```text
32 字节帧的数据区长度能算出 25
64 字节帧的数据区长度能算出 57
能解释 timestamp 为什么在 CRC 前一个字节
```

### 第 5 天：写十六进制格式化函数

任务：

```text
1. 写 std::string format_frame_hex(const std::vector<uint8_t>& frame)
2. 每个字节输出 2 位十六进制
3. 字节之间用空格分隔
4. main 里打印整帧
```

需要练的库：

```text
<sstream>
<iomanip>
std::ostringstream
std::hex
std::setw
std::setfill
static_cast<int>
```

验收：

```text
输出类似：eb 90 0c 01 02 00 ...
uint8_t 不被当成字符打印
```

### 第 6 天：综合小程序

任务：

```text
1. 创建 32 字节 frame
2. 填入帧头和路由字段
3. 打印是否有合法帧头
4. 打印 frame_type/source_id/destination_id
5. 打印 data_area_length
6. 打印整帧十六进制
```

验收：

```text
所有函数由你自己写
main 函数只负责组织流程
每个函数不超过 15 行
```

### 第 7 天：复盘和 review

任务：

```text
1. 整理第 1 周写过的代码
2. 选一个你最满意的版本发给 Codex review
3. 选一个你最卡的点写进 training/logs/week01.md
4. 对比 frame_structures.hpp，记录 3 个差异
```

第 1 周通过标准：

```text
你能不看原文件写出基本字段访问、长度计算、帧头检查和十六进制打印
```

## 第 2 周：二进制帧和 CRC8

目标：能构造合法帧，能理解 CRC 校验为什么通过或失败。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/include/telemetry_telecommand/frame_structures.hpp
```

本周重点：

```text
CRC8
位运算
左移
异或
多项式
指针 data()
const uint8_t*
```

### 第 1 天：先调用项目 CRC，不自己实现

任务：

```text
1. 新建一个小程序，包含 frame_structures.hpp
2. 创建 32 字节 frame
3. 填好帧头、字段、假数据、timestamp
4. 调用 calculate_crc8
5. 把结果写入最后一个字节
6. 调用 validate_crc8
```

验收：

```text
validate_crc8 返回 true
你知道 CRC 覆盖范围不包含最后一个 CRC 字节
```

### 第 2 天：写破坏测试

任务：

```text
1. 在 CRC 写好之后，修改 frame[10]
2. 再调用 validate_crc8
3. 观察返回 false
4. 修改回原值，再验证 true
```

验收：

```text
能解释为什么改任意一个参与 CRC 的字节都会失败
```

### 第 3 天：手写 reflect8

任务：

```text
1. 不看原文件，自己写 uint8_t reflect8(uint8_t value)
2. 输入 0b00000001，应该得到 0b10000000
3. 输入 0b10100000，应该得到 0b00000101
```

验收：

```text
能说出 & 0x01U 的作用
能说出 value >> 1 的作用
```

### 第 4 天：手写简化 CRC8

任务：

```text
1. 只实现 standard CRC8
2. polynomial = 0x07
3. initial_value = 0x00
4. xor_out = 0x00
5. 不处理 reflect_input / reflect_output
```

验收：

```text
你的计算结果和项目 calculate_crc8 对同一帧一致
```

### 第 5 天：封装 Crc8Config

任务：

```text
1. 定义 struct Crc8Config
2. 把 polynomial / initial_value / xor_out 放进去
3. compute_crc8 接收 const Crc8Config&
```

验收：

```text
能解释 struct 和一堆独立变量相比有什么好处
```

### 第 6 天：构造帧生成器

任务：

```text
1. 写 std::vector<uint8_t> make_test_frame(std::size_t frame_length)
2. 支持 32 字节和 64 字节
3. 自动填 CRC
4. 返回完整合法帧
```

验收：

```text
32 字节和 64 字节都能 validate_crc8 通过
```

### 第 7 天：复盘和 review

任务：

```text
1. 把 make_test_frame 发给 Codex review
2. 让 Codex 只指出边界问题，不要重写完整代码
3. 写 week02.md，记录 CRC8 最难理解的 3 个点
```

第 2 周通过标准：

```text
你能构造合法帧、写入 CRC、验证成功、故意破坏后验证失败
```

## 第 3 周：字节流解析和同步

目标：不碰真实串口，先从模拟字节流中找出完整帧。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp
```

重点函数：

```text
consume_bytes
try_find_sync
read_exact 的思想
receive_data 的同步状态机
```

本周重点：

```text
流式数据
噪声字节
固定帧切分
双帧头同步
滑动窗口
同步丢失后重同步
```

### 第 1 天：模拟输入流

任务：

```text
1. 使用第 2 周的 make_test_frame 生成两帧
2. 创建 stream
3. stream 前面放 5 个噪声字节
4. 后面追加 frame1 和 frame2
5. 打印 stream 十六进制
```

验收：

```text
能明确指出第一帧 EB 90 在 stream 的哪个下标
```

### 第 2 天：写 find_header

任务：

```text
1. 写 bool find_header(const vector<uint8_t>& stream, size_t& offset)
2. 找到第一个 EB 90
3. 找不到返回 false
```

验收：

```text
噪声 + 合法帧能找到
全噪声返回 false
只有一个 EB 且后面没有 90 时返回 false
```

### 第 3 天：写 consume_bytes

任务：

```text
1. 写 void consume_bytes(vector<uint8_t>& buffer, size_t count)
2. 删除前 count 个字节
3. count 大于等于 size 时清空
```

简单写法可以先用 `erase`。理解项目里的 `memmove` 后再优化。

验收：

```text
consume 5 个噪声字节后，buffer[0] 是 0xEB
```

### 第 4 天：写双帧头同步

任务：

```text
1. 写 try_find_sync(buffer, frame_length, offset)
2. 同时检查：
   buffer[i] 是 EB 90
   buffer[i + frame_length] 也是 EB 90
3. 找到后返回 true
```

验收：

```text
能解释为什么只找一个 EB 90 可能误同步
```

### 第 5 天：切出候选帧并校验

任务：

```text
1. 找到 offset
2. 丢掉 offset 前面的噪声
3. 取前 frame_length 个字节作为 candidate
4. validate_crc8
5. 打印字段
```

验收：

```text
能从 stream 中提取第一帧
能确认 CRC 正确
```

### 第 6 天：模拟坏帧和重同步

任务：

```text
1. 在第一帧中间改坏一个字节
2. 第二帧保持合法
3. 第一帧校验失败后，滑动 1 字节继续找
4. 最后找到第二帧
```

验收：

```text
能说出同步失败后为什么不能一次丢掉所有数据
```

### 第 7 天：写迷你解析器

任务：

```text
1. 输入 vector<uint8_t> stream
2. 输出 vector<vector<uint8_t>> valid_frames
3. 自动跳过噪声
4. 自动跳过坏帧
5. 收集所有 CRC 正确的帧
```

第 3 周通过标准：

```text
你能独立写出一个从字节流提取合法固定帧的迷你解析器
```

## 第 4 周：ROS2 订阅调试节点

目标：自己写出 `frame_inspector`，正式进入当前项目代码。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/src/frame_visualizer.cpp
ros2_ws/src/interfaces/msg/SyncedFrame.msg
ros2_ws/src/telemetry_telecommand/CMakeLists.txt
```

本周重点：

```text
rclcpp::Node
create_subscription
declare_parameter
回调函数
自定义消息
CMake add_executable
ament_target_dependencies
```

### 第 1 天：读懂 frame_visualizer

任务：

```text
1. 逐行读 frame_visualizer.cpp
2. 写出这个类有几个成员函数
3. 写出订阅的话题参数在哪里声明
4. 写出 callback 的输入类型
```

验收：

```text
能口头说明从收到消息到打印日志的流程
```

### 第 2 天：新建最小节点

任务：

```text
1. 新建 src/frame_inspector.cpp
2. 写 class FrameInspector : public rclcpp::Node
3. 构造函数里 Node("frame_inspector")
4. main 里 init / make_shared / spin / shutdown
```

限制：

```text
先不订阅消息
只要节点能编译并运行
```

验收：

```text
ros2 run telemetry_telecommand frame_inspector 能启动
```

### 第 3 天：加入订阅

任务：

```text
1. declare_parameter<std::string>("topic", "fc_tm_synced_frame")
2. create_subscription<interfaces::msg::SyncedFrame>
3. callback 里只打印 frame_data.size()
```

验收：

```text
节点能订阅指定 topic
收到消息时能打印长度
```

### 第 4 天：解析字段

任务：

```text
1. 引入 frame_structures.hpp
2. callback 里检查长度
3. 打印 frame_type/source_id/destination_id
4. 打印 protocol_timestamp
```

验收：

```text
收到长度不足的消息不会崩溃
字段输出正确
```

### 第 5 天：加入 CRC 检查

任务：

```text
1. 使用 validate_crc8
2. 默认使用 CRC8_STANDARD
3. 打印 crc_valid=true/false
```

验收：

```text
对合法帧显示 true
对坏帧显示 false
```

### 第 6 天：改善输出格式

任务：

```text
1. 输出 frame_type/source_id/destination_id 使用 0xXX
2. 输出 frame length
3. 输出 timestamp_ns
4. 输出 protocol timestamp
5. 输出 crc result
```

验收：

```text
日志一眼能看懂这一帧是什么
```

### 第 7 天：review 和整理

任务：

```text
1. 发 frame_inspector.cpp 给 Codex review
2. 发 CMakeLists.txt 改动给 Codex review
3. 让 Codex 检查是否符合项目风格
```

第 4 周通过标准：

```text
你能自己给 ROS2 包添加一个可运行的调试节点
```

## 第 5 周：简化版串口接收器

目标：写出一个能跑的简化固定帧串口接收节点。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/src/posix_serial_port.cpp
ros2_ws/src/telemetry_telecommand/src/fixed_frame_serial_receiver.cpp
```

本周简化条件：

```text
只支持一个串口
只支持 32 字节帧
只支持 CRC8_STANDARD
不支持动态参数更新
不支持多种 CRC 变体
不支持复杂重连
```

### 第 1 天：理解 read 不保证读满

任务：

```text
1. 写一个普通函数 read_exact 的伪代码
2. 输入：read 函数、buffer、size
3. 循环直到 total_read == size
4. read 返回 0 时短暂等待
```

验收：

```text
能解释为什么串口 read 可能一次只返回几个字节
```

### 第 2 天：打开串口的最小封装

任务：

```text
1. 读 PosixSerialPort::open
2. 读 configure_port
3. 画出 open -> configure -> read -> close 流程
```

先以理解为主，不急着重写完整 termios。

验收：

```text
能解释 O_NOCTTY、cfmakeraw、VMIN、VTIME 大概用途
```

### 第 3 天：写节点骨架

任务：

```text
1. 新建 simple_serial_receiver.cpp
2. Node 名称 simple_serial_receiver
3. 参数：port、baud_rate、topic
4. 创建 publisher<SyncedFrame>
```

验收：

```text
节点能启动，能打印当前参数
```

### 第 4 天：加入接收线程

任务：

```text
1. class 里加 std::thread receive_thread_
2. class 里加 std::atomic<bool> stop_requested_
3. 构造函数启动线程
4. 析构函数停止并 join
```

验收：

```text
节点 Ctrl+C 退出时不会卡死
```

### 第 5 天：读取固定 32 字节

任务：

```text
1. 线程里打开串口
2. 循环 read_exact 32 字节
3. 检查 has_valid_header
4. 检查 validate_crc8
```

验收：

```text
合法帧通过
非法帧打印警告
```

### 第 6 天：发布 SyncedFrame

任务：

```text
1. 构造 interfaces::msg::SyncedFrame
2. msg.frame_data = frame
3. msg.timestamp_ns = now().nanoseconds()
4. publisher_->publish(msg)
```

验收：

```text
frame_visualizer 或 frame_inspector 能收到这个节点发布的帧
```

### 第 7 天：复盘和对比正式实现

任务：

```text
1. 对比 simple_serial_receiver 和 FixedFrameSerialReceiver
2. 列出正式实现多了哪些能力
3. 选一个差异作为第 6 周任务
```

第 5 周通过标准：

```text
你能写出一个简化但完整的数据链路：串口读帧 -> 校验 -> 发布 ROS2 消息
```

## 第 6 周：过滤逻辑和参数能力

目标：让接收节点从“能发布所有合法帧”进化到“只发布本机需要处理的帧”。

参考位置：

```text
FixedFrameSerialReceiver::should_process_frame
parse_uint8_list
parse_uint8_list_parameter
handle_parameter_update
```

本周重点：

```text
参数解析
vector 查找
空列表表示不过滤
运行时参数更新
mutex 保护共享数据
```

### 第 1 天：单个 destination_id

任务：

```text
1. 增加参数 destination_id，默认 -1
2. -1 表示不过滤
3. 非 -1 时只发布 destination_id 匹配的帧
```

验收：

```text
destination_id 不匹配时不发布
```

### 第 2 天：多个 destination_ids

任务：

```text
1. 用 vector<uint8_t> destination_ids_
2. 空 vector 表示不过滤
3. 写 contains_byte
```

验收：

```text
多个目的 ID 均可匹配
空列表通过所有目的 ID
```

### 第 3 天：handled_frame_types

任务：

```text
1. 增加 handled_frame_types_
2. 空 vector 表示不过滤
3. destination 和 type 都匹配才发布
```

验收：

```text
目的 ID 匹配但帧类型不匹配时不发布
```

### 第 4 天：字符串参数解析

任务：

```text
1. 写 parse_uint8_list("0C,0D,10,11", 16)
2. 支持逗号和空格
3. 支持 0x 前缀
4. 去重
```

验收：

```text
"0C, 0D 0x10" 能解析成 0x0C 0x0D 0x10
非法值能报错
超过 0xFF 能报错
```

### 第 5 天：参数更新回调

任务：

```text
1. 添加 add_on_set_parameters_callback
2. 参数合法时更新过滤表
3. 参数非法时返回 successful=false
```

验收：

```text
运行中 ros2 param set 后过滤规则生效
非法参数不会破坏旧配置
```

### 第 6 天：加 mutex

任务：

```text
1. 接收线程读取过滤表
2. 参数回调写入过滤表
3. 用 std::mutex 保护
4. should_process_frame 内使用 lock_guard
```

验收：

```text
能解释为什么多线程读写同一个 vector 需要锁
```

### 第 7 天：review

任务：

```text
1. 发过滤逻辑给 Codex review
2. 要求重点检查边界条件和线程安全
```

第 6 周通过标准：

```text
你能独立实现目的 ID 和帧类型过滤，并支持基础参数配置
```

## 第 7 周：存储节点第一版

目标：先写简单可用的存储节点，再去理解双缓存。

参考文件：

```text
ros2_ws/src/telemetry_telecommand/src/serial_storage.cpp
```

本周先不要写双缓存。

### 第 1 天：最小存储节点

任务：

```text
1. 新建 simple_serial_storage.cpp
2. Node 名称 simple_serial_storage
3. 参数 storage_file
4. 打开 std::ofstream，binary | out | trunc
```

验收：

```text
节点启动后能创建文件
```

### 第 2 天：订阅一个话题

任务：

```text
1. 订阅 fc_tm_synced_frame
2. 收到 frame_data
3. output_file_.write 写入原始字节
4. flush
```

验收：

```text
收到消息后文件大小增加
```

### 第 3 天：订阅三个话题

任务：

```text
1. 参数 fc_tm_topic
2. 参数 c_tc_topic
3. 参数 l_tc_topic
4. 三个 subscription 共用一个 handle_frame
```

验收：

```text
三路话题都能写入同一个文件
```

### 第 4 天：统计信息

任务：

```text
1. frames_received_
2. bytes_received_
3. bytes_written_
4. 析构时打印统计
```

验收：

```text
退出时日志能看到接收帧数和写入字节数
```

### 第 5 天：支持目录和 append

任务：

```text
1. storage_file 可以带目录
2. 目录不存在时 create_directories
3. 参数 truncate_file 控制 trunc 或 app
```

验收：

```text
写入 nested/path/storage.bin 不报错
append 模式不会清空旧文件
```

### 第 6 天：错误处理

任务：

```text
1. 文件打不开时 throw runtime_error
2. write 失败时打印 RCLCPP_ERROR
3. 空 frame_data 不写入
```

验收：

```text
空消息不改变文件大小
```

### 第 7 天：对比正式存储节点

任务：

```text
1. 对比 simple_serial_storage 和 serial_storage
2. 列出正式版为什么需要双缓存
3. 写 week07.md
```

第 7 周通过标准：

```text
你能写出一个简单但真实可用的 ROS2 二进制存储节点
```

## 第 8 周：多线程双缓存

目标：理解并能写出项目里最复杂的存储结构。

参考位置：

```text
SerialStorage::append_bytes
SerialStorage::queue_active_buffer_locked
SerialStorage::writer_loop
SerialStorage::stop_and_flush
```

本周重点：

```text
生产者消费者
Active / Ready / Writing / Empty
mutex
condition_variable
atomic stop flag
线程退出和 flush
```

### 第 1 天：普通生产者消费者

任务：

```text
1. 写一个普通 C++ 程序
2. 主线程每 10ms 生成一段字节
3. 写盘线程从队列取数据写文件
4. 用 mutex 保护队列
5. 用 condition_variable 唤醒写盘线程
```

验收：

```text
程序能正常退出
文件中有数据
```

### 第 2 天：固定 4KB buffer

任务：

```text
1. 定义 array<uint8_t, 4096>
2. 定义 buffer_size
3. append_bytes 时按剩余空间复制
4. 满 4096 后写入文件
```

验收：

```text
超过 4096 的数据能分块写
最后不足 4096 的数据不会丢
```

### 第 3 天：双 buffer 状态机

任务：

```text
1. 定义 enum class BufferState
2. 支持 Empty / Active / Ready / Writing
3. 两个 buffer 初始一个 Active，一个 Empty
```

验收：

```text
能画出 Active -> Ready -> Writing -> Empty 的流转
```

### 第 4 天：实现 queue_active_buffer

任务：

```text
1. Active 有数据时置为 Ready
2. push 到 ready_buffers
3. 找 Empty buffer 作为新的 Active
4. 如果没有 Empty，active_buffer_index = -1
```

验收：

```text
两个 buffer 都满时，生产者会等待
```

### 第 5 天：实现 writer_loop

任务：

```text
1. 等待 ready_buffers 非空
2. 取出一个 Ready
3. 状态改为 Writing
4. 解锁后写文件
5. 写完后状态改为 Empty
6. 通知生产者
```

验收：

```text
写文件时不长期持有锁
```

### 第 6 天：实现 stop_and_flush

任务：

```text
1. stop_requested_ 置 true
2. Active 中剩余数据转 Ready
3. notify 所有等待线程
4. join writer_thread
5. flush 和 close 文件
```

验收：

```text
程序退出时最后几帧不丢
析构重复调用不会出错
```

### 第 7 天：回到项目正式实现

任务：

```text
1. 重读 serial_storage.cpp
2. 给 append_bytes 写中文流程说明
3. 给 writer_loop 写中文流程说明
4. 给 stop_and_flush 写中文流程说明
```

第 8 周通过标准：

```text
你能解释并写出一个双缓存异步写盘的简化实现
```

## 每周交付物

每周结束时至少保留这些东西：

```text
training/logs/weekXX.md
本周最重要的 1 个练习 cpp
本周最卡的 1 个报错
本周让 Codex review 的 1 段代码
本周自己总结的 3 个概念
```

建议目录：

```text
training/
  logs/
    week01.md
    week02.md
  cpp_basics/
  frame_protocol/
  stream_parser/
  ros2_nodes/
  serial_receiver/
  storage/
```

## 卡住时的处理流程

遇到问题时按顺序做：

```text
1. 先读第一条编译错误，不要从最后一条看起
2. 找到报错文件和行号
3. 看这一行附近 5 行
4. 猜测是语法、类型、include、链接还是逻辑问题
5. 修改一个最小点
6. 重新编译
7. 15 分钟还不行就问 Codex
```

常见问题分类：

```text
语法问题：
少分号、括号不匹配、函数声明和定义不一致

类型问题：
uint8_t 打印成字符、size_t 和 int 混用、const 不匹配

include 问题：
忘记 include vector/string/iostream/iomanip/sstream

链接问题：
CMakeLists 没加源文件、没链接库、没 ament_target_dependencies

运行逻辑问题：
下标越界、长度没检查、CRC 覆盖范围错、线程没 join
```

## 使用 Codex 的正确方式

推荐说法：

```text
请用提示引导我，不要直接给完整答案。
请检查我这段代码的 bug，先不要重写。
请给我一个更小的子任务。
请问我 3 个问题，确认我是否真的理解。
请根据当前项目风格 review。
```

不推荐说法：

```text
直接帮我写完整代码。
帮我一次性实现这个节点。
把所有 bug 都修好。
```

如果确实需要 Codex 写参考答案，要这样用：

```text
我已经写了两版，这是我的代码。
请给一个参考实现，并标注我原代码和参考实现的关键差异。
```

## 第一个训练任务

从一个普通 C++ 程序开始，不用 ROS2。

目标：

```text
创建一帧 32 字节数据，然后打印 frame_type、source_id、destination_id。
```

要求：

```text
使用 std::vector<uint8_t>
帧头是 EB 90
frame_type 放 0x0C
source_id 放 0x01
destination_id 放 0x02
打印结果用十六进制
先不要写 CRC
先不要用 ROS2
```

完成后把这些内容发给 Codex：

```text
1. 你的 cpp 文件内容
2. 编译命令
3. 运行输出
4. 你觉得最不确定的地方
```

Codex 的指导顺序应该是：

```text
先看你是否能编译
再看输出是否正确
再看函数是否拆得合理
最后才对比项目风格
```
