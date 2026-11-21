# OpenXR Independent Session Authentication

2026-10-11, the default registration form is running as
`D:\KRVR Native\krvr_xr2\krvr_xr2.runtime.json`。
No changes to the registration entry, no switch to VDXR for Virtual Desktop.

Interface query successfully: Run-time `KRVR XR Runtime 2.0.0`,system name
`KRVR Apple Vision Pro (remote)`. Supporting D 3D 11、D 3D 12 , report rotation and position tracking support.
I recommend every eye. 4096×3288，sampleCount=1. The recommended size does not equal the current actual image or connection status.

12 sec D 3D 12 session bit test:1078 Frames;1078 The frame VALID / TRACKED is complete.
1078 The frame should be Render,1077 Frame FOCUSED. Session appointment 90 Hz, but head position is constant (0,0,0)，
Four constants (%)0,0,0,1) Only one head position. Left and right eye X is  ⁇0.0355000012 Rice.
Runtime log report headset history=0Hz、pace feedback recv=0/s。

Therefore: the session and API are clear; no remote headline connection has been confirmed, nor is there any physical movement in real time.
The default position cannot be considered a successful tracker simply by the VALID/TRACKED logo.

5 Second independent video test:447 Frames, successful submission 447 Two eyes,
Every eye image 512×512. GPU first readback validation:

| Eyes. | Background RGB | Cross RGB | Cross pixels | FNV 64 |
| --- | --- | --- | --- | --- |
| Left | (64,5,5) | (204,204,204) | 9728 | 18273855778751525669 |
| Right | (5,5,64) | (204,204,204) | 9728 | 2312199282545496869 |

I've got eyes. 252416 Background pixels, size and content are consistent with the test map design.
Normal exit from consumption STOPPING and EndSsession; no game injection.
The headline is not observed by the user and cannot be claimed to have been normal.

Bitmathematic tests for bit conversion are obtained by: Right/ Top/ Front-direction of OpenXR, symbol of pitch/yaw/roll,
Mi to preset game units, non-zero attitude back,71 mm eye,10000 Group random rotation matrix round-trip and
Four-dollar q/-q equivalent, zero four-dollar and NAN rejected.
The mathematical engagement here has yet to be validated with the real game prototype matrix and real head start.

Post-input check and test for subsequent addition: default not to apply header input, visible enable post-check
100 ms freshness, complete tracking signs, focus, sequence and limited values; when lost, return to the clean spot of the game.
Restores again. Test passed; no game or transfer layer is yet connected.

Original logs, CSV, GPU return images ignored in Git `analysis/openxr/`。
See you at the tool. `tools/openxr-probe/README.md`。

Quest 3 / VDXR physical header and header display verified. See
`docs/11-headset-connection-validation.md`. Default postures and unobserved displays of the page
Limit description to an earlier KRVR experiment; cannot be inferred from this 3 Follow-up tests failed.

---

## 中文

# OpenXR 独立会话验证

2026-10-11，本机默认注册表运行时为
`D:\KRVR Native\krvr_xr2\krvr_xr2.runtime.json`。
未修改该注册表项，未切换到 Virtual Desktop 的 VDXR。

接口查询成功：运行时 `KRVR XR Runtime 2.0.0`，system 名称
`KRVR Apple Vision Pro (remote)`。支持 D3D11、D3D12 等，报告旋转和位置跟踪支持。
推荐每眼 4096×3288，sampleCount=1。推荐大小不等于当前实际图像或连接状况。

12 秒 D3D12 session 位姿测试：1078 帧；1078 帧 VALID / TRACKED 位均完整，
1078 帧 shouldRender，1077 帧 FOCUSED。会话约 90Hz，但头部位置恒为 (0,0,0)，
四元数恒为 (0,0,0,1)，仅一个唯一头部位姿。左右眼 X 为 ±0.0355000012 米。
运行时日志报告 headset cadence=0Hz、pace feedback recv=0/s。

因此：会话和 API 已通；没有证实远程头显连接，也没有证实实时物理头动。
不能仅凭 VALID/TRACKED 标志就将默认位姿当成成功跟踪。

5 秒独立画面测试：447 帧，成功提交 447 个双眼 projection layer，
每眼图像 512×512。GPU 第一次回读验证：

| 眼睛 | 背景 RGB | 十字 RGB | 十字像素数 | FNV64 |
| --- | --- | --- | --- | --- |
| 左 | (64,5,5) | (204,204,204) | 9728 | 18273855778751525669 |
| 右 | (5,5,64) | (204,204,204) | 9728 | 2312199282545496869 |

两眼各有 252416 个背景像素，尺寸和内容均符合测试图设计。
正常退出已消费 STOPPING 并 EndSession；没有游戏注入。
未由用户在头显里观察，不能声称头显端显示正常。

位姿转换数学测试通过：OpenXR 的右/上/前方向转换、pitch/yaw/roll 的符号、
米到预设游戏单位、非零姿态回正、71mm 眼距、10000 组随机旋转矩阵往返及
四元数 q/-q 等价、零四元数与 NaN 拒绝。
这里的数学约定尚需与实际游戏原生矩阵和真实头动共同验证。

后续增加位姿有效性检查及测试：默认不应用头显输入，显式启用后检查
100ms 新鲜度、完整跟踪标志、焦点、序列和有限数值；丢失后返回游戏干净视点，
恢复时重新回正。测试通过；尚未接入游戏或传输层。

原始日志、CSV、GPU 回读图像均在 Git 忽略的 `analysis/openxr/`。
工具见 `tools/openxr-probe/README.md`。

后续 Quest 3 / VDXR 实际头动与头显端显示已经验证，见
`docs/11-headset-connection-validation.md`。本页前述默认姿态和未观察显示的
限制描述的是较早的 KRVR 实验；不能据此推断 Quest 3 的后续测试也失败。
