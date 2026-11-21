# Microdiggle verification of six-axis primary view point

2026-10-11 Single-person first-class prison clearance, static character, PID 39988。
To the original, SceneView. `RVA 0x15cf08` Internal, Caller `0x15d0ca` The viewer readings are subject to temporary deviation.
Change only the temporary position/rotation of the call; no person location or CameraCache.

## Experiments and results

45 Seconds of collection:3 Second baseline, six axes. 4 Second sine microturbage, then recover.
Rotate ±273 Rotator units (approximately  ⁇ )1.5°,Classed  ⁇2 Game units.
Front/right, defined by clean shot yaw, top world Z.

| Phase | Scene Record | Non-zero offset records | Different View Matrix |
| --- | ---: | ---: | ---: |
| Baseline | 821 | 0 | 1 |
| Pitch | 1126 | 1124 | 512 |
| Yaw | 1128 | 1126 | 508 |
| Roll | 1085 | 1084 | 514 |
| Back and forth. | 1125 | 1125 | 1119 |
| Left and right | 1114 | 1114 | 1114 |
| Up and down. | 1136 | 1136 | 1129 |
| Restore | 4972 | 0 | 1 |

All 12507 The matrix of a sample of scenarios is limited; the projection matrix remains the same.
The view matrix for the recovery phase is fully aligned to the baseline. Output - source values are the largest deviation between login and logout
As agreed 0.00003092 Game units, matching the float plus minus and CSV output accuracy.
Source rotation always (0,16404,0) Source location is only one recorded Z has an approximate 0.000031 The floating point is bad.
Independently read and observed. 319 The following is a sample of the CameraCache samples:
The observation is being launched late. It can't be considered as a whole. 27 Full cache evidence during a second microturbation.

Event Packing Functions Together 62542 (a) A record that only the target caller accepts deviation;
No deviations when the focus is lost. 12509 Article, failure to report HRESULT.
Logs are abandoned 6 Article, therefore not without damage.
Organisation `disable_status=0 active=0`, the test state is complete.
No additional remote bytes reading after this round has ceased; the status log cannot be equated with an additional independent byte verification.
The game process is out while working; no automatic re-starting of the game for this purpose.

## The distribution problem.

The original MinHook trackline search is near the target. 1 GiB。
The current process has a low address with more space debris and recently has been aligned to the empty space view entrance 1155 MiB，
Cause `MH_ERROR_MEMORY_ALLOC`, two failed DLLs didn't enable Hook.

Generate a copy of the buffer.c to retain the licence while constructing, and move the search range to 1.5 GiB，
Do not modify the original source code for third parties or release the game memory. Try again the last four Hooks return MH_OK.
Each load still checks the version, player object and original function entry bytes and refuses to collect them in parallel.
DLL and trampoline stay after stop to the game exit, without automatically unloading.

## How?

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\native-capture\analyze_sweep.py J:\gearsVr\analysis\runtime\sweep-39988.csv --output J:\gearsVr\analysis\runtime\sweep-axis-validation.json
```

Eight checks passed: full phase, caller/focus limit, matrix limited, offset algorithm, six axes each changing view,
The projection is unchanged, baseline/recovery is not included and recovery is consistent with the baseline.
Original data ignored in Git analysis/runtime; report and analysis process submitted to Git.

This proves that the primary view point of the current level accepts three axes rotation and three axis plane transfer.
No axle-by-axis observations and no real head start entry; no proof of bi-eye stereo, first person position,
Scale, aim or bunker/extra-scenes behaviour, not considered complete 6DoF VR Mod。

---

## 中文

# 六轴原生视点微扰验证

2026-10-11，单人第一章监狱关卡、人物静止，PID 39988。
对原生 SceneView `RVA 0x15cf08` 内、调用者 `0x15d0ca` 的视点读取施加临时偏移。
只改这次调用返回的临时位置/旋转；不写人物位置或 CameraCache。

## 实验与结果

45 秒采集：3 秒基线，六轴各 4 秒正弦微扰，之后恢复。
旋转 ±273 Rotator 单位（约 ±1.5°），平移 ±2 游戏单位。
前/右方向按干净镜头 yaw 定义，上方向为世界 Z。

| 阶段 | 场景记录 | 非零偏移记录 | 不同视图矩阵 |
| --- | ---: | ---: | ---: |
| 基线 | 821 | 0 | 1 |
| Pitch | 1126 | 1124 | 512 |
| Yaw | 1128 | 1126 | 508 |
| Roll | 1085 | 1084 | 514 |
| 前后 | 1125 | 1125 | 1119 |
| 左右 | 1114 | 1114 | 1114 |
| 上下 | 1136 | 1136 | 1129 |
| 恢复 | 4972 | 0 | 1 |

所有 12507 个场景样本的矩阵有限；投影矩阵始终相同。
恢复阶段的视图矩阵与基线完全一致。输出−源值与记录偏移的最大差
为约 0.00003092 游戏单位，符合 float 加减及 CSV 输出精度。
源旋转始终 (0,16404,0)，源位置仅一条记录的 Z 有约 0.000031 的浮点差。
独立只读观察的 319 个 CameraCache 样本位置、旋转、FOV 各只有一个值；
该观察启动较晚，不能当成整个 27 秒微扰期间的完整缓存证据。

事件包装函数共 62542 条记录，仅目标调用者接受偏移；
没有失去焦点时的偏移。Present 12509 条，未报告失败 HRESULT。
日志丢弃 6 条，因此不是无损追踪。
停止状态 `disable_status=0 active=0`，测试状态为完成。
本轮停止后未另做远程入口字节读取；不能把状态日志等同于额外的独立字节核验。
继续工作时游戏进程已退出；没有为此自动重启游戏。

## 本次遇到的分配问题

原版 MinHook 的 trampoline 搜索范围是目标附近 1 GiB。
当前进程低地址空间碎片较多，最近对齐空闲页距场景入口约 1155 MiB，
造成 `MH_ERROR_MEMORY_ALLOC`，两个失败 DLL 没有启用 Hook。

构建时生成保留许可证的 buffer.c 副本，将搜索范围调到 1.5 GiB，
不修改第三方原始源码，也不释放游戏内存。重试后四个 Hook 均返回 MH_OK。
每次加载仍核对版本、玩家对象和原函数入口字节，并拒绝并行采集。
停止后的 DLL 与 trampoline 留驻到游戏退出，不自动卸载。

## 如何复核

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\native-capture\analyze_sweep.py J:\gearsVr\analysis\runtime\sweep-39988.csv --output J:\gearsVr\analysis\runtime\sweep-axis-validation.json
```

八项检查通过：全部阶段、调用者/焦点限制、矩阵有限、偏移算术、六轴各自改变视图、
投影不变、基线/恢复不写入、恢复与基线一致。
原始数据在 Git 忽略的 analysis/runtime；报告和分析程序提交 Git。

这证明当前关卡的原生视点接受三轴旋转和三轴平移。
没有逐轴由用户观察，也没有真实头显输入；尚未证明双目立体、第一人称位置、
尺度、瞄准或掩体/过场镜头行为，不能称为完成 6DoF VR Mod。
