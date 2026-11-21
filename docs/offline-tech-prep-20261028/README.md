# Total transfer of previous technical programmes

2026-10-28. Scope includes 12 The physical interaction of weapons, model handling/compensation, original behavioral portals, and bunker interactions, collisions, body, attack and weapon co-location. Currently only technical programs and offline assets are prepared, without modification to the production of C++, game configuration, construction or candidate packages, without starting games/XR/simulators and without increasing the number of operators.

| Access | Contents |
| --- | --- |
| [Weapons programme](../weapon-specs-20261028/README.md) | 12 A specification card, original/new model, original function, gun-by-gun modification of task |
| [Shelter programme](../cover-specs-20261028/README.md) | 19 Item, call/submission order,20 A receiving scene. |
| [Common rules for the two sets of programmes](shared-contracts.md) | Same input attribution, active cancellation, bunker and drop speed, life cycle and door-to-door control |
| [Technical border at the asset back end](asset-backend.md) | Positioned original creation/surveillance/release chain, input protocol and access certification responsibilities |
| [Mark the handover form.](calibration-plan.json) | 12 Targeted objects, data fields, coordinates and failed processing of weapons; undesired data do not fill false values |
| [Complete request check](requirements-audit.md) | Current evidence, completion level and access phase limits for each original requirement |

Use current unified checkout:`C:/Users/11/.codex/worktrees/6dof-single/gearsVr`. Mainline M 4 Still advance independently, the line numbers in the old index are only survey snapshots. Future work begins by comparing changes in main lines with symbols; no old workktree is restored, no old module switches are restored and no Python-specific reference model of this package is connected to the game.

## Fixed order of implementation

1. Re-use the main line body/input/replace ownership; implement public keys, action cancellations and cross-modular door controls of this package, expanding already purely logical.
2. Completion of the life cycle of original inventory services and original components; initial validation of original asset resource creation capacity, separately, not matched 12 Weapons of all kinds are developed separately for loaders.
3. Physical exchange of ammunition, original inventory and mechanical endorsement along the existing Lancer chain, followed by weapon package P 3–P 6 Access to other guns and throwing.
4. The bunkers follow their I.1–I 6 Implements context, collisions, input submission, C 0, action/hit and gun-student; public interface reuse 1 Step. The prototype can be developed in modules, but the shared implementation end is integrated by the same achiever sequence.
5. Only the modified checks are performed at each stage and the short basic operations that were approved at that time; source code, compilation, game action, eyes and entity header feedback are recorded separately.

This order does not require the selection of keys, mechanical steps or model lists. The conditions that involve the internal memory/call of the current process, collision assurance, original resource creation and actual hit are required to obtain a genuine certificate in the code; no previously declared function is available. Specific technical limitations are found on topics without documentation completing numerical replacements for running evidence.

## Offline Packet & Recapture

Delivery to ZIP: Workspace `analysis/technical-prep-20261028/gearsvr-offline-tech-prep-20261028.zip`. Keep in the package docs/assets/tools目录结构, document by SHA 256 List (numbers are given in boxage-manifest.json). Original game extract, original script/native evidence, analytical database and production of C++ are not in ZIP; the link to these evidence in the document still points to the workspace and cannot be mistook as a complete independent game source package.

The following four pure asset commands can be run from the decomposition base directory; only the Python Standard Library is used, no game/XR starts and no main line changes are made:

```powershell
py -3 -X utf8 tools/weapon-models/offline-v2/build_offline_assets.py --check
py -3 -X utf8 tools/weapon-models/offline-v2/validate_offline_assets.py
py -3 -X utf8 tools/weapon-models/offline-v2/build_engine_inputs.py
py -3 -X utf8 tools/weapon-models/offline-v2/validate_engine_inputs.py
```

The first check is that the duplicated files are consistent with the author's source; the second rereads the actual GLB and examines the pouncer, LoD, Six-Hongway and ballistic co-operation. The last two reconstructed and checked primary vertex inputs, containing four new models, and three parts of the starter. All results do not prove that the engine actually loaded or the physical appearance.01 The static technology route has been closed; this package does not automatically access the main line.

