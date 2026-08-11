# GameInfer Desktop

[English](README.md) | [简体中文](README.zh-CN.md)

GameInfer Desktop is a standalone desktop workflow for converting vocal audio into MIDI with [OpenVPI GAME](https://github.com/openvpi/GAME). It adds a queue-oriented Qt interface and a managed source-separation step to the GameInfer application originally developed in [OpenVPI dataset-tools](https://github.com/openvpi/dataset-tools).

This repository is maintained by FutureIdiot as a derivative distribution. It is not an official OpenVPI release. See [UPSTREAM.md](UPSTREAM.md) for the exact source lineage and [NOTICE](NOTICE) for attribution.

## Initial release

The `v0.1.0` Windows package is the first PC-validated build. It includes the GAME 1.0.3 small ONNX model, a managed separator runtime, and DirectML support. No system Python installation is required.

On first use, source separation installs its locked Python environment and downloads the selected separation model. Keep the PC online and allow extra setup time; subsequent runs reuse the cache.

Known limitation in `v0.1.0`: if a separated vocal still contains a continuous silence-sliced segment longer than 60 seconds, MIDI conversion may fail. Manual waveform cropping is not included in that release.

## Current main branch

The current development branch adds a workspace for intermediate separation/slicing files and a waveform dialog for manually splitting a failed segment. Confirming a split replaces the failed source with two shorter queue items; obsolete working files can be cleared from the workspace settings.

## Source layout

- `src/apps/GameInfer`: desktop application and managed separator worker
- `src/libs/game-infer`: GAME inference integration
- `src/libs/audio-util`: audio decoding and slicing utilities
- `src/libs/qsmedia`, `src/libs/sdlplayback`: waveform playback support
- `src/tests`: focused GameInfer tests

## Building

The project requires Qt 6, CMake, Ninja, vcpkg dependencies from `vcpkg.json`, ONNX Runtime, and `uv`. The CI workflows in `.github/workflows` are the reference Windows and macOS builds. CI artifacts are development builds; published Releases are promoted separately after validation.

ONNX Runtime is prepared beneath `src/libs/onnxruntime` with:

```sh
cd src/libs
cmake -Dep=cpu -P ../../scripts/setup-onnxruntime.cmake
```

Use `-Dep=dml` on Windows for DirectML.

## License

Unless a file states otherwise, this derivative repository is distributed under the Apache License 2.0. Bundled and third-party components retain their respective licenses.
