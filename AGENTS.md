# GearsVR - Agent Development Code

## 1 Basic information on the project

- Projects: Gains of War Reloaded PCVR / OpenXR Mod.
- Current main working area:`J:/codexWorktree/gearsVr`
- Original items and delivery catalogue:`J:/gearsVr`
- Current priorities are based on the latest user directives and actual project status.
- Keeps the current functionality, source code, user configuration, archiving, testing evidence and reversionable versions.
- Do not duplicate an old document because it failed or expired.

## 2 MultiAgent/ Multi-Mechanism

Project may be 20 One or more Cordex sessions developed in parallel.

- Each session is responsible only for its own mandate and for the module with clear delegation of authority.
- Use independent Git Worktree, stand-alone build catalogues and test output as a priority.
- Checks changes to the current Git status and other sessions before changing shared files.
- Unsubmitted changes that do not cover, back off, and delete other sessions.
- No unauthorized modifications to other weapons, workstations or public modules.
- When changes are needed to share modules, the scope of changes will be clearly defined and minimal changes introduced.
- Do not close other sessions, games or work processes to release resources.
- No unauthorized creation of a son, Agent; only clear scope is assigned where necessary.
- No repetition of tested results with unchanged source code.
- Do not add a streaming account, test logs per round or full chat history to this document.

## 3 Development principles

- Priority is given to repairing real problems without re-engineering with no clear benefits.
- The actual code path and evidence of failure are confirmed before the change is made.
- Reuse existing modules, readers, testers and running tools as much as possible.
- Maintain normal life cycles, competencies and error handling.
- The period of validity, identification, tracing or security protection for the purposes of testing is not relaxed.
- It is prohibited to forge controls for input, game status, test success results or time stamp.
- All conclusions must be distinguished: source code achievement, compilation success, simulator validation, actual game certification, physical header verification.
- Compiled successfully does not equal the functional completion; a test is not equivalent to an inter-historic breakdown.
- First clear error checked in case of failure, not blindly retrying the whole process.

## 4 VR Mod Core Constraints

- Use OpenXR to maintain real eye-sterilisation 6 DoF head tracking.
- Maintains the only final camera/eye benchmark and necessary bone ownership.
- Do not introduce a second competitive camera or bone writer.
- The original bone length, skin skinning, physical input and weapons are retained.
- Head movement changes only the direction of observation and does not automatically change the actual orientation of the gun.
- The gun, the gun, the auxiliary handheld and the actual firing line must be in a consistent relationship.
- Hover Snap is responsible only for visual adsorption and does not automatically acquire physical control.
- Active Grab must be authorized by genuine and effective control of input and current conditions of exposure.
- Physical push, cartridge filling, shooting and sawing must be in compliance with the actual game state and inventory rules.
- Retain tracking freshness, input edges, identity, life cycle and re-entry checks.
- The simulator posture is not considered a real headprint tracking or end-user experience certificate.

## 5 Play and simulator tests

Controlled games, XRs and simulators are authorized in the tasks concerned without having to repeat each query.

- Prefer Reuse Already `isolated_sim.py`, do not create a duplicate startup platform.
- Do not directly start the game EXE, must use the already controlled startup entrance.
- Each new test instance uses a brand-new and unique name.
- Test examples use independent configuration, archiving and process identity.
- No focus on the current desktop, without prejudice to software being used by users.
- Current Visible Test Mode `--visible --lowest --render-scale 0.5`。
- Use as most recent request on startup `--wait-resources-seconds 0`。
- Reservations `-nomoviestartup` And the longest 180 Second player waits.
- Just... `startup_status.startup_confirmed=true` The first of these is the launch of a new video, which will be used to confirm success.
- Current performance tests do not record and no additional video collection is enabled.
- No fixed multi-volume cap; no human-set memory residual threshold is denied.
- Disk space, stand-alone configuration, process identity and normal exit conditions are still checked.
- There are examples of cases that are not closed for new example.
- The government has been able to provide information on the situation in the country, but it is not clear that the process and resources are being used to clean up its own identity.
- Tests to end the restoration of temporary changes in settings, inputs, audio and other user profiles.
- The user ' s latest clear operational instructions can override the test parameters of this section.

