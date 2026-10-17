# AnchorPrep独立测试包：包含最新启动优化

2026-10-17。此为新的离线开发候选，未启动游戏、模拟器或GUI，不替换本地默认VRDof60、用户设置及旧ZIP。`release_ready=false`，最新组合的`runtime_verified=false`。

包内原生DLL为冻结VRDof61（源码88cfae6，SHA256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`），包含死亡两眼景深兼容与连续启动就绪屏障。Python／打包源码取014fe3e，含fa74d8a的[相机读取窗口优化](118-anchor-sampling-window-optimization.md)，以及中英部署说明中的VR景深关闭说明。它不是未经修改的旧61快照，也不是新的原生DLL版本。

ZIP：`analysis/release-zips/GearsVR-V1-development-Vrdof61-AnchorPrep-20261017.zip`，13,646,105字节、701成员、700个payload文件43,455,582字节。ZIP SHA256 `0bc2da272b4394e5f16d58e7af56a4d4c7e4f508ffbb84e01a5ce5235a5e3092`；inventory SHA256 `0c770b19b9ba961b3b3ab3c739f3e1eaa3f0b79aef4d41066e6f57ac7b76b557`。

26项打包测试、stage及全新解压目录的包内Python隔离预检通过。全部payload、原生／XR和更新的两份Python源文件哈希复核一致。没有游戏、模拟器、SQLite、快照或伪代码；保留空的运行日志目录。完整回执为ignored `analysis/runtime/vrdof61-anchorprep-offline-receipt.json`。

部署仍是完整解压、启动GearsVR Launcher.exe并选择受支持的GOWDE-Steam.exe。Quest3／VirtualDesktop用VDXR选项；包不修改系统Runtime注册表。当前包没有新的游戏实测或头显验收，不把旧61运行结果移作最新Python组合的通过证明。启动偶发认证失败、UI／机枪、长期运行与头显帧节奏仍按现有进度记录保持开放。
