# 兽族步枪普通空仓换弹候选检查点

目标仍为完整兽族步枪开发和验证。本检查点不代表整枪完成或头显验收。

当前候选 hammerburst-prepared-20261110-000625-empty-first-failure，prepared SHA256 2f53b54a1cd435270551e4c176a9a1bb0b3725966f59af07bf063a18b9e6427c。PG a05a6c3ce20b4fa6a82f41a03a38ab690b69543d84193ad96c3b75f8619f0d5c，Native 806dcbd6a92ecd337923f4ecf315e56ab7afd475e247a8a3cacdfe2ab30b8b0c；公共 v71 未改。

生产修复是已有手动换弹权限分类补入 HB，使原 CanReload 拦截可覆盖本枪。编译和离线分类检查已过；真实回调尚待本候选实测。严格空仓脚本遇到第一次锁扣拒绝即保存并停止，禁止旧版恢复输入重试。

本次目标链：核心六发后 L72/R78，十二次真实六连发至 L0/R78，一次空扳机验证不补弹、不产生射线且真实 veto blocked；随后真实非零 reload serial、原模型骨姿开盖、旧匣提取释放、肩取新匣、座入关盖，L78/R0，再六发回射至 L72/R0。首败停止。尚未运行，不把包装器退出零等同动作通过。

Boom 独立受理当前 v3，同一原排队位；证据 analysis/boomshot-development-20261109/successor-HB230535-admission-v3-check.json，SHA256 c94e2ba7357e9e5e4efbb6d5bf92c2e31a270323d4d284fedd5f6ec9d1697529。当前无最终交窗、无 claim/ticket、未 stage/run。必须等 Boom 新实际运行及完整正常清理、审片、编码、十基线恢复与保留收尾后的唯一明确交接。

入口 v3 SHA256 d60ba49bc8aaa71c0ca1b45e80223a03fa1144703da26fa549b7d29dd313734c；ready 4edac018f8651d7b6677445458f61b83631f98c1165f1ff299a7207b62665c5d；plan b7286f9d4712a866a72ef7ecd607f9fb029a74d9d2eebf18b5ac28f54deac371。精确 prepare-only 已通过。v2/v3 除四个固定身份字符串外 AST 相同；内层实际 Python 环境检查通过，消除继承优化及 safe-path 干扰。源及两项复核程序见相邻 source.patch。此检查不启动游戏或获取运行权。

本次另独立检查唯一后继 Frag232727 v3 的最终交接身份、精确十路径基线及子进程环境隔离，并实际仅执行 prepare-only：8860 host files、冻结源/六制品/运行依赖 bad=[]、六 import 检查通过，队列字节不变、真实 claim 不存在。证据 analysis/hammerburst-20261108/successor-Frag232727-admission-v3-check.json SHA256 7159cb8b1060a88761643c0549591bcadb183f1beb09a10a62bac62be079b91d。仅登记为本枪未来完整收尾后的唯一后继，不授当前执行权。

上一轮 50148 的真实开盖骨姿失败与 serial0/提前补弹观察保留。当前候选不继承旧 PG 的动作通过结论。手型贴合、完整换弹、全部中断条件及头显感受仍未验收；本轮不扩展耗尽总备弹和无限离线手型拟合。

## 运行前证据边界复核

当前源码确认 needs_reload 为未满弹匣且有备弹（或无限备弹），只有该分支调用原始 begin 并取得 serial。50148 的 L78/R0/serial0 走 visual_only 检视分支，因此其骨姿失败不能证明真正空仓原始换弹分支同样失败。保持真实骨姿阈值，不据此提前改变动画实现；下一局必须先观察非零原始 serial。五组纯谓词例子见 analysis/hammerburst-20261108/empty-reload-source-path-audit-20261110.json，不是引擎验证。

另发现现有 empty driver 的 manual_veto_consumption 是全局累计日志，blocked>0 没有本次触发前后增量、武器或 press 绑定。因此只能称本局曾发生原始 CanReload 拦截，不足以独立证明本次空扳机回调；未来报告必须把该弱证据与同枪 typed L0/R78 前后不变、零新射线的可观察行为分开。本次不改已受理候选、不重冻结或扩大实测，保留此限制，不将弱证据升级为单次调用证明。

## 护木四态旋转回归

在现有 weapon_hammerburst_support_test.cpp 中新增五种枪旋转姿态，使用本枪真实已编写握点坐标和原生产 SupportGripParticipant/SupportSocket mode。每种依次检查 Snap/single、移动枪与手后 Snap/single、按 Grip Grab/two、移动后保持 Grab/two、释放回 Snap/single、离开范围 Free/single；Snap 必须无租约，观察不能改写物理 Frame。MSVC /W4 /WX 且 assert 开启，编译0、执行0。原四态和11拒绝检查保留。receipt: analysis/hammerburst-20261108/support-authored-rotations-check.json。该回归只证明合成输入下策略与仲裁，不证明游戏枪变换消费、皮肤接触或头显自然手型；未更换当前已受理运行候选及制品。
