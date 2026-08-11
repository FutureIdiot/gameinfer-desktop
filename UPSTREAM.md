# Upstream lineage

GameInfer Desktop was extracted from the GameInfer application in [openvpi/dataset-tools](https://github.com/openvpi/dataset-tools), which is licensed under Apache-2.0. The inference implementation and distributed model format originate from [openvpi/GAME](https://github.com/openvpi/GAME), licensed under MIT.

The standalone repository intentionally keeps only the application and libraries needed to build GameInfer. The original `FutureIdiot/dataset-tools` fork remains available for syncing with OpenVPI and preparing upstream contributions.

## Import map

- OpenVPI dataset-tools base: `9a06218` (`Game-0605`)
- Source separation pipeline begins at FutureIdiot dataset-tools commit: `a898dc8`
- PC-validated Windows package source: `b7dda56`
- Equivalent stabilized application source on the integration branch: `215a86c`
- Standalone extraction paths: `src/apps/GameInfer`, `src/libs/game-infer`, `src/libs/audio-util`, `src/libs/qsmedia`, `src/libs/sdlplayback`, `src/tests`, and the required build scripts

Future standalone commits retain the corresponding source commit identifiers in their messages or documentation so changes can be traced back to the dataset-tools integration branch.

