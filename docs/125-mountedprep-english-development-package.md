# MountedPrep English 开发测试包

2026-10-17。本轮根目录启动器已可替换，未关闭任何进程。新 EXE 与此前编译／隐藏自测通过的文件 SHA256 均为 `35bde70d109aa12a2c7ec388144fc35890ac57a6966ce562ad669338398b7c97`。它包含首次默认英文及显式中文偏好保留；不再是此前因文件占用而暂存待替换的状态。

独立开发包：`analysis/release-zips/GearsVR-V1-development-Vrdof61-MountedPrep-English-20261017.zip`。

- ZIP SHA256：`06457aa02529c7538eb4e4781416948cdfa73e57728e663c1f15d86c57af33a9`。
- 13,647,569 字节，701 成员；700 个 payload 文件合计 43,460,645 字节。
- inventory SHA256：`8b4ac165ffff61bfdfc1e865ff30b207609641a4a48c796998232a508643a0af`。
- Native 为冻结 VRDof61，源码 `88cfae6`，DLL SHA256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`；XR 为既有 Borderless1。
- Python／打包源码基于 `24f9dd1`，包含相机读取窗口优化、mounted 采集局部复用和暂态分类、重绑定内部拒绝详情，以及默认不启用的 UI 布局采集选项。

新目录 stage 与 ZIP 全新解压后的包内 Python 隔离预检均通过，全部 payload、原生／XR 及上述修改源文件哈希一致。默认英文、游戏路径为空，需要选择支持的游戏 EXE；VDXR 路线不改变系统 Runtime 注册表。相机距离和 HUD 的便携默认未导入本机用户设置。按游戏分辨率，DLSS／DLAA 锁定关闭，6DoF 实现保留。

完整回执在 ignored `analysis/runtime/vrdof61-mountedprep-english-package-receipt.json`。旧 VRDof60 与 AnchorPrep ZIP 的哈希复核未变，本地默认 Native60 清单和用户配置未替换。新包没有游戏、模拟器、数据库、快照或伪代码。

本组合 `runtime_verified=false`、`headset_verified=false`、`release_ready=false`。机枪黑屏尚未实际回归，岔路 UI 尚未启用位置调整，长时间游玩与头显卡顿仍开放。此次打包是保存可回滚、可独立测试的开发组合，不是 Discord 正式发布。
