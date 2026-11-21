# Simulator automation focus policy

`focus.patch` applies to the separate `OpenXR-Simulator-Source` repository at
commit `8de34575bfb823254297663aaf4997bc27da0ef8`. It includes only the runtime
focus change, its new header/test, and CMake registration. It excludes unrelated
local changes. `provenance.json` records source, dependency, and DLL hashes.

Set `XR_SIMULATOR_FORCE_FOCUS=1` in the simulator host's launch environment before
loading the DLL. The value is read once; unset, `0`, and other values preserve
normal OS focus behavior. This is an explicit simulator automation policy.
Actual OS activation logs and mouse release remain intact. Only transitions
between VISIBLE and FOCUSED change; activation cannot restart an ended session.

From the workspace root, apply or reverse the patch:

```powershell
git -C OpenXR-Simulator-Source apply ../tools/openxr-probe/simulator-focus/focus.patch
git -C OpenXR-Simulator-Source apply --reverse ../tools/openxr-probe/simulator-focus/focus.patch
```

Build-only reproduction (run in an x64 Visual Studio developer environment with
CMake and Ninja on PATH):

```powershell
git clone --depth 1 --branch v1.3.290 https://github.com/KhronosGroup/Vulkan-Headers.git analysis/deps/Vulkan-Headers-v1.3.290
git -C analysis/deps/Vulkan-Headers-v1.3.290 rev-parse HEAD
# Expected: b379292b2ab6df5771ba9870d53cf8b2c9295daf
cmake -S OpenXR-Simulator-Source -B analysis/simulator-focus-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMXR_VULKAN_INCLUDE_DIR="$PWD/analysis/deps/Vulkan-Headers-v1.3.290/include"
cmake --build analysis/simulator-focus-build --target openxr_simulator focus_policy_tests projection_timing_tests ui_resolution_tests
ctest --test-dir analysis/simulator-focus-build --output-on-failure
```

CMake outputs the new DLL and manifest to `OpenXR-Simulator-Source/bin`.
The installation script is not used. This does not overwrite the deployed
`OpenXR-Simulator-v1.5.0-win64` DLL, register a runtime, or launch an XR process.

Validation: simulator DLL compiled; all three CTest tests passed. The focus test
performs 82 checks with assertions active in Release, including all session
states, default behavior, explicit opt-in, and duplicate inactive notifications.
This establishes CPU policy behavior; focus events and game rendering still
require runtime validation.

---

## 中文

# 模拟自动化焦点政策

`focus.patch` 适用于单独的 `OpenXR-Simulator-Source` 存储器位于
commit `8de34575bfb823254297663aaf4997bc27da0ef8`中,它只包含运行时间
焦点变化、其新标题/测试和 CMake 注册。它排除了无关的内容
本地变化。 `provenance.json` 记录来源、依赖和DLL散列。

Set `XR_SIMULATOR_FORCE_FOCUS=1` 模拟器主机的发射环境
装入 DLL。该值读一次; 未设置, `0`,并保存其他值
普通 OS 焦点行为。这是明确的模拟自动化政策。
操作系统激活日志和鼠标释放程序仍然完整。只有转换程序
介于 VISIBLED 和 FOCUSED 更改之间; 激活无法重新启动结束会话 。

从工作空间根, 应用或反转补丁 :

```powershell
git -C OpenXR-Simulator-Source apply ../tools/openxr-probe/simulator-focus/focus.patch
git -C OpenXR-Simulator-Source apply --reverse ../tools/openxr-probe/simulator-focus/focus.patch
```

仅构建复制( 在 x 中运行)64 视觉工作室开发者环境
路边的CMake和忍者:

```powershell
git clone --depth 1 --branch v1.3.290 https://github.com/KhronosGroup/Vulkan-Headers.git analysis/deps/Vulkan-Headers-v1.3.290
git -C analysis/deps/Vulkan-Headers-v1.3.290 rev-parse HEAD
# Expected: b379292b2ab6df5771ba9870d53cf8b2c9295daf
cmake -S OpenXR-Simulator-Source -B analysis/simulator-focus-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMXR_VULKAN_INCLUDE_DIR="$PWD/analysis/deps/Vulkan-Headers-v1.3.290/include"
cmake --build analysis/simulator-focus-build --target openxr_simulator focus_policy_tests projection_timing_tests ui_resolution_tests
ctest --test-dir analysis/simulator-focus-build --output-on-failure
```

CMake 输出新的 DLL 并显示为 `OpenXR-Simulator-Source/bin`.
安装脚本没有使用。 此操作不会覆盖已部署的
`OpenXR-Simulator-v1.5.0-win64` DLL,注册运行时间,或启动XR进程.

校验: 模拟 DLL 编译; 所有三次测试都通过。 焦点测试
performs 82 检查是否在发布中,包括所有会话中,使用活跃的断言
状态、默认行为、明确的选择和重复不活动通知。
这确立了 CPU 策略行为; 焦点事件和游戏渲染仍然
需要运行时间验证。
