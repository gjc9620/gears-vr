# DLSS/DLAA 运行时锁：实际设置调用链

2026-10-15。目标是首版运行期间禁止重新启用未经测试的DLSS/DLAA，同时保留其他设置的正常应用。启动INI关闭已存在；本记录的观察版尚未修改设置，不计为锁定完成。

## 实际游戏证据

`manifest-graphics-observer1.json`，PID46520，游戏SHA256 `bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4`，模块base `0x140000000`。静态对照来自PID16812历史运行时代码节，SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`；启动时三个入口各32字节重新校验，不把相同RVA当作版本证明。

观察版在Reserve阶段只建立未启用trampoline，进入continuous阶段才启用三个入口。每个原函数仍调用一次；collector保留原始RAX，不宣称恢复了完整C++返回类型。只记录固定上限32行/入口；命令192字节、vtable及DLSS字段只在原调用前复制，原调用后不再访问可能已退休的命令。默认宏`GEARS_GRAPHICS_LOCK_OBSERVER=OFF`。

| 入口 | 本次实际调用 | 已确认内容 |
| --- | --- | --- |
| 菜单RVA280da4 | 7次，caller return28047a，TID51592，mode0，EAX1 | 正常菜单Apply经过已知setter；未测试mode3 |
| collector RVA2ac894 | 27374次，首32行caller return1f9a9b，TID51592，TLSdepth1 | singleton=base+1da57f0，normal/alternate均0，原RAX为singleton |
| commit RVAff6f4 | 27374次，首32行caller return19f2f3，TID52244，TLSdepth0 | command vtable=base+1343588、DLSS dword+78=0，原EAX=c0；queueflag=1且selector_thread=52244 |

排队调用点前后32字节已保存，栈回到19f2f3→282b8d→282de3。TID51592/52244仅属于本进程，不能写死到下一版。collector/commit保留样本在前32帧即已满，因此不能称它们直接记录了后续每次Apply的payload。direct调用分支、开启DLSS请求和未知caller仍未实际覆盖。

测试通过正常手柄菜单，把各向异性过滤4x调到8x，应用并确认，再调回4x并确认。首次确认超时后界面仍显示暂存8x，随后重新Apply；这说明暂存标签不能代替生效值。抗锯齿一直为NONE、缩放NATIVE；退出后INI确认MaxAnisotropy=4、DLSS=False。停止后独立只读检查确认normal/alternate=0、ScreenPercentage/FINAL=100、缩放因子1。

会话运行406.420698秒，36422次projection、26848张场景新图，最大提交间隔333.701ms；这是含菜单切换/设置应用的运行，不声明无卡顿。正常停止、XR销毁、38入口恢复及游戏正常关闭。DLL SHA256 `ca8aaa94cafb8e8311ff1563d8836006ca8123ba0db86bb061e116c535cc364c`。首次启动在自测阶段因新构建缺FrameLinkTest等exe中止，尚未启动游戏；补建仅所需测试目标后成功，冻结DLL未变化。

证据：`analysis/graphics-lock/graphics-observer1-46520.audit.json`、`analysis/runtime/continuous-stereo-46520-20261015-021439-a72cff7f.*`、同目录`graphics-observer1-options-*`和`graphics-observer1-after-stop.json`。源码后续把collector TLS恢复改为RAII，并增加嵌套/异常恢复CPU自测；冻结Observer1不包含此收尾改进。

## 接下来的实际修改

结合排队dispatcher静态指令与真实线程选择字段，认证每次queued调用，复用现有graphics_lock/routes策略：在已认证menu拒绝mode3、collector仅清normal+3c8、commit仅清既有命令+78，保留原命令地址、其余payload、原collector/commit调用及返回值。任何身份或写入失败撤销锁有效声明并安全停止VR。

还需把锁的生命周期覆盖到进入关卡前的菜单，并实测用户重新选择DLSS/DLAA时生效字段持续为0、其他设置仍可应用。当前观察只覆盖已激活continuous会话，不能作为首版完整锁定。
