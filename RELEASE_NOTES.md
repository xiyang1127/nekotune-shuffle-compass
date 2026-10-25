迁移到 NekoTune SDK 3，沿用既有随机选曲规则和已保存的二维偏好。

- 使用 `org.nekotune.IExtensionPlugin/3` 与 `context.shuffle()` 注册算法。
- 同步 SDK 3.0.0 接口，保留 `frequency`、`variety` 设置。

安装：下载对应系统的 ZIP，在 NekoTune「扩展」中导入并确认信任。`SHA256SUMS.txt` 用于校验，无需导入。

兼容性：需要 NekoTune v2.0.0 或更新的兼容 SDK 3 宿主，SDK 2.x 宿主无法加载。Linux 使用 Ubuntu 24.04 / GCC 13 / Qt 6.11.2；Windows 使用 MSYS2 UCRT64 / GCC 16 / Qt 6.12.0。原生插件的 Qt 小版本与编译器 ABI 必须和宿主兼容。
