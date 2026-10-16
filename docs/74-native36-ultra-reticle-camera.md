# Native36：4K输出、准星外圈、镜头与采集边框

2026-10-17。用户反馈Native35在4K／Ultra不能进入双目，准星中心图标正常但扩散外圈有问题，希望启动器可拉近人物镜头，并询问黄色采集边框。用户随后离开休息，明确授权继续修复与无人值守测试；后台实体手柄保持关闭。

## 4K发布失败

失败会话`continuous-stereo-24660-20261017-031525-76586caf`中，相机／左右眼提交通过1637次而GPU发布为0。原生状态的`final_eye_failure`行135／137记录同一有效票据的左右1920×2160半区，目标3840×2160、format24、quad caller `2b11a0`，每眼817次；`eye_valid=1`但draw gate拒绝。现有路径的五段运行时代码哈希认证均通过。这是具体输出路径拒绝证据，不是仅凭分辨率猜测。

旧判断只允许此调用写入中间纹理，排除了直接写入当前backbuffer。2a98301允许已认证的逐眼路径同时写入这两类目标，保留票据、UV、几何、格式、Draw／Execute／Present及未知写入检查；没有开放`d83cbf`或重复整张最终画面。dd874f6补齐GPU合成自测遗漏的caller scope，不放宽产品判断。

## 准星外圈

缓存UnrealScript `WarfareGame.WarHUD.DrawWeaponCrosshairGOW3`（artifact2，导出SHA256 `082c66d6ce62a562f645929c9f09e6e2cff0c5914362bd3a13afdc7b3fa5e4d0`）显示扩散图标可由四次DrawIcon绘制，位置随Scale／XOffset／YOffset变化；脚本仍含未解析native token，不当作完整引擎还原。

旧原生准星策略要求每个GPU segment单独居中，分开的象限会回退而中心icon仍能校正。3ab3e90／cd77fbc把同一语义root、已经独立认证为纯Canvas准星的segment bounds联合用于居中门禁，沿用同一票据投影；不改源顶点、扩散间距或借用不同root／HUD的几何。离线测试覆盖四象限分段、独立中心icon、不同root和偏心回退。当前实际回归没有成功开镜／开火，`ui_reticle_direction considered=0`，所以这仍是实现与离线验证，不能宣称用户的外圈问题已实际解决。

## 镜头与边框

f294764新增`camera_closer_metres`（0–0.5，缺省0）及双语启动器控件，能力字段`camera_distance_control`。以原游戏相机前向同时平移两眼，保留IPD／6DoF；仅普通人物相机应用，排除cinematic和mounted，并保留至人物眼部至少0.4米的前向距离。这是实验性渲染镜头调整，不改游戏射击／碰撞逻辑，也未认证墙体遮挡与新距离的头显体验。本机配置设0.3米；控件可调回0，便携包缺省仍为0。

4c7a402在菜单／电影WGC采集上请求Windows Borderless权限并设置`IsBorderRequired(false)`，不改关卡原生纹理发布路径。保持正常挂起／恢复，权限失败时保留可用采集。此次日志`wgc_borderless_access_status=4`、`wgc_borderless_request=accepted`、`border_required=false`；这证明系统接受API请求，尚不是肉眼检查边框消失。黄色系统采集提示本身不能证明原生双目方案有误。

## 冻结构建与实际回归

Native36b源码冻结于dd874f6，目录`analysis/native-source-gameplay36b`，复用已记录的MinHook依赖；不含工作区未提交的旁路修改。DLL `tools/native-capture/build-gameplay36b/GearsCaptureGameplay36.dll` SHA256 `3a669ae8f64567462fc2e31c1cbf37ecb7e7f4fd701b4e3768768b667db16f73`。XR `tools/openxr-probe/build-borderless1/OpenXRPoseProbe.exe` SHA256 `2c27d29ca68a1629cb987addeedbc49ea59f2f7355408802a4b84ca7d96fe5eb`。14项原生检查、56项启动器检查及GUI构建／自测通过。首次36冻结自测失败保留在`gameplay36-selftests.json`，修正后结果为`gameplay36b-selftests.json`。

实际模拟器会话`continuous-stereo-30096-20261017-034227-40dd3b0b`沿用保存的4K画质配置，显式模拟器Runtime且后台手柄未启用。自动进入现有战役后发布，死亡菜单经完整身份／菜单树校验后用目标窗口Enter读取检查点，world generation 1→2并恢复双目。没有开启新战役、瞬移或修改存档。操作中Windows拒绝再次获取焦点，未绕过后台手柄限制，因此未完成开火准星采样。

- 原生发布1763、capture attempts1767、busy2、linked1767；无ticket／rectangle拒绝，存在2次污染帧。
- 镜头0.3米实际应用1776次、inactive0／rejected0；HUD布局应用4053次、fallback197，HUD序列错误0。
- 请求图像1：票据480、source serial472、3840×2160，saved monotonic ns `41414615792200`；原始GPU图与缩略图显示两眼人物／场景。没有用两眼像素不同代替头显融合验收。
- XR记录4410次projection提交、1760张图像复制。跨度174.598秒中含死亡菜单停留，最长projection间隔125.308秒，不能称为连续游玩无黑屏通过。
- 正常停止，native cleanup、入口恢复、XR退出0全部验证；身份核验后WM_CLOSE正常关闭游戏，没有强制终止。

原始状态、日志、图像、构建清单及回执均在ignored `analysis/runtime`。新离线候选`analysis/release-staging/gameplay36-offline-20261017`隔离预检通过：695文件、42673824字节，包含新的camera helper与中英文README；`release_ready=false`，没有做干净玩家机器或真头显部署验收。

下一步仍需：真头显4K／Ultra、开火外圈、更多CG／机枪／载具、完美换弹、长期游玩、卡顿及启动延迟。此次4K修复不证明上述全部完成。
