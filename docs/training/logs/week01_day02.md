# Week 01 Day 02 - 字段访问函数

## 训练目标

把第一天直接访问下标的写法：

```cpp
frame[2]
frame[3]
frame[4]
```

改成有语义的字段访问函数：

```cpp
frame_type(frame)
source_id(frame)
destination_id(frame)
```

同时把裸下标改成 offset 常量，让代码更接近当前项目风格。

## 完成代码

练习文件：

```text
training/cpp_basics/week01/day02_field_access_function.cpp
```

核心常量：

```cpp
constexpr std::size_t FRAME_TYPE_OFFSET = 2;
constexpr std::size_t FRAME_SOURCE_ID_OFFSET = 3;
constexpr std::size_t FRAME_DESTINATION_ID_OFFSET = 4;
```

核心函数：

```cpp
uint8_t frame_type(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_TYPE_OFFSET];
}

uint8_t source_id(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_SOURCE_ID_OFFSET];
}

uint8_t destination_id(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_DESTINATION_ID_OFFSET];
}
```

## 编译和运行

建议命令：

```bash
cd /home/zkxt/MissionComputer
g++ -std=c++17 -Wall -Wextra -pedantic training/cpp_basics/week01/day02_field_access_function.cpp -o /tmp/day02_field_access_function
/tmp/day02_field_access_function
```

期望输出：

```text
Frame Type: 0x0c
Source ID: 0x01
Destination ID: 0x02
```

## 今天的迭代过程

### 第 1 版：函数没有 return

一开始写了：

```cpp
uint8_t frame_type(const std::vector<uint8_t>& frame){
    
}
```

问题：

```text
函数声明返回 uint8_t，就必须 return 一个 uint8_t 值。
```

理解：

```text
字段访问函数的目标是“给它一帧，返回某个字段”。
```

### 第 2 版：错误使用 frame.data()

一开始在 `main` 里写了：

```cpp
frame_type(frame.data());
```

问题：

```text
frame_type 的参数类型是 const std::vector<uint8_t>&，
但 frame.data() 返回的是底层字节指针 uint8_t*。
```

正确调用：

```cpp
frame_type(frame);
```

### 第 3 版：函数职责混在一起

曾经在 `frame_type` 函数里直接打印：

```cpp
std::cout << "Frame Type: 0x" << ...
return frame[2];
```

问题：

```text
字段访问函数同时做了“取值”和“打印”两件事。
```

改进：

```text
函数只负责 return。
main 负责接收返回值并打印。
```

最终结构：

```cpp
uint8_t type = frame_type(frame);
std::cout << "Frame Type: 0x"
          << std::hex
          << std::setw(2)
          << std::setfill('0')
          << static_cast<int>(type)
          << std::endl;
```

### 第 4 版：用常量替代裸下标

从：

```cpp
return frame[2];
```

改成：

```cpp
return frame[FRAME_TYPE_OFFSET];
```

意义：

```text
2 只是数字，FRAME_TYPE_OFFSET 是语义。
代码读起来更接近协议定义，也更接近项目里的 frame_structures.hpp。
```

### 第 5 版：补全 source_id 和 destination_id

最终补全：

```cpp
uint8_t source_id(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_SOURCE_ID_OFFSET];
}

uint8_t destination_id(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_DESTINATION_ID_OFFSET];
}
```

过程中出现过一个典型语法错误：

```cpp
return frame[FRAME_SOURCE_ID_OFFSET]
```

问题：

```text
少了分号。
```

正确：

```cpp
return frame[FRAME_SOURCE_ID_OFFSET];
```

## 今天理解的概念

```text
函数有返回值类型，就必须 return 对应类型的值。
const std::vector<uint8_t>& 表示只读引用，避免复制整个 vector。
frame.data() 是底层指针，不等于 vector 本身。
main 里可以用变量接住函数返回值。
函数职责要尽量单一：字段访问函数只取值，不打印。
constexpr offset 常量比直接写 2、3、4 更清晰。
```

## 当前代码和项目风格的关系

练习版本：

```cpp
uint8_t frame_type(const std::vector<uint8_t>& frame)
{
    return frame[FRAME_TYPE_OFFSET];
}
```

项目版本更接近：

```cpp
inline uint8_t frame_type(const uint8_t * data)
{
  return data[FRAME_TYPE_OFFSET];
}
```

当前练习先使用 `std::vector<uint8_t>&` 是合适的，因为训练目标是理解字段偏移和函数返回值。后续熟练后再过渡到项目里的 `const uint8_t * data` 写法。

## 今日评价

完成了第二天核心训练目标。最重要的进步是把“下标访问”升级成“语义函数访问”，并开始理解函数职责、参数类型和返回值。

今天的训练节奏是对的：每次只加一个小函数，每一步都能编译、能解释、能改正。

## 下一步

进入第三天训练：实现 `has_valid_header`。

下一步只做一个小目标：

```text
写 bool has_valid_header(const std::vector<uint8_t>& frame)
长度小于 2 返回 false
frame[0] == 0xEB 且 frame[1] == 0x90 返回 true
否则返回 false
```
