# Mounted Turret 的流式 Level 证书

2026-10-17。只修改源码和合成测试；未操作游戏、模拟器、输入、焦点或 IDA，未替换冻结 Native36。

实际只读证据 `analysis/runtime/mounted36-gameplay-live-scout.json` 中，两台注册的非 Default Turret_TroikaCabal 与本地 WarPC_1 共用反射 Actor.WorldInfo，但 Turret 的直接 Level 与 WorldInfo/PC 的 Level 不同。两台 Driver/Controller 均为空，本地 PC.Pawn 也为空；这只能说明旧版同 Level 限制过严，不能算真实上机枪或相机/GPU/头显验收。Level 的流式关系仍是推断，没有读取未知 Level 结构偏移。

schema3 保持原有字段和 INI 格式：role_level 仍是受控 Turret 的直接 Level。证书分别保存并反复校验 Turret、Driver、Controller、Camera、WorldInfo 的直接 Level 身份；每一个必须是注册的非 Default 对象，具有经 Class 身份和 Super 链认证的 Level 类型，并与受控 Level 的实际类相同。不同 Level 可以共存，但 Controller、Turret、Driver、Camera 仍须通过经过属性身份、所属类和布局认证的 Actor.WorldInfo 指向同一 WorldInfo。没有猜测 Level.WorldInfo 或其他偏移。

原有 Turret/Vehicle 类型、Driver 的 WarPawn 类型、Driver/DrivenVehicle、Controller/Pawn 互反关系、Driver.Controller 为空、背包所属 Driver、相机目标等于受控 Turret、POV 和时间戳括号继续生效。各 Level 放入现有身份表，原生 parser 拒绝缺少 Level 身份或错误 Level 类型的 mounted 配置，运行时 guard 重读所有身份和注册表槽。schema3 叠加 cinematic 时，只允许同一已认证 mounted WorldInfo 具有不同 Level；普通 schema1/2 行为不变。

自动重绑定快照现在还包含五个 Level 的独立身份，因此 Actor 指针不变、Level 地址不变但 Level 身份变化也会要求重新准备/提交证书。UI 继续从人类 Driver 读取背包；准星查询暂停/下机恢复协议不变。

验证：116 项 Python 测试通过，覆盖 mounted/cinematic/anchor/UI、重绑定准备及监控。独立 CPU 构建位于 ignored `analysis/tests/mounted-stream-levels/build`；MountedObserverTest 170 检查通过，AnchorConfigLoaderTest 124 检查通过，AnchorObserverTest、UiObserverTest、WorldRebindMetadataTest 通过。跨 Python→INI→原生的保存内存合成 fixture 同时覆盖单 Level、五个不同 Level、cinematic 组合和正常下机；反例包括错误 WorldInfo、错误/Default/缺失/未注册 Level、每个 Level 身份变化及读取中变化。此结果不建立实际游戏 mounted 验收。
