# Shelter interactive contract

This contract continues [Confirmed bunker design](../6dof-cover-interaction-design-20261024.md), only the engineering boundary is filled. This is achieved on the basis of the current unified body/hand main line and does not restore the old independent operating package.

## Player Behaviour

| Operation | Fixed behaviour |
| --- | --- |
| Original entry, adsorbing, moving along the wall, side/back exit | Keep original legitimacy and status chain; grab the edges cannot lock exit |
| Real stand, crouch, right, left, right, right. | HMD actual homogenization continues to take effect without LGrip; pure rotation does not generate additional homogeneity |
| Left edge: Left hand to right | Body moves to the local left side of the bunker; reverse indentation |
| Right edge: Left hand to left | Body moves to the local right side of the bunker; reverse indentation |
| Top: Left hand down | Body rises; moving up lower |
| Untie me, LGrip. | The reservation has been submitted to virtual offsets, with immediate access of the left hand to other interactions; real head is still free to move |
| Catch it again. | Current hand position, currently submitted body deviation as a new starting point; locking one side and one axis at a time |
| LT／RT | Keep sighting, open-scopes, shooting; remove them from the side probe/uptick and camera deflection that automatically occurs in this VR mode |
| Hold your gun in the right hand, hold your side with your left hand. | Legal single-handed shooting is allowed; the gun remains under cover and HMD cannot be an authoritative target source |
| Original flipping, CoverSlip, SwatTurn, running into bunkers. | Original special action takeover; stop scratching, return posture ownership and re-establish context after completion |

Left-hand leases have been given priority, including armed aids, ammunition/brackets and mechanical parts. In the absence of a lease, existing single arbitrations are to be followed: only requests made through the edge of real proximity/accessible inspection; currently eligible cover requests are to be prioritized with no additional second arbitrator. One can only be used by one consumer if the lease is lost or tracked down, but must be effectively unloaded again without automatic retrieving.

## Three sets of coordinates must be separated.

1. `CoverBasis`The body coordinates for the wall, with the right axis derived from current legal surfaces and gravitational forces to the world; do not rotate with real-time head. Not only Pawn yaw+thirium is used. Curve/neighbor changes create new base formation.
2. `ViewBasis`: Main Line Current C 0/ First person refers to the point of view benchmark, continuing the observation direction before entering. Body cannot force head-over towards wall; suppress original cover `BeginTurn` Automatic camera alignment on this instance.
3. `TrackingSpace`Original OpenXR position and valid reference. Hand-held projection to frozen bunker axis, virtual shift in millimeters; world conversion is based on current unit ratio and does not have hard coded historical slot size.

One-axis pull as `candidate = grab_offset - gain * dot(hand_now - hand_at_grab, locked_axis)`. The calculation uses the original real hand position, and cannot use a rendering position that has been accompanied by virtual body deviations. HMD horizontally and virtual deflection is added once each, with the same submission for the left eye. The point of contact for the left hand visual is fixed to the edge; the non-arranged right hand uses the same body transformation as the gun. Turning head, edge brightness, animation weight cannot reverse this calculation.

## Default value and edge determination

The following is the D-level default, non-original constant, non-receipt hand: pull gain 1.0;point of hand-held contact to edge capture distance 0.10 m; two candidates with a distance less than 0.02 m Refusal of new capture when no clear surface attribution is available; collision safety margin 0.02 m. Maintain status. No longer forced disconnects with capture distance, otherwise normal hand-to-hand exercises will throw them away. 100 ms Tracking caps and stricter source limits for main lines;100 ms freshness cannot replace current collision occupancy certificates.

Marginal evidence must include: original Link/slot/authorities, actual Actor/Component identity, collision shape/change version, local surface, grab-band/curve zone, orientation, approach side, accessible path and sample.`bLeanLeft/bLeanRight/bCanPopUp` The top of the wall is not automatically allowed to go up, even if it is visible. No ordinary walls, backs, wall stretches, or ambiguous corners are asked for any kind of grab.

The search is narrowed by the original slot and method, then the surface of the surface is located by a limited range of real collisions.17 It's... 96–112 The units height range is used as a precise top line, and it is not possible to set the altitude on all maps. The circle bunker uses a local cut-off and maintains a single grab axis; cross/directional changes require new capture.

## Collision and retention

Discards the interpretation "scattered rays to prove that the whole rectangle is mobile". `CoverRange` Only for cases where the whole rectangle is actually certified; formal extension**Current shift to candidate body size sweep**, and check the endpoints. The two axes must accumulate in a manner that will also destroy the whole posture of the combination. `collision_verified`。

The maximum proven position on the same axis is submitted when virtual movement is blocked; reset the hand/discretion starting point of the axis after exposure to avoid sudden jumps as the blocked position shifts. Unprovenly, candidate positions cannot be simply clamp declared secure. Waiting for queries is an independent result: keep this lease and recently prove available to handle real releases; `complete(false)`。

