# 机枪接近与准星裁剪一致性

2026-10-17。第一版仍未完成；本轮实际接近没有占有机枪。

## 实际导航

Caption38会话 `continuous-stereo-28480-20261017-070418-89b899ac`，游戏PID28480／creation134366654448946138。只读扫描找到两台无Driver的Troika，逐角色身份及字段重读稳定；扫描保留6项发现错误，不能当完整世界快照。坐标只作导航提示，没有修改游戏内存、瞬移或修改存档。

首次运行导航时已处于死亡菜单，未进行移动。确认菜单焦点后，一次选择并读取检查点，再发送一次数字方向键，进行有限的转向和前进。POV yaw从20373变为22562，再到25595 RU，朝向误差由约33.63°缩至5.39°，确认这次VR下注入后的虚拟转向生效。

两次前进观测约1261及475游戏单位，随后一次前进仅约1单位，脚本按门禁停止。640×360窗口图显示人物处在掩体后；这不等于导航路径已找到，也不能据此断言游戏移动故障。没有发送机枪交互，没有Controller／Turret／Driver占有证据，因此机枪实际双目兼容性仍开放。

正常停止VR后，原生清理与游戏普通关闭通过。证据位于ignored `analysis/runtime/mounted53-*`，包括两次导航审计、菜单动作回执、只读扫描及 `mounted53-normal-close.json`。普通关卡图像不能替代上机枪后的GPU图像。

## 准星裁剪的代码级问题

对当前legacy UI路径的定向审查发现：有限距离准星规划若因某眼变换后的scissor完全被裁掉而返回失败，调用者会尝试无限距离规划。同一组、同一几何的不同片段可能仅因scissor不同而采用不同投影。

这是可构造的数学反例。现有实际手枪／步枪记录的有限投影回退计数为0，不能把它认定为用户外圈偏移的根因。修正应保留有限投影及合法空裁剪，不通过改变投影让原本被裁掉的内容重新可见。原始非法状态与无效几何仍须拒绝。

已修正规划器：变换后交集为空时保留原投影，用眼内零面积scissor继续；不扩大可见范围。回归用例在修正前失败，修正后重新编译并运行UiReticleDirectionPlanTest通过；覆盖左右眼分别为空、上下两眼为空、replay转发与恢复、整数矩形转换，以及第二眼几何非法和原始非法裁剪仍拒绝。协调器独立重运行该测试通过。另尝试旧布局构建目录的GpuViewportStatsTest时该可执行文件不存在，未记为通过。

源码回归、冻结DLL及实际游戏验证分别记录；默认Caption38与已生成测试ZIP不因源码修改自动更新。

## 独立候选构建

源码提交 `ab91ecc` 导出为 `analysis/native-source-reticle39`；MinHook逐字节复用Caption38冻结副本，保留同样的构建选项，排除工作区未提交的UI旁路。全部137个构建步骤完成，DLL为 `tools/native-capture/build-reticle39/GearsCaptureReticle39.dll`，SHA256 `f1865f70c0f50ed367e28e7fb2800ee09f7231a13ddb08ea8c6bafae73f60dbb`。

19项相关自测通过，包括准星、viewport replay、HUD布局、UI收据／索引／材质、mounted、FrameLink和DLL continuous／frame-link ABI。首次MountedObserverTest失败是执行目录没有mounted.ini／memory测试夹具；改用既有 `analysis/tests/mounted-stream-levels` 后通过，未改测试或降低断言。构建、源码逐文件哈希和自测结果分别保存在reticle39-build.log、reticle39-source.json、reticle39-selftests.json。

实际游戏回归使用显式实验清单 `analysis/runtime/manifest-reticle39.json`；这不修改默认清单、玩家配置或之前的ZIP。

## 实际检查点回归与采用

Reticle39会话 `continuous-stereo-8828-20261017-071258-2e45ae03`，游戏PID8828／creation134366659655727649。启动准备3795毫秒，相机就绪至实际首投影6771毫秒。连续两次死亡菜单读取检查点，world generation依次1→2→3；从实际按键发送到脚本观察到新代至少10次提交分别7487、7937毫秒。该时间包括加载、认证和脚本观测，不等于全程黑屏时间。

四份3840×2160 GPU图像回执的ticket为279／2233／2997／4014，source serial为277／2228／2988／4000。每眼均有非黑场景，恢复图已目视确认。正常停止后原生清理、XR退出和游戏普通关闭通过，回执 `reticle54-audit.json`、`reticle54-normal-close.json`。这没有验证特殊载具、头显融合或持续开火期间用户所说的外圈偏移。

完成上述回归后，本地默认改为 `manifest-reticle39-borderless-vd.json`。Caption38命名清单与此前测试ZIP保留回退，玩家配置未覆盖。新源码默认不改变此前ZIP内容；发布仍须真机和剩余场景验收。
