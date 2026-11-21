# State-based autonomous game testing

This runner starts a private, background VR test session, performs bounded game
actions, checks their effect through reflected state, and writes issues and
coverage evidence under ignored `analysis/playtest/`. The default run does not
capture images, record video, or send images to a model. Normal game/VR rendering
still runs. An explicit `--gpu-snapshots` run enables the existing simulator eye
image snapshot controls for visual feature verification. It records this choice
in provenance; it does not capture the desktop or other windows.

The current implementation exercises movement and turning, aim/fire response,
pause/resume, ordinary checkpoint reload, cinematic control inhibition, and
reflected path-choice triggers. It does not yet replace full
campaign play. Current objective schemas, interaction events and live AI targets
are now observed. Optional interaction and combat policies still need successful
gameplay acceptance; special traversal, mounted use and chapter completion remain
in progress. Coverage reports retain these missing requirements. A successful
component check does not imply campaign or headset acceptance.

## Run

Use the existing project Python runtime. No dependency installation is needed.

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --navigation --seconds 180 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --navigation --interact --combat --branch-side right --seconds 420 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --interaction-goals --branch-side right --seconds 420 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --interaction-goals --combat --look-input-check --branch-side right --seconds 900 --fixture J:\gearsVr\analysis\playtest-fixtures\<verified-progress>.json
& $py J:\gearsVr\tools\game-playtest\run.py run --navigation --world-state --interact --combat --look-input-check --forward-input-check --branch-side right --target -949.862 -9413.523 -138 --seconds 480 --fixture J:\gearsVr\analysis\playtest-fixtures\<verified-progress>.json
```

The optional fixture is an explicit JSON manifest with `files` entries containing
`name`, absolute `path`, and `sha256`. Only existing profile filenames in the
protected SaveData set can be replaced. Fixtures and game-derived data belong in
ignored `analysis/`, never in a release package. Without verified mouse movement
and turn bindings, ordinary exploration records a missing capability and stops.
The optional `--forward-input-check` instead requires current reflected `aBaseY`
input to produce forward travel and settle after neutralization. It calibrates
again after a pawn/world change, bounds each pulse to 0.25 seconds and 64 units
of sampled displacement, and certifies the input field neutral before closing
its write handle. With accepted look calibration, navigation uses signed yaw
input as well. These are input fields; saved bindings are not changed.

Combat observations include the pawn and body mesh render timestamps and hidden
flags. The current policy defers targets without a recent body-render hint and
rechecks that hint before input. Recent rendering does not establish line of
sight, a clear bullet trace, visible hit geometry, or exclusive damage attribution.
After twelve seconds of continued combat decisions, the policy gives navigation
an eight-second interval. This applies across targets for the current pawn/world
so repeated aim corrections cannot keep route progress suspended indefinitely.

Current SpecialMove byte state is observed independently of the cinematic flag.
Nonzero special moves inhibit ordinary movement, aim, fire and pickup admission.
For a verified DoorOpen event inside its matching door trigger, pickup is held
up to0.45 seconds and released when a special move starts. Outcome observation
allows up to5 seconds for the normal callback and animation. No game functions
are invoked directly. Scripted motion does not become a post-release drift
candidate solely from its position change.
After an accepted door interaction, a short exit goal follows the live alignment
actor rotation (or the approach direction) beyond the trigger and pawn cylinders.
Crossing requires the same pawn/world, completed special move, far-side distance,
lateral bounds and floor bounds. An open event alone does not complete a door
navigation goal. Repeat `--next-target X Y Z` to queue ordered goals; each starts
only after the preceding interaction and crossing. A completed explicit sequence
stops instead of selecting an unrelated exploration route.
Sparse navigation graphs may end outside the interaction volume. A final ground
hint of at most400 units and180 units of height difference is appended instead
of repeatedly declaring the last graph node reached. This segment remains an
unverified walking hint: normal collision, input/state guards and obstacle
recovery still decide whether travel succeeds.

Navigation discovery reads the current pawn cylinder and each link's typed
reachFlags, CollisionRadius and CollisionHeight. The walking input policy
admits only the exact supported flag mask1 with dimensions at least as large as
the current cylinder. Missing requirements, additional flag bits, and narrower
links are excluded from both routes and goal reachability. Forced ground hints
must pass the same admission checks. These authored requirements do not prove
that the current path is physically clear; stalled movement is still held for
inspection. Older graph exports without this admission evidence are excluded.

A position change greater than1000 units between certified observations also
invalidates the old route when the pawn and world generation remain the same.
Automatic mission mode reacquires goals; explicit target mode holds input until
a fresh target is provided. An existing operator hold stays held. The position
change is recorded without inferring its cause or crediting the previous goal.

Continuation fixtures also retain observed checkpoint signals. Matching requires
the game hash, complete semantic sequence outer path and complete FName pair;
addresses and checkpoint numbering are not used across processes. Positive
checkpoint-event observations are exported with their source path and hash.
A separately verified current-checkpoint binding can mark its own signal as
already reached. Reloading a profile can reset live event counters, so these
scoped records prevent selecting an earlier checkpoint approach again. They
guide navigation and do not establish chapter or campaign completion.

Exploration keeps its selected destination across spatial graph refreshes.
Replanning preserves the same current node identity and position; selecting a
route does not retire its goal. The goal is retired only after route arrival.
New mission goals, changed context, cinematic repositioning and explicit control
requests may replace it. Missing or changed destination identities are recorded;
an existing destination without a current walking route holds for inspection.

When checkpoint and path-choice hints provide no touch goal, bounded discovery
also examines player-only touches authored upstream of level-visibility waits.
Only currently visible loaded origins, matching pawn proximity classes, unused
enabled events, distance at most1800 and height difference at most300 qualify.
This lets the controller approach a story/streaming entrance before walking
into a door whose opening depends on that entrance. Link conditions and physical
clearance remain unverified; actual touch counters and effects are recorded.

`--test-health-assist` is an explicit assisted flow-test option. It writes9999
to the positive Health IntProperty of the current test-owned WarPawn when health
is below1500. Ownership, world, writable private data, background input state and
behind-window conditions are checked; readback and handle closure are recorded.
It does not establish survival or normal-damage acceptance and cannot prevent
every scripted death. Its reports and issue groups are separate from tests with
ordinary health. Protected profiles/settings are restored after owned game exit.

The runner waits for other games/launcher owners to exit and requires three
seconds of idle state. It uses the verified `build-test-behind2` startup and
matching atomic simulator candidate, with the three required unattended flags:
behind windows, muted game audio and observe-only tiny-output monitoring. It
does not change system OpenXR selection. Runtime snapshots check test-window
occlusion without capturing desktop pixels or moving windows.

Settings and all existing profiles are copied and hashed before the test.
The test temporarily selects a 1280x720 windowed desktop size. The named launcher
object remains open through native shutdown, normal game close, and exact file
restoration. New gameplay inputs require the exact launcher identity started by
this run. Physical background controller handling must be in its original state.

Create a file named `stop` inside the current run directory, or hold F8, to stop
new actions. Every dispatched mouse down and up is logged. Release remains
conditional on the original game/window identity being available.

## Evidence and issues

The primary product under test is the VR Mod. Navigation obstacles and runner
defects are separate support issues, and movement after input release remains
unattributed until reproduction proves its cause. They are not confirmed Mod
bugs. After an owned run closes, review the Mod's actual runtime evidence with:

```powershell
& $py J:\gearsVr\tools\game-playtest\mod_audit.py J:\gearsVr\analysis\playtest\<finished-run>
```

This creates `mod-report.json` with exact game/native/XR versions, source hashes,
producer publication continuity, native faults, invalid submitted tickets and
existing pause/checkpoint/cinematic coverage. It reads the separate menu ledger
at the native measurement time: intentional movie common-panel output must not
be counted as a gameplay stereo freeze. A projection gap does not by itself
prove black output; successful submission does not prove pixels or headset view.
Candidate findings require reproduction and causal attribution.

A separate passive observer can follow a long owned game without changing its
running controller or sending input. Use the exact session and original run:

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py observe --session <session.json> --source-run <owned-run/report.json> --seconds 21600 --output J:\gearsVr\analysis\playtest\<mod-observer>
```

