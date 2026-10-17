# 过渡提示卡的独立 GPU 检查

2026-10-17。承接 docs/103 中实际检查点切换的黑矩形疑点。没有修改发布版提示卡代码、安装工具或更改全局OpenXR运行时；使用现有RenderDoc1.46和显式模拟器Runtime。

## 实验身份与覆盖

ignored `analysis/transition-card-probe84/TransitionCardProbe84.exe` 原样包含项目TransitionCard。独立创建真实OpenXR／D3D12会话、绑定运行时指定适配器，只有双眼VIEW quad，不加载游戏，也不生成投影层。EXE SHA256 `17b0315f59ff1c2f060a431ff83db82715d9275c20af1e55f45c99c8ebe2b759`。原头文件和工具哈希见同目录build-hashes.json。

官方renderdoccmd从启动时接入；程序只查询已加载RenderDoc API，显式指定绑定的D3D12 device，在卡片首次上传前开始抓帧、xrEndFrame后结束。实际PID29960／creation134366765692637630；Start后的IsFrameCapturing=1、EndFrameCapture=1、active_after=0。生成新的 `run1/probe.log.capture_capture.rdc`，不是历史捕获。程序运行40秒，共3463次双眼quad提交／一次上传，requestExitSession成功，卡片gpu_unsafe=0，space／session／instance销毁均0，最终result=0。

控制脚本两次读仍被占用的CRT日志报PermissionError。没有因此重启或强杀程序；以独立进程身份、CIM进程消失及退出后完整日志核验上述结果。最初观察脚本的失败与实际探针成功分别保留。

## GPU 内容

使用官方replay API打开新抓帧并回放到event38。纹理350为768×256 BGRA，读回786432字节；转换为RGBA后与CPU源图**全部字节一致**，alpha均255，有完整文字。原始读回SHA256 `d639c4eb5804af55229956a4175041fb4b0349e4c7f3b493bf3b4860967fd0da`。证据 `run1/gpu-card-audit.json`、`gpu-card-350-bgra.bin`及PNG。

同帧还包含两次六顶点绘制（event26／30）。最终1280×588输出纹理337的实际GPU读回，两眼均有正常提示卡和文字；并非只检查上传源。输出与操作清单见 `run1/replay-composite.log`、`after-composite-337-1280x588-bgra.bin`及PNG。

这证明同一提示卡代码在独立首帧中能完成CPU→GPU→双眼合成。它没有重现游戏投影与多种菜单／静态quad之间的切换，也未读取原Reticle84黑矩形发生时的实际GPU资源。因此尚不能认定原问题仅是CPU预览误报，不能据此更改产品上传策略或宣布所有过渡显示正常。下一步应定位混合层切换时的实际输入纹理／合成输出／读回三者，或补充可复现该差异的最小序列。
