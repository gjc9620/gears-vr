# SteamVR start-up menu image compatibility repair

2026-10-23 17:09 and 17:10 Manually started games are successfully created, primary entrance pre-positioned and effectively loaded SteamVR/OpenXR, D 3D 12 Device with XR.17:10♪ Back to the letter as ♪ `continuous-stereo-29940-20261023-171044-7c2cbc1c`;and report thereafter `No game display swapchain format`The failure is different from the previous offline account configuration detection intercept and does not mean that the user did not set SteamVR as OpenXR.

The current round of SteamVR count is as follows: `[29,91,2,10,24,40,55,45,20]`Including RGBA/BGRA 8 BitsRGB format 29／91, no normal UNORM format previously accepted for menu paths 28／87。`GameScreen`The transition tip card uses the same two candidates and there is a similar gap.

Add `menu_swapchain_format.h`, two menu path shared selection rules: keep the normal UNORM priority and accept the corresponding RGBA/BGRA sRGB when they are missing; no 10 Bits, floats or typeles are directly optional menu formats. Layout checks accept the selected typeped format and homotypes of typeless resources, still reject channel mismatches, other typed formats, size/array/mip/sample/ copy border errors. Uploading, returning to read verification is by route group judgement, overwhelms the sGB format, avoiding red-blue channel error.

CPU uploads SDR code bytes that keep captured images without extra application of the gamma curve. OpenXR instruction requires a statement of interpretation of such images in sRGB format and indicates that DXGI resources can be read in the corresponding typeles format at running time, as per the application selected format. [Khronos XrSwapchain Document](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSwapchain.html). This does not mean that the current round has verified HDR, synthetic colour or header.

Copy the current default XR-related frozen source code independently, retaining existing native, double-sink protocol, FFO/ready/ACK, world handoff and all original XR; no other XR changes to the development area are directly mixed. `analysis/xr-build-steamvr-menu-srgb-c` By relying on proof, record 529 Headers depend on them, confirming that the compiler contains a new selection policy.22 The independent check is passed, including format selection, layout, start-up menu, recovery/world/ticket and exit return.

Add D 3D 12 WARP Upload/Readback Check Overwrite 28／87／29／91 Four formats, one for each typeed and peer typeless resource.513×3 The following is a list of the most recent examples of the pixels, non-talent line spacing and SIMD tail.8 Examples 1539 The check does not create games, XR sessions or user windows and does not constitute SteamVR/Practical Headin Recognition.

Initial A build because no hides the buildup UTF for Console 8 Output refused at pre-defixing stage of the compiler; B exposed a copy of the freeze to the absence of native header after filling the environment, and Pause MenuGeometryTest failed to compile. Two failed logs/recipts were kept. C complete the same original frozen header file without additional changes in production logic, build and 22 Check through.

Fixed Access Normal Default: XR Directory `tools/openxr-probe/build-steamvr-menu-srgb1`，EXE SHA 256 `5f8d67696d6617b58e0faed16da569192a33c1afd091e9e73bb6f566615fe3f5`; default list changes only xr_build and xr_sha 256 New SHA 256 `52e820d26074dad2b8aa5b0b6b448d6278ab29e629183061307a253e91e766b4`. native hold on `7571fee6…bab9`, OpenXR loader bytes are maintained and the old XR is kept. No changes have been made to GUI, user settings, game files or system OpenXR selections, no new packages have been published.

User Follow-up during Quest 17:14 Session `continuous-stereo-32304-20261023-171428-46c9975c` Actual use of Virtual DesktopXR, passed 39 The frame starts pre-screening, activates the birth and records the first successful eye program. It is built using the old XR, which is independent VDXR operating evidence and does not act as a test for SteamVR for this restoration; this item does not stop or interfere with the Bureau. New construction is ready for next launch, and the actual retrospection and headography of SteamVR are still in progress.

Original failed confirmation, source snapshot/Hashi, build check and integration confirmation ignore directory `analysis/steamvr-menu-srgb-20261023/`. Mark and maintain historical evidence.


## Next show route correction

The user subsequently clarified that the SteamVR menu was visible only on the desktop and had no image; the head-visibility explanation for the previous response to the short option was withdrawn. Further checking the SteamVR log, it found that krvr_native became active HMD first, the oculus of VD was shown as "not able to add a second HMD" was subsequently rejected. KRVR desktop application did not run without an additional driver unloaded. Backup configuration and normal restarting after driver_krvr_native.enable was discontinued, and the active HMD has been removed from the VD counterpart; users confirm that the SteamVR interface is restored. The repair of this undismounted KRVR, nonstarted game, Mod menu and level card are still awaiting actual validation.analysis/steamvr-hmd-routing-20261023/。

---

## 中文

