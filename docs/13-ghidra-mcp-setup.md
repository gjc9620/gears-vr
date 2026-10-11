# GhidraMCP 连接准备

本机扩展为 themixednuts 的 GhidraMCP 0.8.0，extension.properties 目标版本 12.1；
运行的是 Ghidra 12.1.4。扩展已安装在用户 AppData/Roaming/ghidra 中。
当前会话没有向助手暴露原生 Ghidra MCP 工具，因此另外准备了只读协议发现脚本。
实测默认 8080 端口尚未监听，HTTP 连接被拒绝；尚未宣称连接成功。

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

脚本仅访问 loopback，每次请求 5 秒超时并限制响应尺寸，读取服务信息、工具定义和
项目程序列表。不调用程序修改工具，不会改游戏、重命名符号或修改 Codex 全局配置。
后续应先确认正确游戏和地址基址，再只读反编译/查询已知原生镜头入口。
当前第一人称 Mod 的两个 xxx 文件是 UE3 内容包，已用包解析工具研究，
不需要把整个内容包当成 x64 EXE 导入 Ghidra。
