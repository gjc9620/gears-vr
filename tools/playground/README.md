# happyotaku playground

Enabled in the ordinary 0.1.0 development launcher. Select `happyotaku playground`
at the bottom of the game's main menu. The launcher loads the
version-gated DLL during startup and reserves a disabled trampoline before the
engine's later allocations. After the registries are ready it enables the bridge.
It adds a styled button to the active
`WarUIScene_MainMenu`, then travels locally to `MP_WarMachine` with `WarGameSP`.
No cooked game package is patched or distributed.

The range uses the map's existing Troika, eleven equipment pickups behind it,
and six stationary Locust targets along the firing lanes. Targets respawn after
three seconds; equipment is replenished after pickup. Infinite spare ammunition
preserves the original magazine and reload behavior. Only the requested range
world and its local player are maintained, including while occupying the Troika.

Build with the existing MSVC/CMake toolchain:

```powershell
cmake -S tools/playground -B tools/playground/build-local -A x64
cmake --build tools/playground/build-local --config Release
& tools/playground/build-local/Release/PlaygroundHookTest.exe
```

`playground_menu`, `playground_early_reserve`, `playground_build`, `playground_dll`, and `playground_sha256`
bind the optional component in the launcher manifest. The normal native/XR
artifacts remain separately pinned. `stage_portable.py` includes only declared
runtime files and the DLL; analysis evidence and game assets remain excluded.

Engine work is scheduled on the observed local game thread. Indexed native
functions use an FFrame with explicit native parameters and out-parameter links;
unindexed functions use the original ProcessEvent. The loader checks executable
identity and live entry bytes, and independently verifies restoration on stop.
Shutdown also releases a reserved hook when startup stops before activation.
An optional initialization resource failure may continue VR only after an
independent full-entry restoration check; identity and certificate failures still
stop startup. Allocation diagnostics distinguish the trampoline and hook table.
Status and logs are PID-specific beside the DLL. `PlaygroundTestEnter` is gated to
the explicit muted, behind-windows autonomous test environment.

Game-version offsets, layouts, and observed behavior are specific to the checked
Reloaded executable. See the development record in `docs/releases/0.1.0/PROGRESS.md`
for completed validation and remaining headset acceptance.

The weapon-6DoF branch also has a **private, opt-in Hammerburst R2 candidate**.
Build with `GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2=ON` and a distinct
`GEARS_PLAYGROUND_OUTPUT_NAME`; this exposes `PlaygroundVrFireEnable`,
`PlaygroundVrFireDisable`, and `PlaygroundVrFireStatus`. Enable it only after
the matching stereo and controller-input publishers are live. It consumes exact
XR frame keys on the local player's `PlayerTick` and forwards right-trigger
edges to the original `ButtonPress/Release('R2')`. Short bridge gaps keep the
last valid frame until its 100 ms limit; longer loss safely releases this
verified automatic weapon. Disable the adapter before `PlaygroundShutdown`.
The default build excludes these exports and does not forward VR fire.

`GEARS_PLAYGROUND_VR_R2_SIM_TEST=ON` additionally builds the isolated
`VrFireTestPublisher` and accepts simulated input only in the muted,
behind-windows test child. It is a verification tool, not a launcher option or
headset acceptance. Other weapons remain gated until their original
press/release, cancellation, and co-input behavior are verified.

The additional `GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2=ON` candidate requires
the R2 frame bridge. The same explicit `PlaygroundVrFireEnable/Disable` session
then forwards left-trigger edges to the original `ButtonPress/Release('L2')`
and exposes `PlaygroundVrAimStatus`. It is limited to Hammerburst while the
game's `bToggleTargeting` setting is false. Toggle-to-aim is gated because a
release does not cancel its latched state. One direct original-button retail
probe confirmed hold-to-aim, and the edge state machine passes isolated tests;
the combined XR-frame-to-game L2 candidate still needs a successful running
game test and headset acceptance. The default DLL excludes all VR input
exports.

For private inventory verification, build with
`GEARS_PLAYGROUND_EQUIPMENT_OBSERVE=ON` and a distinct
`GEARS_PLAYGROUND_OUTPUT_NAME`. Every 500 ms on the local player's game
thread, the observer checks the four original inventory slots, each occupied
weapon's `HasAnyAmmo`, current weapon, switch permission, switch animation flag
and grenade charges. It writes
changes to the PID-specific Playground log. The grenade charge read follows
the original human-player `HasAnyAmmo`/spare-ammo rule; unknown quantities
fail closed in the equipment state model. This candidate only observes: it
does not move a gun, suppress an animation, play a sound or call
`SetWeaponFromSlot`. The default build excludes the observer, and a successful
compile does not establish a valid retail runtime snapshot.

