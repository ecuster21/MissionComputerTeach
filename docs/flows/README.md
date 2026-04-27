# Flowcharts

这个目录只保留对理解运行链路有帮助的流程图。

当前保留 3 张：

- [serial_pipeline.md](serial_pipeline.md)
  从 PC 三路串口发送到 ROS 接收、过滤、显示、存储和 CAN 提取的整体链路
- [receiver_sync.md](receiver_sync.md)
  `FixedFrameSerialReceiver` 的同步、校验、目的 ID 过滤和发布流程
- [storage_double_buffer.md](storage_double_buffer.md)
  `serial_storage` 的双 4KB 缓冲区写盘流程

旧的细分同步图已合并到 `receiver_sync.md`，避免同一段接收逻辑在多份文档里重复维护。
