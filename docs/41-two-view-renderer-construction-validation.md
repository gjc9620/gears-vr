# Construction verification of original retrofitted objects from two-view view

2026-10-12 Original single-person level, PID 49024. Construct an unsubscribed object without a screenshot and without changing the actual submitted pool.

## What did you do?

Get the canvas and transformation parameters of the current frame in the Real Rendering Constructive Entry, create a deflection view and stand-alone status. Copy the full text. 0x60 bytes to assemble the context, and replace the view array of local copies with " original view + offset view. Use the original rendering function to generate temporary 0x140 object.

Check and then destroy the temporary rendering object, the temporary source view, and release the independent state. Copies of the temporary construction are called to be separated from normal frame logs so that the non-submitted object is not considered a success in normal consumption.

## Result

- `renderer_pair_enabled=1 checks=255 retained=0`Inputs are different, distributed, tectonic returns, two separate arrays, owner/state/serial number relationships, maintenance of both matrices, primary destruction return, original collections remain unchanged and eight were adopted.
- Render view storage consists of two steps 0xd30; the collection points to each of these two, and the individual is the assembly within the rendering object with a serial number 0 and 1。
- The original view is in its current state, and the new view is independent. No recreates either state are claimed.
- (a) The new source view and the existing inspection of independence were passed and released normally;11 Byte recovery after the opening of a portal has stopped.
- 20 Seconds 89,440 Article, discard. 39 Article;6,882 Subprecent does not record. The construction, life cycle and preparation analysis of the normal single view frame are passed.

Local evidence:`analysis/runtime/renderer-pair-49024.*`;specific analytical tools:`tools/native-capture/analyze_renderer_pair.py`。

## Border

This time it proved that the two views were properly constructed and destroyed in a raw rendering object, that they had been rendered, and that there was no new GPU image or head-high stereo output.

The structure is abnormally designed to keep the object and its dependence on process exits, avoiding destruction of unknown aliases; pure values fail but the ownership structure is identifiable. Self-detection covers normal two copies, cleanup when matrix is not consistent, group names/micronumber separation, distribution failure and shared status rejection.

The next step is to be physically limited, observing consumption, preparation and destruction of both views and then receiving and inspecting the respective image areas.

---

## 中文

# 两视图原生渲染对象的构造验证

2026-10-12，原版单人关卡，PID 49024。构造一个未提交渲染对象，不截图，不改变实际提交的集合。

## 做了什么

在真实渲染构造入口取得当前帧的画布与变换参数，创建偏移视图及独立状态。复制完整 0x60 字节集合上下文，把局部副本的视图数组改为“原视图＋偏移视图”。调用原生渲染构造函数生成临时 0x140 对象。

它从未交给消费者，也没有入队。核对后依次销毁临时渲染对象、临时源视图，再释放独立状态。临时构造中的副本调用从正常帧日志中分离，避免把未提交对象当作正常消费成功。

## 结果

- `renderer_pair_enabled=1 checks=255 retained=0`：输入状态不同、分配、构造返回、两个独立数组、owner／状态／序号关系、两份矩阵保持、原生销毁返回、原集合保持不变，八项均通过。
- 渲染视图存储含两项，步长 0xd30；集合分别指向这两项，各自 owner 为渲染对象内的集合，序号为 0 和 1。
- 原视图用原状态，新增视图用独立状态。没有宣称两个状态都重新创建。
- 新源视图及独立状态的既有检查通过，正常释放；11 个启用入口停止后字节恢复。
- 20 秒记录 89,440 条、丢弃 39 条；6,882 次 Present 无记录失败。正常单视图帧的构造、生命周期和准备分析通过。

本地证据：`analysis/runtime/renderer-pair-49024.*`；专用分析工具：`tools/native-capture/analyze_renderer_pair.py`。

## 边界

这次证明了两视图能在一个原生渲染对象中正确构造和销毁，没有证明它们已经被渲染，也没有新 GPU 画面或头显立体输出。

结构异常时会保留对象及其依赖到进程退出，避免销毁未知别名；纯数值对照失败但所有权结构可识别时仍走正常清理。自测覆盖了正常两副本、矩阵不符时清理、数组别名／错误序号隔离、分配失败及共用状态拒绝。

下一步是受限实际提交，观察两视图的消费、准备和销毁，再验收各自画面区域。
