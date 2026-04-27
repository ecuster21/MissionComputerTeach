# 固定帧接收流程

这张图合并说明 `FixedFrameSerialReceiver::receive_data()` 的关键路径。

```mermaid
flowchart TB
    A[节点启动] --> B[声明参数<br/>port / baud_rate / frame_length / crc8_variant / destination_ids / topic]
    B --> C[打开并配置串口<br/>115200 8N1 raw]
    C --> D{串口打开成功?}
    D -- 否 --> E[等待后重试]
    E --> C
    D -- 是 --> F[进入接收线程]

    F --> G{已同步?}

    G -- 否 --> H[读取字节到搜索缓冲区]
    H --> I{找到双帧头?<br/>buffer[i] 和 buffer[i + frame_length] 都是 EB90}
    I -- 否 --> H
    I -- 是 --> J[丢弃同步点前的噪声字节]
    J --> K[is_synced = true]

    G -- 是 --> L[按 frame_length 取出候选帧]
    K --> L

    L --> M{帧头正确且 CRC8 正确?}
    M -- 是 --> N{destination_id<br/>是否匹配本机号?}
    N -- 是 --> O[发布 SyncedFrame]
    N -- 否 --> P[忽略该帧<br/>保持同步]
    O --> Q[继续下一帧]
    P --> Q
    Q --> G

    M -- 否 --> R[打印 CRC8 诊断日志]
    R --> S{候选帧来自哪里?}
    S -- 直接 read_exact --> T[整帧放回搜索缓冲区<br/>is_synced = false]
    S -- 用户态缓冲区 --> U[只丢弃 1 字节<br/>重新寻找同步点]
    T --> G
    U --> G
```

## 保留这个图的原因

- 它覆盖了原来分散在多张图里的同步、校验、目的 ID 过滤和重同步逻辑。
- 双帧头同步是当前接收逻辑最容易误解的部分，保留为主图更便于维护。
