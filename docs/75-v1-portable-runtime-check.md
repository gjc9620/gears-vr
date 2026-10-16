# V1独立候选包实际启动检查

2026-10-17。Native36／Borderless1、自测前移已启用。用户授权离开期间继续修复测试。本次不改游戏存档或图形配置，不启用后台实体手柄；模拟器仅作为外部测试Runtime提供，候选包不包含它。

## 发现并修复的安装路径问题

独立包以包内Python启动，游戏EXE位于包外正常安装目录。首次测试PID7124停在菜单：`menu_navigation.navigate`调用`game_io.game_window()`，后者按模块所在仓库拼出固定游戏路径，无法选择玩家指定的外部安装。

c688c46改用本轮预留PID对应的Backend窗口查找，保留navigate_core每次派发前的完整进程身份、HWND、截止时间、菜单树及Stop/F8检查。第二次测试PID11376随后暴露最终`send_confirm`还会再次调用原来的固定路径查找，因此在第一个Enter前拒绝。c451f4c将同一个已绑定窗口选择器传到最终派发门禁；独立旧工具未传入时继续保留原行为。19项导航及36项菜单控制测试通过，覆盖自定义安装、窗口改变／消失、错误PID、停止，以及旧入口。

两次失败均没有激活原生场景Hook，priming入口恢复通过；随后正常关闭游戏。首次关闭脚本向多个已枚举窗口发关闭时，第二个窗口随游戏退出消失，产生WinError1400；之后独立确认进程已退出，没有强杀，回执如实记录。失败记录保留，不能将其当作游戏闪退或双目黑屏复现。

## 修复后的包内实际运行

候选目录`analysis/release-staging/gameplay36-portable3-20261017`，源代码截至c451f4c；隔离预检700文件、43408726字节。运行命令使用包内`runtime/python/python.exe -I -B`与包内启动脚本，配置只指定外部游戏EXE和100单位／米；使用玩家初始默认HUD65°、镜头拉近0米，而非本机调试配置的45°／0.3米。

实际会话`continuous-stereo-4108-20261017-041112-7bf9825f`由包内Python owner32664创建，游戏PID4108。预留DLL、原生启动脚本、native build、OpenXR程序路径均落在该候选包内。自动菜单导航成功进入现有战役，没有开始新战役；匹配原生票据后发布714张双目图像，capture attempts715、busy1、ticket／rectangle拒绝0。HUD布局应用3859次、回退0、镜头拉近保持0。

原生准备4516.560ms，首次实际projection在相机就绪标记后7539.886ms，仍未达到即时进入VR。正常停止后native cleanup、入口恢复及XR退出0均通过，随后身份核验正常关闭游戏。运行后全部700项清单payload哈希仍一致；运行产生的状态／日志不作为发布payload。

原始会话与快照在包内ignored `analysis/runtime`，汇总`analysis/runtime/portable36-actual-runtime-audit.json`记录确切owner／game身份、native／XR命令和清理结果；失败回执、预检及构建日志同在根目录ignored analysis下。本轮证明本机独立候选包能对外部安装启动并进入双目，不等同于干净玩家机器、Quest／VDXR、机枪或长期游玩验收。

## 机枪排查获得的新证据

主菜单的PID7124只读扫描确认WarPC_Menu.Pawn为空，不能据此判断关卡没有机枪。进入关卡的PID4108发现两台已注册、非默认模板的Turret_TroikaCabal，Health100，Driver／Controller为空，共用玩家WorldInfo；位置分别为(-4052.71,-12215.18,-315)及(-638.85,-10770.90,-62)。机枪直接Level与WorldInfo／控制器的PersistentLevel不同，提示当前mounted证书的同Level假设可能拒绝合法流式关卡。

证据`mounted36-gameplay-live-scout.json`包含PID／FILETIME／EXE路径、磁盘哈希、模块基址、注册表与类链复读、属性描述符及限制；有三项不可读条目，不能宣称完整实例枚举。采样时玩家已死亡／未占有，未进入机枪。本次仅确立针对性修复方向：分别认证各Actor的直接Level身份，继续要求共同WorldInfo、人物／机枪控制关系等；不得仅放开任意Outer或称机枪已验收。