The private `GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST` candidate additionally
requires the inventory observer and `GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST`.
With the certified native controller observer active, a right-grip press near
the left/right shoulder or trouser holster publishes only the matching original
slot number plus current game identities. Playground discards old or overflowed
events, then re-reads the local player's inventory on the game thread and
requires the private owned arena, behind-windows and muted test environment
before calling the original slot function. Empty grenade charges fail closed.
When a requested slot is empty or its weapon has no usable ammunition, the
private adapter tries the slot weapon's
original `FireNoAmmoSound` cue, then the current weapon's cue, through the
original local playback function. Status 9 means neither cue could be played.
This is a wiring test: original switch animation, visible stow, an audio cue
when neither weapon has one, and playable weapon pose transfer are not yet validated.
The default DLL has no XR equipment-intent consumer.
The same private bridge also carries left-shoulder ammo-grab presses. The
adapter checks the held weapon's original spare count and infinite-spare rule:
status 1 means ammo exists, 2 means empty with the original cue, 3 means empty
without a cue, and 4 means the game identity or count could not be validated.
No round or magazine is spawned or deducted at this stage.

The separate `GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST` option requires the
certified equipment-intent candidate and remains off in the default DLL. A
fresh R3 event can start the original Lancer `Reloading` state in the owned,
muted, behind-windows arena; the adapter replaces its 2.5-second completion
timer with an eight-second original-game fallback. The test-only start and
complete exports exercise the same game-thread path without physical input.
Explicit completion invokes the original `Reloading.EndOfReloadTimer` state
function, which performs the original ammo settlement. No VR magazine stroke,
bolt stroke, perfect reward, or default-launcher behavior is implied. GVE6
events include input epoch and are rejected against a stale hand-pose epoch;
each certified hand pose also carries the same XR ticket's game units per metre
for physical gesture distances. Older GVE5 publishers cannot open this bridge.

`GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST=ON` is a separate, default-off
original-input boundary trial in the owned muted arena. Its test exports queue
one Lancer `ButtonPress('B')` on the game thread, clear the original
`CheckForMeleeTarget` timer immediately and on each owned tick, and release B
on explicit stop, identity loss, timer failure, or Playground shutdown. A
release is accepted only after the original `bHoldingButton_B` field clears.
The separate `GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST=ON` option compiles a
default-disarmed `PlaygroundLancerSawXrPullArm/Stop/Status` trial. Only a fresh
GVE6 same-ticket left-hand pull near the provisional Lancer starter requests
the existing original B hold; no enemy contact, visible starter or default
launcher input is enabled. Status 2 is armed, 3 pulling, 4 B sent, 5 original
hold, 6 original animation/deferred stop, 7 restored, 8 stopped, and 9 a state
or release failure. The private trial auto-requests stop after 15 seconds.
Once the original attack special move 34 begins, Stop and shutdown defer B
release until the original attack state leaves; an animation longer than the
shutdown wait can make shutdown refuse, preserving the live hook until a
later safe retry.
The retail no-victim trial confirmed sustained original blade/melee state and
timer suppression; a second trial closed Playground while B was held and
confirmed the original B, melee and blade states returned to idle. This is
not yet driven by the hand-pull solver and does not perform blade contact,
paired execution, visual starter placement, or headset verification.

`GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST=ON` additionally requires the private
mesh-follow helpers and adds a read-only, post-PlayerTick sample of the
original `b_AR_Chainsaw` and `b_AR_ChainsawEnd` bone matrices with the original
gun matrix in the same game-thread tick. Separate start exports sample an idle
or already-running original saw. Both observed 16-frame retail trials found
stable pivots about 22.47 original units apart. This locates bone pivots only;
the visible tooth edge, target collision, occlusion, and paired execution
remain unverified. The sequential remote-reflection attempt was discarded
because the original gun component moved between calls.

`GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL=ON` is another private, default-off
trial. It uses the current original gun matrix and a provisional housing-side
starter anchor, feeds a 19 cm synthetic left-hand pull through the same
world-scale gesture model, then requests the original B hold only when the
gesture qualifies. The owned retail trial observed one pull request at about
18.81 cm in 297 ms, nine subsequent original blade/melee samples with the
automatic target timer off, and clean B release. The test publishes no XR
hand frame and draws no starter; the mount is not calibrated for a Quest hand.

While certified hand poses are being observed, the private bridge also logs a
read-only `weapon_magazine_probe` beside `weapon_mesh_probe`. It validates the
current weapon's original `MagazineMesh` component, names its `StaticMesh`,
reads `HiddenGame`, and asks the pawn's skeletal mesh whether the component is
attached there. `known=0` rejects an incomplete or unexpected layout; `present=0`
with `known=1` means this weapon has no current independent magazine component.
The adjacent read-only `weapon_ammo_bone_probe` now uses the original
`FindSkelControl(AmmoControl)` lookup and records `ControlStrength`,
`StrengthTarget`, and `BoneScale` when present. A missing control is
recorded separately from an invalid reflection layout. These values are
not by themselves proof of rendered magazine visibility. Neither probe
changes meshes or reload state; a retail runtime sample is still required.

