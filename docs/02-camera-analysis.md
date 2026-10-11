# Reloaded 摄像机调查（2026-10-11）

## 当前结论

已从本机实际游戏包找到摄像机类、视点缓存及原生计算候选。尚未找到双眼渲染函数、运行时对象地址或可用 hook。当前成果属于静态分析，未修改游戏或注入进程。

## 已核实的环境

- 游戏：`Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded`。
- EXE：`Binaries_x64/GOWDE-Steam.exe`，PE x64，首选 ImageBase `0x140000000`（不是当前进程加载地址）。
- SHA256：`bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`。
- Engine.u、WarfareGame.u：包版本 835/76，EngineVersion 3340，CookerVersion 111。UELib 构建识别名为 GoWUE，不能据此把游戏认成 Ultimate Edition。
- 压缩标志 32。实际 LZ4 解压成功：Engine 55 块、WarfareGame 364 块，每个子块及整块长度已校验。
- 解压分析副本分别为 58,588,993 和 429,444,378 字节，原始游戏包未改动。
- 对象导出表：Engine 37,299 项；WarfareGame 149,763 项。

## 证据与调用关系

下列关系来自反编译函数体；仍需运行时确认玩家实际经过哪个分支。

```text
Engine.Camera.UpdateCamera
  -> WarPlayerCamera.UpdateViewTarget（默认风格且目标为 Pawn 的分支）
     -> WarPlayerCamera.PlayerUpdateCamera
        -> Pawn.CalcCamera：成功时应用 CameraModifiers
        -> 否则 UpdateCameraMode + PlayerUpdateCameraNative
  -> NewPOV = ViewTarget.POV（切换视点时另有混合分支）
  -> FillCameraCache(NewPOV)
     -> CameraCache.TimeStamp = WorldInfo.TimeSeconds
     -> CameraCache.POV = NewPOV

PlayerController.GetPlayerViewPoint
  -> PlayerCamera.GetCameraViewPoint
     -> CameraCache.POV.Location / Rotation
```

具体文件都位于 `J:/gearsVr/analysis/packages/`：

| 证据文件 | 意义 |
| --- | --- |
| WarfareGame/Class_WarfareGame.WarPlayerCamera_.uc | native 类，继承 Camera；默认 FOV 70 |
| WarfareGame/Function_WarfareGame.WarPlayerCamera.UpdateViewTarget_.uc | 默认、观战、FaceCam 等分支 |
| WarfareGame/Function_WarfareGame.WarPlayerCamera.PlayerUpdateCamera_.uc | CalcCamera 与原生计算分支 |
| WarfareGame/Function_WarfareGame.WarPlayerCamera.PlayerUpdateCameraNative_.uc | native 函数声明；没有原生函数体 |
| Engine/Function_Engine.Camera.UpdateCamera_.uc | 视点更新、混合、缓存写入 |
| Engine/Function_Engine.Camera.FillCameraCache_.uc | 最终 POV 缓存写入 |
| Engine/Function_Engine.Camera.GetCameraViewPoint_.uc | 从缓存读出位置与旋转 |
| Engine/Function_Engine.PlayerController.GetPlayerViewPoint_.uc | 玩家读取摄像机视点 |
| Engine/objects.tsv、WarfareGame/objects.tsv | 类、函数、属性的包对象索引 |

WarfareGame 对象表中 WarPlayerCamera 类索引为 27485，PlayerUpdateCameraNative 为 27389；Engine 中 Camera.UpdateCamera 为 6036，Camera.GetCameraViewPoint 为 6028。这些是**包对象索引，不是内存地址或 EXE RVA**。

## 对双目渲染的意义

CameraCache.POV 是值得首先追踪的中心视点候选，但仅改变这个缓存不能证明立体渲染实现。还需找到摄像机视点进入场景视图/投影矩阵的位置，在同一逻辑帧建立左右眼视图，并处理各眼深度、后处理历史和渲染目标。

Engine.LocalPlayer 的对象表包含 ViewState、LastViewLocation、Origin、Size、ViewportClient；WarfareGame 有 WarLocalPlayer、WarGameViewportClient，可作为原生场景视图调查线索。尚未核实 CalcSceneView 或其他原生渲染入口的地址。

## 工具与限制

目录原有 Unreal-Library 1.12.0 是源码；当前 PATH 中 dotnet 没有 SDK。使用 NuGet 官方发布的同版本 net48 DLL 和系统 C# 编译器构建了小型静态导出工具。

UELib 在压缩包上仅读包头就返回，零对象不代表没有脚本。`tools/package-inspector/decompress.py` 为已观察到的 835/76、None 文件夹头布局写了专用解压程序，只输出分析副本。不能将它用于任意 Unreal 包或直接写回游戏。