For a private multi-instance game, pass its three `GEARSVR_INSTANCE_*` context
variables as for its controller. The source report must have the same game and
owner identities and all three game/native/XR hashes. The attachment freezes
that report and retains its assistance provenance; observation sends no input.
Matching verified source cleanup ends observation normally, without claiming
campaign completion. Abrupt or unverified closure remains incomplete.

Native UI/material fault-counter increases create unconfirmed `mod_candidate`
issues once per increase. First attachment records a positive baseline with
unknown prior timing. Missing values are not zeros, and counter resets are not
new faults. Semantic-incomplete and guard-rejection counters remain diagnostic
observations rather than confirmed bugs. Summary issue groups retain XR hashes
as well as the game and native hashes; `confirmed_mod_bugs` requires explicit
confirmation and is separate from legacy unattributed product candidates.

Each run contains:

- `report.json`: identities, build/source hashes, coverage, issues and lifecycle.
- `events.jsonl`: accepted/rejected observations, menu/key receipts, movement,
  ammunition and FOV changes, state transitions and issue evidence indices.
- `backup/`: original settings/profile bytes, hashes, post-test copies and the
  restoration receipt.
- `launcher.log`, `manifest.json`, `config.json`: exact test startup inputs.
- `runner-source/`: the exact code snapshot used for deferred scenario imports.
- `navigation-*.json`, `path-choice-*.json`: bounded navigation and current
  path-choice key/ownership evidence, when those scenes are encountered.
- `world-state-*.json`: reflected objectives, the current PC interaction event,
  same-world PawnList records, hostile targeting proof and collision dimensions.

The `--seconds` argument bounds exploration after optional flow scenarios.
Startup, individual state waits and cleanup have separate finite limits.

```powershell
& $py J:\gearsVr\tools\game-playtest\summarize.py
```