In the same private owned arena, `weapon_reload_native_probe` reads the
original `WarWeapon` UFunction objects for `GetActiveReloadValues` and
`DetermineActiveReloadSuccessOrFailure`. It validates their class ownership
and executable entry pointers inside the current game image, then logs only
module-relative addresses, native indices, and parameter sizes. It does not
call, detour, or patch either function. Unlike the hand-pose probes, this
lookup needs only an authenticated equipped weapon, so simulator sessions
without physical controllers can reach it. A build pass alone provides no
retail address or perfect-reload deadline; a valid runtime log and byte
comparison are required before native analysis or timing claims.
The retail owned-arena sample located the two UFunction wrappers and matched
their first 256 bytes to the historical runtime snapshot (the shorter judge
wrapper was matched in full). Those wrappers dispatch to virtual slots `0x638`
and `0x630` on the equipped weapon. The probe now also validates and logs the
two live virtual targets as RVAs. In a later owned retail arena, the equipped
Lancer resolved the getter to RVA `0x3e81b0` and the judge to RVA `0x3e7d28`.
The first 4 KiB at each target matched the corresponding historical runtime
snapshot byte for byte. This covers the main function blocks, but not the
getter's separate tail block. The probe remains read-only; this match does
not establish any weapon's perfect-reload deadline.
The adjacent `weapon_reload_window_probe` validates the current weapon's
reflected float offsets and reads the original duration array and owner index
on the same authenticated game thread. It never calls the getter or the judge.
One owned retail Lancer instance yielded start `0.7`, array index 0 and width
`0.1`, giving a derived endpoint near `0.8` seconds. This is a checkable
candidate, pending a direct original getter-output comparison.

`GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST=ON` adds an even narrower private
Lancer trial to the equipment-intent build. It requires an owned, muted,
behind-windows Playground range and an already held Lancer. Call
`PlaygroundWeaponMeshFollowEnable` after the certified hand-pose publisher is
live; `PlaygroundWeaponMeshFollowStatus` returns bit 16 for an active component
override, bit 17 while the left support grip is latched, and a low status of 2
while the original `Weapon.Mesh` is following the right hand. Only the
`COGAssaultRifle` asset is accepted. A fresh LGrip press near the measured
left-palm point on that mesh enables two-hand steering; tracking loss, grip
release, or moving out of range drops back to the right-hand pose. Holding
LGrip through component binding does not count as a new press. Call
`PlaygroundWeaponMeshFollowDisable`, wait until bit 16 is
clear and status 6 reports restored (or 7 reports a destroyed component), then
call `PlaygroundShutdown`. Shutdown returns 20 while restoration is still
pending. The first tracked frame retains the game's current gun pose, so this
trial checks component motion without claiming a correct per-weapon grip mount
or a measured controller-to-palm offset. The original-pose contact coordinate
comes from the 24-frame retail sample described below. It has pure transform
and build coverage, but no successful mesh-motion runtime or headset result
yet; the normal DLL contains none of these exports.

The private `PlaygroundWeaponMeshFollowTrialStart` entry exercises the same
game-thread component path without pretending that the simulator supplies a
Quest controller. In a muted, behind-windows owned arena, first use the
private slot-2 tap to establish VR ownership of the equipped Lancer. The
trial then applies twelve bounded synthetic right-hand poses, holds each long
enough for the original component world matrix to update, checks translation
and rotation, disables the override, and requires restoration. Its status
encodes stage in the high 16 bits and completed samples in the low 16 bits;
3/12 means the trial passed, 4 means failed. The retail PID 40068 sample
passed with a maximum stable X error of 0.00977 original units and returned
`PlaygroundShutdown=0`. This verifies component motion/restoration only;
the actual OpenXR hand publisher, headset rendering, firing ray and recoil
still need their own tests. `lancer_support_probe.py --mode mesh_follow_trial`
records the bounded evidence under ignored `analysis/`.

The same private DLL has a separate read-only Lancer support-hand probe.
`lancer_support_probe.py` equips the original Lancer in an owned muted range,
calls `PlaygroundLancerSupportProbeEnable`, and requires status 3 with 24 samples
before accepting the ignored `analysis/` receipt. The callback captures the
original character's left IK, left/right hand bone matrices, and unchanged
pawn/gun component matrices after each original `PlayerTick`. Status 4 means a game
identity, bone, matrix, or test-environment guard rejected the probe. Do not
enable mesh following during this sample; either export refuses that overlap.
The matrices are evidence for later grip-point calibration, not a validated
grip offset or a component to ship with the Mod. An owned retail capture did
complete 24 settled Lancer frames with left-hand IK active. The median left
palm point in the original gun frame was `(8.556242, 43.525180, 0.165157)`
raw game units; maximum frame drift was `0.000231` units. The capture and
derived report stay under ignored `analysis/lancer-support-probe-20261022/`.
The same bounded callback can read the original `MagazineMesh` component and
`AmmoControl` without changing them. One owned retail held-Lancer sample found
`Assault_Rifle_Magazine` present and `HiddenGame=false`, but not attached to
the pawn mesh; `AmmoControl` was present with strength, target, and bone scale
all zero. These idle-state fields do not establish final rendered visibility
or the handoff timing during the original reload.

