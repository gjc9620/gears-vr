# Frag 碰撞适用性：60288 几何过滤已验证，投前检查与 PG 清理失败

**15:54 最新实录：** 145754 已在用户授权的旧副本完成一次受控运行，实际 PID60288 / creation134386834683807200。八次原 Actor.Trace 与真实 UProperty/HitInfo/命中组件/Owner/flags 检查完成，Bridge closed、pending0。同一方向的零 extent 命中 `mp_warmachine_SMC` 的实际 StaticMeshComponent，x=-4350.245605、CollisionType 数值0、BlockZeroExtent=true；五单位 extent 先命中 BlockingVolume31，x=-4330、数值6、BlockZeroExtent=false、BlockNonZeroExtent=true。两平面相差20.245605单位，证实前一局选墙需要区别几何过滤。但本局没有弹体，不能把射线结果推广为实际投射物碰撞证据。

原步行到近侧93.807617单位，真实裤兜 Grip 抓取后，在读取 `bDisplayingArc` 的检查处失败并停止，投掷0、弹体样本0、库存检查未到达、特殊 HitWall 未执行。两页完整双路总览和12张实际原图已亲看，所看画面没有可见蓝弧；标志失败不能直接断言像素泄漏。现有生产代码只替换原 ShouldDrawTrajectoryPrediction 返回值，未直接写 bDisplayingArc。整份 PG 日志显示46次返回值抑制，最后94条重复 gate11 / stage6 / preparation_owed1。其根因未确定，不能以旧局通过将其注销。

Native/entry/pad/XR/launcher 正常结束，游戏进程已退出，config 原字节、settings 原 SHA、原先不存在的 head 文件均精确恢复。**PG unified_weapons_cleanup 实际 fire_disable_code2、hand_stop_code4、drained=false，因此整体清理验收失败。** Native 的 stopped/active0/drain1/resources0/entries1/timeout0 不代替原准备结清。runner2800=0、operator19708=1、encode27199=0、sole retention54755=0；保留该失败，不能以外层 exit0 称通过。媒体清理1525文件/250359259bytes/errors0，保留最新60288/62728/33732及48小时策略，源码、数值、日志、存档未删。两个输出跨度59.016987/59.066987秒，最大采样缺口1716.697/266.667ms，非渲染 FPS。启动第三人称、等待卡和 HUD 保留。

全部运行、编码、保留与恢复终态，独立 idle 后，重新逐项核对 Snub144724 的完整 prepared/plan/ready/inputs、4043归档、4制品、21根helper+AST和构建源码快照，已最终唯一交窗；Snub已确认并开始自己的短段。当前不占窗口、不在旧槽追加。本会话下一步仅离线查现有 lifetime/stop 的拒绝分支，再决定窄修复或有界只读证据；不削弱原身份、释放、时限或反重放。正式5d32手动预览和publicv71未变，Smoke按用户新指令在Frag完成后再开发。证据为 `analysis/frag-grip-20261108/60288-review.json`、`60288-cleanup-addendum.json` 与 `wall-applicability-queue.json`。

**14:58 同一未受理尾位的读取器更正：** 旧 144552 未 stage/run，保留为已撤销部署的冻结记录。当前候选改为 `equipment-controls-prepared-20261109-145754-frag-wall-applicability-read`，prepared SHA256 `f7bf5137f198685e0c2cbfa6cfbfc522134e9cfc520152c54a41846a4dab82b8`；当前计划 `wall-applicability-read-plan.json` SHA256 `6e830bea958311617342acfa62bcc4cf9f105d71585739ddd23f45bbee50d6eb`。只改手雷私有操作器对原启动报告读取的内存替换，遇到 PermissionError 或 JSONDecodeError 时在原 45 秒期限内重读，原身份、错误和就绪判定保留。共享 `run_menu_firstperson_check.py` 未改，六份二进制和以下碰撞范围全部不变。八项受控文件/时钟检查通过；1077 源、六制品、19 根依赖、24 冻结分析源及 PG187/Native381 重新逐项预检通过。该修正来自刚在同一共享启动读法中复现的文件写入竞争，未据此宣称游戏交互修复或复验通过。当前仍无窗口、尚未运行；旧身份更正已发至真实运行链路由。