This writes `analysis/playtest/summary.json` and `issues.csv`. Reports distinguish
unconfirmed product candidates, navigation obstacles, observation churn and
test infrastructure. Separate binary versions are not merged into one issue.
Paused menus, changed world generations and missing counters break stall
measurement; missing observations never become zero FPS. A publication stall
requires camera time to advance while the same world's publication counter
does not advance for three seconds. It does not prove black pixels.

An interrupted restoration can be completed only after verified native/entry
cleanup and owned-process exit:

```powershell
& $py J:\gearsVr\tools\game-playtest\recover.py J:\gearsVr\analysis\playtest\<run> --wait-seconds 600
```

Recovery acquires the shared launcher object before touching files and keeps
each pre-restoration copy. An occupied game or an unverified cleanup leaves
the backups and reports available; it does not overwrite a running session.

## Read-only observation and navigation

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py observe --session <exact-session-report> --seconds 30
& $py J:\gearsVr\tools\game-playtest\navigation.py --session <exact-session-report> --output J:\gearsVr\analysis\playtest\navigation.json
```

Observation never adopts session input ownership. Navigation discovery reads
the current World's NavigationPoint list and ordinary ReachSpec links within
explicit bounds. It scans and retains at most 2048 points. Selected graph reads
have a 12-second check, 32768 unique address/size entries and an 8 MiB bound;
resolver discovery and fresh identity reads are separate. It rechecks identities
and bytes. Returned paths
are hints: special traversal, dynamic collision, pawn clearance and mission
goals still require independent admission and actual movement verification.
Ground-bounded `ForcedReachSpec` references can also become walking hints when
their ancestry and endpoints are validated, their length is at most 1200 units
and endpoint height difference is at most 180 units. Their actual clearance and
walkability remain unproven. Mantle and swat-turn references remain unsupported
and are counted separately in graph diagnostics.
Graphs are retried after an acquisition rejection and refreshed after movement
beyond 1200 units from the acquisition origin. A changed world discards the old
graph. A bounded menu-read conflict during a pulse releases input and returns to
fresh observation without treating the partial action as a verified failure.
`--navigation` connects these hints to exploration. An optional `--target X Y Z`
ranks navigation destinations near an observed test trigger; movement remains
ordinary scoped input. The default path-choice policy selects the current
left trigger, with `--branch-side right` or `none` available. The trigger must
be freshly reflected, unambiguous and supported; unknown keys are not guessed.
Certified cinematics inhibit gameplay input. Navigation stalls can request at
most two state-checked checkpoint recoveries during an exploration run.

`--world-state` reads objective values and current interaction/PawnList records.
ObjectiveInfo notification flags and timestamps do not count as mission progress.
`--interact` enables world-state acquisition and allows one E/F pickup-key edge
per fresh interaction identity/world, only if its current binding is exactly
`Game_Pickup`. Event TriggerCount advancement is checked independently. A close
`--target` approach includes short walking steps, followed by a bounded
turn-and-wait phase so backward movement does not leave the player facing away
from a button before entering its touch cylinder.

`--interaction-goals` enables navigation, world state and interaction input. It
chooses an enabled, unused same-world trigger with a nearby node in the current
directed walking component. A geometrically close trigger on the disconnected
side of the room cannot win solely on distance. An unused trigger reachable by
ordinary links takes precedence over one reachable only through forced links;
the route also keeps an ordinary detour before considering a forced shortcut.
Forced links can cross doors whose current state has not been established.
Fresh reflected player/trigger
collision radii determine the final approach radius. Timed-out goals are retired
for that world, and successful activation forces new graph/goal acquisition.
These are route hints; current PC.InteractEvent and actual movement still decide
whether an action can be sent. Mission ordering is not inferred from positions.

Loaded path-choice actions also supply bounded authored flow hints. A focused
parent sequence read follows incoming links to unused, enabled Touch events,
validates their actor/world and collision-component ownership, and reads reflected
BoxSphereBounds. Brush volumes can have an Actor Location of zero, so a nearby
reachable navigation node supplies the elevation and a short approach inside the
observed bounds. Touch arrival waits without turning around; it never sends a
pickup key or activates a sequence operation. Authored connections do not prove
condition results or execution order. A route that reaches no new waypoint for
30 seconds is reacquired; three such attempts retire that goal for this world.

Waypoint walking adapts its bounded hold duration to the next corner and measured
walking speed. Short final-approach pulses use a corresponding response threshold.
Successful movement clears the local collision streak. Cinematics and route-choice
transitions discard old navigation even when player displacement is small.

`--combat` also enables world-state acquisition. Live AI targeting this player,
or a live member of this player's exact team while belonging to the opposing
verified team, can be selected. Squad admission requires current WarTeamInfo/PRI
world links, distinct0/1 opponent team indices and matching ally team identity.
Mounted pawns additionally require reciprocal DrivenVehicle/Driver links and the
AI controller's Pawn backlink to that vehicle. Every input rechecks this control
and hostility evidence. Mounted aim uses an upper-body point inside the observed
capsule and receives priority within the admitted engagement range. A valid
pitch calibration permits elevated targets within2500 units in3D; without it,
the shorter1800-unit/150-height engagement bounds remain. Fresh reciprocal controller links, collision
dimensions, current camera orientation and observed pawns in the firing lane
gate input. Line of sight remains unproven; declining a shot
does not establish a game defect. Shots are capped per target and event records
keep ammunition response separate from target-health change. A health decrease
after firing is temporal evidence; teammate damage may contribute.

`--look-input-check` (alias `--pitch-check`) calibrates pitch and yaw through the
current player's reflected `aLookUp`/`aMouseY` and `aTurn`/`aMouseX` input floats.
Each pulse requires this run's launcher, current controller/input/pawn links,
float32 metadata, a neutral initial value, a behind-window desktop check and
disabled physical background controller handling. Writes use a process handle
checked by creation time and image path, and target committed writable private
data. Positive pulses are limited to 0.25 seconds; sampled angular thresholds
request an early stop. Scheduling can overshoot a threshold, so actual angular
response and hold duration are recorded. A fresh certificate gates neutralization,
and an unverified neutral state terminates the run. Calibration requires both
controller and camera response plus neutral readback and stable post-release
angles. Reports mark this synthetic input backend as `assisted=true`; issue
grouping keeps its results separate from ordinary HWND-input runs.
An acquisition conflict during a pulse is recoverable only after fresh slot
certification, neutral readback and writer closure. Calibration can then make one
new attempt from a fresh baseline; a partial native write failure is not retried.
Input-slot certificates permit registry count growth only when table storage and
capacity stay fixed and every selected identity and metadata value still matches.
Table shrinkage, reallocation and selected-object changes remain rejection cases.
Calibration below five degrees per second at its measured input value is not used
as an aiming backend.

With calibration available, combat can use signed pitch/yaw corrections and a
sampled angular target before scoped fire. Fire and turn budgets are tied to the
target identity and current encounter area, so moving around an obstacle can
establish a new bounded engagement attempt.
Near the aiming target, correction magnitude decreases and leaves angular
headroom for metadata checks and release latency; a minimum-duration pulse must
not retain the full calibration magnitude. Combat-loop reports save periodically
before the next action, including iterations that continue after aiming/firing.
An aimed burst holds the owned right mouse button across a fresh world/target
and firing-lane read. Reduced FOV must be observed before a left-button pulse;
both mouse flags preserve the overlap and both releases are checked. The bounded
aim scope may make up to two neutralized angular corrections, using only an
observed current-hold response estimate with unchanged pawn/world. Mounted aim
uses a narrower upper-capsule angular band. Neither that geometry nor reduced
FOV establishes line of sight or guarantees a hit. Combat input pauses the
navigation no-waypoint-progress timer.

An exact `DeltaGameover` screen focused on player zero's `btnCheckpoint` permits
one reviewed checkpoint confirmation. Recovery checks the new world, restored
control and advancing stereo publication, with at most three recoveries per run.
`death_reload` additionally requires a fresh read of nonpositive health on the
previously controlled pawn; an unfamiliar failure screen does not establish death.
Owned follow-up diagnostics use separate reports and leave session cleanup to
the original launcher.

## Continue test progress

```powershell
& $py J:\gearsVr\tools\game-playtest\progress.py J:\gearsVr\analysis\playtest\<finished-run> --output J:\gearsVr\analysis\playtest-fixtures\<progress>.json
```

The export requires verified native/entry cleanup, game exit, exact restoration
and matching post-test profile hashes. It references immutable post-test copies
under ignored `analysis/`, records the game version and never reads or writes the
current user profile. Pass the result as the next run's `--fixture`; runtime
observations must verify which checkpoint actually resumed. A changed profile
file alone does not prove mission progression.

For a failed scenario that nevertheless completed all owned cleanup/restoration,
`--allow-incomplete` permits a separate continuation fixture. It retains the
source phase and `source_scenario_completed=false`; it does not convert a failed
scenario into a pass or permit an active/unrestored run to supply profile bytes.
Normal process closure is also separate from scenario success: a run can finish
at its death-recovery budget without completing its gameplay target.

For bounded read-only spatial discovery during an already-running session:

```powershell
& $py J:\gearsVr\tools\game-playtest\mission_hints.py --session <exact-session-report> --output J:\gearsVr\analysis\playtest\mission-hints.json
```

This inventories loaded objective actions and interaction subclasses, validates
originator/world links and records enabled state, trigger counts and positions.
It does not invoke sequence operations or prove mission ordering/walkability.

## Validation

```powershell
& $py -m unittest discover -s J:\gearsVr\tools\game-playtest -p 'test_*.py' -v
```

Tests cover state/identity boundaries, obstacle classification, interrupted
input release, input ownership, disabled physical background controller input,
pause animation gating, unknown prompts, file backup/restoration, version-aware
issue grouping and directed route selection. Live runs remain separate evidence.

---

## 中文

# 国家自主游戏测试

此导师开始一个私有背景 VR 测试会话, 执行有界限的游戏
行动,通过反映的状态检查其效果,并写问题和
忽略的覆盖证据 `analysis/playtest/`。默认运行不
捕获图像, 记录视频, 或将图像发送到模型。 正常的游戏/ VR 渲染
仍然运行。一个明确 `--gpu-snapshots` 运行启用已有的模拟器眼
图像快照控制用于视觉特性验证。 它记录此选择
;它不捕捉桌面或其他窗口。

当前执行行动的变化和转动、目标/火力反应,
暂停/恢复、普通检查站重新装货、电影控制抑制和
返回路径选择触发器。它尚未完全替换
运动游戏。当前的目标计划、互动活动和现场AI目标
备选互动和作战政策仍然需要取得成功。
游戏游戏接受率;特殊曲面、挂载使用和章节完成率仍然不变
覆盖报告保留了这些缺失的要求。
组件检查并不意味着运动或耳机接受。

## Run

使用已有的 Python 运行时间。 不需要依赖性安装 。

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --navigation --seconds 180 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --navigation --interact --combat --branch-side right --seconds 420 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --flows --interaction-goals --branch-side right --seconds 420 --fixture J:\gearsVr\analysis\playtest-fixtures\scoped-movement-vrdof124.json
& $py J:\gearsVr\tools\game-playtest\run.py run --interaction-goals --combat --look-input-check --branch-side right --seconds 900 --fixture J:\gearsVr\analysis\playtest-fixtures\<verified-progress>.json
& $py J:\gearsVr\tools\game-playtest\run.py run --navigation --world-state --interact --combat --look-input-check --forward-input-check --branch-side right --target -949.862 -9413.523 -138 --seconds 480 --fixture J:\gearsVr\analysis\playtest-fixtures\<verified-progress>.json
```