Run `lancer_support_probe.py --mode reload_visual` in a separate owned range
to seed one missing round in the private arena and invoke the original
`ForceReload`. The private game-thread callback records up to five seconds of
`AmmoControl`, `MagazineMesh` pawn attachment, original state, and ammo counts;
`tools/weapon-models/analyze_lancer_reload_visual_runtime.py` reports sampled
event intervals. One retail Lancer run captured 318 samples: gun magazine
control turned on at elapsed 0.258–0.275 s, the separate hand magazine
attached at 0.774–0.791 s, both reverted at 1.479–1.500 s, and original
reload/one-round ammo commit finished at 2.497–2.510 s. These are sampled
intervals from one original animation, not VR interaction deadlines. The
normal DLL does not export this probe.

`lancer_support_probe.py --mode socket` samples the original Lancer
`Magazine` socket and the gun component matrix in the same bounded game-thread
callback. `--mode socket_reload` repeats this during one original `ForceReload`.
Both modes require the same private, owned retail test range; their reports
remain under ignored `analysis/`. Use
`tools/weapon-models/analyze_lancer_socket_runtime.py` to project the samples
into the gun frame. The original socket is stable at idle but follows the
magazine bone far away during the reload animation. A VR feed-port guide must
freeze a gun-frame reference instead of following that animated socket. The
probe establishes neither insertion direction nor controller grip calibration,
and the normal DLL exports none of its controls.

The same private callback now also samples `b_AR_Magazine`'s original bone
matrix. An owned retail idle capture placed its pivot near the top of the
magazine and the socket near the bottom, separated along the gun's local `-Z`
axis. The pivot is a candidate upper feed-port anchor; controller-held item
orientation and the visible receiver opening still need validation before a
physical insertion can commit inventory.

The opt-in `GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST` candidate reuses the
original independent `MagazineMesh` after the private R3 reload hold starts.
An owned retail synthetic-left-hand trial established component attachment,
motion and release restoration. Its subsequent insertion-stroke observer
compares the held magazine against the current fixed gun-frame feed anchor,
requiring aligned, continuous movement from outside the port to a near-seated
position. The thresholds and hand mount remain uncalibrated for Quest; the
new observer has synthetic geometry tests and compiles. A retail follow-path
rerun showed no false insertion for an unaligned synthetic hand trajectory;
a separate aligned trajectory yielded exactly one arrival candidate in the
owned retail test. Both use simulated hand poses, not Quest controllers. An
insertion candidate only writes a private
status/log: it cannot settle ammo, complete a reload or grant a perfect bonus.

`lancer_support_probe.py --mode chainsaw_semantics` is a bounded owned-retail
test of the original Lancer melee input. It presses and releases original B,
reads the pawn/weapon's melee and blade state, clears the original
`CheckForMeleeTarget` timer in an empty arena, and confirms that the blade
continues until release without acquiring a victim. It does not implement a
VR pull gesture or exercise the paired execution.

The opt-in Lancer R3 reload-hold trial also projects this fixed upper anchor
through the current original weapon mesh matrix while the original reload is
held. It verifies the same mesh identity on every update and leaves the
original eight-second fallback timer in place if the mesh becomes invalid.
An owned retail trial captured seven bounded world-anchor rows while the
original explicit completion still settled ammo. These rows do not represent
a tracked left-hand magazine or a physical insertion.

The read-only original-reload visual probe now records matrices for the
separate `Assault_Rifle_Magazine`, original left-hand bone, and gun in one
callback while the magazine is attached to the pawn. The offline grip
analyzer finds the near-rigid middle hold and reports the moving pickup/return
portions separately. One retail run yielded 43 attached frames and a 21-frame
stable middle hold. This reference is in the original hand-bone frame, not a
Quest controller grip frame; it cannot commit a VR insertion on its own.

`GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` is a separate default-OFF, read-only
pickup/melee lifecycle observer. Its Start/Stop/Status exports sample only the
owned, behind-windows, muted test arena. It separates real ProcessEvent Hook
hits from bridge calls that bypass that Hook, logs current body-slot child
weights/targets/blends and eight original hand controls, and always leaves
`ownership_proved=0`. See the [probe schema and limits](../../docs/hand-animation-probe-20261022.md).

---

## 中文

# 欢乐游乐场

普通启用 0.1.0 选择 `happyotaku playground`
游戏主菜单底部。
启动时为版本配制的 DLL , 在启动前保留一个已禁用的蹦床
引擎的后期分配。在注册系统准备好后,它就可以使桥架得以通车。
它在活动器中添加了一个样式按钮
`WarUIScene_MainMenu`,然后在当地旅行 `MP_WarMachine` with `WarGameSP`.
没有编成的游戏包被补丁或分发.

范围使用地图上的三驾马车 后面有11个装备小卡车
目标在发射道沿线重新喷发
3秒钟;在接车后补充设备。
保存原杂志并重新装入行为。仅请求的区域
维持世界及其当地玩家,包括在占领三驾马车时。

