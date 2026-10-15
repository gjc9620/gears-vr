# 检查点恢复修正记录

2026-10-15。优先顺序保持：①同进程检查点恢复与首次接入延迟；②手枪准星；③HUD尺寸/位置；④CG与游玩黑屏。当前未完成第一版验收。

## 已实施

- LOCAL参考空间变化不再发布空样本并退出持续会话。按OpenXR `changeTime`生效后更新空间代次；原生相机重新建立参考，接收端拒绝旧代次图像。只有新代次成功`xrEndFrame`才记录恢复。
- 相机就绪检查与原生anchor检查统一；激活前相机暂时切换时只重取完整读取证书，不重试已发生的注入或激活。保留取消、身份、签名和时间上限。
- 启动报告区分XR预检、相机等待、原生准备、running状态和实际第一张projection。
- 协议配套版本：StereoBridge 3、GPU共享包4。新二进制必须配套使用；`manifest-game-recovery1.json`是本轮实验，玩家默认仍保留原版。

## 本轮证据

本地16个验证命令通过（原生门控/桥接、生命周期、OpenXR CPU与共享映射检查、Python launcher/runner/menu监测）；准备重取专项12项通过。无真头显验收。

实际游戏PID10940，运行记录`analysis/runtime/continuous-stereo-10940-20261015-171139-11827aa4.*`：两次相机非Pawn拒绝后自动恢复准备；XR预检0.817秒、菜单/相机等待17.682秒、原生准备15.278秒、首次双眼提交距runner开始36.776秒。累计10099次图像复制、19813次projection提交，之后停更。

已用游戏状态读取和定向窗口消息打开暂停菜单，再确认`ConfirmCheckPointLoad`，在同一PID重载检查点。测试未通过：native继续running、anchor拒绝0，但新图像没有恢复。

冻结日志确认另一个直接缺陷：swapchain首次从3840×2160变1920×1080时，buffer对象也重建。库存观察器永久rejected，`changed=10918`是后续拒绝次数，非resize次数；共享纹理和XR眼纹理仍只支持初始大小。不能以进程运行或模拟器FOCUSED宣称画面恢复。退出时native资源排空、hook恢复、XR退出完成。

## 下一步

在DXGI resize边界完成缓冲区代次更换、旧票据退休、共享GPU资源握手与XR眼纹理重建，再重复同进程检查点测试。尚未证明VDXR原点事件在真实头显上恢复；第二至四项修复与视觉验收仍待完成。

参考：[OpenXR参考空间变化事件](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrEventDataReferenceSpaceChangePending.html)、[DXGI ResizeBuffers资源释放要求](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-resizebuffers)。
