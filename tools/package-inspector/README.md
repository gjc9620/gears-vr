# 静态包调查工具

`Inspect.cs` 使用官方 NuGet Eliot.UELib 1.12.0 net48 构建。依赖 System.Runtime.CompilerServices.Unsafe 6.1.2；DLL 位于本目录。Python 使用 Codex bundled Python，依赖 lz4 与 pefile 安装在本目录 python-libs。

```powershell
$pythonExe = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$cooked = 'J:\gearsVr\Gears.of.War.Reloaded.v1.0.1.3816947-P2P\Gears of War Reloaded\WarGame\CookedPC'
foreach ($name in @('Engine', 'WarfareGame')) {
    & $pythonExe 'J:\gearsVr\tools\package-inspector\decompress.py' (Join-Path $cooked ($name + '.u')) ('J:\gearsVr\analysis\packages\decompressed\' + $name + '.u')
    if ($LASTEXITCODE -ne 0) { throw 'Decompression failed' }
    & 'J:\gearsVr\tools\package-inspector\Inspect.exe' ('J:\gearsVr\analysis\packages\decompressed\' + $name + '.u') ('J:\gearsVr\analysis\packages\' + $name)
    if ($LASTEXITCODE -ne 0) { throw 'Inspection failed' }
}
```

这是部分导出，不是完整反编译。objects.tsv 是包对象索引；uc 文件可能包含解析错误。错误见 errors.txt 和 analysis/packages 下日志。原始游戏文件不应作为输出路径。

## 自动定位当前玩家摄像机

游戏载入单人关卡后，在后台执行：

```powershell
& 'J:\gearsVr\tools\package-inspector\Resolve-Camera.ps1'
```

脚本自动选取唯一 GOWDE-Steam 进程，查询实时模块基址，核对 EXE SHA256，再从版本对应的 GNames / GObjects RVA 查找本地玩家摄像机。CameraCache 和 POV 等业务字段偏移从运行时属性元数据读取，不使用旧 JSON 中的对象地址。默认结果位于 `analysis/runtime/camera-resolved.json`。

`reloaded-profile.json` 中仅保存版本对应的模块 RVA 与已核实的引擎元数据布局偏移，不保存堆地址。换游戏版本后必须重新调查；哈希不一致、多个本地摄像机、反射关系异常或读取失败均拒绝返回有效地址。分屏不在本版支持范围内。

已验证自动解析与手动调查一致，并在 PID 35088、32532 两次启动中独立定位成功；PID 32532 内检查点重载后也通过。此结果不覆盖所有关卡或任意对象重建事件。对象生命周期内可通过已解析的 Controller.PlayerCamera 字段重新读取指针；Controller 或引擎数组本身重建时应重新运行完整解析。该工具是定位原型，不是注入式 VR Mod，也尚未接入自动生命周期监听。
