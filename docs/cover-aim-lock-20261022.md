# The target locks were repaired when the bunker was holding a gun.

2026-10-22, integrated into 0.1.0 Develops a normal starter. This time the bunker is being repaired with a live/relevant HUD returned to its head location missing, which does not constitute a new release or head-over acceptance.

## Reasons and changes

Original from `UpdateHUDActions` Query reply extract for hit points `bIsTargeting`And write it in the frame. `aiming` Mark. During the bunker, queries may not be running or generate a reply that can be used in the current scene; even if the controller is still aiming at it,`aiming` Also keep the default false, normal HUD returns to the header plane. Whether a limited hit point is available or not is carrying a gun is two different conditions.

Read the certificated controller target properties independently when constructing the current frame. Read the frame anchovies/Pawn as a controlr, double check registration identity, class inheritance, two-way holding relationship, attribute type/size/diversion/bit mask and Bool access assignment; only submit it when the sight position of the read is identical. This reading does not require searching for script frames, local variables or midpoints, nor depends on the last weapon remaining. Use the current status after detargeting without retaining the upper frame.

The hit point is still checked by the original identity, time and camera but its authorized results no longer cover the frame of the scene with a gun. The star-point and HUD continue to share the same frozen eye geometry: use the world point when available for limited points and use the original game source view when missing. So head postures change only their projection in the eyes and do not re-choose head centre because of missing queries. This direction does not amount to an actual weapon-launching ray certified; this time there are no weapons, ballistics or role animations.

## Automatically check

- 13 The primary inspection is carried out, including new search frames that are not readable, old weapons that are not readable after a change of gun, de-targeting, changes in state during reading and cases of identity/ attribute rejection; and the simultaneous performance of checks which are already aimed at HUD, parastar projection, frame life cycle, first person designation and final output.
- HUD Projectivation Overwrite 1,215 Group head position/rotation/target depth; quasi-star projection overwrite 729 Group posture. The limited points are still checked for source world direction when they are not available, and ordinary non-targeting behaviour maintains the planned results.
- 22 The first step is to get the original check passed before it's started.208 The item starter check passed. An old test that started the diagnosis directly compared mtime to a high-precision wall clock and obtained a false old determination on Windows; bound only the fresh test sample to its actual mtime, retaining missing, erroneous PID, old time stamp and dissensitisation claims, without changing the logic of the diagnostic. The first failure and rerun record is kept.
- MSVC/Ninja Record 484 The item relies on confirmation that the actual compilation contains two original documents, the original first person ' s achievement and the original output strategy after the modification.

The primary test binder is complete with valid, two-way metadata on the objectProperty; the missing and failed initial test parameters are kept in the ignored directory and not considered as a result.

## Retail back-office evidence and limitations

