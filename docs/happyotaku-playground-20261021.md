# hopyotaku playground test site

2026-10-21, accessed 0.1.0 Develop a default starter. Users request to add a new entry with the same name to the game menu, providing guns, enemy targets, machine guns and unlimited ammunition, and authorizing Astra Max to consult on the direction.

## Achieved

Using Astra Max to find it from the saved package list `MP_WarMachine`The map already contains operational Troika, Player Birthplace, Boomshott, Torque Bow and Hammer of Dawn resources. Use original versions. `WarGameSP` Local load, not dependent on an online match. The menu is reset to the original button style and added to `GameUISceneClient.ActiveScenes` ; the resource template is separated from the running example.

Platform placement behind machine gun 11 Weapons pick-up: Lancer, Hammerburst, Gnasher, Longshot, Snub, Boltok, Boomshott, Torque Bow, Hammer of Dawn, fragmentation hand grenades, smoke bombs. `WarDroppedPickup` The weapons model and the pick-up process; four storage tanks are maintained to replace equipment through a ground gun depot. Six orcs without an AI controller are placed as a still target that can be injured, by the point of the surface ray, and are reborn after death.

The original target conditions and the caloric rules for special weapons are maintained. The model will be discontinued when you leave the world, without modifying the default object, archive or resource package of the weapon type.

## Version and Evidence

