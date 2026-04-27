# 从串口完整帧的数据区提取出的单包 CAN 数据。
# CANID 来自数据区每包 CAN 的 2 字节 ID 字段。
int16 id
uint8[8] data
