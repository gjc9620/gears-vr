# Native67 development candidate

2026-10-17. The repository launcher now selects Native67 after the ordinary-scene GPU result in docs139, the two checkpoint recoveries in docs140 and the pause/resume correction in docs141. Only `native_build`, `native_dll` and `native_sha256` changed in its manifest. Local player configuration and the Borderless1 XR executable are unchanged. This is a development selection, not V1 completion or Discord publication.

Native source is frozen at `f55f71313df32c96e588f658b7d4caf76eea2777`; DLL SHA256 is `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`. The build retains the previous feature profile and explicitly enables guarded final-transfer normalization. Root-scene and call-boundary observers remain OFF. The frozen normalization and DOF CPU tests passed with assertions enabled.

The preceding Native60 manifest is preserved at ignored `analysis/runtime/default-native60-before67.json`; its DLL/build remain available. The selection receipt is `native67-default-selection.json` in that directory. Git also preserves the previous manifest.

## Portable artifact

`analysis/release-zips/GearsVR-V1-development-Vrdof67-TransferPause-English-20261017.zip`

- SHA256: `0a0b534152716fea98ccc7f1dc655f69fba886a6c21ef899b7b48333b685d067`
- 13,684,546 bytes; 702 archive members.
- Both new staging and fresh extraction passed isolated bundled-Python preflight; every payload digest and the inventory digest matched.
- English remains the first-run language, with Chinese selectable. Players select their own game EXE and OpenXR runtime. No game, simulator, reverse-engineering database, snapshot or pseudocode is bundled.
- Game resolution remains authoritative, DLSS/DLAA remain disabled, and physical background controller input stays disabled. Local camera/HUD preferences were not imported into the portable defaults.

Packaging did not launch the game or simulator. The underlying native candidate has simulator evidence; the freshly extracted portable artifact itself has not undergone gameplay or headset acceptance. Existing ZIP hashes were checked and preserved. The ignored full package receipt is `analysis/runtime/vrdof67-transferpause-english-20261017-package-receipt.json`.

Mounted-machine-gun output, PathChoice placement, recoil/spread-ring alignment, general UI coverage, arbitrary resolution transitions, headset pacing and extended play remain open. The artifact retains `headset_verified=false` and `release_ready=false`.

---

## 中文

# Native67 发展候选人

2026-10-17。现在,寄存器启动程序将选择原生67 普通场景 GPU 生成文件后139,两个检查站的回收文件140 和文档中的暂停/重现更正141 仅选择 `native_build`, `native_dll` and `native_sha256` 更改其列表中的内容。本地玩家配置和无边框1 XR 可执行文件不变。 这是开发选择, 不是 V1 完成或Discord 出版物。

原生源被冻结在 `f55f71313df32c96e588f658b7d4caf76eea2777`? ; DLL SHA (美国英语).256 is `74ddd522acac38e352498a8f40fc6dc4968a4b3256cb962770f3c914f5a26c13`。该建筑保留了以前的特征,并明确允许谨慎的最终移交正常化。根幕和呼叫边界观察员仍然关闭。冻结的正常化和DOF CPU测试通过,并启用了断言。

上一个土著60 显示保存在被忽略的状态 `analysis/runtime/default-native60-before67.json`;其 DLL/ 构建仍然可用。选择收据是 `native67-default-selection.json` 。 Git 也保存了上一个列表 。

## 便携式文物

`analysis/release-zips/GearsVR-V1-development-Vrdof67-TransferPause-English-20261017.zip`

- SHA256: `0a0b534152716fea98ccc7f1dc655f69fba886a6c21ef899b7b48333b685d067`
- 13,684,546 bytes; 702 存档成员。
- 新的装配和新鲜提取都经过了孤立的捆绑-蟒蛇飞行前;每个有效载荷文摘和清单文摘都相符。
- 英语仍然是第一流语言,中文可以选择,玩家选择自己的游戏EXE和OpenXR运行时间,没有游戏,模拟器,逆向工程数据库,快照或伪码捆绑.
- 游戏解析度仍然具有权威性,DLSS/DLAA仍然禁用,物理背景控制器输入则仍然禁用. 本地相机/HUD偏好没有导入到便携式默认中.

包装没有推出游戏或模拟器。 基本本地候选人有模拟证据; 刚提取的便携式文物本身没有经过游戏或耳机的接受。 已检查并保存了现有的 ZIP 散列。 忽略的散列器 完整软件包收件方式 `analysis/runtime/vrdof67-transferpause-english-20261017-package-receipt.json`.

挂载式机枪输出、PathChoice定位、后坐力/扩展环对齐、通用UI覆盖、任意解析过渡、耳机间隔和扩展播放仍然开放。 `headset_verified=false` and `release_ready=false`.
