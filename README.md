# 漫游罗盘 · Shuffle Compass

NekoTune 原生随机算法扩展，提供与二维偏好图对应的控制界面：

- 左右：偏好播放次数较少／较多的歌曲。
- 上下：偏好与当前歌曲不同／相近的歌手、专辑。
- 拖动、点按或方向键调整；`Home` 回到中心，`Shift`＋方向键精细调整。
- “完成”保存后关闭弹层，“取消”丢弃草稿并关闭；保存失败保留草稿。
- 背景、文字、网格、按钮与控制点使用宿主主题色，随明暗主题和强调色实时更新。

需要支持 **NekoTune SDK 3**（插件接口 `org.nekotune.IExtensionPlugin/3`）的宿主。旧宿主会拒绝加载并在管理页提示接口不匹配。
源码与下载：[xiyang1127/nekotune-shuffle-compass](https://github.com/xiyang1127/nekotune-shuffle-compass)。

**当前版本仍为开发接口预览版**：目前公开的 NekoTune v1.2.0 发行包不支持 SDK 3，
不能仅按宿主版本号判断兼容性。需要提供 SDK 3 及播放模式菜单设置入口的开发版宿主。
原生安装包还要求与宿主匹配的 Qt 小版本、编译器 ABI 和运行库；详见对应 Release。

## 安装和使用

将 Release 中的 `nekotune.shuffle-compass-<版本>-linux.zip` 或 `windows.zip` 导入宿主的扩展管理页，
信任并启用。在队列页或队列抽屉的播放模式菜单选择“随机 → 漫游罗盘”，
再点击同一菜单中的“调整随机算法…”，控制盘在播放模式按钮附近弹出，不占用侧栏。
也可从“设置 → 扩展设置 → 随机选曲偏好”进入。

移动蓝点后点击“完成”即可保存。从扩展设置进入且算法尚未启用时，可点击“启用算法”。启用只选择该算法并切换至随机模式，
不会开始播放或打断当前歌曲。也可从队列的“随机 → 随机算法”菜单选择本扩展。
参数全局保存；其他窗口会同步已保存的设置，未保存的草稿保留，点“取消”读取最新设置。

## 选曲规则

只在当前队列中选曲，不下载、联网或分析音频。“风格”以非空歌手相同占 70%、
非空专辑相同占 30% 计算，不表示音色或曲风识别。没有当前歌曲时忽略风格轴。
播放次数取非负 `log(1 + playCount)` 后在候选中归一化。两个轴的得分相加后转为
指数权重进行抽样，因此是偏好而非硬性筛选；位于中心时播放次数和风格不加权。

有其他歌曲时排除当前歌曲的所有重复条目；剩余都是同一歌曲时尽量选不同队列条目，
单条目可重复。最近三首的权重降至 20%，避免小列表被强制排成固定循环。
最近记录与随机进度由宿主在切歌保存成功后提交，查询或预加载不会消耗进度。
保存新偏好会重新注册算法以取消旧预加载，并重建宿主的随机播放历史；当前歌曲继续播放。
“下一首播放”和上一首历史仍由宿主处理。

## 本地构建

Qt、编译器必须与目标宿主匹配；默认只依赖 Qt Core，测试另需 Qt Test、QuickTest、
QuickControls2。仓库携带 SDK 3 源码快照，无需额外下载 SDK 即可构建：

```sh
cmake -S . -B ../../build/shuffle-compass -G Ninja \
  -DCMAKE_BUILD_TYPE=Release
cmake --build ../../build/shuffle-compass --parallel 2
ctest --test-dir ../../build/shuffle-compass --output-on-failure
```

也可用 `-DCMAKE_PREFIX_PATH=/path/to/sdk` 指定已安装的 SDK 3，
或用 `-DNEKOTUNE_SDK_SOURCE_DIR=/path/to/NekoTune/sdk` 指定目标宿主的 SDK 源码。
输出目录 `extensions/nekotune.shuffle-compass/` 可在宿主挂载开发；ZIP 位于 `packages/`。
生产打包需使用与目标宿主一致的工具链；本机生成的包不保证能在旧发行版运行。

Linux 可额外运行真实宿主验收，使用临时配置和暂停状态的音频队列，不影响日常资料：

```sh
uv run --no-project tests/smoke_host.py \
  --host /path/to/NekoTune/build/backend/nekotune-backend \
  --package ../../build/shuffle-compass/packages/nekotune.shuffle-compass-linux-x86_64.nekotune.zip
```

宿主还可通过 `NEKOTUNE_EXTERNAL_TESTS_DIR=/path/to/shuffle-compass/tests/host`
和 `NEKOTUNE_COMPASS_PACKAGE=/path/to/plugin.zip` 加载可选集成测试。
构建 `compass_host_ui_test`，运行 `ctest -R '^compass_host_ui$'`，会在独立 QML 引擎中
验证队列页与抽屉菜单入口、弹层位置、保存关闭、取消与停用后的卸载；测试使用临时资料目录。

## CI 与发布

`.github/workflows/ci.yml` 使用仓库内 SDK 快照，分别在 Arch Linux 与 Windows UCRT64
构建、测试和打包，规范产物保存为 Actions artifacts，版本标签触发完整校验与草稿 Release。滚动发行版的工具链可能发生变化，
构建时校验 Qt 与编译器环境并记录到 CI 日志，不上传 JSON 附件；下载前须确认与宿主工具链匹配。
CI 通过不代表已在所有宿主上完成加载验证。

发行说明见 [CHANGELOG.md](CHANGELOG.md)。发布时根据已验证的平台上传 ZIP 和 SHA-256 校验文件，并明确预览状态和宿主要求；源码归档包含复现构建所需的 SDK 快照。


安装包及校验文件的统一命名与复现命令见 [发布规范](RELEASING.md)。本次更新见 [发行说明](RELEASE_NOTES.md)。
