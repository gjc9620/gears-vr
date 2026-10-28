# 当前四项的统一体验与待确认边界

> 最新覆盖：用户授权继续一个M4开火节点，现第十包已交付。Lancer实际扣弹420→419→405、同shot枪口起点／方向写回与正常收尾已触达；长按、连续跟手、准确命中和头显仍待联合反馈。PG49e1ce0，Native／物理XR复用v9。以下v9记录保留为历史，原三项仍未物理确认。

当前入口：[GearsVR Launcher.exe](</J:/gearsVr/candidates/6dof-unified/GearsVR Launcher.exe>)。功能候选6dof-unified-20261028-9，源码9e0ad18；后续文档提交不代表另一个功能包。原设置、物理XR及回退保持。

## 已交付与尚缺证据

第一人称／空间固定暂停菜单已有基本画面；独立双手位置与右yaw/pitch、三槽切换和实际Lancer握持侧移已有Source1基本证据。用户实体头显、完整roll、自然触达范围和舒适度尚未确认。模拟器手柄接口当前只有yaw/pitch，不能把它当作完整翻腕验收。

源码核对：free_hand_eye_targets.h::wrist_from_eye使用完整头／Grip四元数，按轴变换后组合最终眼点，不丢弃roll；hand_wrist_calibration.h::native_wrist_target通过Grip×挂点四元数保留朝向；body_c0_hands.h::compose_common只对超范围的位置投影，旋转不裁掉；hand_two_bone_pose.h::solve将完整target四元数赋给最终wrist，随后写回肩／肘／腕并核对残差。上述四文件与功能候选提交9e0ad18无差分。这证明代码链没有刻意删除roll，不能证明物理设备输出、游戏最终显示或手感已通过。

## 画面判断纠正

私有模拟预览反复重启后缩到1920×56，曾裁掉下方手部。恢复1920×900后已看到左右独立移动、右手转向与实际Lancer跟随。旧包README中“个别画面回原动画”的确定表述应结合本次纠正阅读：裁切图和pending计数不足以诊断最终画面缺陷；本次也未宣称每帧过渡稳定。封存ZIP未改，未晋升实验补丁，无第十包。

## 醒来后的三个动作

1. 同一入口进Playground，左右手分别举起、移动、转向和翻腕，观察第一人称手臂是否随各自控制器。
2. 暂停后转头，菜单应留在空间中；选择继续后应恢复游戏。
3. 有真实臂展记录后，右Grip在两侧肩后／裤兜取实际库存武器，再移回身前。缺测量则沿已有流程：双手平举、同时握持2秒后松开；自由手与第一人称不要求这次测量。库存需有相应武器，Lancer可按游戏正常方式拾取。

直接反馈哪一步、哪只手、实际看到什么即可；无需调试测试脚本。普通日志使用启动器Export logs，运行记录在J:/gearsVr/candidates/6dof-unified/analysis/runtime。M4基础开火已获授权并交付；在第三个动作取出Lancer后可加右扳机短按／松开观察开火与扣弹。换弹／掩体／其他枪继续暂缓，按当前四项具体反馈修正。

[总看板](</J:/gearsVr/docs/6dof-progress-dashboard.md>) · [完整画面证据与纠正](</J:/gearsVr/analysis/6dof-progress/solo-full-preview-correction-20261028.json>)