## 6 Performance analysis

Current performance target:**Practical game new eye frames reached 120 Hz, Single-Specific Budget 8.33 ms。**

- Strictly separates the game Precent, the new eye frame and XR Runtime loop frequency.
- XR 90/120 Hz does not mean the actual rendering of the game 90/120 FPS。
- The main FPS displays priority for actual game frames, which cannot be impersonated by XR loop count.
- The real bottlenecks are measured before the optimization is decided.
- The following are the most important examples of how to get a picture of the game, how to do it:
- The screen may not be used as a root cause for the absence of video tests where frame rates are still low.
- Performance A/B comparisons must ensure comparable conditions; other examples are not valid when they interfere with the marking and do not automatically close them.
- Do not consider the reduction of the number of calls, log numbers or successful compilations directly as FPS upgrades.
- Keeps real measurements, test conditions and failure results.

## 7. Lancer and Weapons Work

- Lancer ' s current assignment is based on up-to-date and real user feedback, desk data and evidence of effective testing.
- The first group of people in the country is the one who has been working with the government and the other, which is a member of the National Police.
- Focus on performance, left-hand trigger flashing, C grab position, natural direction of the gun and hand alignment.
- The user has confirmed that the real Grab Pose, Volume, Snap parameters have priority over old auto-generated postures.
- No unauthorized hand-collection coverage.
- No complete Lancer is claimed to have been completed because of a local mechanical test.
- No changes to the current user access point unless the current task explicitly requires delivery or replacement.
- The public version, the candidate for segregation and the historical retreat are clearly distinguished.

## 8. Quest 3 Weapons workstation

- The main source code for the workstation is located in `tools/quest-workbench`。
- Retain independent weapons, master handcuffs, Second Hand, deputy postures, ammunition and parts editing mechanisms for activity components.
- Use of real attitude confirmed by current users and latest equipment data, without reverting to historical attitudes that have been denied by users.
- Backup of original collection files, project configuration and existing AAPK.
- Current Authorization Allows Quest 3 On-line and USB is authorized to complete the new version of the build validation and backup after overlay installation.
- Do not modify PCVR Mod, other devices or games without permission because of the update of the workstation.
- The workstation simulation interactive, successful collection, successful AAK installation and real game hand-checking are recorded separately.

## 9 Reverse engineering

- Priority is given to existing IDA, Ghidra, knowledge base and stored analytical evidence.
- Add a new inverse first query `analysis/re-kb/knowledge.sqlite`。
- Distinguishing disk EXE, running memory, historical snapshot and UnrealScript metadata.
- The same code address does not mean the original bytes, and the version must be checked against identity.
- Do not allow multiple Agents to write the same IDA database at the same time.
- Original game resources, back-compilation results and private analysis may not be mixed into public distribution packages.
- Static analysis cannot replace real running or GPU/headprint validation.

## 10 Documentation and historical records

- `AGENTS.md` Only long-standing project rules are retained.
- `docs/` (c) Preservation of structures, current status, operational descriptions and development plans.
- `analysis/` Maintain logs, diagnostic data, historical tests, SHA, operational returns and laboratory results.
- Each session can be in its own right. `analysis/` Subdirectories maintain independent progress.
- Stops adding complete test flowbooks to the `AGENTS.md`。
- Historical records are accessible on demand and should not be fully loaded at the start of each mission.
- Old failures are known to be retained in historical evidence but not automatically become a barrier for new missions.
- The testing media generated by the project follow existing clean-up strategies; maintain source code, configuration, archiving, text evidence and back editions.
- Do not clear the user's personal file, other session data or the Cordex session database.

## 11 F. Reporting

In each case where a real impediment to the completion of a mission occurs, a brief description is given:

1. Do what you do.
2. Which documents have been modified.
3. Which compilers or tests actually pass.
4. What problems remain.
5. Gives the next step if necessary.

Only confirmed facts are reported and the results of tests are not false.

**Core principles: Fewer logs, less repetition tests and less context waste; protection of other sessions with priority given to the delivery of operationally operational functions.**

---

