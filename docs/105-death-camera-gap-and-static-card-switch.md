# 死亡相机空 Pawn 与静态提示卡切换

2026-10-17，继续推进第一版。Reticle47仍为本地默认；本轮先分离已确认的票据中断原因与尚未复现的黑矩形。

## 静态卡 A→B→A 的实际 GPU 对照

独立探针85原样包含产品TransitionCard，两张不同内容的静态swapchain每120次成功quad帧切换。实际PID8500／creation134366769365670087，EXE SHA256 `7be2499fd565090c523d1716fec9ce5c62179aaac93be635baaea10c135f8bad`。官方RenderDoc分别在A首次上传、B首次上传、返回A首帧捕获；三次Start/End成功，运行3490次双眼quad，A/B各上传一次，40秒后正常请求退出，全部销毁0、gpu_unsafe=0、result=0。

三份新抓帧回放后，768×256 GPU纹理逐字节分别匹配CPU源A、B、A；三份1280×568 GPU最终合成输出的两眼均有对应文字。由此排除了该独立序列中“两张同尺寸静态swapchain切换必然混淆”的假设；没有覆盖实际游戏投影与quad混用。原始捕获、逐纹理哈希及合成图在ignored `analysis/transition-card-probe85/run1`。首次回放检查器只导出固定1280×588输出，未覆盖实际568高度，导致分析断言失败；改为有界动态尺寸后重放同三份抓帧，没有重新启动探针，旧分析文件保留。

## 已确认的死亡过渡原因链

Reticle84实际记录：首个anchor拒绝QPC638851276737，`stage=pawn field=controller_pawn actual=0 actual_valid=1`；前16条保存记录均如此，总计855次。anchor_observer强制非空Controller.Pawn，因此scene receipt不生成，后续renderer没有receipt也不生成新双目票据。该计数与anchor_submission rejected855、scene/family identity_failed855、renderer no_receipt856一致。

最后新票据984的源时间63885095010700ns，约在首个空Pawn读之前32.7毫秒。旧图继续复用；first_missing 63887096924200ns，距其源时间约2.002秒，随后632.427毫秒无非空提交，直到死亡菜单的startup_panel捕获恢复。632毫秒不是新图停止的全部时长。没有找到Health≤0直接关停条件，也没有证明Camera对象已更换；不能据此删除非空Pawn门禁。

## 下一项定向观察

加入默认关闭的GEARS_PAWNLESS_CAMERA_OBSERVER，只在实际成功读到Controller.Pawn为空时有界采集本地玩家／控制器／相机／Level的已认证地址、注册表／数值身份、已有方向链接及相机POV／timestamp前后读取。Config没有已认证local.controller偏移，明确不猜测／不读该反向链接；稳定零值也不算身份匹配。

Scene前后及同线程后续renderer还记录family／source指针和矩阵副本，最多16项、采样间隔250毫秒。不生成Anchor，不放宽票据时间，不授权绘制，不增加游戏Hook。helper独立CPU测试通过，包括读失败被写零、身份／链接／注册表／POV变化及越界拒绝。真实空Pawn期间是否仍有有效场景视图，必须由下一轮候选运行验证。

## DeathObs48 实际观察

源码dee61c5，诊断DLL SHA256 `aa2f9edb02db5aa921c9417faedb3ceefdf75ee4cb68ca3cc2ab355fc28cee4f`。观察器ON/OFF两种完整DLL均编译通过，9项聚焦可执行测试通过；玩家默认保持Reticle47。实际会话 `continuous-stereo-3368-20261017-102437-2d618a24`，游戏PID3368／creation134366774644915963、XR PID12304。实际一次检查点恢复world1→2，新代20次提交；正常停止及独立清理／普通关闭通过，四个存档恢复并核验。

16个空Pawn场景样本均在同线程到达后续renderer，原始source视图0x430字节完全相同（read_mask=63）。64个内层Pass均四角色数值身份匹配0xf、已有方向链接匹配0x1d、注册表有效3、全部已有字段成功读取且稳定，POV有限/FOV80。16次覆盖QPC654106972907至654145841610，相机时间24.7831573至26.7992649；这不是原子引擎快照，也未重认证名称文本／类链。

各Camera.ViewTarget均0x207ce680，与本轮UTF16 AnchorObserver配置role_pawn=3的完整启动证书一致（Pawn_COGMarcus，outer为已认证Level）。说明并非相机突然丢失：PC清空Pawn后，仍以同一个原角色作为ViewTarget并继续建立／渲染视图。`Pawn.Controller`本轮没有读取，不能声称已观察到0。汇总及完整原始记录见ignored `deathobs86-summary.json`和会话状态。

据此下一最小实现是单独的无占有视图目标模式：仅对已认证同一Pawn作为ViewTarget的情况，重新完整校验已有身份与方向链接，明确要求Controller.Pawn和Pawn.Controller均为0，并以相机POV为锚点。该方案仍需设计审查、负向CPU测试和实际运行；不能把本轮诊断等同于双目死亡相机已修复。
