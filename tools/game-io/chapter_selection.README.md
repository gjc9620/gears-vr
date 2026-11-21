# Reflected chapter selection caller contract

Module: `tools/game-io/chapter_selection.py`.
Offline tests: `test_chapter_selection.py` (10 passed; no OS key messages/process handles).

```python
from chapter_selection import select_chapter, HAMMER
result = select_chapter(
    reader, (game['pid'], game['creation_time']), dict(HAMMER),
    verify=verify_current_owned_session,
    window=checked_owned_game_window,
    send=checked_ordinary_key_pair,
    deadline=time.monotonic() + 30,
)
```

This is a call sketch only; it has not been run against a live game. Add the private module directory to the caller's import path. `reader` is an exclusively owned current MenuReader; caller owns its lifetime and must not share it with a monitor thread while its read ledger is temporarily installed.

Required before calling:

1. Current process/creation, owner/session and cancellation must be authenticated by `verify()`; it raises on any change. Reader handle identity is independently checked via existing `menu_control.reader_process_identity` by default. `window()` returns the same identity-checked `(pid, hwnd)` throughout; no foreground requirement or activation is used.
2. Already be at the actual `DeltaSelectMap` / `WarUIScene`, player0, zero input blocking, focused/enabled `lstMaps` UIList. Startup/Continue/Act menu navigation is not duplicated here. Unknown dialogs and difficulty/confirmation screens reject.
3. Caller must have protected, hashed settings/profile/checkpoint backups and normal post-exit restoration, and must authorize the intended chapter selection. This helper does not back up, restore, edit or authorize saves. Normal chapter selection may lead to overwrite confirmation.
4. `send(action, target)` is a synchronous **ordinary owned-HWND key-message** pair, releasing key-up in `finally`, checking the target and raising on any uncertain delivery. Actions are `up`, `down`, `confirm`; confirm can reuse `menu_control.send_confirm`. Arrow pairs can reuse the already-reviewed `vrdof125-action.py` mapping (VK_UP38/scan0x48/extended, VK_DOWN40/scan0x50/extended). No SendInput, focus changes, native invocation, teleport or save edits. A dispatch failure is propagated; no retry is attempted.
5. After an arrow the caller must allow the UI to acknowledge exactly one index step. The helper waits150ms then requires that exact current Index; it stops rather than repeating an uncertain key. At most32 key pairs, a60-second absolute cap,128 provider rows and bounded exact read ledger. No guarantee of an atomic game snapshot/ABA exclusion is made.

What it certifies: fresh reflected provider slot/string/type reads, complete current provider triple uniqueness, scene/list/provider identity and ordinary byte rereads, independent full recapture immediately before each dispatch, exact existing fingerprint and reader/window identity. Target index is found anew from `FriendlyName + MapName + ChapterPointID`; no old index/PID/heap address is used. Fresh registry/header/name churn still rejects; this module introduces no new fixed-prefix policy or retries.

Return boundary: `chapter_enter_dispatched=True` means only a normal Enter on the selected chapter. `chapter_load_completed=False` and `next_screen_requires_independent_review=True` are deliberate. Difficulty selection and checkpoint overwrite confirmation require separate current semantic/identity checks and protected backup evidence; this helper never confirms them. Any returned/raised outcome must be recorded by the caller; never blindly rerun after a potentially partial key pair.

Evidence: historical `vrdof125-providers-act1.json` identifies HAMMER/sp_hos_p/LoadChapter=5, not a current provider or a pickup location. The guarded reflected read and normal key route is extracted from `analysis/runtime/vrdof125-chapter-providers.py` and `vrdof125-action.py`; those old scripts remain unexecuted and retain old process guards. No assertion is made that HAMMER chapter spawn immediately exposes a Hammer pickup. Walking awaits current live state.

---

## 中文

# 反映的章节选择召客合同

Module: `tools/game-io/chapter_selection.py`.
离线测试 : `test_chapter_selection.py` (10 已通过;没有 OS 密钥信件/进程控件).

```python
from chapter_selection import select_chapter, HAMMER
result = select_chapter(
    reader, (game['pid'], game['creation_time']), dict(HAMMER),
    verify=verify_current_owned_session,
    window=checked_owned_game_window,
    send=checked_ordinary_key_pair,
    deadline=time.monotonic() + 30,
)
```

这只是一个呼叫草图; 它没有针对一个活游戏运行。 将私有模块目录添加到呼叫者的导入路径中 。 `reader` 是一个独有的当前菜单读取器;调用器拥有其寿命,在它暂时安装读取分类账时,不得与显示器线索共享。

打电话前必须:

1. 当前的进程/创建、所有者/会场和取消必须经过认证 `verify()`; 它会对任何更改提出。 阅读器处理身份会通过已有的 `menu_control.reader_process_identity` 默认。 `window()` 返回同一身份检查 `(pid, hwnd)` ;没有使用前景要求或激活。
2. 已经是实际的了 `DeltaSelectMap` / `WarUIScene`,玩家0,零输入屏蔽,焦点/启用 `lstMaps` UIList. 启动/ 继续/ Act菜单导航在此不重复。 未知对话框和困难/ 确认屏幕拒绝 。
3. 呼叫者必须保护、散列设置/ 配置/ 检查点备份和正常的退出后恢复, 并授权设定的章节选择。 此帮助者不会备份、 恢复、 编辑或授权保存 。 普通章节选择可能导致 Ove rwrite 确认。
4. `send(action, target)` 是同步的 **普通自带的 HWND 密钥信息** 键入 `finally`,检查目标并增加任何不确定的交付。 `up`, `down`, `confirm`; 确认可以重复使用 `menu_control.send_confirm`。箭头对可以重复使用已审查过的箭头 `vrdof125-action.py` 映射( VK_ UP)38/scan0x48/ 延长, VK_ DOWN40/scan0x50/ extended) 。 没有发送、 焦点更改、 本地引用、 远程传送或保存编辑。 发送失败会被传播; 没有尝试重试 。
5. 箭头后, 调用者必须允许 UI 完全承认一个索引步骤。 帮助者等待150 ms然后需要准确的当前索引;它停止而不是重复一个不确定的密钥。32 键对, a60-第二绝对上限,128 提供者行和精确读取的分类账。没有保证原子游戏快照/ABA排除。

其证明内容:新反映的提供者槽/字符串/类型读取、完整当前提供者的三重独有性、场景/名单/提供者身份和普通字节重读、在每次发送前独立地完全重取、准确的指纹和 读取器/窗口身份。目标索引从 `FriendlyName + MapName + ChapterPointID`;没有使用旧索引/ PID/ 堆肥地址。新注册/标题/名称 churn 仍然拒绝;此模块没有引入新的固定前缀政策或重写。

回转边界 : `chapter_enter_dispatched=True` 仅指选定章节上的普通 Enter。 `chapter_load_completed=False` and `next_screen_requires_independent_review=True` 难以选择和检查点覆盖确认需要单独的当前语义/身份检查和受保护的备份证据;此帮助者从未确认过。任何返回/得出的结果都必须由呼叫者记录;绝不显示 在一个潜在的部分密钥对后 dly 重运行 。

证据:历史 `vrdof125-providers-act1.json` 识别 HAMMER/sp_hos_p/Load Chapter=5,而不是当前供应商或小提款机位置。 `analysis/runtime/vrdof125-chapter-providers.py` and `vrdof125-action.py`旧的剧本仍未执行,并保留了旧的流程守护。 没有断言HAMMER章节立即暴露出一个锤子小卡车。 步行等待着当前状态的出现。
