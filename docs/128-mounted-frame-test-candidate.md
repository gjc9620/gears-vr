# Mounted-frame retry test candidate

2026-10-17. Offline staging only; no game, simulator or launcher GUI was started and no foreground/runtime setting was changed.

The new independent developer package includes the outer mounted-camera frame-change retry from `d803216`, which the preceding MountedPrep ZIP did not contain. A changed frame is discarded and retried within the existing acquisition bounds; role/link/schema changes still reject. This is a candidate for the reported mount-black/dismount-recover case, not proof of its cause or resolution.

- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof61-MountedFrame-English-20261017.zip`
- SHA256: `5880480df02f13f94ae2d6f9267716dd42f2b87357b8f273ef792f95a43a364a`
- Size: 13,647,810 bytes; 701 archive members.
- Python and launcher payloads: source checkout `c39e090`; launcher defaults English, game selection is empty, personal settings are not imported.
- Native: unchanged VRDof61, frozen source `88cfae6`, SHA256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`.
- XR: unchanged Borderless1, SHA256 `2c27d29ca68a1629cb987addeedbc49ea59f2f7355408802a4b84ca7d96fe5eb`.

Staged and freshly extracted bundled Python isolated preflights both passed. Every archive payload digest and inventory digest matched. All preceding ZIP digests remained unchanged. Full receipt and packaging script are under ignored `analysis/runtime/vrdof61-mountedframe-english-20261017-package-receipt.json` and `analysis/runtime/package-mountedframe-candidate.py`.

The local default native manifest was not replaced. The new C++ shared-group placement helper is not in the frozen DLL, and PathChoice placement is not enabled. This candidate remains `runtime_verified=false`, `headset_verified=false`, `release_ready=false`.

## Next coordinated actual test

Use a fresh launch with this candidate's matching Python/native combination; first observe normal gameplay, then mount and dismount the problematic machine gun. Preserve the session outputs to distinguish registry change, timestamp retry, late acquisition, native rendering and XR submission failures. Extend to checkpoint/CG and headset pacing after mounted acquisition is understood. The user currently uses the computer; no test launch is authorized until they explicitly indicate readiness.

## Existing pacing evidence boundary

The saved session `continuous-stereo-24696-20261017-150718-1be1dbc1` retains only the first 4096 selected diagnostic observations. A bounded initial projection window contains 364 successful new-image observations, with median adjacent interval 33.3292 ms and p95 41.68408 ms. The observations occur after `xrEndFrame`; they are neither physical display timing nor whole-session coverage. They do not establish a VD network cause. Exact source digest and window are preserved in `analysis/runtime/user24696-render-cadence-audit.json`.

---

## 中文

# 挂载的机身重试候选

2026-10-17。离线中转;没有启动游戏、模拟器或发射器图形界面,也没有更改前景/运行时间设置。

新的独立开发者软件包包括从外挂式相机帧修改重试 `d803216`,而上一个挂载的Prep ZIP中并不包含。一个已更改的框架被丢弃,并在现有的获取界限内重新测试;角色/link/schema 更改仍然拒绝。这是所报告的挂载/卸载回收案件的候选条件,而不是证明 其事业或决议。

- ZIP: `analysis/release-zips/GearsVR-V1-development-Vrdof61-MountedFrame-English-20261017.zip`
- SHA256: `5880480df02f13f94ae2d6f9267716dd42f2b87357b8f273ef792f95a43a364a`
- Size: 13,647,810 bytes; 701 存档成员。
- Python和发射装置有效载荷:源检出 `c39e090`; 启动器默认英语,游戏选择为空,个人设置不导入.
- 原生: 不变 VRDof61,冻结源 `88cfae6`, 莎莎 (英语).256 `d7f41e7248b8ac957545c0bddde1eb857bfa9a65220f6dc4e09e0685ceb873e5`.
- XR: 无边形不变1, 莎莎 (英语).256 `2c27d29ca68a1629cb987addeedbc49ea59f2f7355408802a4b84ca7d96fe5eb`.

分阶段和新提取的捆绑的 Python 隔离飞行前飞行都经过, 每一个归档的有效载荷摘要和清单摘要都匹配。 ZIP 摘要之前的所有摘要都保持不变。 完全接收和包装脚本被忽略 。 `analysis/runtime/vrdof61-mountedframe-english-20261017-package-receipt.json` and `analysis/runtime/package-mountedframe-candidate.py`.

本地默认本地运算表没有被替换。 新的 C++ 共享组安放帮助器不在冻结的 DLL 中, 路径选择安放功能也没有启用。 此候选人仍然保留 。 `runtime_verified=false`, `headset_verified=false`, `release_ready=false`.

## 下一次协调实际测试

使用新推出的 Python/ 本地组合; 首先观察普通游戏, 然后挂载和卸载有问题的机枪。 保存会话输出以区分注册更改、 时间戳重试、 晚获取、 nat ive 渲染和 XR 提交失败。 安装获取后扩展至检查点/ CG 和头盔间距。 用户目前使用计算机; 除非明确表明准备状态,否则不允许进行测试。

## 现有间距证据边界

保存的会话 `continuous-stereo-24696-20261017-150718-1be1dbc1` 只保留第一个 4096 选中的诊断性观测。 初始投影窗口包含 364 成功的新图像观测, 中间相邻间隔 33.3292 毫秒和p95 41.68408 ms. 观测结果在 `xrEndFrame`; 它们既不是物理显示时间,也不是全场覆盖。它们没有建立 VD 网络原因。精确源摘要和窗口保存在 `analysis/runtime/user24696-render-cadence-audit.json`.
