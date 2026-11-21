# Visible simulator preview without automatic activation

The user wants a continuously visible preview on a second monitor, without taking
desktop focus. This option does not minimize the window or suppress manual clicks.
Normal continuous VR sessions have no preview timeout; stopping the XR host still
closes its preview. No window positioning loop is installed.

Apply `quiet.patch` after `../simulator-focus/focus.patch` to the source revision
recorded in `provenance.json`. Set `XR_SIMULATOR_NO_ACTIVATE=1` in the child process.
Unset values, `0`, and any other value retain the simulator's original activation
behavior. This option is independent of `XR_SIMULATOR_FORCE_FOCUS`.

Automatic window creation, persistent-window adoption and aspect fitting use
nonactivating flags. Explicit foreground activation at startup is skipped. Manual
clicks and keyboard commands retain their normal behavior.

The local compiled copy is `analysis/runtime/simulator-quiet1/openxr_simulator.json`.
The original installed runtime and earlier focus-only copy were not overwritten.
The continuous runner and pose diagnostic default the option only in their child
environment; the GUI and SimulatorVR entry points select the quiet local runtime.
No registry change or global focus Hook is involved.

Build: private source copy, existing local Vulkan headers, Ninja Release. The three
existing simulator CTest suites passed. Forward/reverse patch checks passed.
An isolated 5-second XR test submitted 445 test-image layers; 532 foreground polls
and a WinEvent foreground listener observed no activation by the owned XR process.
The foreground HWND was unchanged from start to finish. This verifies that one
preview startup; it does not prove game startup or every Windows activation path.
Evidence is in ignored `analysis/runtime/simulator-quiet1-check.*`.

## Optional quiet2 swapchain handle fix

Apply `swapchain-handles.patch` after the focus and quiet patches. Its independent
source/build/DLL are recorded in `swapchain-handles-provenance.json`; this does not
change the default runtime or overwrite quiet1.

The old runtime assigned `g_swapchains.size()+2` and ignored failed insertion.
Actual game9j evidence showed menu2 (1920×764), replacement3 (1920×1080), destroy2,
then both requested 960×1080 eyes returning handle3. The fix allocates monotonically
without wrapping and reserves an empty map node before transferring resources.
All backends check publication; failed Vulkan/GL candidates retain cleanup ownership.

The coordinator ran the isolated real OpenXR+D3D12 test against both DLLs:

| Runtime | Exit | Result |
| --- | --- | --- |
| quiet1 | 1 | Left eye reused handle3; all three textures were the menu's 1920×1080 format87 resources, despite a 960×1080 format24 request. |
| quiet2 | 0 | Eyes4/5 were distinct 960×1080 format24 resources; recreated eyes6/7 validated 1280×720 and640×360. |

Saved results: `analysis/runtime/simulator-quiet2-evidence/runtime-old.txt`
(SHA256 `6e31b0416266130ce04e81c7ede7205d1ec0a3dbc6cdeb582ff432ba85af4878`)
and `runtime-new.txt`
(SHA256 `4916d95a55ac3ccdd46a19eb82265ec260d7095814d606f3ef940751b4a624ab`).
The test calls neither BeginSession nor EndFrame and requests no preview/window.
It verifies resources and handle ownership, not gameplay, submitted frames or a headset.
CPU tests additionally passed 8,031 checks; all four simulator CTest suites passed.

### Rebuild from source

Use an independent directory containing quiet1's `include`, `src`, `tests` and
`CMakeLists.txt`; omit its `bin`, build directories and `.git`. Apply this patch in
that copy. The following commands use the already installed x64 MSVC environment,
local CMake/Ninja and Vulkan headers, from the repository root:

```bat
E:\CMake\bin\cmake.exe -S analysis/simulator-quiet2-source -B analysis/simulator-quiet2-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_MAKE_PROGRAM=E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DSIMXR_VULKAN_INCLUDE_DIR=J:/gearsVr/analysis/deps/Vulkan-Headers-v1.3.290/include
E:\CMake\bin\cmake.exe --build analysis/simulator-quiet2-build
E:\CMake\bin\ctest.exe --test-dir analysis/simulator-quiet2-build --output-on-failure
```

The generated manifest is `analysis/simulator-quiet2-source/bin/openxr_simulator.json`.
The preserved local deployment is `analysis/runtime/simulator-quiet2/openxr_simulator.json`;
its DLL SHA256 is `760dccf583ef05d2c8dbb2d26f514e91f3a67e21142870afae70da651683432c`.
No dependencies are installed by these commands.

Build the independent integration test in an existing empty output directory:

```bat
cl /nologo /std:c++17 /EHsc /O2 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Itools/third-party/openxr-loader/include /Fe:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.exe /Fo:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.obj tools/openxr-probe/simulator-quiet/swapchain_runtime_test.cpp tools/third-party/openxr-loader/x64/lib/openxr_loader.lib d3d12.lib dxgi.lib
copy tools\third-party\openxr-loader\x64\bin\openxr_loader.dll analysis\simulator-quiet2-runtime-test\openxr_loader.dll
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet1/openxr_simulator.json
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet2/openxr_simulator.json
```

`--runtime` creates an isolated child with its own `XR_RUNTIME_JSON`; it does not
change the registry or parent/global environment. The child rejects any runtime
whose reported name is not exactly `OpenXR Simulator Runtime` before creating its
D3D12 session. Existing container concurrency requirements remain unchanged.

---

## 中文

# 不自动激活的可见模拟器预览

用户想要在第二个显示器上连续可见的预览,而不取
桌面焦点。此选项不会最小化窗口或抑制手动点击。
正常连续的 VR 会话没有预览超时; 仍然停止 XR 主机
关闭其预览。没有安装窗口定位环。

