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
