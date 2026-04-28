# Week 01 Day 03 - 帧头检查和测试用例

## 训练目标

实现 `has_valid_header`，判断一帧数据是否以固定帧头开头：

```text
frame[0] == 0xEB
frame[1] == 0x90
```

并写 3 个测试用例：

```text
正确帧头
错误帧头
空 vector
```

## 完成代码

练习文件：

```text
training/cpp_basics/week01/day03_check_frame_head.cpp
```

核心常量：

```cpp
constexpr std::size_t FIRST_BYTE = 0;
constexpr std::size_t SECOND_BYTE = 1;
```

核心函数：

```cpp
bool has_valid_header(const std::vector<uint8_t>& frame)
{
    if (frame.size() < 2) {
        return false;
    }
    if (frame[FIRST_BYTE] == 0xEB && frame[SECOND_BYTE] == 0x90) {
        return true;
    }
    return false;
}
```

## 编译和运行

命令：

```bash
cd /home/zkxt/MissionComputer
g++ -std=c++17 -Wall -Wextra -pedantic training/cpp_basics/week01/day03_check_frame_head.cpp -o /tmp/check_frame_head
/tmp/check_frame_head
```

实际输出：

```text
Right header test passed.
Wrong header test passed.
Empty frame test passed.
```

## 今天的迭代过程

### 第 1 版：测试函数先调用了后定义的函数

一开始测试函数写在 `has_valid_header` 前面，测试函数中调用：

```cpp
has_valid_header(frame)
```

问题：

```text
C++ 从上往下编译。函数在被调用前，编译器至少需要先看到函数声明。
```

解决：

```text
把 has_valid_header 放到 3 个测试函数前面。
```

当前训练阶段采用这种方式更直观。

### 第 2 版：offset 类型从 uint8_t 改成 std::size_t

一开始写成：

```cpp
constexpr uint8_t FIRST_BYTE = 0;
constexpr uint8_t SECOND_BYTE = 1;
```

改成：

```cpp
constexpr std::size_t FIRST_BYTE = 0;
constexpr std::size_t SECOND_BYTE = 1;
```

原因：

```text
FIRST_BYTE 和 SECOND_BYTE 是 vector 下标。
下标、长度、大小更适合用 std::size_t。
```

### 第 3 版：测试用例互相独立

一开始 3 个测试共用 `main` 里的同一个 `frame`：

```cpp
std::vector<uint8_t> frame(32, 0);
test_right_header(frame);
test_wrong_header(frame);
test_empty_frame(frame);
```

问题：

```text
每个测试都会修改同一个 frame，测试之间可能互相影响。
```

改进后：

```cpp
void test_right_header()
{
    std::vector<uint8_t> frame(32, 0);
    ...
}
```

每个测试函数内部创建自己的 `frame`，测试更独立。

### 第 4 版：空 vector 测试

最终空帧测试使用：

```cpp
std::vector<uint8_t> frame;
```

这比先创建 32 字节再 `clear()` 更直接表达“这是空 vector”。

当前代码中还保留了一行：

```cpp
frame.clear();
```

这行对空 vector 没有副作用，可以删除。下一次清理代码时可以去掉。

## 今天理解的概念

```text
bool 函数返回 true 或 false。
访问 frame[0] 和 frame[1] 前必须先检查 frame.size()。
空 vector 如果直接访问 frame[0] 会有越界风险。
std::size_t 适合表示下标和长度。
测试函数最好互相独立，不共享会被修改的输入数据。
函数要么定义在调用前，要么先写函数声明。
```

## int、uint8_t、size_t 的区别

今天额外理解了这 3 个类型：

```text
int：
普通整数，通常可以是负数，常用于普通计算。

uint8_t：
无符号 8 位整数，刚好 1 个字节，范围 0 到 255。
适合表示串口帧里的原始字节，例如 0xEB、0x90、0x0C。

std::size_t：
无符号整数，适合表示大小、长度、数量和下标。
vector.size() 返回的就是 std::size_t。
```

在当前项目中可以这样记：

```text
帧里的每个字节：uint8_t
帧有多长、第几个字节：std::size_t
普通计算和临时数值：int
```

## 当前代码和项目风格的关系

练习版本：

```cpp
bool has_valid_header(const std::vector<uint8_t>& frame)
{
    if (frame.size() < 2) {
        return false;
    }
    if (frame[FIRST_BYTE] == 0xEB && frame[SECOND_BYTE] == 0x90) {
        return true;
    }
    return false;
}
```

项目版本更接近：

```cpp
inline bool has_valid_header(const uint8_t * data, std::size_t frame_length)
{
  return frame_length >= FRAME_HEADER.size() &&
         data[0] == FRAME_HEADER[0] &&
         data[1] == FRAME_HEADER[1];
}
```

练习版本先使用 `std::vector<uint8_t>&`，更适合入门；项目版本使用指针和长度，更适合底层协议解析。

## 今日评价

第三天训练目标完成。最重要的进步是开始写“测试用例”，不只是写一个函数然后靠感觉判断对不对。

这一天也开始接触边界条件：空 vector、长度不足、错误帧头。对串口协议解析来说，边界条件非常重要。

## 下一步

进入第四天训练：写长度计算函数。

下一步目标：

```text
minimum_frame_length()
data_area_length_for(frame_length)
protocol_timestamp_offset_for(frame_length)
```

先从 32 字节帧和 64 字节帧的长度计算开始。
