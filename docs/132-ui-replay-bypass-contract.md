# UI consumer isolation during internal publication

2026-10-17. Offline source and dedicated loader-process verification only. No game, simulator, headset, GUI, runtime switch or controller input was used.

`StereoPublishBypass` tells the D3D hooks to forward internal publisher/mirror work without updating game-frame observation. Native UI consumers must preserve that same contract if entered within such a scope. Previously a simple Execute could consume a pending payload receipt and clear parent TLS while its indexed-draw callbacks were bypassed; material Execute/upload had analogous state changes. The optional root observer could also consume sample slots or increment a parent row's counters.

Simple Execute, material Execute, material upload and the optional scene-root hook now forward directly before Scope admission, TLS replacement, receipt claims, completion bookkeeping or diagnostics. Execute callbacks are counted by the root observer only after the bypass check. Original arguments, call count, return bits and exceptions remain forwarded.

This fixes a conditional hook contract. The reviewed publisher/mirror paths call D3D Reset/barriers/copy/ExecuteCommandLists and, for mirroring, DrawInstanced. No explicit source edge from those paths into native engine UI Execute/upload/root rendering was established. Actual occurrence and any relation to the machine-gun black screen remain unproven.

## Dedicated actual-hook tests

The DLL loader accepts these modes; each runs in its own process:

- `--ui-bypass`: pending simple receipt, parent payload/index cursor/TLS, exact native/indexed forwarding, non-48 return, nesting, exceptions and observer-on non-bypass control.
- `--ui-material-bypass`: pending material entry, parent material/upload context, completion and synthetic stereo frame pair; calls the actual Execute/upload/nonindexed draw hooks; non-36 return, nesting, exceptions and observer-on control.
- `--ui-root-bypass`: parent root row, sample-slot/counter preservation, actual simple/material Execute calls, 64-bit return bits, nesting, exceptions and observer-on control.

Material/root modes return `ERROR_NOT_SUPPORTED` when their feature is not compiled, rather than reporting a pass. These modes are not added to the existing fixed startup suite because older supported DLLs do not export them; local default manifests and existing packages remain unchanged.

## Results

The private experimental build uses the prior VRDof61 feature options plus the optional root observer. Full DLL compilation, ABI check and all three hook modes passed. DLL SHA256: `ee95be1a68ec05a4c1aec7b15e2c59452f14b69a2aceaddf11b8b9510cc8566e`.

An independent private source copy removed the four new bypass guards. The same loader detected all three regressions, returning 311, 337 and 405 respectively. That negative DLL SHA256 is `24beec34821e288d33fd0832b4761e288c8f54bd85f3dc4e711ea79df84ae0ef`; it was never installed or substituted into the working tree/player package.

Frozen source hashes, build logs and positive/negative receipts are under ignored `analysis/runtime/ui-bypass2-*`. Reproduction scripts are `build-ui-bypass2.py`, `check-ui-bypass2.py`, `build-ui-bypass2-negative.py`, `check-ui-bypass2-negative.py` in that directory. Existing local tools and MinHook sources were reused; no tools were installed. Builds used two compiler workers. The game/headset status remains unverified.

This is an unattended offline regression result, not a solution claim for mounted-camera black screens, PathChoice placement, reticle spread rings or headset pacing. Those first-version issues remain open.

---

## 中文

# 内部出版期间UI消费者隔离

2026-10-17。离线源和专用加载器进程验证。没有使用游戏、模拟器、耳机、图形用户界面、运行时切换器或控制器输入。

`StereoPublishBypass` 告诉D3 D 钩子可以转发内部出版商/ 模拟工作, 但不更新游戏帧观察。 本地用户界面消费者如果在这样的范围内签订合同, 必须保留同样的合同。 以前简单的执行程序可以消耗待发的有效载荷收据并清除父机 。 TLS 在其索引拖曳回调被绕过时; 材质 Execute/ upload 具有相似的状态变化。 可选的根观察者也可以消耗样本槽或加量父行的计数器 。

简单执行、材料执行、材料上传和可选场景根钩子现在直接在“范围”接收、TLS替换、接收要求、完成记账或诊断前转发。执行回调由根观察者进行计数,但仅提前进行。r 绕行检查。原始参数、呼叫计数、返回位数和例外仍然在转发。

这会修正有条件的钩子合同。 被审查的出版商/ 镜像路径称为 D3 D 重置/屏障/复制/执行指挥器,用于反射, DrawInstanced。 这些路径到本地引擎UI Execute/upload/root渲染的源边没有确定。 实际发生和与机枪黑屏幕的任何关系仍然存在 。 unproven.

## 专门的实际体能测试

DLL 装载器接受这些模式; 每一个模式在自己的进程中运行:

- `--ui-bypass`: 在收到简单通知前,母有效载荷/索引光标/TLS,准确的原生/索引转发,非48 返回、筑巢、例外和观察者非通道控制。
- `--ui-material-bypass`:待补材料输入,母材料/上载上下文,完成和合成立体框对;调用实际的Execute/upload/非索引抽图钩;无36 返回、筑巢、例外和观察者控制。
- `--ui-root-bypass`: 母根行,样本槽/相机保存,实际简单/材料执行呼叫, 64- 位值返回位值,筑巢,例外和观察者控制。

材料/根模式返回 `ERROR_NOT_SUPPORTED` 当其特性没有编译,而不是报告一个通过。这些模式不会添加到现有的固定启动套件中,因为旧支持的DLL不导出它们;本地默认的列表和现有的软件包保持不变。

## Results

私人实验楼使用前作的VRDOF61 特性选项加可选的根观察者。完整的 DLL 编译、 ABI 检查和所有三个钩模式都通过。 DLL SHA256: `ee95be1a68ec05a4c1aec7b15e2c59452f14b69a2aceaddf11b8b9510cc8566e`.

独立的私人源代码复制件移除了四个新的绕行守护符。同一装填器检测到所有三个回归,返回 311, 337 and 405 分别是,那个负 DLL SHA256 is `24beec34821e288d33fd0832b4761e288c8f54bd85f3dc4e711ea79df84ae0ef`;它从未安装或替换到工作树/玩家包中。

冻结源散货、建立日志和正/负收款被忽略 `analysis/runtime/ui-bypass2-*`复制脚本 `build-ui-bypass2.py`, `check-ui-bypass2.py`, `build-ui-bypass2-negative.py`, `check-ui-bypass2-negative.py` 。当前本地工具和 MinHook 源被重新使用; 没有安装工具。 构建使用了两个编译器工作人。 游戏/ 头集状态仍未验证 。

这是一个未处理的离线回归结果,而不是对挂载的相机黑屏幕、PathChoice 定位、 Retuble 散开环或耳环间隔的解决方案要求。这些第一版问题仍未解决。
