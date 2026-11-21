# Frag 投后补雷显示：53616实录通过，独立预览已更新

2026-11-09，`codex/6dof-single`。本次沿用28120的实际画面问题，处理投出后原补雷重新附到手上的下一枚显示。53616已完成一次原投掷、原mode5期间的组件隐藏、原补雷完成和新裤兜Grip恢复同组件的实录；本轮有限显示流程通过，完整贴合／实体头显仍未验收。独立5d32预览已交付，旧预览和公开v71保留。

## 已确认的现状与边界

28120 的原投掷全 once1、库存 4→3，原 StopBounce 调用返回和爆炸已记录。原图 game-01149／simulator-01294 附近可见出手张手与手边链雷形状，game-01155／simulator-01301 仍可见该形状；完整模型过渡未验收。

原脚本 `ThrowingGrenade.FinishedThrow` 在有备用弹时 PerformReload 并进入 EquipNextGrenade。`PlayGrenadeReEquip` 保留原 EquipTime，在 0.15 秒计时调用 AttachNewGrenade，原函数重新 AttachWeaponToHand；GrenadeReEquipped 最后恢复 mode0／Active。现有 Frag 代码在原 Active、原准备结束和附着已证后才释放旧逻辑 hold，由 idle-unheld adapter 隐藏模型。28120 日志也记录了原 completion、return 与随后 visibility acquired 的顺序。

这说明当前存在等待原补雷完成的显示窗口，与实际短暂模型显示相符；它尚不足以逐像素确认所见形状的组件身份，仍须下一次检查实际 HiddenGame、原 mode、组件身份和双眼画面。

## 私人实现

新 PG `5d324ef39b111d70b2c22bb6fc32fcae1566c876bab4b868d244b3b27e043c4c` 基于确切生产 PG4bea，仅三个编译输入改变：

- `playground.cpp`：现有 Frag namespace 的函数前置声明与关闭配置的空实现。
- `weapon_frag_runtime.inl`：同一次实际原投掷的 return continuation 保存已释放 Grip 计数和原验证时刻；在原 EquipNextGrenade／mode5 期间申请现有可见性记录。普通 Active 判断仍沿原元数据验证。
- `weapon_equipment_stow_native.inl`：复用现有 HiddenGame lease／SetHidden 路径，隐藏原本已经重新附着的下一枚显示模型。

申请必须仍是同一原成功投掷的 lifetime、hold、父身份、Native／世界／参考基准，源当前且原验证不超过 50ms；Grip 必须仍处于原释放计数与已释放力度。新 Grip、失追／停止、过期来源或不同身份不能借此获得新的隐藏许可。开始隐藏仍须确切 Frag 类／资产、原手部唯一附着、九个当前对象身份和四个库存槽一致。普通资产读取继续拒绝隐藏组件。

补雷结束的读取例外只针对**同一份未 fault／revoke 的现有 HiddenGame 记录**。它核对九身份、四库存槽、确切组件／资产和原手部唯一附着，不将其他隐藏模型当作当前武器，不清理外国 setter 或覆盖债务。这样原 Active 到来时可正常释放旧逻辑 hold，之后仍由原 idle-unheld／新裤兜 Grip／shutdown 路径持有、恢复或撤销同一可见性记录。

不增加机械 holster、骨骼／姿态写者或 Hook；原 Spawn、扣弹、引信、反弹、补雷、附着与 timer 不改。Native 继续是已有实录的 5e51。投掷速度与 2 kg、直接跟手、原链条运动保持。私人碰撞观察器不在这版 PG 中。

## 已有验证

生产 PG 编译成功，原 93 项选定 cache／58 项 GEARS 选项保留；187 项实际 owned 编译输入只有上述三项不同。实际源和制品封存在 `analysis/frag-grip-20261108/20261109-130619-return-visibility/`。

86 项检查编译并执行确切生产 `same_parent`、`return_visibility_current`、`released_frag_attached`，使用真实 Frame 与 WeaponStowLease，身份／时间／元数据由控制 Host 提供。检查新 Grip、过期时刻、不同父身份、原模式改变、外国 setter、不同隐藏组件、记录撤销与正常恢复。

