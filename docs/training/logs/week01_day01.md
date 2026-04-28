# Week 01 Day 01 - 固定帧字段打印

## 训练目标

创建一帧 32 字节的模拟数据，并打印其中的 `frame_type`、`source_id`、`destination_id`。

本次练习对应当前项目中的协议布局：

```text
frame[0] = 0xEB
frame[1] = 0x90
frame[2] = frame_type
frame[3] = source_id
frame[4] = destination_id
```

## 完成代码

练习文件：

```text
training/cpp_basics/week01/day01_frame_fields.cpp
training/cpp_basics/week01/day01_practice_day.cpp
```

关键代码能力：

```cpp
std::vector<uint8_t> frame(32, 0);
frame[0] = 0xEB;
frame[1] = 0x90;
frame[2] = 0x0C;
frame[3] = 0x01;
frame[4] = 0x02;
```

打印时使用：

```cpp
std::hex
std::setw(2)
std::setfill('0')
static_cast<int>(frame[2])
```

## 编译和运行

正确编译命令：

```bash
cd /home/zkxt/MissionComputer
g++ -std=c++17 -Wall -Wextra -pedantic training/cpp_basics/week01/day01_frame_fields.cpp -o /tmp/day01_frame_fields
/tmp/day01_frame_fields
```

期望输出：

```text
Frame Type: 0x0c
Source ID: 0x01
Destination ID: 0x02
```

## 今天踩到的问题

### 1. 文件路径找不到

曾经出现：

```text
cc1plus: fatal error: training/cpp_basics/day01_frame_fields.cpp: No such file or directory
```

原因：

```text
文件实际曾经在 training/cpp_basics/cpp_basics/day01_frame_fields.cpp，
命令里找的是 training/cpp_basics/day01_frame_fields.cpp。
后续已统一整理到 training/cpp_basics/week01/day01_frame_fields.cpp。
```

解决：

```text
把练习文件整理到 training/cpp_basics/week01/day01_frame_fields.cpp。
以后编译前可以先用 ls 或 find 确认路径。
```

### 2. 编译参数写错

曾经写成：

```bash
g++ -std=c++17 -Wall Wextra -pedantic ...
```

问题：

```text
Wextra 前面少了 -
```

正确写法：

```bash
g++ -std=c++17 -Wall -Wextra -pedantic ...
```

### 3. 编译产物位置误解

命令里使用：

```bash
-o /tmp/day01_frame_fields
```

含义：

```text
可执行文件被放在 /tmp/day01_frame_fields，不在当前目录。
```

运行方式：

```bash
/tmp/day01_frame_fields
```

错误理解：

```bash
./day01_frame_fields
./tmp/day01_frame_fields
```

`./tmp/day01_frame_fields` 表示当前目录下的 `tmp/day01_frame_fields`，不是系统根目录下的 `/tmp/day01_frame_fields`。

### 4. gcc 和 g++ 的区别

曾经使用：

```bash
gcc -std=c++17 ...
```

出现：

```text
undefined reference to `std::cout'
```

原因：

```text
gcc 是 C 编译器入口，编译 C++ 时链接阶段不会自动按 g++ 的方式链接 C++ 标准库。
```

正确做法：

```bash
g++ -std=c++17 ...
```

## 今天理解的概念

```text
0x 表示十六进制数字前缀。
uint8_t 表示一个字节。
std::vector<uint8_t> frame(32, 0) 表示创建 32 个字节，初始值全是 0。
frame[2] 表示第 3 个字节，因为下标从 0 开始。
static_cast<int> 可以避免 uint8_t 被当成字符打印。
```

## 今日评价

完成了第一天训练目标。虽然卡在路径、输出位置和 `gcc/g++` 上，但这些都是真实 C++ 开发中常见的基础问题。今天的重点不是写了多少代码，而是完成了第一次从源文件到可执行文件再到运行输出的闭环。

## 下一步

进入第二天训练：把直接访问 `frame[2]`、`frame[3]`、`frame[4]` 改成字段访问函数。
