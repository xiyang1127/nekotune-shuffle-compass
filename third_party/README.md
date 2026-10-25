# NekoTune SDK 3 快照

`nekotune-sdk/` 来自 NekoTune 主仓库的 `sdk/`，用于独立构建此扩展。
上游仓库为 <https://github.com/yang208115/NekoTune>；快照基线为本地 `main` 提交
`f7c089859f5f41349a4f2d74b99cdc12d494c8c0`（SDK 3.0.0，插件 IID `org.nekotune.IExtensionPlugin/3`）。
该提交尚未推送到上游，不应将快照误认为已发布的 SDK 版本。

快照保留 SDK 原始目录结构和使用说明，只提供构建接口，不提供播放器运行时。
SDK 正式发布后，可以通过 `CMAKE_PREFIX_PATH` 或 `NEKOTUNE_SDK_SOURCE_DIR`
改用与目标宿主一致的官方 SDK。
