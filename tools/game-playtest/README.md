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
