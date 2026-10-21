本次为发布规范维护版本，插件功能和设置不变。

- 原生安装包统一为 `<插件ID>-<版本>-linux.zip` / `windows.zip`；平台无关主题使用 `<插件ID>-<版本>.zip`。
- 下载附件只保留安装包和 `SHA256SUMS.txt`，不再提供 JSON 构建信息附件。
- `SHA256SUMS.txt` 覆盖所有 ZIP，构建环境在 CI 中校验并记录。
- 发布前自动校验标签、清单版本、ZIP 结构和平台完整性。

安装：下载对应系统的 ZIP，在 NekoTune「扩展」中导入。`SHA256SUMS.txt` 无需导入。

兼容性：需要包含 SDK 2.1 `IShuffleExtensionContext` 和随机算法设置入口的开发版宿主。公开的 NekoTune v1.2.0 不具备此接口。Linux 使用 Arch Linux 工具链，Windows 使用 MSYS2 UCRT64；Qt 小版本和编译器 ABI 必须与宿主匹配。本版本仍属于开发接口预览。
