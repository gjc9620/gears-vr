# 死亡画面的 HDR 源像素边界

2026-10-17。独立诊断 Hdrpixels53/54；默认玩家候选仍为 Reticle47。DEATH_SOURCE_CONTROL 关闭，没有复制左图替代右图。诊断、原始 HDR 和游戏派生资料只保存在忽略目录，不加入发布包。

## 读回的生命周期修正

Hdrpixels53 源提交 f159914，实际93会话 continuous-stereo-5048-20261017-115145-a9ce65f7。原复制后成功追加一次同源复制，唯一 Execute 与 GPU fence 完成均被观察到，但退休校验失败（failure=6），没有保存 RAW。旧记录缺少实际 Reset HRESULT 和当时 TrackerRef，不能宣称当时直接观察到 TrackerRef=0。

代码审查发现 frame_link.h 的 seal_locked 在 Present 时清空发布窗口的列表记录。这不等于真实 D3D12 command list 已 Reset。提交72de166将退休证据改成：AddRef 保持的相同 canonical IUnknown 上实际调用 original Reset 成功；同时仍要求唯一已跟踪 Execute、同 DIRECT queue、成功 Signal 和 GPU 完成。重复执行、未知成员、跨队列或失败均拒绝读回。仅 fence 完成仍不允许读取或释放。

Hdrpixels54 完整编译及15项冻结 CPU 测试通过。源72de166b77117aa5bf186249b06d4cce06851990；DLL SHA256 33aeae0a358fb5c14d11193e57df0a058c116c4758fb2b0904ad1e2020b2b2dc。

## 实际94结果

会话 continuous-stereo-29512-20261017-115838-57e1b71b；选择第49个无占有票据901/world1，在两个2ab6d9之后第二次匹配全幅copy（序号166、caller1c6093）紧邻原复制，追加一次同源READBACK复制。原调用只转发一次；不新增queue、list或engine barrier。运行范围1c6033起128字节匹配既存SHA256身份。

读回成功：executed/signalled/retired/gpu_complete/saved均1，failure=0。实际 original Reset HRESULT=00000000，canonical3e43ff1d0与保持对象相同；此时发布追踪器identity/generation均0，直接验证了这次实际运行中的身份记录差异。

RAW为3840×2160、格式26 R11G11B10_FLOAT、row_pitch15360、offset0、33177600字节；SHA256 8a9d6956546f89c61f1ebcf1c830b928217303e11ae6988acd9904024bfb044c。离线解码保留原始字节，共用色调映射的预览中左右都是完整死亡场景。

| HDR统计 | 左半 | 右半 |
| --- | --- | --- |
| 有限通道占比 | 100% | 100% |
| RGB均值 | .034205/.036191/.033668 | .022234/.025208/.025466 |
| RGB全零像素占比 | .000940% | .001061% |

该帧时间线175行，六个COM身份，无事件丢弃或读取失败；Clear、Barrier、其他线程及非首RTV仍不完整覆盖。最后copy的源canonical3e37486a0，目标3e3747d10；同一票据后续左右d80a7b均记录采样目标3e3747d10。

最终GPU截图死亡票据870/897右眼仍近黑，正常游玩284/1217双眼有场景。但这些最终截图不是HDR票据901，不能冒充同一帧的前后像素证明。当前直接结论是：票据901的第二次全幅copy源两半都有场景；不能再将该边界之前的右眼几何缺失当作既定根因。

检查点world1→2恢复观察6421ms（含加载）。原生正常清理、游戏正常关闭、四份存档哈希恢复通过。头显未参与本轮，不构成真机验收。

证据：analysis/runtime/hdrpixels94-*、上述会话.native-status.txt及.hdr-timeline.json；RAW和解码统计在tools/native-capture/build-hdrpixels54/death-hdr-source-29512-901-710576041210.*，均忽略。下一步围绕该copy之后到d80a7b输入/输出之间定位，不修改UV、不将双眼同图作为修复。