Disk EXE SHA 256 Yes `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`. History Code Section SHA 256 Yes `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, History Base `0x140000000`. The list of scripts/packages that are retrieved and stored as required before development only a small number of call entrances are exported on original IDB private copies, and the holding IDA case is closed.

At the entrance to the current active reflect event 64 Bytes are the same as byte of this historical entry; checking does not equal to a full run-time code. Function parameters, attribute layouts and local players are read again from the current process. Accesses such as the index native Spawn/Trace cannot be called directly as normal ProcessEvent; access to restricted FFrame is achieved and a chain table for Trade's output parameters is created. Special evidence, snapshots, IDA export and running backback are ignored `analysis/playground-20261021` Down, don't enter the distribution package.

## Completed Phase Validation

- Independent backstage process validated main menu button text `happyotaku playground`, you can get focus and enter the map through a game window.
- Local player, ready to go. Troika,11 A weapon picks up and...6 Full generation of a target is passed, state is `ready`Generate failure count 0. The actual pick-up of Lancer is then entered into local inventory and the gun shed automatically returns an increase in the number of times.
- Regularly stopped authenticating ProcesseEvent several times 64 Byte reverts, clean-up returns pass; individual examples are used in private archives/sets and audio is restored.
- Launcher Final 189 The offline inspection was passed, containing additional components, Hashi, path constraints, portable white lists and independent entry back-checks.
- `build-playground2` The new process validates the upper-down navigation chain (original menu linedown, enter Enter) after it is compiled, without having to run and temporarily relink.
- Add `-NoPauseUnfocus` Independent Game Process Confirms `MatchInProgress`、`bWaitingToStartMatch=false`、`bMatchHasBegun=true`、GameStatus 3, the game clock is increasing and the image's associated sign is normal. No additional calls are required for StartMatch or force changes made to match tag.
- Actual Lancer, gun exchange, pinshot, target life value 200→0 Regular reloading, standby ammunition 600→600 This is the actual injury result. The camera direction/HUD cache is not used as proof of a life-line. The third person says that the shooting test is adjusted to the test with real time point, and that short firing backs are maintained before the shooting.
- By the original version `CanEnterVehicle/TryToDrive/DriverLeave` The Troika machine gun, player/carryer holds a two-way relationship and continues to maintain and return to its original role during the ride. The handheld is not replaced by a rewritten handheld pointer; the machine gun has an unlimited spare ammunition sign.0- As a machine gun launch certificate.
- After the independent game is stopped, the menu bridge will be closed.64 Byte is back through, test the audio-state recovery pass and then confirm that the game process has been withdrawn.

Independent non-VR diagnostic process due to lack of normal start-up path `-NoPauseUnfocus` Maintains the parameter and the box switch-and-shoot test is not passed, and cannot be used as a combat check. The first complete starter validates duplicate CR-shifting of tested copies before the game is created, has been modified to test pre-set scripts and failed to confirm retention; it is not a game or DLL crash.

Second regular starter connection validated the full scene generation (11/6/ Troika, fail count 0) and enters through the Down/Enter Navigator of the Game window; orientation links are added `build-playground2` The main VR component then stopped at the front frame because of a lack of verified eye images.`draw_mask=0`、`unknown_draws=3`, the video has not been released. The wheel cannot be recorded as VR pass; the stop/entry of the standalone menu component is back on track. Follow-up line-by-line opening, loading and output path without easing the main rendering verification bypassing failure.

Subsequent light diagnosis limits the problem to FXA blued output: same ticket 2 The left and right half eye drawings have arrived down 640×350 Target, quad returns RVA `0xe27709`The source pool is 1280×720 pixel mapping of the source/target eye rectangle is consistent; the old classification does not accept the call. The main menu has moved out of ActiveScenes, opening and loading are normal, and failure cannot be attributed to the front-end interface residuals. Extension tracking does not fully save exit data, and the same evidence is obtained after using eight entries of already existing values in it.

Retrieval by knowledge base first `0xe27500..0xe27772`, mechanically export this segment in private IDB, SHA 256 `31691ed732d3d4a05ecb63ec4a7a03cc1c68f5afc8bf851c9d876c9fafc2f6e5` History 1 In a consistent way, please close. The shadows register the connection to position it as FXA blued path; this is a reprocessing output that does not claim to be copied in pure pixels. Candidates must also check the full section of the function when the actual process is activated by adding the output path only under the current owned renderer/world/ticket and the item-by-line left-eye pixel area without expanding the generic backffer caller without modifying the follow-up check for Draw/Execute/president/HUD.

Can not open message `045ca20a…b9d` ..is created on the frozen source copy, reuse all 72 Build switches, only the above-mentioned repairs and related positive and negative examples; MSVC/Ninja 484 Item relies on tracking validation and `FinalTransferPolicyTest` Adopted. Complete VR results are still based on the following follow-up records.

Full normal starter/simulator connection after FXA recovery has passed round: actual XR two-eye projection successfully submitted, released when exit 18,008 Frame, stop replying normally. `passed=true`、`native_cleanup_verified=true`、`entries_restored=1`、XR exit 0, the menu bridge is also returned. The actual Lancer consumption is also passed.6 The target's life value 100→0 We're back up.360→360 The target is reborn, and the original Troika machine gun goes through.17,917 HUD/material error at frame 0;carrying in the interim achor refuses to keep alone, not as an error 0 Or frame rate comparison.

High Resolution Recheck with verified bend-windowsQuest 3 / VDXR monitored emulator profile, VR ratio 0.5♪ Every eye ♪1536×1632, Game Window Start 1280×720, without adjusting the running window. Actual shooting, reloading, re-entry of target passes; GPU read it back 3072×1632 Two-eye images confirm scenes, guns, Troika and Orc targets are visible.37,157 Frame, native portal revert/cleanup, XR exit 0 The menu bridge is back through. The normal battle original menu continues to be released automatically.6,248 Frames are stopped normally; test site is unrequested, firearms/targets 0 These operations are not a comparison of the FPS and do not claim frame rates on this basis.

Final Launcher 189(a) Check and export-restricted geometrical approvals;721 Documentation,41,920,718 The byte separation portable directory check is passed, and contains a new menu of DLL/loader/runtime, excluding analytical evidence and game assets. Complementing the new PE that injects DLL relies on context, maintaining only recognition of its physical system dependency, not because it has a DLL in the same directory to admit that it can be loaded on the game.

20:48–20:55 Current Default Build Compensation (Independent PID)18840) On a case-by-case basis 11 Models of gun storage, original pick-up/exchange and player inventory are in place for unlimited backup ammunition markings and the armoury is automatically returned 16 The scene is maintained.11 A pick-up/6 Targets/failures 0. By the original version `FinishOnQuitToMainMenu` Exit, re-enter with the menu Down/Enter and recreate the whole new world; stop testing field maintenance during the menu. Troika press original `PreButtonPress/ ButtonPress('R2')` Create a holddown, actually through 0.5 Second heat in. `WeaponFiring`Fire in a row.28 Let go, and come back. `Active`Normal up and down machine guns passed. Previously, only the diagnosis of Weapon.StartFire was not established. `Controller.bFire`, cancelled by the original Turret.Tick; there is also a test script parameter/name scanning error, all of which are retained as game failure or evidence. The correction test triggers the test, and you can then read it back.11 The complete inspection of the weapon and machine gun is again passed without replacement.64 Byteback, audio recovery and process exit. Complete current status audit `analysis/playground-20261021/completion-audit.json` Yes `passed=true`。

Current default list enabled component: native SHA 256 `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`, menu DLL SHA 256 `e1d07405d0b8c3393783d5de549c44d7e20647efcf5bcc2e82b0646e6cd038ee`. The default list `b902bfa3…f6a3b` Update only 7 Relevant fields, new list SHA 256 `fc54cd221bdb61925b1f39a19bb067148840f6da3ac93c218e11403f0c54e3bd`;configuring SHA 256 `3defcd29…e43d` Keeps the same. Once you re-open the normal starter, you can select a new entry at the end of the main menu. The headline display, the physical VR handle operation and the actual experience are subject to user acceptance; this development record is not a new version of the release or headover check, and it has been published 0.0.1 Packages and labels remain unchanged.

---

## 中文

# happyotaku playground 测试场

2026-10-21，已接入0.1.0开发版默认启动器。用户要求在游戏菜单中新增同名入口，提供枪械、敌人靶子、机枪和无限弹药，并授权向 Astra Max 咨询实现方向。

## 实现

采用 Astra Max 从保存的包清单中找到的 `MP_WarMachine`：该地图已经包含可操作的 Troika、玩家出生点、Boomshot、Torque Bow 和 Hammer of Dawn 资源。使用原版 `WarGameSP` 本地加载，不依赖在线对局。菜单复用原版按钮样式，新增到 `GameUISceneClient.ActiveScenes` 中实际激活的主菜单实例；资源模板与运行实例分开处理。

机枪后方的平台放置11种武器拾取物：Lancer、Hammerburst、Gnasher、Longshot、Snub、Boltok、Boomshot、Torque Bow、Hammer of Dawn、破片手雷、烟雾弹。使用原版 `WarDroppedPickup` 的武器模型和拾取流程；保留四个库存槽，通过地面枪库更换装备。六个无 AI 控制器的兽人作为可受伤的静止靶，按地面射线落点放置，死亡后延时重生。拾取物使用后补回。

无限备用弹药只作用于请求进入的测试场及当前本地玩家／机枪武器，保留正常弹匣消耗、普通装填和主动装填。特殊武器的原版目标条件与热量规则保留。模式会在离开该世界时停止维护，不修改武器类默认对象、存档或资源包。

## 版本与证据

磁盘 EXE SHA256 为 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。历史代码节 SHA256 为 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`，历史基址 `0x140000000`。开发前按要求检索知识库并复用保存的脚本／包清单，仅在原始 IDB 的私有副本上导出少量调用入口，已关闭所持 IDA lease。

