# 无占有角色的同一视图目标认证

2026-10-17。根据 docs/105 的实际空Pawn场景证据，加入独立默认关闭的 `GEARS_UNPOSSESSED_VIEW_TARGET` 实验构建开关。玩家默认仍是Reticle47，本记录不表示实际修复已验收。

路径仅允许当前Config中已完整认证的同一个Pawn继续作为Camera.ViewTarget，同时Controller.Pawn和Pawn.Controller均成功读到0。cinematic配置与WorldInfo／角色／注册表／名称／类链／Outer校验保持；不跟随其他同类Pawn或CameraActor。由初次绑定确定明确的unpossessed模式，并纳入scene、family、renderer所有绑定比较。模式或链接变化不能伪装成timestamp-only暂态。

该模式使用双次逐字节一致的28字节相机POV、有限位置和0<FOV<180，眼高为0；原票据超时、原始引擎视图、source矩阵、线程及世界代际条件不变。镜头拉近和武器准星关联／冻结明确不适用于该模式。完整Config每次加载后、首次bounded_guard前按试验开关设置，包括世界重新绑定。

独立MSVC /W4 /WX /O2测试通过：AnchorObserverTest包含默认关闭、精确null/null成功、读失败写零、错误目标、受控／无占有模式变化、WorldInfo／身份／POV变化及时间边界；MountedObserverTest170项检查通过。试验接下来必须证明实际Pawn.Controller为空、持续产生新票据、GPU图像来自这些新票据、死亡菜单及检查点恢复正常。

本次不扩展到Config证书之外的“最近重新占有角色”。同代换成另一Pawn后死亡仍可能被拒绝；若实际遇到，需要单独的最近成功认证身份设计与证据，不能悄悄扩大fallback。