与现有的 MSVC/CMake 工具链构建 :

```powershell
cmake -S tools/playground -B tools/playground/build-local -A x64
cmake --build tools/playground/build-local --config Release
& tools/playground/build-local/Release/PlaygroundHookTest.exe
```

`playground_menu`, `playground_early_reserve`, `playground_build`, `playground_dll`,以及 `playground_sha256`
将可选组件装入发射装置的显示器。正常的原生/XR
文物仍被单独封存。 `stage_portable.py` 仅包括已宣布的
运行时间文件和 DLL; 分析证据和游戏资产仍然被排除在外.

已预定在可观察到的本地游戏线索上进行引擎工作。 indexed number
函数使用带有明确本地参数和外部参数链接的FFrame;
未索引函数使用原始进程Event。加载器检查可执行文件
身份和活的输入字节,并独立验证停机时的恢复。
关闭也会在启动前停止时释放一个保留钩子.
可选初始化资源失败只能在
独立全程返回检查;身份和证书故障
停止启动。分配诊断可以区分蹦床和钩子表。
状态和日志是 DLL 旁边的 PID 专用的 。 `PlaygroundTestEnter` 门建为
明亮的微声, 后窗的自主测试环境。

游戏版本的抵消、布局和观察到的行为是检查对象所特有的
重新装入可执行文件。 请参看开发记录 。 `docs/releases/0.1.0/PROGRESS.md`
用于完成验证和其余耳机接收。

武器... 武器... 武器... 武器... 武器... 武器...6 联邦移民局分支也有一个 **私人,选入的锤子爆破R2 candidate**.
与 `GEARS_PLAYGROUND_EXPERIMENTAL_VR_R2=ON` 和独特的
`GEARS_PLAYGROUND_OUTPUT_NAME`; 这暴露了 `PlaygroundVrFireEnable`,
`PlaygroundVrFireDisable`,以及 `PlaygroundVrFireStatus`。仅在
匹配的立体声和控制输入的发布器是实时的。它消耗精确的
本地玩家的 XR 帧键 `PlayerTick` 右向移动
与原画的边缘 `ButtonPress/Release('R2')`短桥缺口保持
最后一个有效框架直到其 100 毫秒限制; 较长的丢失安全释放此值
已校验的自动武器。在适配器之前禁用 `PlaygroundShutdown`.
默认的构造排除了这些出口,并且不会向VR火势前进.

`GEARS_PLAYGROUND_VR_R2_SIM_TEST=ON` 新建孤立的
`VrFireTestPublisher` 并且只接受哑巴中的模拟输入,
窗后测试程序。它是一个验证工具,而不是一个发射器选项或
其他武器在原武器之前一直被封门。
核实了新闻/释放、取消和共同输入行为。

追加 `GEARS_PLAYGROUND_EXPERIMENTAL_VR_L2=ON` 需要候选人
编号2 架式桥,同样清晰 `PlaygroundVrFireEnable/Disable` session
然后向左向左移动边缘到原边 `ButtonPress/Release('L2')`
和暴露的, `PlaygroundVrAimStatus`。它仅限于在
game's `bToggleTargeting` 设置是虚假的。切换到一个im被封门,因为
发行时不会取消其已挂载状态。 一条直接的原扣零售
探测器确认是悬浮式的 边缘状态机器通过孤立测试
组合 XR 帧对游戏 L2 候选人还需要成功竞选
游戏测试和耳机接受。默认 DLL 排除所有 VR 输入
exports.

用于私人库存核查,与
`GEARS_PLAYGROUND_EQUIPMENT_OBSERVE=ON` 和独特的
`GEARS_PLAYGROUND_OUTPUT_NAME`中,每个 500 本地玩家游戏上的 ms
线程,观察者检查四个原始的库存槽,每个占用
weapon's `HasAnyAmmo`,当前武器,切换权限,切换动画旗
和手榴弹的指控,写道
更改 PID 专用游戏场日志。 读取的榴弹充值如下
原始的人类玩家 `HasAnyAmmo`/spare-ammo规则;数量不明
设备状态模型中失败关闭。该候选人只注意到:
不移动枪械,压制动画,播放声音或呼叫
`SetWeaponFromSlot`。默认的构建不包括观察员,并且成功
编译不建立有效的零售运行时间快照 。

私人 `GEARS_PLAYGROUND_EQUIPMENT_INTENT_TEST` 候选人
要求清单观察员, `GEARS_PLAYGROUND_EQUIPMENT_SWITCH_TEST`.
认证本地控制器观察员活动,右侧的种子压车接近
左/右肩或拖轮套只公布匹配的原件
位置号加当前游戏身份。 游戏区域丢弃旧的或溢出
事件,然后重新阅读游戏线索上的本地玩家目录和
需要私人拥有的竞技场、后窗和哑弹测试环境
。空的手榴弹充电失败。
当请求的空位或其武器没有可用弹药时,
私人适配器尝试槽武器
original `FireNoAmmoSound` 提示,然后是当前武器提示,通过
本地原播放功能。 状态 9 意味着两个提示都无法播放。
这是电线测试: 原开关动画, 可见的 Stow, 音频提示
这两种武器都没有,而且可玩性武器转让尚未生效。
默认的DLL没有XR设备的特制消费者.
同一座私人桥还搭载左肩弹药压压。
适配器检查持有的武器的原始备用量和无限零星规则:
状态 1 弹药的存在, 2 意思是,在原始提示下是空的, 3 表示空
毫无线索, 4 表示游戏身份或计数无法验证.
现阶段没有生产或扣除任何轮子或杂志。

