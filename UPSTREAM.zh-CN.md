# 上游来源

[English](UPSTREAM.md) | [简体中文](UPSTREAM.zh-CN.md)

GameInfer Desktop 从 [openvpi/dataset-tools](https://github.com/openvpi/dataset-tools) 中的 GameInfer 应用提取而来；该上游项目采用 Apache-2.0 许可证。推理实现和所发布的模型格式来自采用 MIT 许可证的 [openvpi/GAME](https://github.com/openvpi/GAME)。

独立仓库有意只保留构建 GameInfer 所需的应用和库。原有的 `FutureIdiot/dataset-tools` fork 会继续用于跟踪 OpenVPI 上游以及准备向上游提交的贡献。

## 导入映射

- OpenVPI dataset-tools 基础提交：`9a06218`（`Game-0605`）
- 音源分离流程起始于 FutureIdiot dataset-tools 提交：`a898dc8`
- 经过 PC 验证的 Windows 软件包源码：`b7dda56`
- 集成分支中与其应用源码等价的稳定提交：`215a86c`
- 默认分离模型与安装提示界面源码：`633427d`
- 手动波形裁切及工作区源码：`a096a1a`
- macOS 资源打包源码：`732e62e`
- 独立提取路径：`src/apps/GameInfer`、`src/libs/game-infer`、`src/libs/audio-util`、`src/libs/qsmedia`、`src/libs/sdlplayback`、`src/tests` 以及必要的构建脚本

独立仓库后续提交会在提交信息或文档中保留对应的来源提交编号，以便追溯到 dataset-tools 集成分支。