## 中文

# GearsVR — Agent 开发规范

## 1. 项目基本信息

- 项目：Gears of War Reloaded PCVR / OpenXR Mod。
- 当前主工作区：`J:/codexWorktree/gearsVr`
- 原项目及交付目录：`J:/gearsVr`
- 当前优先事项以用户最新指令和实际项目状态为准。
- 保留现有功能、源码、用户配置、存档、测试证据及可回退版本。
- 不因为旧文档中的历史失败或过期计划而重复开发。

## 2. 多 Agent / 多会话协作

项目可能有 20 个或更多 Codex 会话并行开发。

- 每个会话只负责自己的任务和明确授权的模块。
- 优先使用独立 Git Worktree、独立构建目录及测试输出。
- 修改共享文件前检查当前 Git 状态和其他会话的改动。
- 不覆盖、回退、删除其他会话的未提交修改。
- 不擅自修改其他武器、工作台或公共模块。
- 需要修改共享模块时，先明确修改范围，采用最小变更。
- 不为了释放资源关闭其他会话、游戏或工作进程。
- 不擅自创建子 Agent；确有必要时只委派范围明确的工作。
- 不重复已经完成且源码未变化的测试。
- 不将会话流水账、每轮测试记录或完整聊天历史追加到本文件。

## 3. 开发原则

- 优先修复真实问题，不做无明确收益的重构。
- 修改前先确认实际代码路径和故障证据。
- 尽量复用已有模块、读取器、测试器和运行工具。
- 保留正常的生命周期、权限和错误处理。
- 不为了通过测试而放宽输入有效期、身份验证、追踪或安全保护。
- 禁止伪造控制器输入、游戏状态、测试成功结果或时间戳。
- 所有结论必须区分：源码实现、编译成功、模拟器验证、实际游戏验证、实体头显验证。
- 编译成功不等于功能完成；一次测试通过不等于历史间歇故障已解决。
- 遇到失败先检查首个明确错误，不盲目重试整个流程。

## 4. VR Mod 核心约束

- 使用 OpenXR，保持真实双眼立体渲染及 6DoF 头部追踪。
- 保持唯一的最终相机/眼点基准与必要的骨骼写入所有权。
- 不引入第二个竞争的相机或骨骼写入者。
- 保留原始骨骼长度、蒙皮、物理输入和武器权限。
- 头部运动只改变观察方向，不自动改变枪口实际瞄准方向。
- 枪体、枪口、辅助握点和实际射击射线必须使用一致的变换关系。
- Hover Snap 只负责视觉吸附，不自动获得物理控制权。
- Active Grab 必须由真实有效的控制输入和当前接触条件授权。
- 物理推拉、弹匣装填、射击及电锯动作必须遵守真实游戏状态与库存规则。
- 保留追踪新鲜度、输入边沿、身份、生命周期和防重放检查。
- 不把模拟器姿态当作真实头显追踪或最终用户体验证明。

## 5. 游戏与模拟器测试

已授权在相关任务中进行受控的游戏、XR 和模拟器测试，无需每次重复询问。

- 优先复用已有 `isolated_sim.py`，不要另写重复启动平台。
- 不直接启动游戏 EXE，必须使用已有受控启动入口。
- 每个新测试实例使用全新且唯一的名称。
- 测试实例使用独立配置、存档和进程身份。
- 不抢占当前桌面焦点，不影响用户正在使用的软件。
- 当前可见测试模式使用 `--visible --lowest --render-scale 0.5`。
- 启动时按最新要求使用 `--wait-resources-seconds 0`。
- 保留 `-nomoviestartup` 和最长 180 秒玩家等待。
- 只有 `startup_status.startup_confirmed=true` 才能确认启动成功。
- 当前性能测试不录像，不启用额外视频采集。
- 无固定多开数量上限；不以人为设定的内存余量阈值拒绝启动。
- 仍检查磁盘空间、独立配置、进程身份和正常退出条件。
- 不为了新实例而关闭已有实例。
- 只清理自己启动且身份经过确认的进程及资源。
- 测试结束恢复临时修改的设置、输入、音频及其他用户配置。
- 用户最新的明确操作指令可以覆盖本节的测试参数。

