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
