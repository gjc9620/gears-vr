# Mounted outer camera cross-frame refused to classify

2026-10-17 Continue offline checking of the discovery, configuration and primary takeover chain after the machine gun has been mounted; no game or real appearance has yet been performed and the actual black screen root cause of the machine gun has not been identified.

`LiveRegistryAnchorSampler._strict_guard` Full mounted command has been obtained before and after normal Guards. Always throw ordinary when the full dictionary was not equal `AnchorRejected`: Even if two Captures are identical in real time, and the camera time stamp is normally entered into the next frame, the existing temporary re-sampling budget cannot be used. This is a different layer than the single Mounted Capture internal time stamp classification that was previously amended.

Now. `_check_mounted_bracket` Still require full equivalence to accept. If not, this failure will be marked only if the complete set is consistent, all non-camera fields are consistent and the time stamp is indeed different `MountedTimestampChanged`; this result is discarded and read again after. The POV is still hard-rejected at the same time, regardless of identity/role/paternity/reflective field/link/Level/mark or keyset changes. No increase or extension of the original five times/200 ms cap, no old success certificate for cache.

Coordinates use the same series in the package Python 3.12 Implementation:

```text
python -m unittest test_mounted_context test_anchor_config test_cinematic_anchor test_world_rebind test_world_rebind_monitor test_world_rebind_runtime
```

116 The item is passed. New real fixture uses the example of a camera frame between complete outer layers: one push must be dropped at first result, and the second stabilization result will pass; each advance reaches the original test limit; the POV change with the same time stamp and static changes accompanying the frame cannot be converted to a temporary retest. The verification range is a synthetic memory and controller test, not an actual cannon camera or GPU/head recognition.

Local Python source will use this change next time start; freeze Native 60／61 DLL is not replaced. `GearsVR-V1-development-Vrdof61-MountedPrep-English-20261017.zip` Keep the photo.**Do not include this outer category change**, do not treat the result of the package as receipt and inspection for this submission.

---

## 中文

# Mounted 外层相机跨帧拒绝分类

2026-10-17。继续离线核对上机枪后的发现、配置生成和原生接管链；尚未运行游戏或真头显，也未确认实际机枪黑屏根因。

`LiveRegistryAnchorSampler._strict_guard` 已在普通 guard 前后各取得完整 mounted capture。此前完整字典不相等时一律抛出普通 `AnchorRejected`：即使两次 capture 分别通过所有实时验证、角色／字段完全相同，而相机时间戳与 POV 正常进入下一帧，也不能使用现有暂态重采样预算。这与此前修正的单次 mounted capture 内时间戳分类是不同的外层缺口。

现在 `_check_mounted_bracket` 仍要求完整相等才接受。若不相等，仅在完整键集一致、所有非相机字段一致、时间戳确实不同的情况下，将这次失败标为 `MountedTimestampChanged`；本次结果照样丢弃，后续重新读取。相同时间戳下 POV 不同、任意身份／角色／父类／反射字段／链接／Level／标志或键集变化，仍硬拒绝。没有增加次数或延长原五次／200ms 上限，没有缓存旧成功证明。

协调者使用包内同系列 Python 3.12 执行：

```text
python -m unittest test_mounted_context test_anchor_config test_cinematic_anchor test_world_rebind test_world_rebind_monitor test_world_rebind_runtime
```

116 项通过。新真实 fixture 用例在外层完整 capture 之间推进相机帧：一次推进必须丢弃第一结果，第二次稳定结果才通过；每次推进则达到原尝试限制后拒绝。相同时间戳的 POV 变化及伴随跨帧的静态变化不能变成暂态重试。验证范围是合成内存与控制器测试，不是实际炮台相机或 GPU／头显验证。

本地 Python 源码下次启动会使用此修改；冻结 Native60／61 DLL 均未替换。此前 `GearsVR-V1-development-Vrdof61-MountedPrep-English-20261017.zip` 保持原快照，**不包含本次外层分类修改**，不要把该包的结果当作此提交的验收。待下一次可协调实际测试前统一生成包含后续修正的候选，避免每个小改动重复打包。
