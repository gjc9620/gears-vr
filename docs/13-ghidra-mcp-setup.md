# GhidraMCP 连接准备

## 当前状态：运行时程序已打开并可分析

2026-10-13 23:44 更新：用户已打开 `render-16812.gpr` 中的 `section_1000.bin`，并完成自动分析。MCP 程序列表确认原始 EXE 与运行时程序同时打开；显式使用 `--program section_1000.bin` 可成功反编译 `1401665dc`、`1401b4ca0`，两处 64 字节均与已验证运行时快照一致。此前的目标内容不匹配问题已解决，无须再次导入或重复全程序分析。

本轮通过只读 MCP 进一步分析颜色目标创建、复制/解析、资源状态与首视图宽度分支。工具目录仍未暴露原生 Ghidra 方法，继续使用现有协议探针。`--start-line` 可分段读取，`--search-text` 可返回最多十处文本匹配和上下文，避免反复导出整个大型函数。原始 EXE 项目未覆盖、函数未重命名。

证据：`analysis/first-person/ghidra-followup-discovery.json`、`ghidra-runtime-binding-check.json`，以及 `ghidra-runtime-*.json`。渲染结论与动态对照见 [文档 56](56-final-input-binding-and-observed-draw-path.md)。

## 先前连接记录

2026-10-13 20:23 复核：用户重新启用后，loopback MCP 的 initialize、tools/list 和程序列表读取成功，返回 14 个工具。本轮原生工具目录仍未暴露 Ghidra 方法，因此通过现有只读协议探针调用；不要把协议可用与当前会话原生工具加载混同。

当前 GUI 打开 `gears:/GOWDE-Steam.exe`。`1401665dc`、`1401b4ca0` 两个绑定函数仍无法反编译，读取的 64 字节分别与经过验证的运行时快照不同。连接成功没有消除程序内容不匹配。下一步在 Ghidra 的 File → Open Project 打开 `J:\gearsVr\analysis\ghidra\runtime-project\render-16812.gpr`，再双击 `section_1000.bin`；不要向原始 EXE 项目覆盖运行时字节。程序列表确认新目标后，可用 `--program section_1000.bin --inspect-address 1401665dc` 定向检查。已有 headless 导出可继续使用，不必等待 GUI 切换。

探针现支持 `--program`，并在反编译成功或失败时都读取 64 字节以校验目标身份。记录：`analysis/first-person/ghidra-mcp-connected-binding-check-20261013.json` 与 `ghidra-mcp-connected-byte-comparison.json`。一次 GhidraGo 尝试未把快照打开到现有 CodeBrowser；它启动的空项目实例和等待助手已关闭，原用户实例保留。

2026-10-13 更新：本会话已暴露并成功调用原生 Ghidra MCP 工具。原项目中的磁盘字节与运行时代码不同，直接反编译仍失败；已建立独立运行时快照项目，定向反编译 80 个函数中 79 个成功。具体限制、调用链与复现见 [诊断报告](53-render-diagnosis-and-runtime-ghidra.md)。下面为早期配置记录。

本机扩展为 themixednuts 的 GhidraMCP 0.8.0，extension.properties 目标版本 12.1；
运行的是 Ghidra 12.1.4。扩展已安装在用户 AppData/Roaming/ghidra 中。
当前会话没有向助手暴露原生 Ghidra MCP 工具，因此另外准备了只读协议发现脚本。
初次检查默认 8080 端口尚未监听；用户完成配置后已连接成功。

用户截图中的 gears 项目初始为空。需要在项目窗口 Ctrl+I 导入：

`J:\gearsVr\Gears.of.War.Reloaded.v1.0.1.3816947-P2P\Gears of War Reloaded\Binaries_x64\GOWDE-Steam.exe`

导入后双击项目中的 EXE 打开 CodeBrowser。首次自动分析可先选 No，
待连接后针对已知镜头入口分析，不必先分析全部游戏。

安装/启用扩展之后重启 Ghidra。若工具未启用该插件，在 CodeBrowser 的
File → Configure 中启用 GhidraMCP。
官方设置位置为 Edit → Tool Options → Miscellaneous → GhidraMCP HTTP Server，
端口默认 8080，启用 Auto-start Server。
[对应版本官方说明](https://github.com/themixednuts/GhidraMCP/blob/v0.8.0/README.md)。

连接检查：

```powershell
$py = 'C:\Users\11\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py J:\gearsVr\tools\package-inspector\ghidra_mcp_probe.py --output J:\gearsVr\analysis\first-person\ghidra-mcp-discovery.json
```

脚本仅访问 loopback，发现请求 5 秒超时并限制响应尺寸，读取服务信息、工具定义和
项目程序列表。可选 --inspect-address（最多四个十六进制虚拟地址）读取内存块、
反编译；反编译超时 25 秒、HTTP 超时 35 秒，最多返回 200 行。
反编译失败时读取该地址前 64 字节以辅助诊断。
不调用程序修改工具，不会改游戏、重命名符号或修改 Codex 全局配置。
后续应先确认正确游戏和地址基址，再只读反编译/查询已知原生镜头入口。
当前第一人称 Mod 的两个 xxx 文件是 UE3 内容包，已用包解析工具研究，
不需要把整个内容包当成 x64 EXE 导入 Ghidra。

## 已连接后的检查

2026-10-11 用户启用服务后，initialize、tools/list 和 ghidra://programs 均成功。
项目 gears 中的 /GOWDE-Steam.exe 已打开，程序内存 Headers 起始地址为 0x140000000。
发现 14 个工具；只调用了 inspect.decompile、memory.list_blocks 和 memory.read。

两个已知原生入口 RVA 0x15cf08 和 0x15c968 的 decompile 返回
“function or instruction not found”，但各自静态 64 字节可以读取。
不能因此宣称镜头代码已反编译，也不能直接推断只是缺少自动分析。
内存块存在 .themida 和 .boot；应先将静态字节与已有运行时指令记录核对，
再决定适合的代码研究路径。本轮没有启动分析、修改程序或操作游戏。

完整连接及检查记录：analysis/first-person/ghidra-camera-inspection.json（Git 忽略）。
