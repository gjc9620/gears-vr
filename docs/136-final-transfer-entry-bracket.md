# Bounded final-transfer entry-to-draw observation

## VRDof64 identity and behavior

Source revision `adcbffb` adds an observation-only scope to the existing quad hook, plus cheap counters in existing Draw/DrawIndexed hooks. No hook target, source sampling argument, admission condition, transfer mapping or publication rule changed. Preflight TLS/COM work is capped at 64 invocations; retained rows have separate quotas of four incomplete and twelve complete entries. The diagnostic runs only with the existing final-transfer, outage and recording gates. After that cap, no further diagnostic preflight resource queries occur.

Rows preserve nested TLS ownership and report normal/exceptional scope exit. Existing Draw-hook calls are counted separately from calls reaching transfer observation, including bypass calls separately. “Exactly one routed draw” concerns those existing direct Draw hooks, not all possible GPU work. Complete entry means the bounded snapshot fields and local pair are available; it does not grant a rendering certificate.

Focused MSVC CPU tests covered comparison differences, attempt/row quotas, missing routing, duplicate draws, mismatched tuples and bypass. Existing transfer policy tests passed. Frozen normal-profile DLL `tools/native-capture/build-vrdof64/GearsCaptureVrdof64.dll` SHA256 `320f885a42decbe95238d9e49b1a81e09b85a71244e1b407a11e726354f0fc53`; source/options/vendor receipt is ignored `analysis/runtime/vrdof64-build-source.json`. Root default and release packages were not promoted.

## Actual PID37760 result

Session `continuous-stereo-37760-20261017-190838-46932807`, using quiet2 simulator and background auto-continue. The 22 same-invocation startup tests passed and native activation completed. The guarded monitor sampled for 30 seconds; publication remained zero, so no GPU snapshot was requested and display acceptance failed.

The collector attempted exactly 64 preflights, retained 14 rows (two incomplete, twelve complete) and stopped extra preflight reads. All twelve complete rows had one routed indexed draw (six indices, one instance), normal scope exit, no bypass, and difference mask **12 = target + rectangles**. The command list/canonical generation, native context, source object/resource/descriptor, source pair/ticket, world and inventory matched entry to actual Draw.

Entry D3D tracked target was still the 2560x1080 source texture, while actual Draw targeted a known 2534x1009 backbuffer. Viewport changed accordingly inside the original quad call. Therefore an entry check based on already-bound D3D OM/viewport state is insufficient for a normalization decision: the engine commits pending state inside that function. This positive CPU observation is not GPU pixel or headset proof.

Native cleanup, entry restoration, XR object destruction, ordinary owned-game exit and four-profile restore were verified. Raw evidence remains ignored: session files, `vrdof113-{audit,bracket-summary,normal-close,save-restored}.json`.

## Next bounded native question

Before changing final sampling, identify the native **pending** destination/viewport and compare it with the actual Draw target. Existing cached historical artifact1 contains quad RVA1af42c and its calls to 1ac950 and 177e90 before DrawIndexed return address1af92b. The knowledge base was searched before reading these saved functions. These are historical runtime derivatives (source blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, base0x140000000), not current byte-attestation merely because the RVA matches.

Do not normalize based on the previous tracked source RTV, force a standard resolution as the release fix, weaken the full-eye mapping checks, or assume a transient intermediate is safe to copy. Pending-state runtime evidence, full code/ABI attestation, representative source/destination pixels, UI coverage and headset acceptance remain separate requirements.

---

## 中文

# 最终移交进入-拖网观察

## VRDof64 身份和行为

源码订正 `adcbffb` 在现有四钩上添加一个只观察范围, 加上现有 Draw/ DrawIndexed 钩上便宜的计数器。 没有更改钩目标、源采样参数、 准入条件、 传输映射或发布规则。 Preflight TLS/ COM 工作上限为 64 引用;保留行有四个不完整和十二个完整条目的单独配额。诊断只使用现有的最终转移、断流和记录门。在上限之后,不再进行诊断飞行前资源查询。

行保存嵌入式 TLS 所有权并报告正常/例外范围退出。现有的 Draw- hook 调用与达到传输观察的调用分开计算,包括单独调用。“精确的说,一个路由抽用”涉及那些现有的直接 D 原始钩, 并非所有可能的 GPU 工作。 完整条目意味着有边框的快照字段和本地对; 它不授予渲染证书 。

焦点 MSVC CPU 测试覆盖了比较差异,尝试/ row 配额, 缺少路径, 重复绘图, 错配拖曳和绕行。 现有的传输政策测试通过。 冻结了正常的 DLL 。 `tools/native-capture/build-vrdof64/GearsCaptureVrdof64.dll` SHA256 `320f885a42decbe95238d9e49b1a81e09b85a71244e1b407a11e726354f0fc53`; 源/选项/供应商收据被忽略 `analysis/runtime/vrdof64-build-source.json`。根默认和发布包没有被推广。

## 实际的 PID37760 result

Session `continuous-stereo-37760-20261017-190838-46932807`,使用静音2 模拟器和背景自动连续。 22 同一启动测试通过,本地激活完成。 30 秒; 发布时保持0, 因此没有请求 GPU 快照, 显示接受失败 。

收藏家试图 64 飞行前,保留 14 排行(两条不完整,十二条完整)并停止额外飞行前读取。所有十二条完整行都有一条路由索引绘图(六个索引,一个实例),正常范围退出,没有绕行和差异遮罩 **12 =目标+矩形**。命令列表/名称生成、本地上下文、源对象/资源/描述符、源对/列表、世界和目录与实际 Draw 匹配。

条目D3 D追踪目标仍然是 2560x1080 源质素, 而实际 Draw 瞄准已知 2534x1009 。视图端口在原四位调用内相应更改。因此,根据已输入的 D 进行条目检查3 D OM/ Viewport 状态不足以决定正常化: 引擎会在此函数内执行待处理状态。 这种正的 CPU 观察不是 GPU 像素或耳机 证明 。

原生清除、条目恢复、 XR 对象销毁、普通自有游戏退出和四大显示恢复等已被验证。原始证据仍然被忽略:会话文件, `vrdof113-{audit,bracket-summary,normal-close,save-restored}.json`.

## 下一个本地问题

在改变最后取样之前,先确定原生样本 **pending** 目的地/视图端口,并将其与实际 Draw 目标进行比较。现有缓存历史文物1 包含四维RVA1af42(c) 要求 1ac950 and 177e90 在绘制已删除的返回地址之前1af92 b. 在读取这些保存的函数之前, 已搜索了知识库。 这些函数是历史运行时间衍生工具( source blob SHA)256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`基数0x140000000),而不是仅仅因为RVA匹配而进行当前字节测试.

不根据先前的履带源RTV实现正常化,强制标准解析作为发布固定,削弱全眼映射检查,或假设瞬间中间体是安全的复制. 等待状态运行证据,完整代码/ABI验证,重写 内源/目的地像素,UI覆盖和耳机接受仍然是单独的要求.