# SteamVR 启动菜单图像格式兼容修复

2026-10-23 17:09 与17:10的手动启动均成功创建游戏、完成原生入口预备，并实际加载 SteamVR/OpenXR、创建D3D12设备与XR会话。17:10回执为 `continuous-stereo-29940-20261023-171044-7c2cbc1c`；随后报 `No game display swapchain format`，关卡原生VR尚未激活，成功提交帧为零。这次故障与此前离线账号配置识别拦截不同，也不是用户没有把SteamVR设为OpenXR。

本轮SteamVR枚举格式为 `[29,91,2,10,24,40,55,45,20]`，含RGBA／BGRA的8位sRGB格式29／91，没有菜单路径原先接受的普通UNORM格式28／87。`GameScreen`在创建菜单交换链前因没有候选而退出。过渡提示卡使用相同的两种候选，也有相同缺口。

新增 `menu_swapchain_format.h`，两个菜单路径共用选择规则：保留既有普通UNORM优先级，缺少它们时接受对应RGBA／BGRA sRGB；不把10位、浮点或typeless当作可直接选取的菜单格式。布局检查接受所选typed格式及同族typeless资源，仍拒绝通道族错配、其他typed格式、尺寸／array／mip／sample／复制边界错误。上传、回读验证按通道族判断，覆盖sRGB格式，避免红蓝通道错位。

CPU上传保留捕获图像的SDR编码字节，不再额外施加gamma曲线。OpenXR规范要求通过sRGB格式声明这类图像的解释方式，并指出DXGI资源可以使用对应typeless格式、由运行时按应用所选格式读取。见 [Khronos XrSwapchain 文档](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSwapchain.html)。这不代表本轮已经验证HDR、合成器色彩或头显观感。

从当前默认XR对应的冻结源码独立复制，保留原有native、双槽协议、FIFO／ready／ACK、world handoff和全部原XR功能；没有直接把开发工作区的其他XR改动混入。新构建 `analysis/xr-build-steamvr-menu-srgb-c` 通过依赖证明，记录529条头文件依赖，确认编译器包含新增选择策略。共22项独立检查通过，含格式选择、布局、启动菜单、恢复／world／ticket与退出回归。

新增D3D12 WARP上传／读回检查覆盖28／87／29／91四种格式，每种typed及同族typeless资源各一例，513×3像素、非平整行距及SIMD尾部，8例各1539像素全部字节匹配。该检查不创建游戏、XR会话或用户窗口，不构成SteamVR／实际头显验收。

最初A构建因未设置隐藏构建console的UTF8输出而在编译器前缀解析阶段拒绝；B在补齐该环境后暴露冻结副本漏复制native头文件，PauseMenuGeometryTest编译失败。两次失败的日志／回执保留。C补齐相同原始冻结头文件，无生产逻辑额外修改，构建及22项检查通过。

修复已接入正常默认：XR目录 `tools/openxr-probe/build-steamvr-menu-srgb1`，EXE SHA256 `5f8d67696d6617b58e0faed16da569192a33c1afd091e9e73bb6f566615fe3f5`；默认清单仅变更xr_build与xr_sha256，新SHA256 `52e820d26074dad2b8aa5b0b6b448d6278ab29e629183061307a253e91e766b4`。native保持 `7571fee6…bab9`，OpenXR loader字节保持，旧XR构建保留。未修改GUI、用户设置、游戏文件或系统OpenXR选择，未发布新包。

排查期间用户后续17:14会话 `continuous-stereo-32304-20261023-171428-46c9975c` 实际使用VirtualDesktopXR，已通过39帧启动预检、激活原生并记录首次成功双眼projection提交。它使用旧XR构建，是独立的VDXR运行证据，不充当本修复的SteamVR验收；本项不停止或干预该局。新构建供下次启动使用，SteamVR实际复测及头显画面仍待完成。

原始失败回执、源码快照／哈希、构建检查及集成回执在忽略目录 `analysis/steamvr-menu-srgb-20261023/`。发布标签与历史证据保持。


## 后续头显显示路由更正

用户后续澄清SteamVR时菜单仅在桌面可见，头显没有画面；此前对简短选项回答的头显可见性解释撤回。进一步核对SteamVR日志，发现krvr_native先成为活动HMD，VD对应的oculus头显随后以“不能添加第二台HMD”被拒绝。KRVR桌面程序未运行不等于附加驱动未加载。备份配置并仅停用driver_krvr_native.enable后正常重启，活动HMD已切回VD对应设备；用户确认SteamVR界面恢复。该修复未卸载KRVR、未启动游戏，Mod菜单及关卡仍待实际验证。详细证据和设置备份在忽略目录analysis/steamvr-hmd-routing-20261023/。
