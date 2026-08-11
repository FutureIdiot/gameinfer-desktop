# GameInfer Desktop

[English](README.md) | [简体中文](README.zh-CN.md)

GameInfer Desktop 是一套使用 [OpenVPI GAME](https://github.com/openvpi/GAME) 将人声音频转换为 MIDI 的独立桌面工作流。它在 [OpenVPI dataset-tools](https://github.com/openvpi/dataset-tools) 原有 GameInfer 应用的基础上，增加了面向队列的 Qt 界面和托管式音源分离流程。

本仓库由 FutureIdiot 作为衍生发行版独立维护，并非 OpenVPI 官方发行版。准确的代码来源参见 [UPSTREAM.zh-CN.md](UPSTREAM.zh-CN.md)，归属声明参见 [NOTICE](NOTICE)。

## 初始版本

`v0.1.0` Windows 软件包是首个经过 PC 实际验证的版本。它内置 GAME 1.0.3 small ONNX 模型、托管式分离运行时以及 DirectML 支持，不需要用户另外安装系统 Python。

首次执行音源分离时，程序会安装锁定版本的 Python 环境并下载所选分离模型。请保持电脑联网并预留额外等待时间；后续运行会复用已经下载的缓存。

`v0.1.0` 的已知限制：如果分离后的人声仍包含无法通过静音检测切到 60 秒以内的连续片段，MIDI 转换可能失败。该版本尚未包含手动波形裁切功能。

## 当前 main 分支

当前开发分支增加了用于保存分离及切片中间产物的工作区，以及手动切分失败片段的波形对话框。确认切分后，失败的源文件会被两个较短的队列任务替代；工作区设置中可以一键清理不再需要的中间产物。

## 源码结构

- `src/apps/GameInfer`：桌面应用和托管式分离 worker
- `src/libs/game-infer`：GAME 推理集成
- `src/libs/audio-util`：音频解码和切片工具
- `src/libs/qsmedia`、`src/libs/sdlplayback`：波形播放支持
- `src/tests`：GameInfer 专项测试

## 构建

本项目需要 Qt 6、CMake、Ninja、`vcpkg.json` 中列出的依赖、ONNX Runtime 和 `uv`。`.github/workflows` 中的工作流是 Windows 与 macOS 的参考构建方式。CI 产物属于开发构建；正式 Release 会在验证完成后单独发布。

可以使用以下命令将 ONNX Runtime 准备到 `src/libs/onnxruntime`：

```sh
cd src/libs
cmake -Dep=cpu -P ../../scripts/setup-onnxruntime.cmake
```

Windows 上需要 DirectML 时，请改用 `-Dep=dml`。

## 许可证

除非文件中另有声明，本衍生仓库以 Apache License 2.0 发布。所包含的组件和第三方依赖继续适用其各自的许可证。
