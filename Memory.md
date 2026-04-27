# Memory

只记录长期有效内容，不记录一次性任务、临时状态或短期结论。

## 用户偏好

- 默认使用中文沟通与撰写说明文档
- 项目开始前优先准备并优化给 Codex 的启动提示词
- 始终作为一个资深的嵌入式开发工程师角度来思考和写代码

## 固定工作流

- `README.md` 维护项目要点、目录入口与运行操作；当结构或运行方式变化时同步更新
- `Log.md` 每次收尾都追加记录，至少包含“这次做了什么 / 改了哪些文件 / 下一步做什么”
- `Memory.md` 只保留长期有效规则；临时过程信息不要写入
- 说明文档、专题分析、流程图、提示词统一放在 `docs/` 目录

## 项目约定

- 当前项目基于 ROS 2 Jazzy 与 `colcon` 工作流
- 主工作区位于 `ros2_ws/`
- `docs/` 当前按 `flows/` 和 `prompts/` 两类组织；流程图集中放在 `docs/flows/`
- 当前串口协议默认为固定帧头 `0xEB 0x90`，后接 `frame_type`、`source_id`、`destination_id`、数据区、1 字节协议时间戳和 CRC8，固定总长 `64` 或 `32` 字节；接收节点按目的 ID 过滤完整帧，CAN 提取节点再按目的 ID 与帧类型过滤
- `serial_storage` 节点订阅 3 路过滤后的 `SyncedFrame.frame_data`，使用双 4KB 缓冲区按到达顺序写入单个文件
- `c_can_pub` 和 `l_can_pub` 从 `c_tc_synced_frame`、`l_tc_synced_frame` 的数据区前两包 CAN 中发布 `interfaces/msg/CanFrame`
