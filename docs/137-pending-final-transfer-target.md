# Pending final-transfer destination: bounded historical evidence

## Source identity and ABI correction

Knowledge-base artifact1 was queried before inspecting saved functions at RVAs1af42c,177e90 and1ac950. Their cached pseudocode hashes were independently checked. The source is the historical runtime code blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`, base0x140000000, blob offset zero at RVA1000. No IDA database was opened or changed.

Independent Capstone decoding of the source bytes shows `lea rcx,[rdi+0x40]` at RVA1af8fe, call177e90 at1af902, and `mov rdi,rcx` at177ebc. Thus this callee receives the state block as its actual first argument in RCX. Decompiled parameter labels such as “a4” do not establish the ABI. The resulting **context-relative** offsets below include that0x40 adjustment.

| Pending field | Context offset | Historical assembly support |
| --- | --- | --- |
| RTV count byte | 0x563 | state+0x523,177fd8/1786d5 |
| RTV view pointer i | 0x590+8*i | state+0x550,178712/178da1 |
| CPU RTV descriptor handle | view+0x0c |178db2→stack handle array→1793c3 |
| Viewport count byte |0x560 |state+0x520,17863d |
| D3D12_VIEWPORT array |0x290 |state+0x250,178636 |
| Scissor count byte |0x561 |state+0x521,178668 |
| D3D12_RECT array |0x410 |state+0x3d0,178661 |
| OM dirty byte |0x9f |state+0x5f,1786bf |
| Viewport dirty byte |0xa3 |state+0x63,17862a |
| Scissor dirty byte |0xa4 |state+0x64,178653 |

The state block has a DSV view at context+0x5d0; it is not needed to reinterpret color RTV wrapper objects. Historical code ultimately forwards CPU RTV handles to command-list vtable offset0x170 (OMSetRenderTargets); viewport/scissor calls use offsets0xa8/0xb0. Raw source-verified instruction ranges, SHA256 values and disassembly are saved under ignored `analysis/runtime/pending-target-historical-asm.json`. Those ranges remain **unverified against current running bytes**. Some pseudocode register/value names are misleading; do not translate its signature literally.

## Smallest next observation

Extend the existing bounded final-transfer bracket rather than adding hooks or changing rendering. Read one pending RTV view/handle, one viewport and one scissor, only under the current validated context/list/world scope and existing64-attempt/16-row limits. Resolve the CPU RTV handle through `frame_target_stats.lookup(handle)`; retain descriptor write/origin provenance from the existing tracker. Do not invoke unknown native resource-conversion methods.

For a prospective entry proof require: one RTV; readable nonnull handle; descriptor provenance intact and unchanged; mip0/format24; current known backbuffer with declared destination dimensions; finite full-destination viewport/depth0..1; covering scissor; even destination width. Existing complete source pair/ticket, source texture, list/generation and inventory/world checks remain required. If a pending field is not dirty, the already-tracked D3D state must agree.

At actual Draw, compare the entry handle/provenance with `binding_provenance(raw_list)`, the entry resolved target with actual target, and pending viewport/scissor with actual tracked rectangles. Reread the pending view/handle and lookup provenance to reject intervening descriptor changes/capacity loss. Within the same scope, existing OM/viewport/scissor hooks can count and record consumed binds without new hooks; dirty pending state requires the expected list/binds. Preserve the independent direct-Draw count and exactly-one routed draw requirement.

This is still observation only. Full-source sampling normalization must wait for current code-range/ABI attestation and successful runtime comparison. It would intentionally preserve complete SBS source eyes rather than retain the native padded/aspect-fit final mapping. Source resolution stays authoritative; it does not restore Low/High or force a fixed player resolution. Real GPU pixel/seam/UI and headset validation remain required before acceptance.

---

## 中文

# 有待最后移交目的地:有限度的历史证据

## 源身份和ABI 更正

知识库文物1 在检查空基站保存的函数之前询问过1af42c,177e90 and1ac950。它们的缓存伪码散列被独立检查。源代码是历史运行时间代码 blob SHA256 `911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6`基数0x140000000,blob 乘以 RVA 时的零1000。没有打开或更改开发协会的数据库。

源字节的独立顶点解码 `lea rcx,[rdi+0x40]` 皇家航空兵1af8 菲,打电话177e90 at1af902,以及 `mov rdi,rcx` at177 ebc. 因此,这个被调用者收到状态块作为它在RCX中的实际第一个参数。4“不建立上诉机构。 **context-relative** 包括以下各项:0x40 adjustment.

| 待决外地 | 页:1 | 历史组装支助 |
| --- | --- | --- |
| RTV 计数字节 | 0x563 | state+0x523,177fd8/1786d5 |
| RTV 视图指针 I | 0x590+8*i | state+0x550,178712/178da1 |
| CPU RTV 描述符控件 | view+0x0c |178db2 装订手柄阵列1793c3 |
| 视图端口数字节 |0x560 |state+0x520,17863d |
| D3D12 VIEWPORT 阵列(_V) |0x290 |state+0x250,178636 |
| 剪刀计数字节 |0x561 |state+0x521,178668 |
| D3D12 RET 阵列(_R) |0x410 |state+0x3d0,178661 |
| OM 脏字节 |0x9f |state+0x5f,1786bf |
| 查看端口脏字节 |0xa3 |state+0x63,17862a |
| 剪刀脏字节 |0xa4 |state+0x64,178653 |

状态块在上下文+ 上下文时有 DSV 视图0x5d0; 不需要重新解释颜色 RTV 包装对象。 历史代码最终将 CPU RTV 控件转发到命令列表 vtable 偏移0x170 (OMSetRenderTalgets); 取景口/剪刀呼叫器使用量偏移0xa8/0xb0 原始源码验证指令范围,SHA256 值和拆解被忽略 `analysis/runtime/pending-target-historical-asm.json`。这些范围保持不变 **未验证当前运行的字节**。一些伪码注册/值名具有误导性;不要按字面翻译其签名。

## 下一个最小的观察

扩展现有的边框最终转移括号, 而不是添加钩子或更改渲染。 仅在当前已验证的上下文/ 列表/ 世界范围和现有范围内, 读取一个待读的 RTV 视图/ 手势、 一个视图和剪刀64-attempt/16 控制 CPU RTV 控件通过 `frame_target_stats.lookup(handle)`; 保留现有跟踪器的描述性文字写法/原产地。不要引用未知的本地资源转换方法。

对于可能的输入证明,需要:一个RTV;可读的非核控柄;描述性出处完整无缺;MIP0/format24; 现有已知的具有已宣布目的地尺寸的后继缓冲器; 有限全目的地视图/深度0..1; 覆盖剪切器; 甚至包括目的地宽度。 仍然需要现有的完整源对/ 盘、源纹理、列表/生成和目录/世界检查。 如果待决字段不脏, 则已经跟踪的 D3 D州必须同意。

在实际 Draw 时,将条目处理/证明与 `binding_provenance(raw_list)`,输入已解决的目标与实际目标,等待查看端口/剪切器与实际跟踪矩形。重读待查看的视图/处理和查找来源,以拒绝干扰描述变化/容量损失。在同一范围内,现有的OM/viewpor t/scissor 钩可以计算和记录所消耗的没有新钩的绑定;脏的待决状态需要预期的列表/宾. 保有独立的直接-拖动计数和精确的路由绘图要求.

仅此而已。 全源采样正常化必须等待当前代码距/ABI的验证和运行时间的成功比较。它会有意保存完整的SBS源眼,而不是保留原生的加法/外观-适配法 fina 。l 映射。 源分辨率仍然具有权威性; 它不会恢复 Low/ High 或强制使用固定播放器分辨率。 接受前, 需要真正的 GPU 像素/ seam/ UI 和头盔验证 。