选择性固定是明确 JSON 的表单,其中 `files` 包含条目
`name`绝对值 `path`,以及 `sha256`中仅显示已存在的配置文件名。
保护的保存数据集可以替换。 固定和游戏衍生数据属于
ignored `analysis/`中,从不在释放包中。没有经过核实的鼠标移动
并转动装订,普通勘探记录 能力缺失和停止。
任择 `--forward-input-check` 需要当前反映 `aBaseY`
输入以生成远期旅行并在中和后结算。它校准
一次棋子/世界变换后,将每个脉冲都限制在 0.25 秒和秒数 64 units
并验证输入字段中性,然后关闭
使用签名的 yaw 来校准其写手柄。
输入。这些是输入字段;保存的绑定不更改。

战斗观察包括当兵和身体网状变形和隐藏时间戳
标记。当前政策推迟目标,而不给最近的身体提示,
输入前重新检查提示。 最近的渲染没有建立行
视觉,清晰的子弹痕迹,可见的撞击几何,或者独家的破坏归属.
继续战斗12秒后, 政策赋予导航
。这适用于当前棋子/世界的目标。
因此,重复的目标纠正不能无限期地暂停路径进展。

目前特别移动字节状态独立于电影旗的观察.
非零特殊动作抑制了普通运动,瞄准,射击和皮卡的录取.
对于一个在匹配的门触发器内经过验证的门打开事件,会举行皮卡
最高0.45 秒, 当特殊动作开始时释放。 结果观察
允许最多5 秒。没有游戏功能
脚本运动不会成为释放后漂移
候选人完全从其职位上改变。
经过一个接受的门交互后,在直播对齐后有一个短的退出目标
演员旋转(或接近方向)超出扳机和当事机圆筒.
穿越需要同样的棋子/世界,完成特殊移动,远边距离,
横向界限和地板界限。单独一个开放的事件不能完成一个门
导航目标。 重复 `--next-target X Y Z` 以队列命令的目标;每个开始
仅在前一次交互和交叉之后。
停止而不是选择一个无关的勘探路线。
磁盘导航图可能结束于交互体积之外。 最终的地面
提示最多400 单位和单位180 高度差的单位改为附件
,并多次声明最后一个图节点已到达。
未验证的行走提示:正常碰撞、输入/状态守卫和障碍
恢复工作仍决定旅行是否成功。

