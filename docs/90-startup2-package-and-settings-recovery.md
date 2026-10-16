# Startup2包与设置菜单返回回归

2026-10-17。上一轮启动解析合并已纳入新本地包；不是正式发布。

`analysis/release-staging/reticle39-startup2-20261017` 隔离预检通过，700个清单文件、43,421,471字节。仅按清单生成ZIP `analysis/release-zips/GearsVR-V1-development-Reticle39-startup2-20261017.zip`，701成员、13,634,871字节，SHA256 `d84b671f89a44d8ce595f8bf2b227bfae38961e5a3986fe80fced56d18f50df1`。旧ZIP保留未改。

## 实际运行

包内Python -I/-B启动包内launcher／Reticle39／Borderless1，使用包外游戏与显式模拟器。会话continuous-stereo-11080-20261017-072757-7e81f5b4；PID11080不能与旧portable55同PID混同，完整创建身份保存在会话及portable57-actual-runtime-audit.json。

准备2992毫秒，相机就绪至实际首投影5997毫秒。与上轮新代码3279／6282毫秒同向，但两轮不能建立稳定性能保证。之后一次死亡菜单读取检查点，再暂停进入图形设置；所有导航均校验实际UI身份／焦点，无截图定位、无内存修改。

设置前记录阴影选项CurrentIndex=3，Left之后为1。首次确认对话超时返回设置，保留原始失败记录。再次变更并用精确ConfirmVideoSettings语义及两次一致读取保护的Enter确认。INI持久变化仅确认ALLOWD3D10=True→False、DEFAULTSETTING=4→5；阴影实际GPU参数变化未被证明，不能称完整画质切换／Ultra压力测试。

返回关卡后申请新GPU图像，严格要求snapshot_index和source_serial超过申请前回执、saved_monotonic_ns不早于申请。新回执ticket16175／source15393，申请QPC55162988267400，保存55163048912400，3840×2160，两眼均非黑场景。它是设置应用及返回后的新图像，不是旧截图。普通菜单／恢复和此次有限设置应用未终止XR；仍不证明历史黑屏全部修复。

正常停止、原生清理、XR退出、游戏普通关闭通过；700清单文件哈希未变。游戏关闭后INI逐字节恢复测试前备份，SHA256 `85620109d28bd0da07c7aa3081fcba45d17ac454f6607ddfb694223e556227dd`。恢复结果以portable57-actual-runtime-audit.json为准；不留下测试画质给用户。

## 同身份恢复审查

world_rebind_monitor在完整snapshot相同且native world_handoff_phase不为4时不重建世界证书；拒绝计数也不直接触发。普通HUD／inventory已有ui_runtime_binding逐调用重新获取，武器更换有rebind_weapon，提交后新鲜性失败进入phase4则允许同基准重试。定向源码审查没有找到已支持的依赖变化导致永久卡死的具体反例，因此本轮没有增加猜测性的重连或用拒绝次数绕过认证。

实际机枪／全部载具、更多过场、长期游玩和真头显卡顿／准星融合仍未验收；新包release_ready=false。
