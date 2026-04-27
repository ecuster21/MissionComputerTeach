# 串口接收与存储总流程

这张图描述当前工程的端到端数据路径：PC 三路串口发送数据，ROS 接收节点完成同步和过滤，再进入可视化与存储节点。

```mermaid
flowchart LR
    PC1[PC COM1<br/>64 字节帧] --> S7[/dev/ttyS7]
    PC6[PC COM6<br/>32 字节帧] --> S3[/dev/ttyS3]
    PC7[PC COM7<br/>32 字节帧] --> S4[/dev/ttyS4]

    S7 --> FC[fc_tm_serial_recv]
    S3 --> CTC[c_tc_serial_recv]
    S4 --> LTC[l_tc_serial_recv]

    FC --> FCF{同步 + CRC8 +<br/>目的ID/帧类型过滤}
    CTC --> CTCF{同步 + CRC8 +<br/>目的ID/帧类型过滤}
    LTC --> LTCF{同步 + CRC8 +<br/>目的ID/帧类型过滤}

    FCF -->|通过| FCT[fc_tm_synced_frame]
    CTCF -->|通过| CTCT[c_tc_synced_frame]
    LTCF -->|通过| LTCT[l_tc_synced_frame]

    FCT --> VF[fc_tm_frame_visualizer]
    CTCT --> VC[c_tc_frame_visualizer]
    LTCT --> VL[l_tc_frame_visualizer]

    FCT --> STORE[serial_storage]
    CTCT --> STORE
    LTCT --> STORE

    STORE --> FILE[(serial_storage.bin)]
```

## 关键点

- 接收节点只发布已经通过帧头同步、CRC8 校验、目的 ID 过滤和帧类型过滤的完整原始帧。
- `serial_storage` 只写 `SyncedFrame.frame_data` 原始字节，三路数据按 ROS 回调到达顺序进入同一个文件。
- 可视化节点只负责调试打印，不参与存储路径。
