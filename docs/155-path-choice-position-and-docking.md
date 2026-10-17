# 岔路 UI 的实际位置与停靠关系

2026-10-18。`50cbfcd` 增加默认不读取的布局细节选项。本轮在冻结 Native70 的实际游戏中取得 `DeltaChoosePath / UIScene_PathChoice` 的完整13节点树及 Position、DockTargets 元数据和值，保留 `read_only=true`、`consistent_reread=true`、23,799项复读验证。它是顺序CPU采样，不是原子快照或GPU裁剪证明。

证据保存在 ignored `analysis/runtime/ui-layout-details7-20261018.json` 的 `path_layout_details`。导航只向自有 HWND 发送已确认绑定的鼠标消息；达到碰撞后停止移动，等待关卡脚本显示岔路菜单。测试结束正常关闭自有游戏，native cleanup、设置与四个profile恢复通过。未启用旧FCanvas平移实验或修改UI位置。

## 当前数据支持的下一步

- 场景下有 SafeRegionPanel 和另一个直接子 `UIImage`；10个已识别岔路内容节点均是 SafeRegionPanel 的直接子。因此移动整个场景会同时影响额外图片，移动SafePanel是范围更准确的候选。
- SafePanel四面的存储单位均为脚本枚举值4（PercentageViewport），无停靠目标。Top约0.0374、Bottom约0.9626，对应实际bounds y约40.392..1039.608。四面比例共同支持UI坐标域1920×1080，不能用Windows窗口的2534×1009替代。
- 10个内容节点的Y位置/高度存储单位均为5（PercentageOwner）。BottomRightBackground的X停靠左背景的Right；Y无停靠目标。该关系支持父级平移的实验假设，仍不证明实际布局及裁剪会随之移动。
- Right/Bottom的原始Value在不同单位下可能表示相对尺寸或端点；不能把所有节点的四个原始数一律当作绝对坐标。右箭头还有有意保留的负宽度，不能归一化后回写。

最小下一实验是在准确认证的游戏线程调用原生 `UIScreenObject.SetPosition`，仅对SafePanel的Top/Bottom作等量小位移，跨正常布局帧观察父/子bounds，再恢复；不在Render调用返回时立即恢复。已有历史实现支持InputType=0保留原存储单位并触发正常布局失效通知，可避免先猜测Getter的原点/单位转换。但调用前仍须认证当前完整函数、实际对象和相关虚函数，实验之后还要核对GPU裁剪及输入区域。

本轮没有调用GetPosition/SetPosition、写游戏内存或交付UI修复。正常双目、额外布局读取、父级平移后的视觉结果及真头显体验是不同验收项。
