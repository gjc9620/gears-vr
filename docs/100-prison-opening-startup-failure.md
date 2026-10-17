# 开场关卡的实际启动失败

2026-10-17。默认Reticle46／Borderless1，未修改DLL。为补充CG覆盖，通过实际UI状态进入战役→单人→加载章节→第一幕→首项→休闲难度。每一步输入前校验完整进程创建身份、窗口、两份一致菜单状态、输入阻塞及预期焦点；没有调用引擎函数或写游戏内存。

## 关卡身份和存档

在DeltaSelectMap当前Index0的UIComp_ListPresenter.ListItems首项中，通过反射DataSourceProvider取得UIDataProvider_WarfareMaps实例Prison-Chap1。实际FriendlyName为“14 YEARS AFTER E-DAY”，MapName=sp_prison_p，ChapterPointID=LoadChapter=0。没有把注册表枚举顺序当成列表顺序，也没有推测原生TMap布局。

进入前双次一致读取并校验备份SaveData四个文件，共1896字节，置于ignored analysis/savebackups/menu79-before-chapter。游戏显示CheckpointWarning后，检查确切标题／正文markup并确认；测试结束、所有者和XR退出、游戏普通关闭之后，保留测试后的副本，再恢复四个基线文件并逐一核验哈希。原玩家进度未保留为本次测试章节。

## 失败证据

会话continuous-stereo-10348-20261017-091047-3aef62e0，游戏PID10348、creation134366730348098669；完整所有者／XR身份见prison80-monitor-ready.json。

加载期间，MovieProgressReader实际观察到播放帧推进：先有89帧短片段，后有1674帧片段。播放计数不是像素或双目显示证明。模拟器MCP截图只有极窄预览条，不能据此评判头显电影画面。

准备失败发生在build_crosshair_assignment_config最终检查：CrosshairConfigRejected，`Certificate changed before completion: address=0x20dceabc size=8`。wait_camera_preparation只将已分类的注册表／相机变化作为暂态，该非注册表变化直接终止启动。地址对应的具体角色尚未在原失败回执中记录，不能仅凭末尾偏移命名为Pawn.Weapon。

最终phase=incomplete、native_cleanup_verified=false，没有独立停止后恢复回执。所有者和XR确认已退出后，按实际创建身份普通关闭游戏。不能把这一轮记为清理核验通过；普通关闭释放了该游戏进程。故障日志、电影进度及存档恢复回执位于ignored analysis/runtime/prison80-*和会话同名文件。

## 修复方向

检查可变化的已认证角色链接与不可变化的对象身份／脚本／代码证据能否分别分类。若仅是受控角色指针切换，应丢弃整份候选，在同一受限准备期限内重新发现并完整校验；不能接受不一致的旧证书，也不能按错误字符串把所有内存变化都放行。必须补上角色明确的失败证据，再用此实际关卡重跑。

menu79导航器曾以自定义异常截停战役菜单，但底层控制器将其作为读错误处理，最终报timeout；该失败保留。后续均以当前状态重新读取和单步认证执行，没有重复发送“继续游戏”。