Physical HMD moves cannot be cut off by virtual clap. Real head penetration leads to a tracking, shielding/simply blocking the view of the wall; no head can be blown back. Body, head-hit area and gunman use the same attitude and collision results separately; arm cannot stretch indefinitely. Real front moves lead to head-to-head piercing, and no authoritative gun should automatically reach outside the walls.

The old position that is kept loose must be re-proven every time before it is released. Barriers move, damage, world offload, slot/baseline change cannot permanently slow down the old “safe” result. The original location is restored when occupied: stop new pull and release of new bodies, keep track of real and untrustworthy views, restore from the original moving/exit chain with a certified short path. Exit this extension without legal restoration; do not move to uncertified zero deviations.

## Transition, exposure and resources

The old grab is terminated by changing the slots, recenter, snap turn, space/reference/control object changes. Normal shift/exit will be submitted to the main line transitioner and cannot jump back in the frame of zero logical deviation. Default transition time 0.15 s, only manual benchmarking of plug-in values; real tracking always folds in time and path collisions take precedence over plug-ins. World/Pawn is replaced without interpolating world-wide data.

The attack cannot be judged solely by whether the eye is passing. It requires a distinction between protected bodies, exposed head/toms/arms and actual hits; it does not allow for single-person blinding fire to protect an extended body. Amending the original protection branch while retaining the type of injury, explosion exception, armour, difficulty and normal course of harm, prohibiting direct withholding of blood. Fire consumption of the same body posture, weapon examples and gun-point evidence, with the bullet still hitting the wall as originally prescribed, without having to swallow the trigger.

The bunker itself is re-used with the original scene collision and model, and this package does not require a “capture model” for the entire map. The resources that need to be prepared are hand contact markers for the existing role skeleton, left-hand contact positions, body neutral calibration, local coordinates of the gun mouth and hand model, and visualized markings used by the editor. 115 Bones signatures can only be used to match the role; other skeletons require their own mapping, and cannot be enabled on the same number as a bone. [Weapons implementation package](../weapon-specs-20261028/implementation-handoff.md)Common contact points and gun contracts, with no repetition of the second set of hands or guns.

---

## 中文

# 掩体交互合同

本合同延续 [已确认的掩体设计](../6dof-cover-interaction-design-20261024.md)，只补工程边界。实现时以当前统一身体／手部主线为基础，不恢复旧独立运行包。

## 玩家行为

| 操作 | 固定行为 |
| --- | --- |
| 原版进入、吸附、沿墙移动、侧／后退出 | 保留原版合法性与状态链；抓边不能锁住退出 |
| 真实站起、蹲下、左右／前后移动 | HMD 实际平移持续生效，无需 LGrip；纯原地旋转不产生额外平移 |
| 左边缘：左手向右拉 | 身体向掩体局部左侧移动；反向缩回 |
| 右边缘：左手向左拉 | 身体向掩体局部右侧移动；反向缩回 |
| 顶沿：左手向下压 | 身体升高；向上移手则降低 |
| 松开 LGrip | 保留已提交虚拟偏移，左手立即可以参与其他交互；真实头部仍自由移动 |
| 再抓取 | 当前手位、当前已提交身体偏移作为新起点；一次锁一条边和一根轴 |
| LT／RT | 保留瞄准、开镜、射击；移除它们在本 VR 模式中自动产生的侧探／上探和相机偏移 |
| 右手持枪、左手抓边 | 允许合法单手射击；枪口仍受场景遮挡，HMD 不能成为权威瞄准来源 |
| 原版翻越、CoverSlip、SwatTurn、跑入掩体 | 原版特殊动作接管；停止抓边、交还姿态所有权，完成后重新建立上下文 |

已有左手租约优先，包括持枪辅助、弹药／弹匣和机械部件。无租约时沿用现有单次仲裁：只让通过真实接近／可达检查的边缘发出请求；当前内核对合格 cover 请求有优先权，不增加第二个仲裁器。一个按下沿只能被一个消费者使用；已有租约丢失、追踪中断后必须真的松开再按，不能自动重新抓取。释放回合应正常结束租约，不把正常释放当故障。

## 三套坐标必须分开

1. `CoverBasis`：朝墙的身体坐标，右轴来自当前合法表面法向与世界重力；不跟着实时头显旋转。不仅用 Pawn yaw+π。曲面／相邻面变化形成新的 basis generation。
2. `ViewBasis`：主线当前 C0／第一人称视点基准，延续进入前的观察方向。身体朝墙不能强制旋转头显；抑制原版 cover `BeginTurn` 在本实例上的自动相机对齐。
3. `TrackingSpace`：原始 OpenXR 位姿及有效 reference。手位先投影到冻结的掩体轴，虚拟位移以米计；世界换算只使用当前单位比例，不硬编码历史槽位尺寸。