导航发现读取当前当量圆柱和每个链接的键入
到达 Flags、 Collision Radius 和 CollisionHeight。 步行输入策略
只承认精确支持的旗面罩1 尺寸至少与
。缺少要求、增加旗标位数和缩小
链接被排除在路线和可达目标之外。
必须通过同样的录取检查。这些撰写的要求不能证明
当前的路径已实际清晰; 移动停滞仍然被牵制
不包括没有这种入境证据的老旧图表出口。

位置变化大于1000 核证的观测结果之间单位
当当当和世界一代保持不变时,则取消旧的路径。
自动任务模式重新获得目标; 明确的目标模式将输入保存到
提供了新的目标。现有操作员持有保留。位置
记录变化时没有推断其原因或计入先前的目标。

继续固定装置还保留了可观察到的检查站信号。
游戏散列,完整的语义序列外路径和完整的FName对;
地址和检查站编号不跨程序使用。
检查站活动观察随其源路径和散列导出.
单独核查的当前检查点绑定可以将自己的信号标为
重新装入配置文件可以重设事件显示器,所以这些
范围化记录防止再次选择较早的检查站方法。
引导导航,不建立章节或运动补全。

探索使其选定的目的地跨越空间图刷新。
重新规划保留相同的当前节点身份和位置;选择一个
路由不会使其目标退役。只有在路由到达后才使其目标退役。
新的特派团目标、改变背景、重新定位和明确的控制
请求可以替换。
现有目的地没有目前的步行路线,需要检查。