2026-11-09 14:47。正式手动预览仍是 Native `5e51dfdd` / Playground `5d324ef3`，保持 2 kg、直接跟手和原链条。此次私有诊断没有替换它，也没有更改 public v71。此前 53616 的投出后手部显示恢复、新 Grip 抓取和正常收尾实录继续有效；当前没有新的游戏内结论。

28120 在 BlockingVolume 31 的五单位 extent 查询平面前生成了原投射物，但后续观测没有证明该平面就是投射物实际碰撞面。保存的原类默认值显示 BlockingVolume 使用 `COLLIDE_BlockAllButWeapons`，原投射物使用零尺寸 Cylinder；类默认值不能代替实例配置。因此保留该局“普通 StopBounce → BeginExplosionSequence 正常返回、原爆炸发生，特殊 HitWall 分支未记录”的失败结果。

新候选 `equipment-controls-prepared-20261109-144552-frag-wall-applicability` 在同一已行走平台执行最多八次原 `Actor.Trace`：四个方向先零 extent，再五单位 extent。调用前读取并校验真实 UProperty、参数范围、Vector 和 TraceHitInfo 的完整成员布局；从真实 HitInfo.HitComponent 取得组件，核对 Actor/Owner/Level 关系，保存实例碰撞标志和 CollisionType 的数值、Enum 身份。没有读取猜测的 Enum.Names 偏移，没有调用 GetEnum；运行时枚举标签仍未解析。

只选择零 extent 实际命中、BlockZeroExtent 为真且满足原有距离和法线范围的墙面。没有合格墙就于抓取前停止。随后只允许一次真实 Grip 松开，使用连续但较快的手部轨迹；原生成后读回真实 Cylinder 的半径与高度，最多 120 条对象观测。射线命中和标志值仍不能证明原生投射物调度，必须单独记录实际调用路径。输入所有权一旦失去，后续不再发布输入；不添加错误恢复手势或通用暂停菜单动作。

私有 Playground `2b657a60c9881105b913632a0b490a9d71a5fb66d876c2b1c5889e9e32e1f8fc` 从正式 5d32 的精确源码构建，187 个编译输入中仅 `weapon_frag_launch.inl` 有差异，93 个选中配置、其中 58 个 GEARS 选项保留。移除观测段和六处观测调用后，原生产控制语句逐字恢复。Native 5e51 不变。观测默认关闭、最多八条 POD：普通 HitWall → HandleCollision 为 site3，保留四条 Begin 配额。只有原 HitWall → BeginExplosionSequence 的 site1 且正常返回 state2 才满足特殊分支证据；site2、site3 和爆炸时间都不能替代。

离线验证已通过：14 个真实布局校验案例、1855 个编译后的实际分类/配额函数受控输入检查、31 个实际新驱动与原轨迹生成器的受控 Host 检查。它们未覆盖真实 Bridge、原生投射物分派或头显。驱动检查覆盖普通/不确定调用不能冒充特殊分支、无爆炸到 120 条硬上限、非法组件维度拒绝，以及永久输入所有权丢失后不发输入。最初准备插入点不存在的错误保存在 `initial-preparation-failure.json`；最初测试夹具用分离的 exec globals/locals 产生 Clock 名称错误，已只修夹具，生产 DLL 未变。

冻结和只读预检已核对 1077 个声明源、六份制品、19 个根驱动依赖、24 个冻结分析 Python 源、PG 187 个实际编译输入、Native 381 个实际编译输入及映射。prepared SHA256 `330841a5050b710eb112fccb105eac6a12b155ab8dca53905999b5ced0b96df5`；计划 SHA256 `927cd52a87bbb47a09d90e0a8c2d8eb9e2d34c4c41a9aea6d3d59541a99f9d64`。目前仅向既有串行运行链询问真实尾位，尚未 stage、启动游戏或取得运行窗口。

证据：`analysis/frag-grip-20261108/wall-applicability-plan.json`、`wall-applicability-preflight.json`、`wall-applicability-driver-check.json`，以及 `20261109-143400-collision-applicability/build-result.json` 和 `policy-check.json`。完整 Frag、特殊分支、旧 19332 拒绝根因、全角度贴合及实体头显手感均未标为完成。
