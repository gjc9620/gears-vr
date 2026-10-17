# Mounted 认证：局部重复读取与暂态错误分类

2026-10-17。针对[机枪真机反馈](120-v1-headset-mounted-feedback.md)的离线改动，不是实际机枪黑屏已修复的验收记录。

`MountedContextCertificate._capture` 在一次采集内多次遍历相同类的继承链、认证同一对象身份。现在仅在该次 `_capture` 内复用已发现身份和继承链；原有最终身份 fresh 读取、注册槽、父类指针、反射布局、链接、POV 和时间戳检查仍执行。下一次采集重新发现，不保留跨采集成功证明。`LiveRegistryAnchorSampler._strict_guard` 的完整前后字典比较保持不变，未删除时间戳或 POV 字段。

对同一 MountedFixture，单次采集读取从 1,598 次降为 473 次；含初始 guard 的 sampler 构造从 4,984 次降为 1,609 次。这只是合成内存 fixture 的读取成本，不是实际进程耗时或帧率数据。

仅当其他最终边界检查全部通过、时间戳本身有限且发生变化时，抛出 `MountedTimestampChanged`。采样器在原有五次／200ms 预算内重新尝试，不复用旧结果。身份、父类、字段布局、互反关系或 POV 变化仍拒绝；有效注册表头变化使用原有 `RegistryHeaderChanged` 并保存准确前后头信息，交给上层重新解析。没有扩大支持的载具类型。

协调者验证命令（cwd `tools/native-capture`）：

```text
python -m unittest test_mounted_context test_anchor_config test_cinematic_anchor test_world_rebind test_world_rebind_runtime
```

88 项通过。覆盖时间戳一次滚动后恢复、持续滚动达到尝试／时间限制、最终身份重复认证，以及时间戳同时伴随身份／父类／字段／链接／POV 损坏时保持拒绝。首轮单独导入 mounted 测试暴露测试路径初始化顺序问题，已修正 fixture 导入顺序后重新通过；不是通过增加重试掩盖该错误。

本轮未运行游戏、模拟器或真头显，未替换冻结 VRDof60/61 DLL，也未修改旧 ZIP。现有根目录 Python 会在下次启动时使用修改；已经结束的真机会话不是此修改的验证。

## 岔路 UI 新证据

同次用户会话保存的 `continuous-stereo-24696-20261017-150718-1be1dbc1.menu.jsonl` 第 14 条记录为序号 1670、observed_ns=82538877186100，顶层 `DeltaChoosePath / UIScene_PathChoice`，input_mode=4，mode=0，consistent_reread=true。这建立了本次实际场景身份，尚无对应绘制批次或几何范围，不直接修改 UObject 坐标或全局 HUD 偏移。后续定位应将该场景身份关联到公共 UIScene 绘制／裁剪，再整体调整同组元素。
