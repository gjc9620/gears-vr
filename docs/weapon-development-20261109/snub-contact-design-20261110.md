# Snub 接触点与手模型的设计依据（2026-11-10）

当前阶段仍是完整逻辑验证。Game17844 已到达空仓新匣入座、未上膛拒射，随后套筒抓取未成功；完整逻辑、体验、GrabPose 和实际头显验收均未完成。遵守主人暂停自动运行的要求，本次只核对资料、既有源码和离线证据，没有启动游戏或模拟器。

## 已核对的公开资料

- [VR Expansion Plugin：Hand Socket Components](https://vreue4.com/documentation?section=hand-socket-components)。作者把手模型的相对摆放、抓握 socket 和动画姿势分别描述，并提供单独修改模型摆放或抓握变换的选项。可借鉴的是明确各变换的职责；该文档不提供本项目角色的固定骨骼地标，也不证明任何接触半径或手枪机构已经正确。
- [Unity XR Interaction Toolkit 3.2.2：XR Grab Interactable](https://docs.unity3d.com/Packages/com.unity.xr.interaction.toolkit@3.2/manual/xr-grab-interactable.html)。Attach Transform 定义附着点；Dynamic Attach 可依据选中时的交互器姿态建立附着关系，位置和旋转分别可匹配，Snap To Collider Volume 可限制该动态附着点所在范围。这支持把附着规则显式表达，不能把动态附着等同于机构已动作。

以上是可复核的通用 VR 实现参考，不是对 Alyx、Pavlov 或其他商业游戏内部实现的逆向结论。原设计文档中的商业产品介绍仍仅支持直接交互的方向，不作为具体按键、库存、弹膛或套筒规则的证据。

## 应用于本项目的判断

下面是根据资料和本项目源码作出的工程取舍，均需由本项目证据独立验证。

| 对象 | 本项目含义 | 验证方式 |
| --- | --- | --- |
| 控制器 grip 位姿 | 原 Source1 模拟输入，经既有坐标映射进入系统 | 原来源、时间、身份与当前帧校验；不是物理头显证据 |
| 固定角色腕点 | 原 Native grip→wrist 映射产生的角色腕点 | 原 C++ mount 前向计算；不能直接替代指间接触点 |
| physical precision 点 | 固定模板腕部坐标系中的真实动作地标 | 原 C0/物理观察及实际候选许可；离线求逆不能授予许可 |
| 部件接触与行程 | 当前枪、当前套筒的闭合接触和运动轴 | 原 stroke 捕获、同一租约、实际原骨满行程、松开及闭合确认 |
| 手模型及 GrabPose | 让皮肤贴合当前持握部件的视觉表现 | 后续全角度画面与实际头显；姿势数值相符不代表机构完成 |

Game17844 的原脚本将腕原点放在套筒骨位置，运行时 precision 点到原闭合接触约 109.83 mm，超过原 45 mm 捕获范围。原 `stroke.begin` 因而拒绝；后续要求先松手的状态不应通过放宽租约来消除。

私有输入修正使用原固定腕映射和 precision 地标规划真实控制器位置，同时保留原 Native 65 mm 候选范围、原 physical 45 mm 捕获范围。离线规划自行使用更小的 55/35 mm 目标余量，无可靠交集则在发输入前拒绝；弹匣输入规划及产品源码不改。这是输入规划的修正，不是运行时吸附、皮肤补偿或接触许可。

原 C++ mount、physical contact 和 stroke 算术核的 243 种条件中，189 种可行、54 种被规划器拒绝；最大 precision 误差 33.01 mm，旧规划重现 109.8328 mm 偏差并被原核拒绝。该检查不证明实际 Native grant、实际 C0、真实套筒运动或手感。详见 `analysis/snub-20261109/precision-input-offline-test.json` 和 `analysis/snub-20261109/reload-17844-review.json`。

进一步用 Game17844 保存的实际 root 矩阵完成原 charging 路径的逆旋转及 `root_scale × units` 换算，原 C++ mount 输出的修正 precision 点在枪身局部坐标中距闭合接触 15.4062 mm，原 stroke 接受；旧输入误差 109.8327 mm，原 stroke 拒绝。保存的 root 三轴比例均约 0.9999997，本项不覆盖任意 root 比例。原 stroke 同时确认：只有手部请求拉至末端、没有原骨 full 确认时，仍停留在 waiting_open，不能完成。测试使用的 kernel header 与当前私有 PG01e6 源码一致，产品和已准备的驱动均未修改。详见 `analysis/snub-20261109/saved-root-contact-offline-test.json`；这仍是旧观察上的离线算术，不是新鲜帧、实际租约或游戏机构验收。

## 下一次允许运行后的验收范围

主人明确恢复运行后，先按届时有效的独立实例入口和后台要求准备，再验证本次修正。完整逻辑仍须实际完成套筒接触、拉动、原骨满行程、真实松开、闭合确认及返回射击；任何一次失败都保留首个实际拒绝并正常清理，不自动重抓补记成功。

完整逻辑通过后，才进入接触 Volume、Snap、容错及反馈体验，最后精修各部件 GrabPose。公开框架的功能不作为绕过这三个阶段的依据。当前新独立实例仅准备，尚未运行；既有短暂后台检查的全面通过结论已撤回，不能据此自行解禁。
