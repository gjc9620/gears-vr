# XR output colour policy compared to sRGB

All XR output was previously converted to sRGB and integrated as requested by user `build-srgb-output1` For comparison, then return to the original source format priority policy by color, and use `tools/openxr-probe/build-runtime-colors1`. The user 's latest correction is "As if it all flashes, not a color problem": both the original colour and the sRGB path seem to be blinking, and the colour retreat does not eliminate it. Current settings are maintained, with the following remaining record of previous comparisons and realizations.

## Current default: restore original colour (2026-10-23）

Game image pre-empts the running-time supported primary format, which supports direct copying of source UNORM; use the original RGBA sRGB when the source format is not available 29、BGRA sRGB 91、RGBA UNORM 28、BGRA UNORM 87 Select the conversion target in order. The menu/video plane and the transition card resume BGRA UNORM 87、RGBA UNORM 28、BGRA sRGB 91、RGBA sRGB 29 . The diagnostic pattern also restores the original UNORM priority rule. `xr_color_policy` Log, dynamic log of actual selection `unorm` or `srgb`。

Check automatically confirmation for the list of actual running time formats saved: VDXR game image restore UNORM 28,menu restore UNORM 87; SteamVR does not provide these formats, the game still selects sRGB 29, menu still selects sRGB 91. This is the rule that was used to allow consultation, not in run-time name guess format, and does not change the OpenXR selection.

Based on the frozen source code that was built by default SRGB, only colour selection, related tips/logs and regression examples are changed; other consolidated changes are retained.25 XR/GPU check passed, containing restored UNORM eye copy, SRGB conversion, menu typed/typeles upload; after access default 78 Item Launcher and 21 The entry is passed with a temporary check, the actual configuration and start-up document Hashi precheck. New XR SHA 256 Yes `58ef6c397af8facc5ae50c8adba659005a13efd603e7bcfee73ba890d1e2539a`, Named List `manifest-runtime-colors1.json` It's completely consistent with the default.

The user 's feedback "No original colour blinking" is kept as it is, has been replaced by later observation "As if all flashing" and cannot be accepted as a non-screen color. This colour retreat does not continue to be checked for root causes or new games or physical header tests, and does not mark newly constructed white questions as repaired. Source/validation/integration attachment is ignoring directories `analysis/xr-original-colors-20261023/`, last corrected list is saved `flicker-feedback-correction.json`. Original native, loader, GUI, user configuration and old candidate reservation.

## User Retest Feedback (Presentation)2026-10-23）

User confirms: "It is a problem of colour that srgb is much clearer." Recording is confirmation of the clarity view after actual comparison of the machine's head. The user then chooses to restore the original color to use a path where its feedback does not blink; and saves this sRGB comparison without increasing its equal rendering resolution or actual details, nor expanding it to complete compatibility acceptance at the time of running.

The user reports that the left eye is still blinking, “just as if there was a white frame”, and explicitly requests “to keep it fixed and to record it.” This item is listed as known problem; it is suspended for repair; it was previously described as the edge of the left eye or local whiteness, with new feedback complementing its temporal senses similar to the white frame, without capturing proof that the whole left eye is all white. [Left eye flash record](steamvr-left-eye-flicker-20261023.md)。

Subsequent corrections: “It seems that all flashes are not a problem of colour.” This is how the scintillation and color perception are recorded separately: SRGB is kept in clearer feedback; colour selection is no longer used as a known reason for blinking, nor is the original colour marked as non-scattered. The decision to suspend repair is maintained because it is unknown; only correction is registered in this round.

## Previous changes to the unified sRGB (retain history)

- Game GPU Image Unanimously Select `R8G8B8A8_UNORM_SRGB`（DXGI 29) Only not 29 Select when `B8G8R8A8_UNORM_SRGB`（91 Runs without using the original UNORM to submit it in a copy.
- The menu/film captured on the desktop is selected in a uniform way with the transition card BGRA sRGB 91, Missing 91 Select RGBA sRGB 29。
- The same sRGB strategy is used for independent diagnostic drawings. No two types of strategies are available. 8-bit sRGB format is run with clear error reporting and retreats silently to UNORM.
- Add `xr_color_policy` Log, Distinguishing `gpu_game`、`window_game`、`transition_card` and `test_pattern`, records the actual format selected; the game GUI path also records the source format and whether or not to convert.

