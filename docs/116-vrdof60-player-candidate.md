# VRDof60：死亡双眼恢复的本地候选

2026-10-17。本地默认切为 `VRDof60 / Borderless1`，命名清单 `manifest-vrdof60-borderless-vd.json`。旧 `manifest-reticle47-borderless-vd.json` 及不可变ZIP保留回退。用户配置未覆盖；后台实体手柄默认关闭，游戏分辨率继续生效，DLSS／DLAA锁仍保留。

源码冻结 `4fd6884a4d1b2d3812ce2696943177bf4effaafd`，DLL SHA256 `599dd838f08b832b2fbd4265bf4251ed90f17535f0af43f97edd8d17a0bfdc66`；XR仍为Borderless1，SHA256 `2c27d29ca68a1629cb987addeedbc49ea59f2f7355408802a4b84ca7d96fe5eb`。延续Reticle47，另加入已认证的无占有死亡视图及两眼一致的景深兼容；舍弃这一路VR景深模糊，避免已复现的死亡右眼全黑。不是复制左眼给右眼。详细作用域、逐眼证明、死亡／检查点／电影回归及边界见[兼容验证](115-vr-dof-compatibility-regression.md)。

## 独立包

本地ZIP `analysis/release-zips/GearsVR-V1-development-Vrdof60-20261017.zip`，SHA256 `032f4d6d065cd7dbe0b041facbf2ed0e627ff849913d5126df10caa3146cc1be`，13,631,352字节、701成员；700个payload文件43,453,835字节，inventory SHA256 `18a4085d50f92a0426bf4a561bdefb2d0771d21ea6e155116c2f7f1c914a8b45`。包不含游戏、模拟器、逆向数据库或伪代码。

全新解压目录以包内Python `-I -B`隔离预检并实际启动包外游戏。Portable104会话 `continuous-stereo-34716-20261017-135841-9e69a18d`，两份3840×2160 GPU图左右均可见完整庭院场景。停止后会话通过、原生独立恢复、普通游戏关闭及四存档恢复全部通过；运行后700个清单文件哈希未变。使用子进程显式模拟器运行时，没有改系统OpenXR注册表；实际Quest3／VDXR未在本轮验收。

ignored证据：`analysis/runtime/vrdof60-zip-proof.json`、`portable104-preflight.json`、`portable104-audit.json`、`portable104-normal-close.json`、`portable104-save-restored.json`、`portable104-payload-recheck.json`。包默认要求玩家选择游戏路径，VDXR为GUI默认；此为本机开发候选，`release_ready=false`，没有公开发布。

## 仍需完成

- 新候选的真头显准星融合、HUD／字幕、机枪／载具和长时间游玩。
- 启动及检查点仍有秒级等待；电影交接首张原生快照有一次全黑，后续恢复，不能称全程无黑。
- 活跃游玩帧节奏／卡顿与干净玩家机器部署。

现有自动证据支持上述死亡路径修复及已测试的恢复流程，不支持“所有黑屏根因已解决”或“首版已可发布”。
