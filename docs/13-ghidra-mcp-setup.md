# GhidraMCP 连接准备

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
