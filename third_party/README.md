# NekoTune SDK 预览快照

`nekotune-sdk/` 来自 NekoTune 开发工作区的 `sdk/`，用于独立构建此扩展。
上游仓库为 <https://github.com/yang208115/NekoTune>；开发工作区基线提交为
`a5492181686ca6f2326dad261ebab7449c174f7d`，本快照额外包含该工作区尚未发布的 SDK 2.1 随机算法接口。
该基线提交本身不包含此接口，不应将快照误认为已发布的上游版本。

快照保留 SDK 原始目录结构和使用说明；未改动原有 `IExtensionContext` 虚表、插件 IID `/2`
及 `apiVersion: 2`，新增能力通过独立 `IShuffleExtensionContext` 动态查询。
这份快照只提供构建接口，不提供播放器运行时；使用扩展仍需具备相应功能的宿主。

SDK 正式发布后，可以通过 `CMAKE_PREFIX_PATH` 或 `NEKOTUNE_SDK_SOURCE_DIR`
改用与目标宿主一致的官方 SDK。CI 使用此目录以避免依赖尚不存在的 SDK 发布附件。