单独 `GEARS_PLAYGROUND_LANCER_RELOAD_HOLD_TEST` 选项要求
认证设备的输入对象,在默认 DLL 中未关闭。 A
新鲜 R3 事件可以启动原始的 Lancer `Reloading` 国家拥有,
哑声,窗后竞技场;适配器取代其 2.5- 第二次完成(续)
带有八秒原游戏倒计时器。仅测试开始和
完整导出时不进行物理输入, 则执行相同的游戏线程路径。
明确完成法援引原件 `Reloading.EndOfReloadTimer` state
函数,用于执行原弹药结算。
螺栓中风、完美的奖励或默认发射者行为是隐含的。 GVE6
事件包括输入时代,并拒绝反对一个僵硬的手持时代;
每个认证的手姿还搭载着每米相同的XR票的游戏单位
用于物理手势距离。5 出版社无法打开这座桥。

`GEARS_PLAYGROUND_LANCER_CHAINSAW_HOLD_TEST=ON` 是单独的, 默认关闭
原输入边界试验在自有的变形竞技场中。 它的测试导出队列
一个兰瑟 `ButtonPress('B')` 显示游戏线索, 清除原作
`CheckForMeleeTarget` 立即和在每一个自有勾选上计时器,然后发布 B
明确停止、身份丢失、计时器故障或游戏场关闭。
仅在原始文本之后才接受释放 `bHoldingButton_B` 字段清除。
单独 `GEARS_PLAYGROUND_LANCER_SAW_XR_PULL_TEST=ON` 选项汇编 a
default-disarmed `PlaygroundLancerSawXrPullArm/Stop/Status` 试镜,只是新鲜的
GVE6 同一盘左手拉近临时的兰瑟启动请求
现有原始 B 控件; 没有敌人接触、 可见启动器或默认值
启动器输入启用。 status 2 武装, 3 pulling, 4 B派来的 5 original
hold, 6 原动画/延迟停止, 7 restored, 8 停下来,和 9 a 状态
私人审判自动请求在 15 seconds.
一旦最初的攻击特别行动 34 开始、停止和关闭
发布到原始攻击状态离开; 动画比
等关机就能让关机拒绝 保留了活钩直到
后经安全复试.
零售无受害者试验证实,原叶片/糖类状态持续不衰,
定时器压制;二审关闭了B场,
证实原B、melee和刀片状态已恢复闲置。
还没有被手推解器驱动 并且不进行刀片接触,
配对执行,视觉启动器放置,或耳机验证.

`GEARS_PLAYGROUND_LANCER_BLADE_PROBE_TEST=ON` 需要私人服务
网格跟踪帮助器,并添加一个只读的 PlayerTick 样本
original `b_AR_Chainsaw` and `b_AR_ChainsawEnd` 原骨基质
枪管矩阵在同一游戏线索勾选中。单独启动导出一个闲置样本
或已运行的原始锯。 16- 发现的零售试验
稳定小点 22.47 原单位分开,只定位骨头
可见的牙齿边缘、目标碰撞、隐蔽和对联处决
仍然无法验证。 相继的远程反射尝试已被丢弃
因为原枪部件在通话之间移动.

`GEARS_PLAYGROUND_LANCER_SAW_GESTURE_TRIAL=ON` 是另一个私人的,默认的
使用当前原始枪械基质和临时住房侧式枪械。
启动主锚, 种子 a 19 cm 合成左手拉通过相同的
手势世界级模型,然后仅在
手势符合条件。自有零售试验观察到一个拉动请求,大约是
18.81 cm 英寸 英寸 297 ms,9个后来的原始刀片/金属样品,与
自动目标定时器关闭, 并清除 B 发布。 测试不会发布 XR 。
手框,不绘制启动器;挂载不为Quest手校准。

正在观察经过认证的手势,但私人桥也记录a
read-only `weapon_magazine_probe` beside `weapon_mesh_probe`。它验证了
原创武器 `MagazineMesh` ,其名称为 `StaticMesh`,
reads `HiddenGame`,并询问当家骨骼网点是否为
随附于此. `known=0` 拒绝不完全或意外的布局; `present=0`
with `known=1` 表示这种武器没有目前独立的杂志组件.
相邻的只读 `weapon_ammo_bone_probe` 现在使用原版
`FindSkelControl(AmmoControl)` 查询和记录 `ControlStrength`,
`StrengthTarget`,以及 `BoneScale` 缺少一个控制器
与无效的反射布局分开录入。这些值是
也证明杂志的知名度
更改 meshes 或重新装入状态; 仍然需要一个零售运行时间样本。