当前运行时的反射事件入口前64字节与该历史入口逐字节相同；此核对不等于完整运行时代码相同。函数参数、属性布局与本地玩家关系从当前进程重新读取。索引 native 的 Spawn／Trace 等入口不能直接当普通 ProcessEvent 调用；实现使用受限 FFrame，并为 Trace 的输出参数建立链表。专有证据、快照、IDA 导出及运行回执都在忽略的 `analysis/playground-20261021` 下，不进入发行包。

## 已完成的阶段验证

- 独立后台进程验证了主菜单按钮文字 `happyotaku playground`、可取得焦点，并通过游戏窗口的一次 Enter 实际进入地图。
- 本地玩家、现成 Troika、11 个武器拾取物和6个靶子的完整生成通过，状态为 `ready`、生成失败计数0。实际拾取 Lancer 后进入本地库存，枪库自动补回计数增加。
- 多次正常停止验证了 ProcessEvent 原始64字节还原，清理回执均通过；各独立实例使用私有存档／设置，音频恢复通过。
- 启动器最终189项离线检查通过，包含新增的组件哈希、路径约束、便携包白名单及独立入口还原检查。
- `build-playground2` 的新进程验证了编译后的上下导航链（原版菜单逐项 Down，再 Enter 进入），无需运行后临时改链接。
- 加上 `-NoPauseUnfocus` 的独立游戏进程确认 `MatchInProgress`、`bWaitingToStartMatch=false`、`bMatchHasBegun=true`、GameStatus 3，游戏时钟递增，载入画面相关标志正常。无需额外调用 StartMatch 或强改 match flag。
- 实际 Lancer 换枪、射击扣弹、靶子生命值 200→0、正常换弹、备用弹药 600→600、击杀后自动重生均通过。此处是实际伤害结果，不把相机方向／HUD 缓存当作命中射线的证明。第三人称射击测试使用实时视点调整测试输入，保留之前未击杀的短射击回执。
- 通过原版 `CanEnterVehicle/TryToDrive/DriverLeave` 完成 Troika 上下机枪、玩家／载具双向持有关系、乘坐期间继续维护及回到原角色。未用改写持有指针代替原版交互；机枪无限备用弹药标志成立。此阶段不把弹匣计数0当作机枪发射证明。
- 独立游戏正常停止后，菜单桥接入口64字节还原通过，测试音频原状态恢复通过，随后确认该游戏进程已退出。

