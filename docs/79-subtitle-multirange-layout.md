# 当前字幕显示与多片段布局

2026-10-17，第一版字幕工作。用户配置 `Subtitles=True`，未为本轮修改配置。先查询现有知识库并复用原有绘制路径；未启动IDA、扩展全量反编译或启用旧owned输出路线。

## Native37实际字幕图像

会话 `continuous-stereo-36360-20261017-051856-59a79d11` 使用未改变的Native37／Borderless1及最新Python启动修正；HUD角宽45°、元素1.2、中心0.8、镜头靠近0.3米。实际进入双目后，确认死亡菜单并正常读取检查点一次；generation2有提交后，在有限的2至8秒窗口请求七次GPU图，总共八张含自动首图。

原生字幕GPU记录包含实际索引成员、相应票据、两眼重放与命令完成；计数本身不等同可见像素。本轮另直接查看源GPU图：index0可读到“Dom: Nice!”，index5可读到“Marcus: Grab hold!”。index5票据2146、源序号及保存时间见 `analysis/runtime/subtitle40-burst-audit.json`；同票据左右眼的原尺寸裁切 `subtitle40-both-eyes-crop.png` 均有相同字幕。`subtitle40-contact.png` 两列分别为两个时间的左眼，不得误标为双眼图。

这个证据排除了本样本“字幕完全未画出”，但字幕相对场景明显偏小。源GPU图不替代头显可读性或全部语言／过场字幕验收。整轮停止与原生清理、XR正常退出通过，游戏正常关闭且确认为已退出，回执 `subtitle40-normal-close.json`。

## 布局缺口

字幕调用经常产生背景与文字等多个payload范围。`publish_ui_text_delta` 只为单范围调用授予 `whole_text_group`，`ui_hud_layout` 因而让多范围字幕沿用HUD平面回退，元素放大和中心收拢不生效。`hud_applied=1` 只说明平面重放，不说明该布局已生效。此前Native37两轮的字幕范围数量与布局回退数量相同支持该方向，但不能仅凭计数相等认定每次回退来源。

下一步在完整字幕producer调用处保存有上限、不可变的共同几何和矩阵证明，将同一组的片段应用同一个仿射变换。保留完整范围与实际索引校验；片段不冒充完整字符串。所有payload矩阵必须在生产时稳定一致，GPU执行时还须与冻结矩阵匹配，否则回退。不会以开关旧渲染路线替代修复。

## Caption38实现与实际回归

提交 `a500ab4` 实现上述路径：一个不可变对象最多保存1024个原始XY位置及64个范围，跨payload共享；每个范围仍有自身实际索引和当前顶点证明，片段的 `whole_text_group` 仍为false。完整位置经冻结矩阵投影，避免把原始AABB四角误当成一般透视／剪切变换下的真实整体范围。矩阵使用既有已验证的payload+0x80读取，并在生产端对所有范围重读、在执行端与当前矩阵比较；不引入新偏移。

构造时矩阵已初始化的静态依据来自历史快照artifact1函数 `sub_1401C0E48`，函数字节SHA256 `e2bcd1a11dd8235aeef5d83c7c32e583130d96bbffef5c57881d447601702469`；该函数先复制矩阵，再把payload挂入item。历史证据不代表磁盘／当前运行代码等价，实际路径仍靠上述当前读数校验。不同root／group对象混入同一个native字幕段、矩阵／坐标／范围／来源不匹配等情况保留回退，独立有效段可继续。

五项CPU检查通过，覆盖跨payload的背景／文字共用范围与投影、字距、失配、透视矩阵、段隔离、生命周期、原索引与源字节保持。提交 `afd364d` 另记录实际完成的字幕布局重放，与既有 `hud_applied` 平面计数区分。

完整候选冻结于 `afd364d`，`tools/native-capture/build-caption38/GearsCaptureCaption38.dll` SHA256 `a99b869aeadb8c9410125b860e6d5e8993e8441dac7f7ba24e263cc6c19f62cd`，沿用Native37依赖及构建开关，未包含工作区旁路修改。完整构建和18项原生检查通过，回执 `caption38-source.json`、`caption38-selftests.json`。

实际模拟器会话 `continuous-stereo-6632-20261017-053301-43e970f7` 保持同样45°／1.2／0.8／0.3米设置，进入双目并读取检查点恢复generation2。380次纯字幕GPU调用中374次明确记录 `layout_applied=1`，6次保留未应用布局，所有保留字幕行的平面／布局和命令完成标记均为1。不是以全局HUD计数替代字幕证据。

GPU index2、票据3016中两眼都可读到“Dom: Another grub hole!”；原尺寸文字和背景未见分离或裁切，缩放／收拢使用现有1.2／0.8参数。检查图 `subtitle41-frame2-band.png`；原图、票据与采样时间见 `subtitle41-burst-audit.json`，汇总 `caption38-subtitle-audit.json`。不同运行的采样时刻和台词没有对齐，不能据此宣称精确像素级A/B提升比例。

原生清理、入口恢复、XR正常退出和游戏正常关闭均通过。尚未做新头显可读性、多行／长字幕、全部语言、过场及那6次回退的逐例像素检查；第一版仍未完成。默认试用候选已选择Caption38／Borderless1，Native37清单保留回退；独立快照 `analysis/release-staging/caption38-offline-20261017` 隔离预检通过，尚未正式发布。