Both active logs are available, SteamVR and VDXR are listed. 29 and 91. The new rules, both game images are selected 29, menu and transition card selection 91. The actual next round will continue to be based on the new log and will not treat the old entries as a new version of the operating acceptance/reception.

Share game source images to preserve original format and pixels; existing `GpuBlit` The CPU menus are uploaded to maintain the original code bytes. No changes have been made to game resources format, per eye resolution, rendering ratio, first person name, aim or other primary rendering behaviour. User has confirmed that the clarity perception after colour path adjustment is improved and no new performance comparison conclusions have been added.

## Validation and integration of previous sRGB build (retain history)

Freezing source as default XR `analysis/steamvr-menu-srgb-c-source` Based on the above options and log changes only, and add this test; no other native/XR changes are being developed from the workspace.

A completely new build with actual MSVC dependent on tracking through.24 XR/GPU check passed, including recorded SteamVR/VDXR format list, rejection of UNORM return, typed/typeles menu upload, layout/menu restoration, and continuous two-eye SRGB image rotation on WARP and inn NVIDIA graphic cards. After access to default,78 Item starter and 21 entry; the actual current configuration and validation of the game/native/XR/Playground file is not started.

New XR SHA 256 Yes `6f00c956889e89f472a4470ae592c8129f55d328d079505536fbdc1a34e50953`. The default list changes only `xr_build` and `xr_sha256`and saves the exact same `manifest-srgb-output1.json`Original DLL `7571fee6…bab9`, OpenXR loader, GUI and user configurations are maintained; old build and release packages are kept.

Freezing source code, build/test results, original default list and integration confirmation in ignore directory `analysis/xr-srgb-output-20261023/`. Integration validation did not start a new game or physical XR session, subsequent user clarity confirmed to be saved `user-feedback.json`, the original integration confirmation is maintained. The left eye flashes remain known, the restoration is suspended at user's request and this change is not claimed to have been repaired. This feedback register only updates the records.

---

## 中文

# XR 输出颜色策略与 sRGB 对比

此前按用户要求将所有 XR 输出改为 sRGB，并集成 `build-srgb-output1` 供比较，随后按其颜色回退请求恢复原来的源格式优先策略，使用 `tools/openxr-probe/build-runtime-colors1`。用户最新更正为“好像都会闪，不是颜色的问题”：原颜色与 sRGB 路径似乎都可能闪烁，颜色回退没有消除它。当前设置保持，下面保留此前对比与实现记录。

## 当前默认：恢复原颜色（2026-10-23）

游戏画面优先采用运行时支持的原生源格式，支持源 UNORM 时直接复制；源格式不可用时，按原先的 RGBA sRGB29、BGRA sRGB91、RGBA UNORM28、BGRA UNORM87 顺序选择转换目标。菜单／影片平面与过渡卡恢复 BGRA UNORM87、RGBA UNORM28、BGRA sRGB91、RGBA sRGB29 的原有优先顺序。诊断图案也恢复原来的 UNORM 优先规则。保留 `xr_color_policy` 日志，动态记录实际选择的 `unorm` 或 `srgb`。

对保存的实际运行时格式列表，自动检查确认：VDXR 游戏画面恢复 UNORM28、菜单恢复 UNORM87；SteamVR 不提供这两种格式，游戏仍选择 sRGB29、菜单仍选择 sRGB91。这是原有能力协商规则，不按运行时名称猜格式，也不更改系统 OpenXR 选择。

以刚才默认 sRGB 构建的冻结源码为基础，只更改颜色选择、相关提示／日志与回归用例；保留其他已集成改动。25 项 XR／GPU 检查通过，包含恢复的 UNORM 双眼复制、sRGB 转换、菜单 typed／typeless 上传；接入默认后 78 项启动器与 21 项便携暂存检查通过，实际配置及启动文件哈希预检通过。新 XR SHA256 为 `58ef6c397af8facc5ae50c8adba659005a13efd603e7bcfee73ba890d1e2539a`，命名清单 `manifest-runtime-colors1.json` 与默认完全一致。

用户“原颜色没有闪烁”的反馈按原话保留为历史，已被后来“好像都会闪”的观察取代，不能作为原颜色无闪的验收。本次颜色回退没有继续闪白根因调查，也没有新增游戏或物理头显测试，不将新构建的闪白问题标为已修复。源码／验证／集成回执在忽略目录 `analysis/xr-original-colors-20261023/`，最新更正另存 `flicker-feedback-correction.json`。原 native、loader、GUI、用户配置和旧候选保留。