独立非 VR 诊断进程因缺少正常启动路径的 `-NoPauseUnfocus` 参数而保持暂停，弹匣切换与射击测试没有通过，不能当作战斗验收。首次完整启动器验证在游戏创建前被测试副本的重复 CR 换行拦截，已修正测试准备脚本，失败回执保留；不是游戏或 DLL 崩溃。

第二次正常启动器联测验证了完整场景生成（11/6/Troika，失败计数0），并通过游戏窗口的 Down／Enter 导航进入；方向链接已补入 `build-playground2` 并在新进程中复验。主 VR 组件随后在首帧输出停止，原因是缺少已核验的双眼画面对，`draw_mask=0`、`unknown_draws=3`、尚未发布画面。该轮不能记为 VR 通过；独立菜单组件的停止／入口还原通过。后续逐项排查开局、载入画面和输出路径，没有放宽主渲染验证绕过失败。

后续轻量诊断把问题限定到 FXAA blend 输出：同一 ticket 2 的左右半眼绘制均到达 owned 640×350 目标，quad 返回 RVA `0xe27709`，源池为1280×720，源／目标眼矩形的像素映射一致；旧分类器没有接纳该调用。主菜单已正常移出 ActiveScenes，开局和载入标志正常，不能把失败归因于前端界面残留。扩展尺寸追踪没有完整保存退出数据，改用八条以内的已有值记录后才得到该同帧证据。

按知识库先检索 `0xe27500..0xe27772`，在私有 IDB 中机械导出该单段函数，SHA256 `31691ed732d3d4a05ecb63ec4a7a03cc1c68f5afc8bf851c9d876c9fafc2f6e5` 与历史 artifact1 一致，lease 已关闭。shader 注册关联将其定位为 FXAA blend 路径；这是后处理输出，不宣称纯像素复制。候选在实际进程激活时还必须核对完整函数字节，仅在当前 owned renderer／world／ticket 和逐项一致的左右眼像素区域下增加该输出路径，不扩展通用 backbuffer caller，不修改 Draw／Execute／Present／HUD 的后续检查。