在同一私人竞技场里 `weapon_reload_native_probe` 读取
original `WarWeapon` 用于 `GetActiveReloadValues` and
`DetermineActiveReloadSuccessOrFailure`. 它验证他们的阶级所有权
和当前游戏图像中的可执行条目指针,然后只日志
模块相对地址、本地索引和参数大小。它不
调用、绕行或补丁任一函数。与手提式探测器不同,此
搜索只需要一个经过认证的装备武器,所以模拟器会话
没有物理控制器, 就可以到达它。 单凭建筑通行证就无法到达它 。
零售地址或完美重载期限; 有效的运行时日志和字节
在提出本土分析或计时要求之前,需要进行比较。
零售自有阿兰样品找到两个UFunction包装器并配对
他们的第一任 256 历史运行时间快照的字节( 较短的法官)
包装符被完全匹配。 将包装符发送到虚拟槽 `0x638`
and `0x630` 探测器现在还验证并记录了
有两个虚拟的现场目标作为RVA. 在后来拥有的零售竞技场,装备了
兰瑟解决了前往RVA的计数器 `0x3e81b0` 法官改为卢旺达援助团 `0x3e7d28`.
第一次 第一次 第一次 4 每个目标都对应了相应的历史运行时间
字节的快照字节。它覆盖了主要函数块,但不包括
Geetter 的单独尾部块。 探测器仍然只读; 此匹配是
并没有确定任何武器 完美的重装期限。
相邻者 `weapon_reload_window_probe` 验证当前武器的
反映浮点数抵消并读取原持续时间数组和所有者索引
同一认证游戏线条上,它从不叫"获得者"或"法官".
一家拥有的Lancer零售公司开始 `0.7`, 阵列索引 0 宽度
`0.1`,在靠近 `0.8` 秒。这是可检查的
候选人,等待直接的原始所得-产出比较。

`GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST=ON` 增加一个更窄的私人
兰瑟试演设备特制建筑,需要自有的,哑巴的,
后窗游戏场和已经持有的蓝瑟。 call
`PlaygroundWeaponMeshFollowEnable` 认证的手持者出版商是
live; `PlaygroundWeaponMeshFollowStatus` 返回位点 16 用于活动部分
超载, 位点 17 左侧支撑握把被绑住,低水平 2
原图 `Weapon.Mesh` 只能跟着右手走,
`COGAssaultRifle` 接受资产。在测量到的附近,有新的 LGrip 压车。
网状上左倾点使双手方向盘能够使用;跟踪丢失,抓住
释放,或移动到区域外会回到右侧位置。
LGrip 通过组件绑定不等于新媒体。 call
`PlaygroundWeaponMeshFollowDisable`,等待到位点 16 is
明确和地位 6 恢复报告(或 7 报告一个已销毁的部件),然后
call `PlaygroundShutdown`关闭返回 20 恢复时仍然
等待。第一个跟踪框保留了游戏当前枪械的姿势,所以
审判检查部分动议,但不要求正确的每件武器控制架
或测量控制器到平面的偏移。原始连接坐标
来源于 24- 框架零售样本,如下所述。
并构建覆盖,但无成功网格运动运行时间或耳机结果
然而;普通的DLL中并没有包含任何这些出口.

私人 `PlaygroundWeaponMeshFollowTrialStart` 输入练习相同
游戏线程组件路径,而不假装模拟器提供了一个
查询控制器。在一个哑巴的、 后窗拥有的竞技场中,首先使用
私人空档 -2 用于建立装备的Lancer的 VR 所有权。
然后,试验12个带边的合成右手姿势, 保持每个长的
足够原始组件世界矩阵更新、检查翻译
和旋转, 取消覆盖, 并需要恢复。 它的状态
高处的编码阶段 16 位数和完成的低位样本 16 bits;
3/12 意味着审判通过, 4 表示失败。 零售 PID 40068 sample
最大稳定X误差 0.00977 原单位并归还
`PlaygroundShutdown=0`仅核实组件运动/恢复;
实际的 OpenXR 手印、头盔渲染、射线和后坐力
还需要自己的测试 `lancer_support_probe.py --mode mesh_follow_trial`
记录被忽略的有限度的证据 `analysis/`.

