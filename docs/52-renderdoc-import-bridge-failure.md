# RinderDoc Call Pointer Check and Failed Test

2026-10-12. Conclusion: Game frame capture is still unsuccessful; the start-up test for temporary replacement device/plant function failed, and command line entry disabled and no automatic repetitions are repeated.

## Read-only Positioning

In PID 33108 Medium Readed Tests 33.8 MB normal image section, block search Windows graphics export function pointers to check RIP relative calls:

| Game Data Tranche RVA | Point function | Normal Call Point RVA |
| --- | --- | --- |
| 1b43200 | Windows d3d12.dll!D3D12CreateDevice | 373dd7、3742ef、417dd7 |
| 1b43228 | Windows dxgi.dll!CreateDXGIFactory1 | 417e3e |

This is evidence of code references and current pointer, not dynamic execution counts. Both the disk and running time PE inventory directory are RVA 2011755、1720 bytes, general import name check not D 3 Item D.

New loaded BridgeProbe DLL own D 3D 12/DXGI import also points to the original Windows export. First use of a function address for getting MSVC local import jumpboards, followed only by confirmed FF 25 RIP relative import springboard, get the real address.

GetProcAddress, LoadLibraryExW, CreateDXGIFactory 1 Imports are actually pointing to RenderDoc. Checking with the same RederDoc DLL SHA by smoke 's GetProcAddress import address 256 and 32 Bytes entry, search for equipment and plant packaging functions that can be obtained from RinderDoc.
This query does not modify game data; it uses internal access points measured on this machine, rather than stable open API.

## Start Failed

PID 32376 In the verified game of Hashi,4 Two data slots were temporarily replaced after a normal call point, an original system function pointer and a RenderDoc Hashi/entry.
This plan 20 Restores the original pointer after seconds, but the game exits early and resumes operations because the process has been terminated. **We can't call it "renewal."**。
Changes are only in the process memory, and the disk EXE and settings files are not modified as a result.

Windows Application Error 1000／WER 1001 Record access anomalyc 0000005- It's a failure. 45357 c, Process ID 0x7e78（32376）。
Started from Normal PID 6088 Decoding read-only: the location is `mov rax,[rcx]`,rcx just taken from the previous factory's false watch +78 Call the output local object; then prepare QueryInterface.
This supports the problem of creating/output objects in the exchange chain, but without an abnormal repository, HRESULT or full crash dump, it is not possible to determine the specific return error or empty pointer value.
Replacement equipment and plant portals alone are not sufficient to prove that the entire graphic object chain has been correctly taken over. Do not continue guessing and rewriting returns or skip errors.

## Recovery and follow-up

Started PID with normal launcher 6088, does not load the current RinderDoc/BridgeProbe; telemetry can read lenses, time updates. User then sees a reset hint of last start failure and suggests " No " to retain the existing settings.
Read it live, DLSS, FSR 2、FSR 3 MFSR is all False.

Save failed source code for review, but `launch_import_capture.py` Direct execution will refuse to run. Follow-up assessments of the official PIX start/synthesis command line capacity, or direct checks on final shadow input resources; test tools cannot be considered successful in their own right as a double-eye success.

Invent:`analysis/renderdoc/game-33108-device-references.json`、`bridge-33108.txt`、`queried-wrapper-33108.json`、`smoke-wrapper-addresses.json`、`import-bridge-32376.json`、`crash-function-disassembly-6088.json`。

---

## 中文

# RenderDoc 调用指针检查与失败试验

2026-10-12。结论：游戏帧捕获仍未成功；临时替换设备／工厂函数的启动试验失败，命令行入口已禁用，不再自动重复。

## 只读定位

在 PID 33108 中读取约 33.8MB 普通映像节，分块搜索 Windows 图形导出函数指针，再核对 RIP 相对调用：

| 游戏数据槽 RVA | 指向函数 | 普通调用点 RVA |
| --- | --- | --- |
| 1b43200 | Windows d3d12.dll!D3D12CreateDevice | 373dd7、3742ef、417dd7 |
| 1b43228 | Windows dxgi.dll!CreateDXGIFactory1 | 417e3e |

这是代码引用与当前指针的证据，不是动态执行计数。磁盘与运行时 PE import directory 均为 RVA 2011755、1720 字节，常规导入名检查没有 D3D 项。

新加载的 BridgeProbe DLL 自己的 D3D12/DXGI 导入也指向原始 Windows 导出。首次使用函数地址得到 MSVC 本地导入跳板，后续只跟随已确认的 FF25 RIP 相对导入跳板，得到真实地址。

独立 smoke 程序的 GetProcAddress、LoadLibraryExW、CreateDXGIFactory1 导入实际指向 RenderDoc。通过 smoke 的 GetProcAddress 导入地址，核对相同 RenderDoc DLL SHA256 和 32 字节入口，查询得到 RenderDoc 的设备和工厂包装函数。
此查询不修改游戏数据；使用的是在本机测得的内部入口，不是稳定公开 API。

## 启动失败

PID 32376 在已核对游戏哈希、4 个普通调用点、原始系统函数指针、RenderDoc 哈希／入口后，临时替换了两个数据槽。
本计划 20 秒后恢复原指针，但游戏提前退出，恢复操作因进程已终止失败。日志里的 restored=false **不能改称恢复通过**。
变更仅在该进程内存，游戏磁盘 EXE 和设置文件没有因此被修改。

Windows Application Error 1000／WER 1001 记录访问异常 c0000005，故障 RVA 45357c，进程 ID 0x7e78（32376）。
从正常启动的 PID 6088 只读解码：该位置为 `mov rax,[rcx]`，rcx 刚取自前一个工厂虚表 +78 调用的输出局部对象；随后准备 QueryInterface。
这支持问题发生在交换链创建／输出对象处理中，但没有异常寄存器、HRESULT 或完整 crash dump，不能据此断定具体返回错误或空指针值。
仅替换设备和工厂两个入口不足以证明整个图形对象链已被正确接管。不要继续猜测并改写返回值或跳过错误。

## 恢复与后续

已改用正常 launcher 启动 PID 6088，未加载本轮 RenderDoc／BridgeProbe；遥测可读取镜头、时间更新。用户随后看到上次启动失败的重置提示，建议选“否”，保留现有设置。
实读 DLSS、FSR2、FSR3、MFSR 均为 False。

保留失败源码以便复查，但 `launch_import_capture.py` 直接执行会拒绝运行。后续评估官方 PIX 的启动／单帧命令行能力，或直接检查最终 shader 输入资源；不能把测试工具自身成功视为游戏双目成功。

本机证据：`analysis/renderdoc/game-33108-device-references.json`、`bridge-33108.txt`、`queried-wrapper-33108.json`、`smoke-wrapper-addresses.json`、`import-bridge-32376.json`、`crash-function-disassembly-6088.json`。