当检查点和路径选择提示没有触摸目标时, 定界发现
还检查了仅玩家触摸的作者在级别可见度等待的上游.
仅当前可见的加载源, 匹配棋子近距离类, 未使用
启用事件, 最多距离1800 和高度差异最多300 qualify.
这样控制器在行走前可以接近故事/流出入口
进入一个门,其打开取决于入口。
清除工作仍未核实;实际触摸计数器和效果记录。

`--test-health-assist` 是一个明确的辅助流程测试选项。它写9999
健康时的WarPawn公司
下方为1500 所有权、世界、可写作的私人数据、背景输入状态和
检查窗口后方条件;记录回放和处理关闭。
它不能证明生存或正常损害的接受,也不能阻止
报告组和发行组与测试组分开
普通健康。自有游戏退出后恢复保护配置/设置。

跑者等待其他游戏/发射机所有者退出,需要3个
状态。它使用已验证的状态 `build-test-behind2` 启动和
匹配原子模拟候选人, 并配有三个需要未覆盖的旗帜 :
窗口后, 声音和只观察的微小输出监视器。 它
切换系统 OpenXR 选择。运行时快照检查测试窗口
隐蔽而不捕捉桌面像素或移动窗口.

设置和所有已有配置文件在测试前被复制并散列 。
测试临时选择一个 1280x720 窗口桌面大小。命名的启动器
通过本地关机、正常游戏关闭和精确文件,对象仍然打开
。新的游戏游戏输入需要精确的发射人身份
运行中。物理背景控制器处理必须处于原始状态。

创建名为的文件 `stop` 当前运行目录中, 或持有 F8,停止(停止)
新建动作。每个已发送鼠标下行和上行都被记录。释放仍然有效
以提供游戏/窗口的原始身份为条件。

## 证据和问题

测试的主要产品是VR Mod. 导航障碍和跑步器
缺陷是单独的支持问题,输入释放后的流动仍然存在
在复制证明原因之前未分配。
错误。 在自有运行关闭后, 请以下列方式审查 Mod 的实际运行时间证据:

```powershell
& $py J:\gearsVr\tools\game-playtest\mod_audit.py J:\gearsVr\analysis\playtest\<finished-run>
```

创建 `mod-report.json` 与精确的游戏/本土/XR版本,源码散列,
制作人出版连续性、本土缺陷、无效的票证和
已存在的暂停/检查点/焚烧覆盖。它读取单独的菜单分类账
本地测量时间:不得故意拍摄普通小板片
被算作游戏播放立体冻结。投影缺口本身并不是
证明黑色输出; 成功提交不证明像素或头盔视图。
候选人的调查结果要求复制和因果归属。

