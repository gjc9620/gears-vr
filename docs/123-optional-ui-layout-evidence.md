# 可选 UIScene 布局采集

2026-10-17。为[岔路原生绘制路线](122-path-choice-native-scene-route.md)补充少截图的状态采集方式。它不调整 UI，不调用游戏函数，也不写进程内存。

`tools/game-io/menu_state.py --layout` 隐含 `--tree`：在原有同一次一致性读取中，遍历当前顶层场景的有界子树，并为每个节点读取反射属性 `RenderBounds`。输出包含四边值、原始字节和反射字段元数据；每次重新解析该字段，不沿用跨采样字段发现结果。静态数组仅接受 FloatProperty、四个四字节元素及有限、非倒置的数值，读取放入既有最终复读 ledger。次序由现有 `UIRoot.EUIWidgetFace` 声明确定为 left/top/right/bottom，未猜测 native RenderBounds 偏移。

示例供后续已协调的游戏测试使用：

```text
python tools/game-io/menu_state.py --pid <当前游戏PID> --layout --samples 1 --out analysis/runtime/path-choice-layout.json
```

默认采样和菜单恢复监视器不启用该选项，不新增常驻读取，不改变输入或启动行为。请求布局但字段未知、布局错误或复读不一致时拒绝该布局样本，不用不完整旧值代替。输出明确标记 `consistent CPU layout snapshot; not actual draw/scissor proof`：即使字段复读一致，也不是与渲染线程原子同步、GPU 裁剪或实际可见性的证明。

后续在岔路显示时，先确认顶层 `DeltaChoosePath / UIScene_PathChoice`，再检查其子树边界，关联原生根场景调用、canvas 及实际绘制范围。仍需确定裁剪坐标和完整变换恢复，才能启用整组位置修正。本轮没有读取实际进程或启动游戏。

验证：菜单状态／控制／导航／显示监视器共 155 项测试通过；随后将字段范围收紧至 `0x10000-16`，对应边界测试通过。覆盖默认路径不额外读取布局、根与后代节点、错误类型／数组维数／元素大小／字段偏移、非有限与倒置边界，以及字段或边界在最终复读时变化的拒绝和重新获取。测试不构成实际岔路界面布局或 GPU 验证。
