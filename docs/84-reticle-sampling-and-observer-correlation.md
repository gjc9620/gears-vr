# 准星采样选择与观察任务的帧间隔关联

2026-10-17。本轮复用缓存与已保存运行证据，没有启动游戏、增加Hook、打开IDA或扩大资料库。

## 先选择确实绘制两部分的武器状态

通过kb.py查询knowledge.sqlite定位artifact2导出，再核对文件哈希。WarHUD.DrawWeaponCrosshairGOW3（SHA256 082c66d6ce62a562f645929c9f09e6e2cff0c5914362bd3a13afdc7b3fa5e4d0）主准星分支中，扩散时使用四次DrawIcon，之后调用Weap.DrawAdditionalCrosshair。WarWeapon基础附加函数在缓存中为空；Weap_AssaultRifle、Weap_LocustAssaultRifle附加函数绘制居中的纹理tile，其透明度依赖GetPlayerNormalizedAccuracy。

这解释了为什么“手枪可见外圈”不足以测试“独立内圈与外圈对齐”。定向索引没有找到手枪独立附加图标实现，但不把未找到当作运行时不存在的证明。导出含未解析native token和类型占位，不宣称完整语义还原或磁盘／当前运行代码相同。

复用Native36会话33008的GPU图3（ticket2113）裁出同一帧双眼中心区域，保存reticle49-rifle-crop.png：场景有步枪枪口火光、白色扩散外圈和近中央红色标记。两部分视觉上接近同心，但不能只凭颜色把红标记认定为DrawAdditionalCrosshair，也没有同ticket完整源纹理／颜色数据，故不补写“内外圈已验收”。此前记录的同ticket16顶点／4顶点payload映射一致仅覆盖那些保留记录，不能和ticket2113混作一帧。

下一次最小样本应使用已确认的步枪，在实际新图和至少10个epoch内提交之后才开始ADS；先捕获稳定开镜、再捕获早期开火，按实际保存时间配对可见中心点、外圈和GPU记录。开火后中心点可能变淡，不能把不可见中心点误判为绘制丢失。证据路径与四个导出文件哈希见analysis/runtime/reticle49-script-evidence.json，原有手枪问题仍开放。

## 周期观察与长帧不能仅凭耗时等同

已保存checkpoint45／46的worker观察一次约0.9–1.3秒。这说明它有CPU工作量，不足以证明造成头显卡顿。使用原始worker时间窗和实际新图提交qpc比较：仅纳入相邻ticket增量1–3、间隔0–150ms、无prepare重叠的记录；边界与其他区间排除。新图间隔完全位于一次成功observe内的归内组，完全不重叠observe的归外组。

| 会话 | 内／外样本数 | 内／外中位ms | 内／外95分位ms | 内／外超过50ms |
| --- | --- | --- | --- | --- |
| checkpoint45 | 917／2470 | 22.681／22.333 | 34.207／33.636 | 4／3 |
| checkpoint46 | 1132／2894 | 22.562／22.308 | 33.879／33.583 | 3／6 |

结果不足以支持显著长帧都集中在观察期间，不能把它称为主要卡顿根因，更不能据此取消世界变化检测。它也不证明观察无影响：比较非随机，游戏状态／负载混杂，过滤条件并不认证纯游玩，后段记录有截断；新图提交不是所有projection或GPU耗时，更不是Quest／VDXR测量。保存analysis/runtime/observe49-frame-association.json，包括最大值、排除数和限制。真实头显帧节奏仍需单独验证。