单独的被动观察者可以跟随长期拥有的游戏而不改变其
运行控制器或发送输入。使用准确的会话和原始运行 :

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py observe --session <session.json> --source-run <owned-run/report.json> --seconds 21600 --output J:\gearsVr\analysis\playtest\<mod-observer>
```

私人多功能游戏,通过三场 `GEARSVR_INSTANCE_*` context
变量。源报告必须具有相同的游戏和
拥有者身份和所有三个游戏/内幕/XR散列。附件冻结
报告并保留其援助来源;观察没有提供意见。
匹配经核实的源清理通常结束观察,而不声称
粗糙或未经核实的关闭仍未完成。

本地用户信息/ 物质错误处理增加, 造成未经证实的 `mod_candidate`
第一次附件记录了一个正基线,其中
未知的上一个时间。 缺少的值不是零, 且反转录器不是
语义不完全和防守拒绝计数器仍然具有诊断性
而非确认的错误。 简要问题组保留 XR 散列
以及游戏和散货。 `confirmed_mod_bugs` 需要明确
与未分配的遗留产品候选人分开。

每次运行包含:

- `report.json`:身份、构建/源码散列、覆盖、问题和生命周期。
- `events.jsonl`:接受/拒绝的观察、菜单/钥匙收据、移动、
  弹药和爆炸物的改变、状态过渡和发布证据指数。
- `backup/`: 原始设置/ 配置字节、 散列、 测试后副本和
  恢复收据。
- `launcher.log`, `manifest.json`, `config.json`: 精确的测试启动输入.
- `runner-source/`: 延迟情景导入所用的精确代码快照。
- `navigation-*.json`, `path-choice-*.json`: 边框导航和电流
  路径选择键/所有权证据,当遇到这些场景时。
- `world-state-*.json`: 反映的目标,当前PC交互事件,
  同一世界的PawnList记录,敌对的目标证据和碰撞维度.

The `--seconds` 参数在可选流场景后将探索范围限制在其中。
启动,单个状态等待和清理有单独的限量限制.

```powershell
& $py J:\gearsVr\tools\game-playtest\summarize.py
```

此写 `analysis/playtest/summary.json` and `issues.csv`报告区分
未经证实的产品候选品、导航障碍、观测
测试基础设施。 单独的二进制版本不合并为一个问题。
菜单被暂停,世界几代人改变,柜台丢失,破摊
测量; 缺失的观测结果永远不会变成零 FPS。 出版时的停顿
需要相机时间推进,同时使用同一个世界的出版计数器
。它不能证明黑色像素。

中断的修复工作只有在经过核实的原生/进入后才能完成
清理和自有进程退出 :

```powershell
& $py J:\gearsVr\tools\game-playtest\recover.py J:\gearsVr\analysis\playtest\<run> --wait-seconds 600
```

回收在接触文件前获取共享的发射器对象并保存
每一份修复前副本。游戏占用或未经核实的清理叶片
备有备份和报告;它不覆盖运行中的会话。

## 仅读观测和导航

```powershell
& $py J:\gearsVr\tools\game-playtest\run.py observe --session <exact-session-report> --seconds 30
& $py J:\gearsVr\tools\game-playtest\navigation.py --session <exact-session-report> --output J:\gearsVr\analysis\playtest\navigation.json
```

观察绝不采用会话输入属性。导航发现为
当前世界导航点列表和普通的 ReachSpec 链接
明确界限。它最多扫描并保留 2048 点。选定图读取
有个 12- 第二次检查, 32768 唯一地址/大小条目和 8 装订的米B;
解析器发现和新身份读取是分开的。它重新检查身份
和字节。返回路径
指向: 特殊的横贯、动态碰撞、 棋子清除和任务
目标仍然需要独立的接纳和实际行动核查。
Ground-bounded `ForcedReachSpec` 当引用时,也可以成为行走提示
它们的祖先和终点被验证,长度最多 1200 units
终点高度差异最多 180 实际清除和清除的地雷
移动性仍未得到证实。 Mantle 和 swat-turn 引用仍然不支持
并分别计入图表诊断。
图表在拒绝购置后重新审阅,移动后刷新
beyond 1200 的单位。
图表。脉冲期间的限定菜单读冲突释放输入并返回到
新的观察,没有将部分行动视为经核实的失败。
`--navigation` 将这些提示连接到探索中。可选 `--target X Y Z`
将导航目的地排在观测到的测试触发器附近;移动仍保留
普通范围输入。默认路径选择策略选择当前
左键触发器, 带有 `--branch-side right` or `none` 可用。必须使用触发器
重新反映、明确和支持;未知的密钥不被猜测。
认证电影院禁止游戏游戏输入。导航站可在
大部分两个国家检查的检查站在勘探过程中收回的款项。

`--world-state` 读取客观值和当前交互/光线记录。
目标信息通知旗和时间戳不计入特派团的进展。
`--interact` 允许世界状态获取并允许一个E/F小提琴边缘
每个新的交互特性/世界,只有其当前约束性是准确的
`Game_Pickup`。事件触发状态推进会独立检查。关闭
`--target` 方法包括步行的短步,然后是有界线的
转弯等待阶段, 这样后向移动不会让玩家面临
进入触摸筒前从按钮中跳出。

`--interaction-goals` 允许导航、世界状态和交互输入。它
选择一个启用的、未使用的、在电流中带有附近节点的同世界触发器
断开的直径导行走组件上的几何近距离触发器
单靠距离不能赢得房间的一侧。未使用的触发器可以被
普通链接优先于只能通过强制链接才能达到的链接;
路径在考虑强制快捷键之前,还保留了普通绕行。
强制连接可以跨越那些尚未确定现状的门.
新鲜反射的玩家/ 触发器
碰撞光度确定最后接近半径。超时目标已退役
成功激活会促使我们获得新的图表/目标
这些是路由提示; 目前的 PC. InteractEvent 和实际移动仍然决定
是否可发出行动命令。任务订单不能从阵地推断。

装入路径选择动作还提供了有限制的作者流提示。
父序列读取的跟随未使用、启用的触摸事件接收到的链接,
验证其行为者/世界和碰撞组成部分的所有权,并阅读反映的
圆圈圆柱 刷子卷可以有零的演员位置 所以附近有
可达到的导航节点提供高程和在高程线内短路。
观察的界限。触摸到达时不转身;它从未发出过
抓取密钥或激活序列操作。 授权连接无法证明
条件结果或执行命令。没有达到新路径的路径
30 3次这样的尝试使这个世界的目标落空。

路标将它的边界长度调整到下一个角并测量
步行速度。短的终极脉冲使用相应的响应阈值。
成功运动清除了局部碰撞。
过渡即使玩家移动小,也会丢弃旧导航。

`--combat` 也可以实现世界状态的获取。 Live AI 瞄准此玩家,
或一名现场队员 属于对手
已核实的团队,可以选择。
与世界的联系,独特的0/1 对手团队指数和匹配盟友团队身份.
骑兵需要相互的驱动车辆/司机连接和
AI控制器的Pawn回路连接到那辆车上,每一次输入都会重新检查这个控制器
目标在观测到的物体内使用上体点。
胶囊,并在允许的接触范围内获得优先权。
投球校准允许在内部提高目标2500 单位单位3 D;没有它,
短数1800-unit/150 新的对接控制器连接 碰撞
尺寸、当前摄像头方向和在射击道上观察到的棋子
门输入。 视线仍未证实; 下降镜头
无法确定游戏缺陷。射击被封杀每个目标及事件记录
保持弹药反应与目标健康变化分开。
射击后是时间证据;队友的损伤可能促成.

`--look-input-check` (alias `--pitch-check`校准投球和通过
当前玩家的反映 `aLookUp`/`aMouseY` and `aTurn`/`aMouseX` 输入浮点。
每个脉冲都需要这个运行的发射器, 当前的控制器/输入/小时链接,
float32 元数据、 中性初始值、 窗后桌面检查和
已禁用物理背景控制器处理。 写入使用进程控件
检查创建时间和图像路径, 以及目标已承诺的可写入的私人
数据。正脉冲仅限于 0.25 秒; 抽样角阈值
请求提早停止。排程可以超标阈值,所以实际角
新的证书门中和,
校准需要两种条件
控制器和相机反应,加上中性回放和稳定的释放后
角度。报告将此合成输入后端标记为 `assisted=true`; 问题
分组使其结果与普通的HWND-输入运行分开.
脉冲期间的获取冲突只有在新槽后才能恢复
校正可以使一个
新的尝试来自新基线; 局部本地写失败没有重试 。
输入槽证书允许登记数增长,只当表格存储和
能力保持不变,每个选定的身份和元数据值仍然相符。
表缩水、重新分配和选定对象的更改仍然是拒绝情况。
未使用按其计量输入值计算每秒五度以下校准
做为目标后端

有了校准,战斗可以使用签名的投球/音响校正和a
瞄准点前取样角目标。
目标身份和当前交锋区域,因此在障碍物周围移动可以
确定新的有限度的接触尝试。
目标附近,校正体积减少,并留角
元数据检查和释放延迟的源头室;必须有一个最小期限的脉冲
战斗启动报告定期保存
包括瞄准/射击后继续进行的重复。
瞄准的爆破会让鼠标右键穿过一个新世界/目标
左键脉冲前必须观察到视野下降;
鼠标标记都保留重叠,并检查两个释放。
目标范围最多可构成两个中和角修正,只使用
观察到的当前响应估计数,卒/世界不变。
使用较窄的上盖角带。无论几何还是缩小
视野确定视线或保证击中。战斗输入暂停
导航无向点-进度计时器.

准确 `DeltaGameover` 屏幕聚焦于玩家 0 `btnCheckpoint` permits
检查了新世界,恢复了
控制并推进立体声出版,每跑最多回收3次。
`death_reload` 还需要在卫生方面重新阅读无阳性健康读物。
之前控制的棋子; 不熟悉的失败屏幕不能确定死亡。
自主后续诊断使用单独报告,留职清理
原发射装置。

## 继续测试进度

```powershell
& $py J:\gearsVr\tools\game-playtest\progress.py J:\gearsVr\analysis\playtest\<finished-run> --output J:\gearsVr\analysis\playtest-fixtures\<progress>.json
```

出口需要经过核实的本地/ 输入清理、 游戏退出、 准确恢复
并匹配测试后配置文件散列。它引用了不可更改的测试后副本
忽略未计 `analysis/`,记录游戏版本,并永远不读或写
当前用户配置文件。 将结果传递给下个运行 `--fixture`; 运行时间
观察必须核实哪个检查站实际上恢复。
仅凭文件并不能证明任务进展。

对于一个失败的情景,虽然它完成了所有自有的清理/恢复,
`--allow-incomplete` 允许单独设置一个连续固定装置。
源位和 `source_scenario_completed=false`; 它不转换失败
假设为通过或允许运行/未恢复的运行以提供配置字节。
正常的流程关闭也与情景成功分开:运行可以完成
在其死亡恢复预算上,没有完成游戏目标。

对于已运行的会话中设定的仅读空间发现:

```powershell
& $py J:\gearsVr\tools\game-playtest\mission_hints.py --session <exact-session-report> --output J:\gearsVr\analysis\playtest\mission-hints.json
```

这份清单中包含客观动作和交互子类, 验证
发端人/世界链接和记录启用状态、触发计数和位置。
它不援引序列操作或证明任务命令/行走能力。

## 验证

```powershell
& $py -m unittest discover -s J:\gearsVr\tools\game-playtest -p 'test_*.py' -v
```

测试涵盖状态/身份界限、障碍分类、中断
输入发布、输入所有权、禁用物理背景控制器输入、
暂停动画 显示 未知提示 备份/ 恢复 版本意识
发布分组和定向路由选择。直播运行仍然是单独的证据。
