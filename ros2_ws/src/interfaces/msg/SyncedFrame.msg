# 完整原始帧：帧头 2 字节 + 数据区 61 字节 + CRC8 1 字节
uint8[64] frame_data

# 接收节点填充的发布时间戳，单位为纳秒
int64 timestamp_ns
