# Caption38独立包与运行中FPS开启

2026-10-17。使用独立候选analysis/release-staging/caption38-rebind5-20261017，包内Python以-I -B运行包内launcher.py；外部临时配置仅指定包外游戏EXE和100单位／米。采用候选默认HUD65°、镜头拉近0，而非本机45°／0.3米。Runtime显式选择包外quiet2模拟器，不改系统XR、不启用后台实体手柄。

## 独立包实际运行

实际会话continuous-stereo-38628-20261017-062218-344ac2d2，由包内Python拥有，游戏PID38628／创建FILETIME134366629256760160。会话、native命令、DLL／自测程序和XR命令均来自候选包；游戏位于包外。自动继续战役进入双目，之后在准确死亡菜单读取一次检查点，恢复world generation2；这不是开始新战役或修改存档来制作场景。

原生停止清理、入口恢复、XR退出与游戏正常关闭通过。运行后700项发布payload的长度／SHA256逐项一致，变化0；运行日志不列入发布payload。回执analysis/runtime/portable47-actual-runtime-audit.json。此检查未启动GUI，也不是干净玩家机器或真实头显验收。

## 运行中开启FPS

缓存资料中的WarUIListButton ...DeltaVideoSettings...optFpsDisplay用于定位；UIDataStore_WarfareGameResource.bUseFrameCtr声明和历史FrameCtr字符串仅是线索，不据此认定绑定。实际菜单状态逐步确认Pause→Options→Display，并通过四次受保护Down到optFpsDisplay；每次动作仍检查确切进程、窗口、两次新鲜菜单状态、输入阻塞和Stop/F8。

早先键盘Right／Enter尝试没有取得持久化开启证据，一次确认对话超时恢复，保留失败回执，不计作切换成功。随后仅在已确认FPS控件时，通过前台虚拟手柄Right切换，再退出显示设置，在MessageText明确为ConfirmVideoSettings的两次一致确认状态下及时应用；FrameCtr由False变为True。

返回游戏后取得新GPU图，票据18457、源序号1681、3840×2160，保存monotonic_ns51254815021100。该帧两眼都有正常非黑场景，右上角均可读到38.87 FPS。检查图analysis/runtime/portable47-fps-full.png及fps-corners.png；后者是同一原始帧两眼各自右上角的裁切，不是不同时间图片拼成左右眼。它证明本次运行中开启并应用FPS后仍能取得双目场景与计数文字，不证明应用瞬间每帧连续、反复开关、所有画质组合或头显均不会黑屏。

停止并确认游戏退出后，仅把当前INI的FrameCtr=True改回初始False，单处替换，其他字节保持；恢复前后哈希见回执。没有保存FPS开启作为玩家默认值。

## 工具调用边界

直接使用-I执行内部run_stereo_continuous.py snapshot时，隔离模式移除了脚本目录导入路径，报run_stereo_probe导入失败；游戏和拥有者仍在运行。这次不把诊断命令失败当作游戏闪退。随后包内Python通过已初始化的包内Backend.runner.request_snapshot成功请求并取得上述收据。GUI／启动器正常入口已通过，但没有据此宣称任意内部脚本都可直接在-I下执行。

第一版仍缺机枪实际占有、更多过场、长时间游玩和真机UI／帧节奏检查。本轮未复现开启FPS后的持续黑屏，不代表原始反馈不存在或所有触发均已修复。