Apply `quiet.patch` after `../simulator-focus/focus.patch` 更改源码
记录 `provenance.json`设置 `XR_SIMULATOR_NO_ACTIVATE=1` 儿童过程中。
未设定值, `0`,任何其他值则保留模拟器的原始激活
行为。此选项独立于 `XR_SIMULATOR_FORCE_FOCUS`.

自动窗口创建、持续窗口的采纳和侧面配置使用
标记。启动时未跳过明确的前缘激活。手动
点击和键盘命令保留其正常行为.

本地编译的复制品是 `analysis/runtime/simulator-quiet1/openxr_simulator.json`.
最初安装的运行时间和早期的焦点副本没有被覆盖.
连续跑步者,且构成诊断默认选项只在其孩子身上
环境; GUI 和模拟VR 的切入点选择静态本地运行时间。
登记事项没有变化,也没有全球焦点虎克。

构建:私自源代码副本,现有本地 Vulkan 头条,忍者释放。
已有的模拟 CTest 套件通过。 向前/反向补丁检查通过 。
一个孤立的 5- 第二次XR测试提交 445 试验图像层; 532 预选投票
和一个 WinEvent 前置听器观察到没有被自有的 XR 进程激活 。
前景 HWND 从开始到结束都没有变化。 这证明它是一个
预览启动; 它不证明游戏启动或每个Windows激活路径.
证据被忽视 `analysis/runtime/simulator-quiet1-check.*`.

## 选择安静2 互换链柄固定

Apply `swapchain-handles.patch` 后焦和静静补丁。它的独立的
源/建设/DLL记录在 `swapchain-handles-provenance.json`; 此选项不
更改默认运行时间或覆盖静态1.

旧的运行时间 `g_swapchains.size()+2` ,忽略失败的插入。
实际游戏9 j 显示的菜单2 (1920×764),替换3 (1920×1080),销毁2,
之后,双方都要求 960×1080 眼睛还原手柄3。固定单调分配
在传输资源之前,不包和保留空地图节点。
所有后端检查发布;失败的Vulkan/GL候选人保留了清理所有权.

协调员运行了孤立的 OpenXR+D3D12 测试两种 DLL :

| Runtime | Exit | Result |
| --- | --- | --- |
| quiet1 | 1 | 左眼再利用手柄3 三个纹理都是菜单上的 1920×1080 format87 资源,尽管 960×1080 format24 request. |
| quiet2 | 0 | Eyes4/5 和我们不一样 960×1080 format24 资源; 重现的目光6/7 validated 1280×720 and640×360. |

保存结果 : `analysis/runtime/simulator-quiet2-evidence/runtime-old.txt`
(SHA256 `6e31b0416266130ce04e81c7ede7205d1ec0a3dbc6cdeb582ff432ba85af4878`)
and `runtime-new.txt`
(SHA256 `4916d95a55ac3ccdd46a19eb82265ec260d7095814d606f3ef940751b4a624ab`).
测试既不呼叫开始会议,也不呼叫 EndFrame , 也不请求预览/窗口 。
它验证资源并处理所有者,而不是游戏玩法,提交帧或一个耳机.
CPU 测试额外通过 8,031 检查;所有四个模拟测试套房都通过了.

### 从源头重建

使用包含静态的独立目录1's `include`, `src`, `tests` and
`CMakeLists.txt`; 省略其内容 `bin`,构建目录和 `.git`。应用此补丁
复制。以下命令使用已安装的 x64 微弱脆弱儿童环境,
本地 CMake/ Ninja 和 Vulkan 头, 发自寄存器根 :

```bat
E:\CMake\bin\cmake.exe -S analysis/simulator-quiet2-source -B analysis/simulator-quiet2-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_MAKE_PROGRAM=E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe -DSIMXR_VULKAN_INCLUDE_DIR=J:/gearsVr/analysis/deps/Vulkan-Headers-v1.3.290/include
E:\CMake\bin\cmake.exe --build analysis/simulator-quiet2-build
E:\CMake\bin\ctest.exe --test-dir analysis/simulator-quiet2-build --output-on-failure
```

生成的运算是 `analysis/simulator-quiet2-source/bin/openxr_simulator.json`.
当地人员得到的部署 `analysis/runtime/simulator-quiet2/openxr_simulator.json`;
其DLLSHA : 英国的"大战"256 is `760dccf583ef05d2c8dbb2d26f514e91f3a67e21142870afae70da651683432c`.
这些命令没有安装依赖性 。

在现有空输出目录中构建独立集成测试 :

```bat
cl /nologo /std:c++17 /EHsc /O2 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Itools/third-party/openxr-loader/include /Fe:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.exe /Fo:analysis/simulator-quiet2-runtime-test/SwapchainRuntimeTest.obj tools/openxr-probe/simulator-quiet/swapchain_runtime_test.cpp tools/third-party/openxr-loader/x64/lib/openxr_loader.lib d3d12.lib dxgi.lib
copy tools\third-party\openxr-loader\x64\bin\openxr_loader.dll analysis\simulator-quiet2-runtime-test\openxr_loader.dll
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet1/openxr_simulator.json
analysis\simulator-quiet2-runtime-test\SwapchainRuntimeTest.exe --runtime analysis/runtime/simulator-quiet2/openxr_simulator.json
```

`--runtime` 创造一个与世隔绝的孩子 `XR_RUNTIME_JSON`;没有
更改注册或父母/全球环境。孩子拒绝任何运行时间
其报告姓名不确切 `OpenXR Simulator Runtime` 创建其前
D3D12 现行集装箱货币要求保持不变。
