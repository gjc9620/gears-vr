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

## 冻结候选同组护木回归

后续核对发现当前工作区 weapon_support_grip.h 已加入后续的可选 SupportBox 及接触缓存，而本枪2f53冻结候选仍是先前球形接触实现。为避免把工作区检查冒充运行制品检查，把同一测试复制到独立 harness，仅将三处顶层 include 指向候选原存档目录，由该目录解析其全部相对头文件；原存档未改。五旋转/四态/11拒绝及物理 Frame 不变检查在冻结头文件下再次编译0、执行0。当前与冻结两者的结果均为有限离线策略证据。源码差异、harness、build.cmd 和各 SHA 收据在 analysis/hammerburst-20261108/support-frozen-rotations/，result.json SHA256 0912949caf3fa725e46d8e68d27dec24e90b1124c2198a7f8fd2ed619f624845。未替换制品、未重新申请运行位、未 stage/run。

## 005428 实际一次启动失败及独立恢复

收到Boom新game24964完整收尾交接后，独立核收据33d4a30c、七原文证据及六原图SHA、十基线、idle与精确prepare-only0，再保存本轮即时十项raw baseline。入口v3真实一次消费并stage；launcher30156/creation134387168841386715在创建游戏前失败：graphics_policy.locate报Multiple settings bindings for the selected installation，reservation_failure_cleanup=no_birth_receipt。未生成游戏、未注入Native、未启动XR/动作/双路录像。启动器exit1；外层随后另KeyError session_report/exit1，两个失败分开保留，无自动重试。

config原有反斜杠P2P key，既有Longshot selection helper又按本枪plan斜杠字符串添加同一安装key；规范化后两个匹配，原唯一绑定保护正确拒绝。独立idle后通过原restore helper精确恢复十项原字节，public只读核；launcher进程已终。一次retention正常完成、删0，保留24964/47996/9764。没有录像则审片/编码不适用，不称encode0或游戏cleanup0。证据analysis/hammerburst-20261108/startup-005428-failure/review.json SHA498f1e012bde619dd2dfb8b7c011926d5bcf18f323e1927a95417ac6b8593612，含原session/config/reserve/done/claim与恢复收据。

新增HB自有选择器，仅在明确owner选择的安装中按与graphics_policy一致的resolve/casefold替换路径别名，保留其他安装绑定，拒绝非法映射。不改共享Longshot helper、不改graphics_policy保护。实际原graphics_policy.locate在临时fixture目录下复现旧函数双别名失败，修复后10例通过：两种slash/case、不同目标的重复别名、无绑定、其他安装、幂等及非法映射拒绝；真实配置/INI未修改、未启动游戏。源码和复现见相邻settings-binding-repair.patch。旧2f53候选已消费，尚未采用新helper、不复用旧ticket/claim；后续需新冻结身份并沿真实尾重新排队。

## 后续入口草稿的无出生失败收尾

新入口v4草稿增加已观察到的无session_report分支：仅在启动器非零退出、done/worker/launcher身份对应、remaining_owned_game明确None、phase=incomplete、无game_identity且no_birth_receipt时可进入；先独立idle，再通过原restore恢复十raw，输出startup_prebirth_verified失败收据且返回非零。不把它当游戏cleanup成功，也不在证据不足时恢复或重试。实际本次归档及12个错误归属/状态反例共13项校验通过；只执行提取的真实校验函数，未导入入口、未stage/restore/game。源码与复现见相邻prebirth-failure-entry.patch。草稿hammerburst-prepared-20261110-010515-settings-alias尚未冻结/受理，不能执行；需同版本GN依赖确认及新身份。原候选和已消费claim/ticket不改。

## 新settings-alias候选仅封存，尚未排队

GN确认4文件版本稳定，原说明SHA4423b4d04cbf0821ef2d500c77525484b6c136c3827006776045105d2f597b5c已核。新候选hammerburst-prepared-20261110-010515-settings-alias：prepared a186945574e57260852df87aca2be769d4db19301843a5932273e35b570ae112，plan aa75ebf064afa17894b1bbd1154575e264aa822f2a9f446d7b27686b7d447704，ready d0a1844503c03d51b86218a84e4aa29ac3a3a66d14027499d05c61cc2a93e632，entry 14ce87a843677ea3985dda7be1b2f1424ea0ff70ef7b1efc2ceb02d45dc73ef9。2270归档源、1338 Native源、六制品核SHA；PGa05a/Native806不变。完整1057 Python边界仅更新四个GN已确认原件，其他1053逐项不变，校验未删除；HB20直接入口不变。采用本枪自己的settings helper，避免依赖仍在编辑的LS辅助函数。

实际精确primary -E -s -B -X utf8 --prepare-only退出0，未查询共享runtime、未stage/start/claim/ticket。新v4完整复制入口八种前置失败与八进程竞争检查通过（仅临时根，1消费7拒绝），真实无出生校验13例通过。入口/冻结复现原文见相邻settings-alias-freeze-source.patch。当前request_ready=false；尚未向尾申请。旧槽仍仅持交接收尾，等待Frag已确认的最小依赖替换，并不利用旧窗口再次运行HB。

## 无出生失败最终交接完成

Frag同槽v4替换仅更新289边界中的两GN稳定原件、计划边界引用和固定入口身份；独立核全部289、差异、精确准备命令退出0，真实claim不存在。最终唯一交接startup-005428-final-handoff-Frag232727-v4.json SHAab032fb57d639d5828ee831bd9648295ce985d7bbd43b37dfc1fb332b7573eff，绑定prepared9d5/plan8182/runtime-ready64f8/entryc8ce/执行ready8edf。无birth收尾范围明确，不冒称游戏cleanup或编码0；十raw与idle再次核过。HB已释放窗口，停止共享运行文件写入。

HB新候选在完成旧交接后仅把request_ready标记改true，prepared/plan/entry不变；最终ready SHA c4315976ff9302b074155d241c3e5379e3d8005e648177347856208d9b3dfe96。原实际prepare-only观察仍保留对应旧metadata SHA，不冒称新运行。当前已沿Frag唯一后继链请求真实尾定位，尚未受理新位。完整单一packet为analysis/hammerburst-20261108/admission-v4/tail-packet.md。
