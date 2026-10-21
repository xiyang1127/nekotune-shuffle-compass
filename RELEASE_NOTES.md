本次为发布规范维护版本，插件功能和设置不变。

- 安装包名称统一包含插件 ID、版本及平台，便于下载、归档和区分更新。
- 构建信息采用与安装包对应的文件名，并记录源码提交。
- `SHA256SUMS.txt` 统一覆盖所有安装包及构建信息。
- 发布前自动校验标签、清单版本、ZIP 结构、平台完整性和构建信息。

安装：下载对应平台的 `.nekotune.zip`，在 NekoTune 中导入。`build-info.json` 和 `SHA256SUMS.txt` 无需导入。

兼容性：需要包含 SDK 2.1 `IShuffleExtensionContext` 和随机算法设置入口的开发版宿主。公开的 NekoTune v1.2.0 不具备此接口。Linux 使用 Arch Linux 工具链，Windows 使用 MSYS2 UCRT64；请核对构建信息中的 Qt 小版本及编译器 ABI。此版本仍属于开发接口预览。
