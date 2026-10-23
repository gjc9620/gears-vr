# Steam 在线、离线与多安装配置兼容

2026-10-23 的两次手动启动在游戏创建前退出。Steam 登录日志显示离线启动已成功，但注册表 `ActiveProcess/ActiveUser` 为零；旧启动器因此无法在多份游戏设置间选出正确文件，错误提示把无法识别配置说成需要登录。

## 当前行为

在线 Steam 的非零 DWORD ActiveUser 继续优先，精确匹配账号设置文件。仅当该字段为零时，读取 Steam 安装目录的 `config/loginusers.vdf` 中账号选择与离线标记。当前客户端使用 AutoLogin；兼容旧元数据的 MostRecent。必须唯一选择、明确 WantsOfflineMode=1；两种选择标记同时存在时必须一致。不以配置修改时间、账号名或目录顺序猜选。

离线回退同时核对注册表 PID 对应的实际 steam.exe 路径，并在读取前后核对客户端进程身份、注册表状态和选择文件内容。读取有大小上限；重复字段、截断、异常嵌套、多个选择及读取期间账号／客户端切换均拒绝。该识别只选择本地设置，不更改 Steam 登录状态或游戏授权流程。

已知旧安装仍走相同的 EXE／Coalesced.ini 哈希校验。单设置文件的原行为保留。多安装使用不同本地配置时，可在 `tools/vr-launcher/config.json` 中加入 `game_settings_paths` 对象：键为所选游戏 EXE 的完整绝对路径，值为对应 EngineUserSettings.ini 的完整绝对路径。保留原有其他配置项。

示意格式（必须替换为本机实际路径，不要原样粘贴）：

```json
{
  "game_settings_paths": {
    "D:/Games/Reloaded/Binaries_x64/GOWDE-Steam.exe": "C:/Users/PLAYER/AppData/Local/Coalition/WarGame/Config/ACCOUNT_IDEngineUserSettings.ini"
  }
}
```

绑定只对该 EXE 路径生效，换安装不会沿用它；仅接受当前本地 Config 目录内已有的游戏设置文件，拒绝其他文件／目录及重复绑定。隔离测试实例忽略共享绑定。没有绑定时按上述在线／离线规则选择；仍不明确时给出配置选择错误，不再直接断言用户未登录。此入口供不同本地配置的安装使用，不解析或修改第三方授权组件。

正常启动器每次启动都会加载这些 Python／PowerShell 脚本，本次无需替换 GUI EXE，也未改冻结 native／XR 清单。便携暂存包含新增识别模块，重新生成空白用户配置，不携带本机绑定、账号选择元数据或存档。已发布标签和包不变。

## 验证边界

本机只读预检已确认当前离线选择与配置文件匹配、DLSS=False、设置字节保持。Steam 库内正版与历史旧副本通过现有 EXE／安装配置校验；这是构建准入，未启动游戏，不构成离线／在线游戏、GPU 或头显验收。预检回执在忽略目录 `analysis/startup-platform-compatibility-20261023/`。

Windows Store／Xbox 版尚未安装，也没有对应原始 EXE、配置或启动证据。本次未增加该平台的构建准入，不能把 Steam 的哈希、地址或启动方式当作 Store 版证明。后续需从该平台的实际安装核对可执行文件身份、版本、配置位置、启动／进程保留方式及原生／XR 关系，再决定所需独立 profile 与验证。未经验证的 EXE 继续拒绝；不提供绕过未知构建校验的开关。
