# 枪体组件变换的可恢复租约候选

此记录属于 `codex/weapon-6dof`。当前为纯C++组件及CPU验证，未接入游戏writer，不授予公开手部或枪体写权限。它是后续枪体随手移动与原版收枪／拾取／近战交接的前置组件。

旧 `vr_weapon_mesh_follow.inl` 会修改 `Weapon.Mesh` 的absolute标志、Translation及Rotation；stow journal则管理人物Mesh.Attachments中的44字节附件姿态。两个位置不是同一份状态。h的双手与附件恢复通过，不能证明旧mesh-follow的组件变换也恢复；旧adapter的地址存活检查也不能证明完整对象代次。因此暂不解除combined与旧mesh-follow的互斥。

## 接口与边界

`tools/vr-common/weapon_mesh_component_lease.h` 提供 `WeaponMeshComponentLease<Key>`，操作序列为 `acquire(expected_key, wanted, backend)`、`restore(backend)`，以及仅clean、非重入时显式 `reset()`。它精确保存三组字段：一次SetAbsolute所使用的三个flags整体、Translation原始12字节、Rotation原始12字节。Absolute不是三个可分别重写的字段，以免恢复某个flag时覆盖同次调用中的foreign flag。

backend必须用完整对象代次及调用上下文认证Key；每个setter在实际调用前再次核对Key和完整expected Value，只能改变自己的组，必须以bool报告错误。false仍可能已经写入，journal在调用前记pending，调用后重新认证读回。它不自行验证游戏线程、动作、GHW1或源时间；未来adapter须在这些字段读写之外提供当前姿态与动作授权。backend若会抛异常、改变未声明的其它字段，或无法进行写前重验，不能直接接入。

组内撕裂或foreign值会撤销该组写权、保留dirty；其它仍owned组可以独立恢复，不覆盖foreign组。Key未知、地址复用、known=false或读失败会撤销全部剩余写权；旧owned值后来重现也不会重新授予写权。撤权组只有在同完整Key下只读观察到精确original时才能清账。`restore()==true` 只表示journaled组已清，不证明never-owned的foreign字段，也不清除sticky fault。父session遇任意fault必须结束，不能用reset静默重开。不可观察的foreign写入后又改回的ABA仍不在本组件证明范围内。

## CPU验证与审阅

独立VS18 x64 Release以 `/W4 /WX /EHsc /UNDEBUG` 构建，修正版268项断言检查和CTest1/1通过，涵盖false前后产生效果、无效成功返回、组内撕裂、foreign组独立恢复、身份丢失、地址复用、写前漂移、重入及精确字节恢复。Astra发现初版在复制wanted前清理自身journal，导致把 `owned_value()` 或 `original_value()` 传回acquire时目标变零；现已先复制desired再清理。新增两getter自别名回归使用不同的非零新backend基线，核对目标、journal及最终原值；在ignored副本恢复旧顺序后，两条回归分别rc1捕获旧bug。Astra再次只读复核问题闭合。

修正版header SHA256 `7116b49ac7442a3288f06083313ba3bbf78762eda6fa92e3ce10294748588bdd`，test SHA256 `7ad9ef71675b16c68453be24d1f5157d8317fd74905f31bd32d11643bb6f3a94`。私有回执 `analysis/weapon-mesh-component-lease-20261023/getter-alias-fix/verification.json`，SHA256 `c3454567b689f8ad65d0e6b43a2b396b78befdf82e50ee11116fff0b5295154d`。初版256检查的72个历史文件及原回执保留，没有把修正结果覆盖到初版。

项目CMake另加 `WeaponMeshComponentLeaseTest` 两行目标定义，在全新目录重建持续ON DLL和该测试目标；flags与C1 trial逐项相同，新目标只执行一次，268 checks通过。9份源码before／after快照及10份记录hash已独立核对，旧C1三DLL和旧报告未变；没有重跑旧CPU或OFF／default构建。集成报告为 `analysis/hand-continuous-session-integrated-20261023/report.json`，SHA256 `ac7f10c2c2a6c386b343b9730ccc769d9be9f59b970fe2a0b72bb4efe70b4f59`；当前CMake `de450edc…` 机械移除新增目标两行可精确重构旧 `86dd65f2…`。新持续候选DLL为 `425f6868aef6d05b24ce153172386c8bb15f658348ad6950f4867f23443e8a0a`；它尚未跑游戏，不替换既有h验收身份。

后续最小运行切片是固定Lancer实例、phase1右手枪体：消费与handstream同一张认证GHW1票据，先取得组件租约再重新认证手部写入；原Tick、动作和stow helper前必须同步交还手部及组件两个journal。phase0／稳定phase2不写枪体。需要同局完整身份、原组件三组值与附件44字节基线，以及实际SetAbsolute／Translation／Rotation行为的证据。当前尚未实现这条adapter、左手副握、射线或正式武器切换，CPU检查不能作为游戏／GPU／头显验收。
