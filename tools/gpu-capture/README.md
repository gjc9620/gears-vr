# GPU Single Frame Diagnosis

Use RinderDoc 1.46 Official handbags, which connect the target through official replay API; no desktop screenshot, continuous video or remote service.

Third Party Directory:`J:\gearsVr\tools\third-party\renderdoc\RenderDoc_1.46_64`。
Source:[Official 1.46 Publication](https://github.com/baldurk/renderdoc/releases/tag/v1.46)，
[Portable bag](https://renderdoc.org/stable/1.46/RenderDoc_1.46_64.zip)。
Download ZIP SHA 256：`9CA4D09ECABA2CC791168660D6FC2A7E3D70FE67146E87EC05C5F8DEA70772F7`。
64 bits and x 86 `renderdoc.dll` Windows Autocente is checked as Valid, signed by Baldur Scott Karlsson.
This SHA is a download record of this machine, not an independently published upstream verification.

Compile required play heads from official warehouses `v1.46/renderdoc/api/replay/`Put it on.
`tools/third-party/renderdoc-download/replay-sdk/`, which contains all of this directory 24 A file.
No third-party documents and compiled products are submitted to Git.

```powershell
cmd /c tools\gpu-capture\build.bat
# 必须在编译成功后执行；运行时使用相同版本 renderdoc.dll。
$env:PATH = 'J:\gearsVr\tools\third-party\renderdoc\RenderDoc_1.46_64;' + $env:PATH
& tools/gpu-capture/build/TargetControl.exe
# 使用本次实际枚举到的 PID，不使用文档里的旧 PID。
& tools/gpu-capture/build/TargetControl.exe <PID>
```

Without PID, just enumerate. With PID only requests a frame for the corresponding process, at most. 60 Seconds, do not force other clients.
First clears the existing notification and records old Capture ID, which requires new frame notice; requests are refused without a registered graphic API.
Exit is only disconnected and does not end the target process. Timeout does not mean that the target will be permanently stopped, and no automatic repeat captures.
The tool itself exports replay marker to avoid being used as a collector.

`D3D12Smoke.exe` Create 320×240 Double buffer, submit blue screens, run maximum 45 Seconds, F.8 Can end.
The game is not compatible with the RederDoc capture route.
Pass. `renderdoccmd capture -c <输出前缀> <程序>` Accessed from startup; successfully returned ID is the target connection ID,**Not Process PID**。

`inspect_imports.py --pid <PID> --output <JSON>` Read only the validated version of the game import table.
Static Import Table No D 3 Item D does not prove dynamic creation, much less the specific cause of Hook's lapse.

Current result: smoke succeeded; game with RenderDoc module and control connection, but no graphic API is registered, frame capture is not successful.
For more details. [Recording of the results](../../docs/51-renderdoc-capture-compatibility.md)。

Follow-up [Call on the finger check and fail record](../../docs/52-renderdoc-import-bridge-failure.md)：
`find_device_calls.py` Read only the normal graphic function pointer and call point in a segment.
`inspect_smoke_imports.py` Checks the actual import address of this item 's smoke process.
`probe_import_bridge.py` This item is checked for DLL, only with your own import queries and without rewriting the game call slot; it is not a strictly read-only external tool.
`launch_import_capture.py` The temporary pointer replacement test resulted in the initiation of an anomaly and was disabled for direct execution.

---

## 中文

# GPU 单帧诊断

使用 RenderDoc 1.46 官方便携包，通过官方 replay API 连接目标；没有桌面截图、连续录像或远程服务。

本机第三方目录：`J:\gearsVr\tools\third-party\renderdoc\RenderDoc_1.46_64`。
来源：[官方 1.46 发布](https://github.com/baldurk/renderdoc/releases/tag/v1.46)，
[便携包](https://renderdoc.org/stable/1.46/RenderDoc_1.46_64.zip)。
下载 ZIP SHA256：`9CA4D09ECABA2CC791168660D6FC2A7E3D70FE67146E87EC05C5F8DEA70772F7`。
64 位和 x86 `renderdoc.dll` 的 Windows Authenticode 校验均为 Valid，签名者 Baldur Scott Karlsson。
这个 SHA 是本机下载记录，不是独立发布的上游校验值。

编译所需 replay headers 来自官方仓库 `v1.46/renderdoc/api/replay/`，放在
`tools/third-party/renderdoc-download/replay-sdk/`，包含该目录全部 24 个文件。
第三方文件和编译产物均不提交 Git。

```powershell
cmd /c tools\gpu-capture\build.bat
# 必须在编译成功后执行；运行时使用相同版本 renderdoc.dll。
$env:PATH = 'J:\gearsVr\tools\third-party\renderdoc\RenderDoc_1.46_64;' + $env:PATH
& tools/gpu-capture/build/TargetControl.exe
# 使用本次实际枚举到的 PID，不使用文档里的旧 PID。
& tools/gpu-capture/build/TargetControl.exe <PID>
```

不带 PID 只枚举。带 PID 只向对应进程请求一帧，最多等 60 秒，不强占其他客户端。
先清空已有通知并记录旧 capture ID，成功需收到新帧通知；没有注册图形 API 就直接拒绝请求。
退出只断开连接，不结束目标进程。超时不代表目标永久停止，不自动重复捕获。
工具自身导出 replay marker，避免被当作被采集程序。

`D3D12Smoke.exe` 创建 320×240 双缓冲、提交蓝色清屏帧，最多运行 45 秒，F8 可结束。
用于验证 RenderDoc 捕获通路，不代表游戏兼容。
通过 `renderdoccmd capture -c <输出前缀> <程序>` 从启动时接入；成功返回的 ID 是目标连接 ID，**不是进程 PID**。

`inspect_imports.py --pid <PID> --output <JSON>` 只读检查已验证版本的游戏导入表。
静态导入表没有 D3D 项不能证明动态创建方式，更不能证明具体 Hook 失效原因。

目前结果：smoke 成功；游戏有 RenderDoc 模块和控制连接，但没有注册图形 API，游戏帧捕获尚未成功。
详见 [实测记录](../../docs/51-renderdoc-capture-compatibility.md)。

后续 [调用指针检查与失败记录](../../docs/52-renderdoc-import-bridge-failure.md)：
`find_device_calls.py` 分块只读搜索普通图形函数指针与调用点，
`inspect_smoke_imports.py` 检查本项目 smoke 进程的实际导入地址。
`probe_import_bridge.py` 会加载本项目检查 DLL，只有自己的导入查询，不改写游戏调用槽；它不是严格只读的外部工具。
`launch_import_capture.py` 的临时指针替换试验导致启动异常，已禁用直接执行。