同一私人DLL有一个单独的只读的兰瑟支持手探针.
`lancer_support_probe.py` 将原蓝瑟装在自有的哑弹范围内,
calls `PlaygroundLancerSupportProbeEnable`,并需要状态 3 with 24 samples
在接受忽略之前 `analysis/` 收件人,回调会抓取
原角色是左IK,左/右手骨基质,没有改变
棋子/枪械组件矩阵,在每张原始文本之后 `PlayerTick`现状 4 表示游戏
身份、骨骼、基质或试验环境警卫拒绝探测器。
允许在此样本中跟踪网格; 任一导出都拒绝重叠。
矩阵是以后抓点校准的证据,不是经过验证的
握住的抵消或用Mod号运送的部件。
complete 24 已安放的 Lancer 框架, 左手 IK 活动。 左中位数
原枪框上的掌点是 `(8.556242, 43.525180, 0.165157)`
原始游戏单位; 最大帧漂移 `0.000231` 单位。
生成的报表在忽略下保留 `analysis/lancer-support-probe-20261022/`.
相同的有限度回调可以读取原作 `MagazineMesh` 构成部分和
`AmmoControl` 找到一个自有的零售商持有的蓝瑟样本
`Assault_Rifle_Magazine` 现时和现时 `HiddenGame=false`,但不附着于
质地网; `AmmoControl` 身处强势、目标、骨骼
全部为零。这些闲置状态的字段无法建立最终的可见度
或原重载时的交接时间。

Run `lancer_support_probe.py --mode reload_visual` 在一个单独的自有区域中
以在私人竞技场中 进行一场比赛并引用原作
`ForceReload`。私有游戏线索回调记录最多5秒
`AmmoControl`, `MagazineMesh` 卒式附件、原状态和弹药计数;
`tools/weapon-models/analyze_lancer_reload_visual_runtime.py` 抽样报告
事件间隔 。 318 样本:枪支杂志
控制器于月经时打开 0.258–0.275 s, 单独的手杂志
所附报告 0.774–0.791 s, 两者均恢复为 1.479–1.500 s, 原文
重新装填/一发子弹 2.497–2.510 s. 已抽样
间隔从一个原动画,而不是 VR 交互截止日期。
普通 DLL 不导出此探测器 。

`lancer_support_probe.py --mode socket` 样本原蓝瑟
`Magazine` 套接字和枪械组件矩阵在同一游戏的线条中
callback. `--mode socket_reload` 在一个原稿中重复此内容 `ForceReload`.
两种模式都要求相同的私人和自有零售测试范围;其报告
仍然被忽视 `analysis/`使用中选择
`tools/weapon-models/analyze_lancer_socket_runtime.py` 用于投影样本
将枪框插入枪框。
杂志骨在重新装入动画时会远离。 VR 输入端口指南必须
冻结枪框引用,而不是跟踪动画插座。
探测器既不确定插入方向,也不确定控制器的握力校准,
普通DLL 则不出口任何控制。

同样的私人回调 现在也抽样 `b_AR_Magazine`原骨头
矩阵。自有零售闲置捕获器将其支柱放在顶端。
枪身和枪口的底部 沿着枪身在局部分离 `-Z`
轴。 枢轴是一个候选的上端种子端口锚; 控制器控件
方向和可见接收器打开时仍需要验证
物理插入可以进行清点。

选择进入 `GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST` 候选重用
原始独立 `MagazineMesh` 私人R之后的3 重新装入控件开始 。
拥有零售的合成左手试验
其随后的插入-中空观察者
将持有的杂志与目前的固定枪具发射锚进行对比,
需要从港口外向接近封闭的
位置。阈值和手挂载对查询来说没有校准;
新观察家有合成几何测试和编译。
重新运行后,没有为不相通的合成手轨显示错误的插入;
独立对齐的轨道,在2005年
拥有零售测试。两种测试都使用模拟手姿,而不是查询控制器。
插入候选人只写私语
状态/日志:它不能结算弹药,不能完成重新装填或给予完美的奖金。

`lancer_support_probe.py --mode chainsaw_semantics` 是一个有边框的自有零售
测试 Lancer melee 原始输入。它按下并发布原始 B,
读出兵士/武器 的精髓和刀片状态,清除原始
`CheckForMeleeTarget` 在一个空的竞技场上计时器,并证实刀片
直至未获得受害人而被释放。
VR拉动手势或锻炼配对的处决.

选入的兰瑟R3 重载试验还预测了这个固定的上锚
穿过当前武器网格矩阵,而原重装则
用于验证每个更新中相同的网格身份,并留下
如果网格无效, 原八秒倒计时器就位 。
一次自有零售审判 捕捉了七条世界锚地
原始的清晰完成状态仍然结算的弹药。这些行并不代表
左手有轨电车或物理插入

仅读的原装重装视觉探测器现在记录了该图像的矩阵
separate `Assault_Rifle_Magazine`左手骨和枪的原型
杂志附在棋子上时回调。离线抓住
分析员发现近刚性中间的夹子 并报告移动的接车/返回
单项 单项零售 43 附加框架和a 21-frame
中位点稳定。此参考文献位于原始手骨框中,而不是
查询控制器抓取框架; 它不能自行执行 VR 插入 。

`GEARS_PLAYGROUND_HAND_ANIMATION_PROBE` 是单独的默认- OFF, 只读
皮卡/冶炼机生命周期观察员。
拥有, 后窗, 哑声测试竞技场。 它分离了真正的进程Event Hook
桥上打出绕过虎克的电话 记录当前身体槽的孩子
重量/目标/血小板和8个原始手控,并始终离开
`ownership_proved=0`。参见 [探测器的图案和限制](../../docs/hand-animation-probe-20261022.md).
