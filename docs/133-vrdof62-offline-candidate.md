# VRDof62 consolidated offline candidate

2026-10-17. This preserves a normal-profile development candidate for the next actual regression. No game, simulator, GUI, focus, controller or XR Runtime action was performed.

## Contents and configuration

Native source is frozen at `bdb23b2`; the supported VRDof61 feature options are retained. The root-scene observer and inline call-boundary observer are OFF. The candidate includes the UI publication-bypass fixes, current Python mounted-camera acquisition changes, English launcher default and optional owner/layout tools. PathChoice placement is not enabled. Game resolution remains authoritative; DLSS/DLAA remain disabled and 6DoF is retained.

- DLL: `tools/native-capture/build-vrdof62/GearsCaptureVrdof62.dll`
- DLL SHA256: `ab3a2cf54c82b72de4080c69cda13bcd1405081311a366fa414bc2289a35c438`
- Explicit development manifest: ignored `analysis/runtime/manifest-vrdof62.json`
- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof62-ReplayGuard-English-20261017.zip`
- ZIP SHA256: `b6194f104692736d976cfcb8e80a09a8da1ad8fdc112cf9a6c5d1fe6c3734530`
- ZIP size: 13,667,906 bytes, 702 members.

Local default manifest/config and every existing ZIP were digest-checked and remain unchanged. Portable defaults are English, 100 game units/metre and an empty game path requiring selection. Personal settings are not imported. Native/XR artifacts and every payload digest were checked after fresh extraction. No game, simulator, knowledge database, pseudocode or snapshots are bundled.

## Verification scope

The complete candidate DLL and all ten required native test executables were rebuilt, together with five focused UI CPU tests. The offline runner completed 18 checks with expected results: 17 successful ABI/hook/CPU test cases and an explicit `ERROR_NOT_SUPPORTED=50` assertion for the disabled root-observer check. That last case proves feature exclusion, not a passed root runtime test. No actual-game GPU/frame-link self-test was invoked.

Staged and freshly extracted bundled-Python isolated preflights both passed. Full source/vendor hashes, build log, per-case outcomes, package inventory and protected previous/default hashes are under ignored `analysis/runtime/vrdof62-*`. The standard same-invocation startup suite still runs on a future actual launch; this offline result does not replace it.

`runtime_verified=false`, `headset_verified=false`, `release_ready=false`. Mounted-camera black screens, reticle recoil/visible center, PathChoice placement and sustained headset pacing still require actual evidence.

## Reticle decision

The existing evidence in docs 83, 93, 96 and 98 was reused before deciding whether another offset change was justified. Cross-payload grouping remains a static coverage gap, but historical ring/icon pairs already had identical actual viewport/scissor maps, and an earlier first-quad provenance hole was independently repaired. Those records do not cover the latest user recoil observation. No new guessed reticle offset or broader kind-1 merge is included. Any future shared group must use the authenticated inline child/body boundary, complete member ranges, compatible transforms and actual ticket/target association; a PostRender parent token alone is insufficient.

---

## 中文

# VRDof62 合并离线候选人

2026-10-17。这为下一个实际回归保留了正常的可显示开发选项。没有执行游戏、模拟器、图形界面、焦点、控制器或 XR 运行时动作。

## 内容和配置

原生源被冻结在 `bdb23b2`; 支持的 VRDof61 特性选项被保留。 root- scene 观察器和内置的呼叫边界观察器被关闭。 候选设备包括 UI 出版- bypass 修正、 当前 Python 挂载的相机获取更改、 英文启动器默认和可选所有者/ layout 工具 s. PathChoice 定位无法启用。 游戏解析度仍然为权威; DLSS/ DLAA 仍然禁用, 6 财政部保留。

- DLL: `tools/native-capture/build-vrdof62/GearsCaptureVrdof62.dll`
- 沙发256: `ab3a2cf54c82b72de4080c69cda13bcd1405081311a366fa414bc2289a35c438`
- 明确发展显示:被忽视 `analysis/runtime/manifest-vrdof62.json`
- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof62-ReplayGuard-English-20261017.zip`
- 兹普沙256: `b6194f104692736d976cfcb8e80a09a8da1ad8fdc112cf9a6c5d1fe6c3734530`
- ZIP 大小 : 13,667,906 bytes, 702 members.

本地默认列表/ 配置和每个现有的 ZIP 都经过了文摘检查, 保持不变。 可移植默认为英语 。 100 游戏单位/ 米和需要选择的空游戏路径。 个人设置不导入。 原始/ XR 文物和每个有效载荷摘要在新提取后都经过检查。 没有游戏、 模拟器、 知识数据库、 伪代码或快照被捆绑 。

## 核查范围

完整的候选 DLL 和所有十项所需的本地测试可执行文件被重建,同时还有五项集中的UI CPU 测试。离线跑车完成 18 与预期成果的核对: 17 ABI/hook/CPU测试成功案例和明确 `ERROR_NOT_SUPPORTED=50` 设置已禁用的根- 观察器检查。最后一个实例证明特性排除,而不是通过根运行时间测试。没有引用实际游戏 GPU/frame-link 自我测试。

已分阶段和新提取的捆绑- Python 隔离飞行都通过了。 完整源/ vendor 散列、 建立日志、 个案结果、 包盘和保护的前/ 默认散列均被忽略 `analysis/runtime/vrdof62-*`. 标准的同调启动套件仍然运行在将来的实际发射上;这种离线结果不能取代它。

`runtime_verified=false`, `headset_verified=false`, `release_ready=false`搭载的摄像机黑屏、回旋后坐力/可见中心、PathChoice放置和持续耳机的间隔仍然需要实际证据。

## 撤销决定

文献中的现有证据 83, 93, 96 and 98 在决定是否有理由进行其他抵消之前,重新使用。 交叉负载组仍是一个静态覆盖缺口,但历史环/虹对已拥有相同的实际视图/剪切图,较早的第一方位出处孔已安装 已修复。这些记录不包括最新的用户后座力观察。没有新的猜测的后座力抵消或更广泛的类型 。1 包含合并。 任何未来的共享组必须使用已认证的内置子/ 体边界、 完整的成员范围、 兼容的转换和实际的票/ 目标关联; 单是后渲染父标记是不够的 。