## 6. 性能分析

当前性能目标：**实际游戏新双眼帧达到 120 Hz，单帧预算约 8.33 ms。**

- 严格区分游戏 Present、新双眼帧和 XR Runtime 循环频率。
- XR 90/120 Hz 不代表游戏实际渲染达到 90/120 FPS。
- 主 FPS 显示优先反映实际游戏新帧，不得用 XR 循环计数冒充。
- 先测量真实瓶颈，再决定优化方向。
- 分别调查游戏线程、Mod Tick、渲染、GPU、同步等待和数据读取成本。
- 无录像测试仍低帧率时，不得将录屏当作根因。
- 性能 A/B 对比必须保证条件可比；其他实例干扰时标注无效，不擅自关闭它们。
- 不把减少函数调用次数、日志数量或编译成功直接当作 FPS 提升。
- 保留真实测量数据、测试条件及失败结果。

## 7. Lancer 与武器工作

- Lancer 当前任务以最新真实用户反馈、工作台数据和有效测试证据为准。
- 保留物理换弹、弹匣吸附、前拉栓 E、电锯握把 C、双手握枪及第一人称动作。
- 重点关注性能、左手扳机闪烁、C 抓取位置、枪口自然方向及手部贴合。
- 用户已经确认的真实 Grab Pose、Volume、Snap 参数优先于旧自动生成姿态。
- 不擅自覆盖真实手部采集数据。
- 不因某次局部机械测试成功而声称整个 Lancer 已完成。
- 不修改用户当前可玩入口，除非当前任务明确要求交付或替换。
- 公共版本、隔离候选和历史回退保持清楚区分。

## 8. Quest 3 武器工作台

- 工作台源码主要位于 `tools/quest-workbench`。
- 保留独立武器、主握把、Second Hand、副手姿态、弹药和活动部件编辑机制。
- 使用当前用户确认的真实姿态与最新设备数据，不恢复被用户否定的历史姿态。
- 备份原始采集文件、项目配置及现有 APK。
- 当前授权允许 Quest 3 在线且 USB 已授权时，完成新版构建验证和备份后覆盖安装。
- 不因为更新工作台而擅自修改 PCVR Mod、其他设备或游戏。
- 工作台模拟交互、采集成功、APK 安装成功与真实游戏手感验收分别记录。

## 9. 逆向工程

- 优先使用现有 IDA、Ghidra、知识库及已保存的分析证据。
- 新增逆向前先查询 `analysis/re-kb/knowledge.sqlite`。
- 区分磁盘 EXE、运行内存、历史快照和 UnrealScript 元数据。
- 代码地址相同不代表原始字节相同，必须核对版本与身份。
- 不让多个 Agent 同时写入同一 IDA 数据库。
- 原始游戏资源、反编译结果和私有分析内容不得混入公开发行包。
- 静态分析不能代替真实运行或 GPU/头显验证。

## 10. 文件和历史记录

- `AGENTS.md` 只保留长期有效的项目规则。
- `docs/` 保存架构、当前状态、操作说明及开发计划。
- `analysis/` 保存日志、诊断数据、历史测试、SHA、运行回执和实验结果。
- 各会话可以在自己的 `analysis/` 子目录维护独立进度。
- 禁止将完整测试流水账不断追加到 `AGENTS.md`。
- 历史记录按需读取，不应在每次任务启动时全部加载。
- 已知旧失败保留在历史证据里，但不自动成为新任务的阻塞条件。
- 项目生成的测试媒体遵循现有清理策略；保留源码、配置、存档、文本证据和回退版本。
- 不清理用户个人文件、其他会话数据或 Codex 会话数据库。

## 11. 工作汇报

每次任务完成或遇到真实阻碍时，简要说明：

1. 做了什么。
2. 修改了哪些文件。
3. 哪些编译或测试真正通过。
4. 哪些问题仍然存在。
5. 必要时给出下一步操作。

只报告已经确认的事实，不虚构测试结果。

**核心原则：少写日志、少重复测试、减少上下文浪费；保护其他会话，优先交付实际可运行的功能。**