另 27 项检查执行确切生产 `pre_equipment`，确认“原 mode5 隐藏 → 原 Active 到来 → 同一隐藏记录允许原补雷完成”的控制流程；撤销／不同组件／新 Grip 均不能冒充有效隐藏读取。Engine、来源与 model mutation 接缝由控制 Host 提供，这不是实际 Game 或 Native 事务证明。原 WeaponStowLease 检查也通过。

新运行驱动的 20 项检查执行实际驱动和实际 trajectory 生成器，使用合成只读结果／回执和时钟，核对恰好一次释放、没有意外重按、拒绝只有较晚隐藏／缺回执／未完成补雷／恢复失败，以及拥有者失效后不再输入。第一次独立 C++ fixture 因全局变量 now／mode 与生产局部名触发 `/WX` 编译失败，原日志保留；只重命名 fixture 全局变量后通过，生产 PG 没有因此变化。

## 53616实测范围与冻结来源

冻结 `equipment-controls-prepared-20261109-131651-frag-return-visibility`：prepared SHA256 `cead6c2e18c95b17fcceec873003f3afc2c5a3fd9d12d1d20c05719d38c665d2`；plan SHA256 `c0a6e1e9fbaffbdb3ffc76ed3df136e20e5817fbf3f607c2a75d4da017e7d711`。1076声明源、6制品、18根依赖、23冻结分析AST、PG187／Native381及map已核，无漂移；收到新Snub131050全终唯一最终交接并独立核idle／全SHA后已stage／运行。

实际53616只在既有中央平台投一枚，库存4→3且原调用全once1。确切weapon14a1340c0/component124f00e30在原mode5期间已隐藏；投掷执行后0.1849546秒首次读取到隐藏，11条mode5-hidden样本，visibility frame973早于原return979。随后新裤兜Grip恢复同组件hiddenfalse/spinningtrue，整份最终PG日志只有一条原投掷；正常停止时stage7/fault0/preparation_owed0。没有追加第二投掷或特殊碰墙／耗尽／其他武器检查。实际typed观察124条，比队列约120估计多4条；脚本没有120总量硬限制，偏差如实保留，不冒称严格120上限通过。

运行维持0.3、后台observe-only、静音、物理控制器后台隔离及全部原保护。3页完整paired时间线与12原图亲审：出手张手、离手链雷下移和新Grip持雷可见；画面中真实投出物的存在不能当成下一枚武器组件仍未隐藏，也不能为了清屏把真正弹体隐藏。短时运动模糊／重影、启动第三人称／等待卡／HUD和完整各角度贴合仍未验收。Native/entry/pad/XR/launcher/game正常收尾，config/settings原SHA/head原缺精确恢复，runner59090/operator33544/encode73338/唯一retention45665全部0。媒体实删1543文件／254008639字节，errors0，保留53616/46592/64316及全部源码文本数值。完整53616-review.json记录边界。精确再核Longshot133536的1592源／6制品／18驱动＋1reader及AST、idle后已最终唯一交窗，不追加本窗case。

## 源码合并与交付

三文件13个片段已逐片合入共享树，保留其他会话的既有源码和新增SupportBox接口；用当前共享头编译执行确切相关生产函数和真实Lease的86＋27检查也通过。没有广编当前整棵PG／Native树，没有改Git index／commit；包使用本局实录的不可变5d32 PG和5e51 Native。备份、shared-merge-receipt.json、shared-merge-checks.json在本次源归档中。

新入口：`J:/gearsVr/candidates/frag-grip-2kg-20261109-5d32-preview/GearsVR Launcher.exe`。ZIP同名，16,053,446字节，SHA256 `94b91101b8352543e44a6c7811485053de61ba393d7902e6635e56bb85c9d31d`。752载荷文件／52,076,730字节；隔离依赖与PE预检、ZIP753成员逐项大小／SHA／CRC回读通过。打包没有启动GUI／Game／XR。portable metadata采用ASCII转义JSON以兼容随包默认GBK读取。原5e51／4bea／d510预览及公开v71保留。

2kg直接跟手／原链条保持。特殊HitWall仍缺执行实录，19332间歇拒绝原因仍未确定，完整贴合／头显／实际提示声未验收；本包不是整份Frag完成或公开发布。