Test only started `J:\gearsVr\Gears of War Reloaded` . A copy of the working version, disk EXE SHA 256 Yes `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. Use independent examples, private archive/sets, validated bend-windows policy, read-only output surveillance, test silent sound and keep the physical handle backstage input closed. No historical archive folders are used to cover user progress.

Move into the bunker through original/shelter key, get it 10 Number of times `CoverType=2`、`CoverAction=7`. The original records are as follows:

| Inspection | Result |
| --- | --- |
| Current scene sighting effective read | 10,017 Frame |
| Raise frame/no HUD query reply | 796／796 |
| Current scene aimed at reading rejection | 0 |
| Aim HUD Planning | 4,354 Every second, use source world direction back |
| Observator budget exceeding limit | 0;maximum single record 0.3312 ms |

This confirms that the actual bunker is still in target UI path when the query is missing. The latter section appears as original `PenderCrossplayInvitePopUp`, automatically enter the stop by the guard;16 The sub-plan entered was not all completed, and the reply retention status was interrupted.

Original strict session reply to incomplete: independent stoppage check 7 System `dxgi.dll` The entry is different from the disk PE byte. This is also the default check limit for building an existing one that cannot be original `entries_restored=1` Separately. This round of independent checks saves another byte of the actual stoppage SHA.256;additional audit prior to installation of line-by-line reconciliation with PID 32 Byte Record,7 All of them are the same as the actual shutdown to read back Hashi, among which 5 The entry also has consistent pre-installation/drying double records. Independent check of the game entrance is passed. Original strict reply was not rewritten as passed, and supplementary audit only confirms that the inspected access returned to pre-installation status.

Native Resource Cleanup Pass, XR exit 0, Playground entrance is back through, audio is back through and then confirmed that the game process exits. The user's original location 10 A set/archived file corresponds to a single copy before it is prepared. No FPS comparison conclusion.

The head motion was not applied to the active simulator: the diagnostic entry was directed at another parallel example and therefore did not modify the overall attitude. The evidence for head movement is derived from the above-mentioned CPU projection; no new GPU pixel alignment, actual launch line/bullet impact point or physical headshot acceptance were added.

## Default Integration

From the latest default `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855` Playground output freezes source code production, replacing only original files and test folders involved in this time; retaining all original build switches, first person names and FXA output changes.

Normal starter is now in use `tools/native-capture/build-cover-aim2/GearsCaptureCoverAim2.dll`，SHA 256 Yes `04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0`. The default list is only updated `native_build`、`native_dll`、`native_sha256`, the rest of the fields and current 0.7 Render ratio is unchanged. Original XR/Playground binary and old freeze build-up reservation; tags/packages are published without change.

Private sources, build, test and supplementary audit responses `analysis/cover-aim-20261022-build2/`、`analysis/native-build-cover-aim2/`、`analysis/instances/cover-aim2-retail/`, Original run report prefix to `analysis/runtime/continuous-stereo-43608-20261022-020338-106b0cf1`. These evidence and game-derived data do not enter the distribution package.

---

## 中文

# 掩体举枪时的瞄准锁定修复

2026-10-22，已集成到 0.1.0 开发版正常启动器。本次修复掩体举枪时准星／相关 HUD 退回随头定位的遗漏，不构成新版本发布或头显验收。

## 原因与改动

原实现从 `UpdateHUDActions` 的命中点查询回执提取 `bIsTargeting`，再写入双眼帧的 `aiming` 标志。掩体举枪期间，查询可能没有运行或没有产生可用于当前场景的回执；此时即使控制器仍处于瞄准状态，`aiming` 也保留默认的 false，普通 HUD 因而退回随头平面。有限命中点是否可用与是否正在举枪，是两个不同条件。

现在在构建当前场景帧时独立读取已认证的控制器瞄准属性。读取以本帧 anchor 的控制器／Pawn 为身份约束，双次检查注册身份、类继承、双向持有关系、属性类型／尺寸／偏移／位掩码及 Bool 访问分派；只在两次读取的瞄准位一致时提交。此读取不需要查询的脚本帧、局部变量或命中点，也不依赖上一把武器仍然存在。松开瞄准后采用当前状态，不保留上一帧的举枪位。

命中点仍走原有身份、时间和相机新鲜度检查，但其授权结果不再覆盖场景的举枪状态。准星和瞄准 HUD 继续共用同一冻结双眼几何：有可用有限点时使用该世界点，缺失时使用原游戏源视图的世界方向。因此头部姿态只改变其在眼内的投影，不因查询缺失而重新选择头部中心。这个方向回退不等于已认证武器实际发射射线；本次没有改写武器、弹道或角色动画。

## 自动检查

- 13 项原生检查通过，包含新的查询帧不可读、换枪后旧武器不可读、松开瞄准、读取期间状态变化及身份／属性拒绝案例；同时运行既有瞄准 HUD、准星投影、帧生命周期、第一人称及最终输出检查。
- HUD 投影检查覆盖 1,215 组头部位置／旋转／目标深度；准星投影检查覆盖 729 组姿态。有限点不可用时仍检查源世界方向，普通非瞄准行为保持既有规划结果。
- 22 项启动前原生检查通过，208 项启动器检查通过。启动诊断的一项旧测试将即时写入文件的 mtime 与高精度墙钟直接比较，在 Windows 上得到假陈旧判定；仅将该新鲜测试样本绑定到其实际 mtime，保留缺失、错误 PID、旧时间戳及内容脱敏断言，生产诊断逻辑没有改变。首次失败和重跑记录保留。
- MSVC／Ninja 记录 484 项依赖，确认实际编译包含修改后的两个原生文件、原第一人称实现和原输出策略。

原生测试夹具补齐已有双向持有 ObjectProperty 的真实尺寸元数据；首轮测试参数遗漏及夹具失败保留在忽略目录，没有当作通过结果。

## 零售版后台证据与限制

测试仅启动 `J:\gearsVr\Gears of War Reloaded` 的正版工作副本，磁盘 EXE SHA256 为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。使用独立实例、私有存档／设置、经核验的 behind-windows 策略、只读小输出监视、测试静音，并保持物理手柄后台输入关闭。未使用历史存档夹具覆盖用户进度。

通过原版移动／掩体键进入掩体，取得 10 次 `CoverType=2`、`CoverAction=7`、当前控制器瞄准位为 true 的采样。原生记录如下：

| 检查 | 结果 |
| --- | --- |
| 当前场景瞄准状态有效读取 | 10,017 帧 |
| 举枪帧／其中无 HUD 查询回执 | 796／796 |
| 当前场景瞄准读取拒绝 | 0 |
| 瞄准 HUD 规划 | 4,354 次，均使用源世界方向回退 |
| 观察器预算超限 | 0；单次记录最大 0.3312 ms |

这验证了实际掩体举枪在查询缺失时仍进入瞄准 UI 路径。后段出现原版 `PenderCrossplayInvitePopUp`，自动输入按守卫停止；16 次计划输入没有全部完成，回执保留中断状态。

原始严格会话回执为 incomplete：独立停机检查的 7 个系统 `dxgi.dll` 入口与磁盘 PE 字节不同。这也是默认构建已有的核验限制，不能把原生 `entries_restored=1` 单独当成独立通过。本轮独立检查另外保存了实际停机字节的 SHA256；补充审计逐项核对同 PID 的安装前 32 字节记录，7 项全部与实际停机读回哈希相同，其中 5 项还具有一致的 prime 安装前／停机双记录。游戏入口的独立检查均通过。原始严格回执没有改写为 passed，补充审计只证明所检查入口恢复到安装前状态。

原生资源清理通过、XR exit 0、Playground 入口还原通过、音频恢复通过，随后确认自有游戏进程退出。用户原位置的 10 个设置／存档文件与独立副本准备前哈希一致。无 FPS 比较结论。

本轮没有对活动模拟器施加头部运动：诊断入口当时指向另一个并行实例，因此没有修改全局姿态。头部运动证据来自上述 CPU 投影检查；没有新增 GPU 像素对齐、实际发射线／弹着点或实体头显验收。相关体验仍由用户之后统一验证，不要求单独启动测试入口。

## 默认集成

从最新默认 native `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855` 的 Playground 输出冻结源码制作，只替换本次涉及的原生文件和测试夹具；保留全部原构建开关、第一人称及 FXAA 输出改动。

正常启动器现使用 `tools/native-capture/build-cover-aim2/GearsCaptureCoverAim2.dll`，SHA256 为 `04ff9ff8c1465c2019c78b8b0309275a18c4e7208117fa65e8e00396041b71e0`。默认清单仅更新 `native_build`、`native_dll`、`native_sha256`，其余字段及当前 0.7 渲染比例不变。原 XR／Playground 二进制和旧冻结构建保留；已发布标签／包不变。

私有来源、构建、测试及补充审计回执位于 `analysis/cover-aim-20261022-build2/`、`analysis/native-build-cover-aim2/`、`analysis/instances/cover-aim2-retail/`，原始运行报告前缀为 `analysis/runtime/continuous-stereo-43608-20261022-020338-106b0cf1`。这些证据和游戏派生数据不进入发行包。
