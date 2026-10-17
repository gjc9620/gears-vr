# Reticle46玩家候选版与实际回归

2026-10-17。本地默认采用Reticle46／Borderless1，保留Reticle39及更早命名清单回退；玩家config未覆盖。源码冻结自e70ee90，目录analysis/native-source-reticle46，DLL SHA256 `35852ca9c02a94bdffc9c16de27457bd87185d7531dd39fa008f6f874b3c5f26`。

该构建关闭GEARS_UI_CALL_BOUNDARY_OBSERVER，不安装诊断调用边界Hook；实际状态compiled=0。包含新批次来源修复。10项聚焦测试及便携包所需TwoViewRendererShadowTest通过，游戏启动另执行同次必要自测。构建证据文件沿用脚本名calltrace46-build-proof.json，内容中的source／dll才是实际Reticle46身份，不能按文件名前缀误判为诊断版。

## 手枪与检查点

pose75会话continuous-stereo-6904-20261017-085738-0638da5a：数字方向下切换，图像确认手枪，LT开镜、RT=0，模拟器实际非中性姿态确认后执行。2937批次尝试／发布、各阶段拒绝0；144次准星考虑／计划／应用、finite144、fallback0；HUD unknown_or_mixed0。图像ticket455／461与保留的四顶点准星记录重合。这里只证明该手枪场景正常，不证明所有图标身份、瞄准命中或物理融合。

reticle76脚本在启动阶段遇到PermissionError，原生激活尚未完成便取消；不能算回归通过。所有者和XR退出后按完整游戏创建身份普通关闭，native_cleanup_verified为空，未宣称原生恢复验证成功。原始audit保留。reticle77只为脚本状态JSON只读采样加入有限重试，保留实际输入前原有身份及菜单检查。

reticle77会话continuous-stereo-5800-20261017-090201-61cff8d5：两次死亡菜单读取检查点，world generation1→2→3，重新观察到新代至少10次提交。输入到恢复观测约8246／5367毫秒，包含加载及脚本观察，不等于黑屏时长。恢复图ticket910／1816两眼均有非黑场景。正常停止、原生清理、独立入口恢复和游戏普通关闭通过。

## 本地便携包

ZIP：analysis/release-zips/GearsVR-V1-development-Reticle46-20261017.zip，SHA256 `90ddca508b5ec149554b2c02d618ceeaf4c3d40d02f83b966d9ab05b5e665fc0`。701成员，13,636,739字节；700个payload文件，43,426,854字节。严格按inventory打包，未递归加入analysis日志、反编译资料、游戏文件或模拟器。

新stage和全新ZIP解压目录隔离预检通过，inventory SHA256 `cc0f4b9e2fd56d0e319bd0aaaca17baeaf4b04e0f5ea9b3caff5f0ba201fd670`。portable78以包内Python的-I/-B、包内launcher、包内DLL及XR程序实际启动外部游戏，使用包外测试config指定游戏和显式模拟器运行时，没有改全局OpenXR注册表。

实际会话continuous-stereo-18232-20261017-090601-c2116435：确认实际首投影、发布及epoch提交后保存3840×2160场景图；正常停止、原生清理及游戏普通关闭通过。运行后700个清单文件哈希未改变。新ZIP未公开发布，release_ready=false；干净玩家机器、GUI实际部署、真头显和剩余场景验收仍未完成。

## 第一版剩余缺口

- 真实占用机枪／载具的持续双目显示。
- 更多CG、长时间游玩及间歇黑屏／卡顿复现。
- 真头显准星融合、UI位置和字幕显示；长字幕、多语言案例。
- 启动及切检查点仍有秒级恢复延迟。
- 干净机器安装和包内GUI验收。

源码、CPU测试、模拟器图像与真头显验收分别记录，不能把本次来源缺口修复当作第一版已经全部完成。
