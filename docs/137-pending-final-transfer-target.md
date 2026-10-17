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