---

## 中文

# 前期技术方案总交接

2026-10-28。范围包括12种武器的物理交互、模型处理／补件、原版行为接入口，以及掩体的交互、碰撞、身体、受击和武器合流。当前只准备技术方案和离线资产，不修改生产C++、游戏配置、构建或候选包，不启动游戏／XR／模拟器，不增加运行者。

| 入口 | 内容 |
| --- | --- |
| [武器方案](../weapon-specs-20261028/README.md) | 12张规格卡、原模型／新模型、原版函数、逐枪修改任务 |
| [掩体方案](../cover-specs-20261028/README.md) | 19项问题、调用／提交顺序、20个验收场景 |
| [两套方案的共同规则](shared-contracts.md) | 同一次输入归属、主动取消、掩体与投掷速度、生命周期与开火门控 |
| [资产后端技术边界](asset-backend.md) | 已定位的原生创建／保活／释放链、输入协议与接入验证责任 |
| [标定交接表](calibration-plan.json) | 12种武器的标定对象、数据字段、坐标与失败处理；未实测数据不填假值 |
| [完整要求核对](requirements-audit.md) | 每项原始要求对应的当前证据、完成层级与接入阶段限制 |

实现位置使用当前统一checkout：`C:/Users/11/.codex/worktrees/6dof-single/gearsVr`。主线M4仍独立推进，旧索引中的行号只是调查快照。未来开工先按符号比较主线变化；不还原旧worktree、不恢复旧模块开关，也不把本包的Python规格参考模型接入游戏。

## 已固定的实施顺序

1. 复用主线身体／输入／原调用所有权；落实本包的公共键、动作取消和跨模块门控，扩展已有纯逻辑。
2. 完成原库存事务与原件组件生命周期；原创资产资源创建能力单独做首件验证，不与12种武器分别开发加载器。
3. 沿现有Lancer链完成物理换弹、原库存与机械回执，再按武器包P3–P6接入其他枪和投掷。
4. 掩体沿自己的I1–I6落实context、碰撞、输入提交、C0、动作／受击和枪口合流；公共接口复用第1步。原型可以分模块开发，但共享执行端仍由同一实现者顺序集成。
5. 各阶段只做受改动影响的检查与届时获准的短基本运行；源码、编译、游戏动作、双眼和实体头显反馈分开记录。

此顺序不要求重选键位、机械步骤或模型名单。涉及当前进程内存／调用ABI、碰撞并发保证、原创资源创建和实际命中的条件需要在编码中取得真实证书；未取得前不对外宣称功能可用。具体技术限制见各专题，不用文档完成数替代运行证据。

## 离线包与复现

交付ZIP：工作区 `analysis/technical-prep-20261028/gearsvr-offline-tech-prep-20261028.zip`。包内保留docs/assets/tools目录结构、逐文件SHA256清单（数量见 package-manifest.json）。原游戏提取物、原脚本／native证据、分析数据库和生产C++不在ZIP内；文档中指向这些证据的链接仍指向工作区，不能将ZIP误称独立完整游戏源码包。

从解压根目录可运行下面四条纯资产命令；只使用Python标准库，不启动游戏／XR，也不修改主线：

```powershell
py -3 -X utf8 tools/weapon-models/offline-v2/build_offline_assets.py --check
py -3 -X utf8 tools/weapon-models/offline-v2/validate_offline_assets.py
py -3 -X utf8 tools/weapon-models/offline-v2/build_engine_inputs.py
py -3 -X utf8 tools/weapon-models/offline-v2/validate_engine_inputs.py
```

第一条检查可重复生成的文件与作者源一致；第二条重新读取实际GLB并检查拓扑、LOD、六孔通路和弹体配合。后两条重建并核对原生顶点输入，包含四件新模型的双LOD和启动件三部件。所有结果均不证明引擎实际加载或实体头显观感。前期交接已完成，N01的静态技术路线已经闭合；本包不会自动接入主线。