部分反编译输出有 `@NULL`、`__NFUN_*`、缺失参数、空函数声明及默认属性错误。函数体中明确可见的调用可作为线索，完整签名、结构布局和字节码仍需核对。`errors.txt` 与 load.log 保留了解析诊断。

EXE 静态导入表没有发现 D3D/DXGI 导入，不能因此判断游戏没有使用它们或直接确认 D3D12。实际图形调用仍待模块/图形抓帧验证。

下一步：在用户完成分辨率调整后，确认当前运行环境，再只读定位玩家摄像机对象及 CameraCache，核对视点是否随移动、转向变化；然后追踪原生视图构建入口。

工具参考：[UELib 官方说明](https://github.com/EliotVU/Unreal-Library)，其中说明 UE3 压缩包需要先解压。

## 本次关卡运行的只读定位结果

用户提供已载入第一章的暂停菜单截图。PID 35088，游戏进程正常加载 dxgi.dll、d3d12.dll、D3D12Core.dll；这支持 D3D12 运行路径，但尚未抓帧证明具体提交调用。

通过四个独立名称条目的指针与索引，推导出同一名称指针表基址 `0x4880000`，校验了 60,193 个条目的索引与内容。相邻名称也有合理关系，如 WarPlayerCamera 与 Default__WarPlayerCamera。

扫描对象头候选后，通过名称、类指针、Outer 和类继承关系进一步筛选，找到一个 WarPlayerCamera 实例：

| 项目 | 本次运行观测值 |
| --- | --- |
| WarPlayerCamera UClass | `0x18e01180` |
| 摄像机实例 | `0x8071c040`，名称编号 3 |
| Outer | PersistentLevel `0x8bf60be0` |
| 摄像机继承链 | WarPlayerCamera → Camera → Actor → Object |
| UObject 名称 / 类 / Outer 字段 | `+0x48` / `+0x50` / `+0x40` |
| CameraCache 属性元数据 | `0x1fe3c560`，Outer 为 Camera，类型 StructProperty |
| CameraCache 实例字段偏移 | `+0x3ac` |
| TCameraCache.POV 偏移 | `+0x4` |
| POV 地址 | `0x8071c3f0` |
| TPOV.Location / Rotation / FOV 偏移 | `0` / `12` / `24` |

属性偏移来自运行时反射元数据；Offset_Internal 候选字段 `+0x94` 用 TPOV 的三种成员布局交叉核对，CameraCache/POV 的结构类型指针也相互吻合。摄像机缓存读出的位置 `(273.8499, 920.5922, 294.6194)`、旋转 `(Pitch=-1173, Yaw=23464, Roll=0)`、FOV `80`，数值合理。

暂停状态采样 3 秒、291 个样本，时间戳 `9.257865`、位置、旋转和 FOV 全部不变，符合暂停状态。尚需恢复游戏并用受控镜头旋转证明输入相关性；尚不能证明这个缓存是最终原生渲染读写接口。

只读类字段遍历额外得到 828 个字段/函数：Camera.PCOwner `+0x224`、DefaultFOV `+0x234`、ViewTarget `+0x3cc`、PendingViewTarget `+0x404`，供后续关系核验使用。

上述地址仅适用于当前 PID 的这一轮运行，重新启动后必须重新解析。证据文件在 `analysis/runtime/`：name-table-candidates.json、camera-object-candidates.json、camera-metadata.json、camera-fields.json、camera-paused.csv/json。所有工具仅请求 QUERY_INFORMATION 与 VM_READ 权限，没有进程写入、调试附加或 DLL 注入。

### 镜头输入与本地玩家关系验证

恢复游戏后静止采样 5 秒、486 样本：游戏缓存时间戳推进约 5 秒，位置、旋转、FOV 全部稳定。

用户执行镜头操作期间采样 40 秒、3,872 样本：1,533 种不同 POV；Yaw 范围 -1893 至 24827 RU，Pitch 范围 -1557 至 -212 RU，Roll 0，FOV 80。证明所定位缓存响应镜头输入。由于第三人称镜头会绕人物转动，镜头位置变化不能单独推断人物移动。

该轮时间戳有一次约 3.611 秒的不推进区间。用户已明确游戏失去焦点会自动暂停；没有同步 Present 和焦点记录，**不能**把这段区间认定为指南中的 VM 停顿，也不能证明最终渲染摄像机停顿。

反射关系验证得到闭环：

```text
WarPlayerCamera 0x8071c040
  PCOwner (+0x224) -> WarPC 0x8cd12ae0
    PlayerCamera (+0x37c) -> 同一个 WarPlayerCamera
    Player (+0x374) -> WarLocalPlayer 0x811f4bd0
    Pawn (+0x224) -> Pawn_COGMarcus 0x7c140100
```

因此定位到的是本地玩家摄像机，不是默认对象或其他 Actor 的摄像机。证据：local-camera-verification.json、camera-focused-still.csv/json、camera-look-test.csv/json。

### 原生入口候选

UFunction `WarPlayerCamera.PlayerUpdateCameraNative` 的运行时字段 `+0x114` 读取到 `0x1403b3af4`（当前模块 RVA `0x3b3af4`）。相同字段在几个脚本函数上共同指向 `0x1401d6ca0`，支持它是函数执行入口字段的解释。

该候选位于普通可执行只读页（保护 0x20），小范围反汇编显示它按 `FFrame.Code +0x28` 解析 VM 参数，然后以 RCX=this、RDX=Pawn 候选、XMM2=DeltaTime 候选、R9=OutVT 候选调用 `0x140110878`。这是结合脚本调用参数与汇编得到的 ABI 推断，尚未动态确认。没有读取或处理 .themida/.boot 区域，也没有解包保护。

这个入口仍属于 VM 到 native 的包装层，不能直接称为独立于 VM 调度的最终渲染写入位置。后续要分析实际调用目标及视图消费者。证据：camera-native-entry.json。

## 不依赖固定堆地址的自动解析

从三个独立类对象的内部索引及对象数组槽位交叉核对，找到 GObjects 数据数组；名称表也由四条独立名称记录核对。随后在模块普通区域寻找 TArray 头，得到当前 EXE 哈希对应的两个锚点：

- GNames：模块 RVA `0x1c34800`。
- GObjects：模块 RVA `0x1c24730`。

当前头部显示名称 61,871 项、对象 212,688 项。定位工具查询实际模块基址而不是使用固定 `0x140000000`；RVA 与结构元数据布局绑定到 EXE SHA256，版本不符时拒绝继续。

解析流程：验证 EXE → 模块基址 → GNames / GObjects → WarPlayerCamera 类 → 运行时反射字段及 TPOV 布局 → 对象数组中的实例 → PCOwner / PlayerCamera 双向链接 → WarLocalPlayer → 当前 POV。

`resolve_camera.py` 不读取任何旧运行时调查 JSON；输入只有当前 PID 与版本适配资料。首次运行约 0.578 秒，独立返回当前摄像机 `0x8071c040`、POV `0x8071c3f0`，与之前输入验证和玩家关系验证一致。它拒绝多个本地摄像机、非法数组或字段、错误版本、数组重分配及摄像机链接变化。

入口：`tools/package-inspector/Resolve-Camera.ps1` 自动选择当前唯一游戏进程；结果：`analysis/runtime/camera-resolved.json`。发现证据：`analysis/runtime/global-anchors.json`。

跨重启与同进程检查点重载验证结果见下文。工具尚未持续监听摄像机生命周期；实时 Mod 集成时还要逐帧核对对象关系并在失效时重新解析。

### 跨重启实测结果

用户重新打开游戏后自动解析成功，PID 从 35088 变为 32532；EXE SHA256 相同。

| 对象 | 旧进程 | 新进程 |
| --- | --- | --- |
| 玩家摄像机 | `0x8071c040` | `0x24d893a0` |
| CameraCache.POV | `0x8071c3f0` | `0x24d89750` |
| WarPC | `0x8cd12ae0` | `0x8d782ae0` |
| WarLocalPlayer | `0x811f4bd0` | `0x914b8bc0` |

新进程解析用时约 0.563 秒，反射属性偏移保持一致，玩家/摄像机双向关系及数值验证均通过。证明该定位路径在这两次启动间可重复；并不意味着所有版本或任意生命周期变化已经测试。結果：`analysis/runtime/camera-resolved-reopen.json`。

暂停状态额外采样 60 次，时间戳及 POV 完全一致，FOV 80，符合用户聊天期间自动暂停的状态。证据：`analysis/runtime/camera-reopen-paused-check.json`。

用户说明每次与助手聊天都会让游戏失去焦点并自动暂停。今后后台只读定位可在暂停状态完成；需要游戏操作或测更新时，必须明确提示用户切回游戏并保持焦点。不能把聊天期间的冻结当成 VM 调度异常。

### 同进程检查点重载结果

用户执行 LOAD LAST CHECKPOINT 后重新解析，PID 仍为 32532；用时约 0.593 秒。摄像机 `0x24d893a0`、POV `0x24d89750`、WarPC 和 WarLocalPlayer 的地址均保持不变；反射布局、玩家双向关系和数值校验通过，读取位置变化、FOV 仍为 80。

本次地址不变不能证明摄像机一定被重建，也不能证明所有重载都会保留对象。这里只证明当前版本、当前关卡的检查点重载后，独立定位仍然有效。

原运行、跨重启、同进程检查点重载三项基础测试均通过。证据：`analysis/runtime/camera-resolved-checkpoint.json`、`analysis/runtime/checkpoint-comparison.json`。未执行任何内存写入或注入。