该候选从当前默认 `045ca20a…b9d` 的冻结源副本上制作，复用全部72项构建开关，只引入上述修复及相关正反例；MSVC/Ninja 的484项依赖跟踪验证与 `FinalTransferPolicyTest` 通过。完整 VR 实测结果仍以以下后续记录为准。

FXAA 修复后的完整正常启动器／模拟器联测已通过一轮：实际 XR 双眼投影提交成功，退出时发布18,008帧，正常停止回执 `passed=true`、`native_cleanup_verified=true`、`entries_restored=1`、XR exit 0，菜单桥接入口还原也通过。此进程中实际 Lancer 消耗6发将靶子生命值100→0，正常换弹完成、备用360→360、靶子重生，以及原版 Troika 上下机枪均通过。玩法后继续发布17,917帧时，HUD／材质错误均为0；乘坐过渡期间的保守 anchor 拒绝单独保留，不当作错误0或帧率比较。

较高分辨率复验使用已校验的 behind-windows Quest 3 / VDXR matched 模拟器 profile、VR比例0.5、每眼1536×1632，游戏窗口初始1280×720，未调整运行中窗口。实际射击、换弹、靶子重生再次通过；GPU读回的3072×1632双眼图像确认场景、枪械、Troika和兽人靶可见。正常停止发布37,157帧，native入口还原／清理、XR退出0、菜单桥接入口还原通过。普通战役原菜单自动继续回归发布6,248帧并正常停止通过；测试场状态为未请求、枪械／靶子0，未对普通战役维护无限弹药。这些运行不是FPS对照，不据此宣称帧率。

最终启动器189项检查及受限输出几何正反例通过；721文件、41,920,718字节的隔离便携目录检查通过，包含新菜单DLL／loader／runtime，排除分析证据和游戏资产。补齐新注入DLL的PE依赖上下文，保持仅承认其实际系统依赖，不因为同目录有DLL就承认能被游戏加载。

20:48–20:55 的当前默认构建补验（独立 PID18840）逐项确认11种枪库模型、原版拾取／换枪及玩家库存，无限备用弹药标志均成立，枪库自动补回16次，场景维持11个拾取物／6个靶子／失败0。通过原版 `FinishOnQuitToMainMenu` 退出，再用菜单 Down／Enter 重新进入，新世界重新生成完整场景；菜单期间停止测试场维护。Troika 按原版 `PreButtonPress/ ButtonPress('R2')` 建立按住状态，实际经过0.5秒预热进入 `WeaponFiring`，连续发射28发，松开后回到 `Active`，正常上下机枪通过。此前仅调 Weapon.StartFire 的诊断没有建立 `Controller.bFire`，被原版 Turret.Tick 取消；另有测试脚本参数／名称扫描错误，全部回执保留，不作为游戏故障或通过证据。修正测试触发后，11武器与机枪的完整合并检查再次通过，无需更换构建。独立停止确认原入口64字节还原、音频原状态恢复及进程已退出。完整当前状态审计 `analysis/playground-20261021/completion-audit.json` 为 `passed=true`。

当前默认清单已启用组件：native SHA256 `bf0a8f730d41718cc6002625848f9f8c3f7ef15aaa4bb0a15153dd3498978855`，菜单 DLL SHA256 `e1d07405d0b8c3393783d5de549c44d7e20647efcf5bcc2e82b0646e6cd038ee`。原默认清单 `b902bfa3…f6a3b` 只更新7个相关字段，新清单 SHA256 `fc54cd221bdb61925b1f39a19bb067148840f6da3ac93c218e11403f0c54e3bd`；配置 SHA256 `3defcd29…e43d` 保持不变。重新打开普通启动器后，可在游戏主菜单末尾选中新入口。头显显示、实体VR手柄操作和实际体验待用户之后统一验收；本开发记录不是新版本发布或头显验收，已发布0.0.1包和标签保持不变。