单轴拉拽为 `candidate = grab_offset - gain * dot(hand_now - hand_at_grab, locked_axis)`。计算使用原始真实手位，不能使用已经附加虚拟身体偏移的渲染手位。HMD 平移和虚拟偏移各加一次，左右眼使用同一提交结果。抓住的左手视觉接触点固定在边缘；非锚定的右手与枪使用同一身体变换。转头、边缘高亮、动画权重不能反向改变这个计算。

## 默认值与边缘判定

以下为 D 级可调默认，非原版常量、非已验收手感：拉拽增益 1.0；手掌接触点到边缘捕获距离 0.10 m；两候选距离差小于 0.02 m 且无明确表面归属时拒绝新抓取；碰撞安全边距 0.02 m。保持状态不再用捕获距离强制断开，否则正常拉手会丢抓。沿用现有 100 ms 追踪上限和主线更严格的来源限制；100 ms 新鲜度不能代替当前碰撞占用证明。

边缘证据必须包含：原版 Link／slot／权限、实际 Actor／Component 身份、碰撞形状／变换版本、局部表面、可抓线段／曲线区间、朝向、接近侧、可达路径和采样时相。`bLeanLeft/bLeanRight/bCanPopUp` 只是权限提示，不是几何证据。高墙的顶边即使可见，也不自动允许上探。普通墙面、背面、隔墙伸手、模棱两可的拐角均不发抓取请求。

先用原版槽位及法向缩小搜索范围，再用有限范围的真实碰撞查询定位表面。顶沿需要同一表面的下方命中与上方自由区以及沿边连续性；侧沿需要端部、绕边空间和身体侧向通路。不得把 CT17 的 96–112 units 高度区间当作精确顶沿，更不能给所有地图写固定高度。圆形掩体采用局部切向，保持一次抓取轴固定；跨面／切向变化需要新抓取。

## 碰撞与保持

弃用“零散几条射线证明整个矩形可移动”的解释。现有 `CoverRange` 只适合整个矩形确实被认证的情况；正式扩展采用**当前偏移到候选偏移的身体体积扫掠**，另检查终点占用。两轴先后累积也必须扫掠组合后的完整姿态。矩形边界仅作为保守操作上限，不能单独签发 `collision_verified`。

虚拟移动被挡时提交同一轴上的最大已证明可达位置；接触后重设该轴的手位／偏移起点，避免积攒被挡住的位移后突然跳动。未经证明不能把候选位置简单 clamp 后宣布安全。等待查询是一个独立结果：保持本次租约和最近已证明仍可占用的位置，继续处理真实释放；不是 `complete(false)`。

物理 HMD 位移不由虚拟 clamp 截断。真实头部穿入障碍时保持追踪，用遮蔽／渐隐阻断穿墙观察；不能把头部弹回。身体、头部受击区域及持枪手分别使用同一姿态与碰撞结果；手臂不能无限拉长。真实前移导致头部穿墙，不应自动使权威枪口获得墙外出生点。

松手保持的旧位置每次发布前也要重新证明占用。障碍移动、破坏、世界卸载、slot／基准改变时不能永久缓存旧“安全”结果。原位置已被占用时进入恢复：停止新拉拽和新身体发布，保留真实追踪并遮蔽不可信视图，由原版移动／退出链与已认证的短路径恢复。没有合法恢复路径时退出该扩展模式；不瞬移到未经证明的零偏移。

## 过渡、暴露与资源

沿墙换槽、recenter、snap turn、空间／reference／控制对象改变均结束旧抓取。正常换槽／退出将已提交身体与眼点交接给主线过渡器，不能在清零逻辑偏移的那一帧回跳。默认过渡时间 0.15 s，只插值人工基准修正；真实追踪始终即时叠加，路径碰撞优先于插值。世界／Pawn 被替换时不插值跨世界数据。

受击不能只根据眼睛是否越沿判断。需要区分被保护的身体、已暴露的头／躯干／手臂与实际命中；不能保留单人原版盲射免伤去保护已经伸出的身体。修正原版保护分支的同时保留伤害类型、爆炸例外、护甲、难度和正常伤害流程，禁止直接扣血替代。开火消费同一身体姿态、武器实例和枪口证据，遮挡子弹仍可按原规则击中墙面，不需要一律吞掉扳机。

掩体本身复用原场景碰撞和模型，本包不要求为整张地图补做“抓取模型”。确需准备的资源是现有角色骨架的手掌接触标记、左右手接触姿势、身体中立校准、枪口与手模型局部坐标，以及编辑器用的可视化标记。现有 Marcus 115 骨签名只能用于匹配角色；其他骨架需要自己的映射，不能仅凭骨数相同启用。与 [武器实施包](../weapon-specs-20261028/implementation-handoff.md)共用接触点和枪口合同，不重复制作第二套手或枪。