## 用户复测反馈（2026-10-23）

用户确认：“就是颜色的问题 srgb 清晰很多”。记录为本机实际头显对比后的清晰度观感确认。之后用户选择恢复原颜色以使用其反馈没有闪烁的路径；保留这份 sRGB 比较结果，不将其等同渲染分辨率或实际细节数量增加，也不扩展为各运行时的完整兼容性验收。

用户同时报告左眼仍偶发闪烁，“就好像有一帧是白色的”，并明确要求“先不修，记录下来”。此项列为已知问题、暂缓修复；此前描述为左眼边缘或局部闪白，新反馈补充其类似白帧的瞬时观感，没有捕获证明整幅左眼每像素全白。详见 [左眼闪白记录](steamvr-left-eye-flicker-20261023.md)。

后续更正：“好像都会闪，不是颜色的问题”。据此将闪烁与颜色观感分别记录：sRGB 更清晰的反馈保留；颜色选择不再作为闪烁的已知原因，原颜色也不再标作无闪路径。根因未知，暂缓修复的决定保持；本轮只登记更正。

## 此前统一 sRGB 的改动（保留历史）

- 游戏 GPU 画面统一选择 `R8G8B8A8_UNORM_SRGB`（DXGI 29），仅在没有 29 时选择 `B8G8R8A8_UNORM_SRGB`（91）。运行时即使同时提供源 UNORM 格式，也不会再走原来的 UNORM 直拷提交。
- 桌面捕获的菜单／影片与过渡卡统一选择 BGRA sRGB 91，缺少 91 时选择 RGBA sRGB 29。
- 独立诊断图案也使用同样的 sRGB 策略。不提供两种 8-bit sRGB 格式的运行时会明确报错，不静默回退到 UNORM。
- 新增 `xr_color_policy` 日志，区分 `gpu_game`、`window_game`、`transition_card` 与 `test_pattern`，记录实际所选格式；游戏 GPU 路径同时记录源格式和是否转换。

两次既有实际运行日志中，SteamVR 和 VDXR 都枚举了 29 与 91。按新规则，两者游戏画面均选择 29，菜单及过渡卡均选择 91。实际下一局仍以新增日志为准，不把旧枚举当作新版本的运行验收。

共享游戏源图像保留原生格式与像素；现有 `GpuBlit` 在写入 sRGB RTV 前做对应解码，RTV 再编码，保持 SDR 编码值，避免额外叠加一次曲线。CPU 菜单上传保持原有编码字节。没有改变游戏资源格式、每眼分辨率、渲染比例、第一人称、瞄准或其他原生渲染行为。用户已确认颜色路径调整后的清晰度观感改善，没有新增性能对照结论。

## 此前 sRGB 构建的验证与集成（保留历史）

以原默认 XR 的冻结源码 `analysis/steamvr-menu-srgb-c-source` 为基础，只应用上述选择与日志改动，并加入本次相关测试；没有从工作区合入其他尚在开发的 native／XR 变更。

全新构建与实际 MSVC 依赖追踪通过。24 项 XR／GPU 检查通过，包括记录的 SteamVR／VDXR 格式列表、拒绝 UNORM 回退、typed／typeless 菜单上传、布局／菜单恢复，以及 WARP 和本机 NVIDIA 显卡上的连续双眼 sRGB 图像轮换。接入默认后，78 项启动器和 21 项便携暂存回归全部通过；实际当前配置与游戏／native／XR／Playground 文件校验通过，没有启动游戏。

新 XR SHA256 为 `6f00c956889e89f472a4470ae592c8129f55d328d079505536fbdc1a34e50953`。默认清单只变更 `xr_build` 与 `xr_sha256`，并保存完全一致的 `manifest-srgb-output1.json`。原生 DLL `7571fee6…bab9`、OpenXR loader、GUI 与用户配置均保持；旧构建和发布包保留。

冻结源码、构建／测试结果、原默认清单及集成回执位于忽略目录 `analysis/xr-srgb-output-20261023/`。集成验证当时没有启动新游戏或物理 XR 会话，后续用户清晰度确认另存 `user-feedback.json`，保留原始集成回执。左眼偶发闪白仍为已知问题，按用户要求暂缓修复，不宣称本改动已修复它。本次反馈登记只更新记录。
