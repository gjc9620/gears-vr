# Steam Online, Offline and Multiple Installed Configuration Compatible

2026-10-23 Two manual startups exit before the game is created. Steam Login shows that offline start has been successful, but registration form `ActiveProcess/ActiveUser` is zero; the old starter therefore cannot select the correct file from multiple game settings, and the error hints that unrecognized configuration is required to log in.

## Current behaviour

Online Steam Non-Zero DWORD ActiveUser continues to prioritize and accurately match account settings. Read Steam Installer directory only when the field is zero `config/loginusers.vdf` Middle account selection and offline tag. Current client uses AutoLogin; MostRecent compatible with old metadata. Only selects and clearly defines WantsOfficeMode=1; both selection marks must be consistent when they exist. They are not guessed by configuration modification time, account name or directory order.

Back off the line and check the actual step.exe path of the PID to which the registration form corresponds, and check the client process identity, the status of the register and the content of the file before and after reading. Read with a maximum size; repeat fields, cut-off, abnormal nesting, multiple choices and read-time account/client exchange are rejected. This recognition selects only local settings without changing Steam login status or game authorization processes.

It is known that the old installation still follows the same EXE/Coalesced.ini Hash check. The original behavior of the single set file is maintained. When more sets are used, you can use different local configurations. `tools/vr-launcher/config.json` Add `game_settings_paths` Object: Key is the full absolute path of the selected game EXE, and the value is the complete absolute path for EngineeringUsersSettings.ini. Save other configuration items.

Icon format (must be replaced by the actual path of the machine, not paste as it is):

```json
{
  "game_settings_paths": {
    "D:/Games/Reloaded/Binaries_x64/GOWDE-Steam.exe": "C:/Users/PLAYER/AppData/Local/Coalition/WarGame/Config/ACCOUNT_IDEngineUserSettings.ini"
  }
}
```

The binding will only take effect on the EXE path and will not be followed for installation; only accept the current local Config setup file, rejecting other files/directors and repeating bindings. The quarantine test example ignores shared bindings. The above-mentioned online/de-link rules are not applied when tied; configuration selection is given when it is still unclear, without any direct assertion that the user has not been login. This entry is used for different local configurations, without decomposition or modification of third party authorized components.

The normal starter loads these Python/PowerShell scripts every time you start. This time, there is no need to replace GUI EXE or freeze the list of native/XR. The temporary storage contains new identification modules, recreates a blank user configuration and does not carry this machine binding, account selection metadata or archive.

## Verify Border

This pre-reader has confirmed that the current offline selection matches the configuration file, DLSS=False, set bytes. Steam Library proper and historical copies are verified through the existing EXE/ Install Configuration; this is built access, not started, does not constitute offline/online games, GPU or head-to-head acceptance. Precheck checkbacks ignore directories. `analysis/startup-platform-compatibility-20261023/`。

Windows Store/Xbox is not installed, nor does it have evidence of original EXE, configuration or startup. This time no additional access to the platform can be obtained by using Steam's Hash, address, or start-up as a Store certificate. The subsequent verification of the executable identity, version, configuration location, start/process retention and parent/XR relationship from the platform's actual installation will then determine the independent profile and authentication required. Uncertified EXE continues to refuse; no switch will be provided to bypass an unknown build check.

---

## 中文

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
