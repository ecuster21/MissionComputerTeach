# Docs Index

这个目录用于存放项目相关的说明文档、流程图、设计记录、提示词和后续补充资料。

## 当前文档

- [flows/README.md](/home/zkxt/MissionComputer/docs/flows/README.md)
  当前运行链路、固定帧接收和双缓存存储的 Mermaid 流程图入口
- [prompts/README.md](/home/zkxt/MissionComputer/docs/prompts/README.md)
  项目协作提示词入口
- [training/cpp_training_plan.md](/home/zkxt/MissionComputer/docs/training/cpp_training_plan.md)
  面向当前项目的 C++ 接管训练计划
- [training/logs/](/home/zkxt/MissionComputer/docs/training/logs)
  C++ 训练过程日志

## 目录划分

- `flows/`：只放对理解系统运行有帮助的流程图。当前保留 3 张图：整体链路、接收同步、双缓存存储。
- `prompts/`：只放项目协作提示词。
- `training/`：C++/ROS2 项目接管训练计划和训练日志。

暂时不再拆更多目录；后续如果协议说明、联调步骤或部署说明变多，再新增对应专题目录。

## 维护约定

- 面向项目整体的入口信息放在根目录 `README.md`
- 面向单一主题的详细说明放在 `docs/` 下
- 新增重要文档后，建议同步补充本文件索引
