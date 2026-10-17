# 死亡绘制 PSO 缓存：恢复实际 VS／PS，并更正初次解析结论

2026-10-17。实验提交fe26067，Hdrpixels57 DLL SHA256cdfc9f78459f02bce62e91d838c87e20814fc196a7f712c434f51c75209eae32；完整DLL及18项冻结CPU／SDK检查通过。玩家默认仍Reticle47／Borderless1，不采用该诊断版本。

只为选中死亡票据观察SDK SetPipelineState、ClearState和ExecuteBundle，复用现有Reset／Draw位置。固定64个列表、最多两个永久保持PSO和两个眼绘制记录；Reset、ClearState、Bundle、未知代次或跟踪容量不足均拒绝继承状态。缓存只在worker中每PSO读一次，保存上限4MiB，不新增GPU命令。缓存内容不保证包含可还原Shader。

实际98会话continuous-stereo-29064-20261017-125624-f91d4b05：票据337/world1两眼d80a7b绘制，同列表canonical3ff5950c0、generation108356，各自具有先前显式SetPipelineState见证，均关联PSO canonical3dfcf9b40。无重复、溢出或拒绝，ClearState／ExecuteBundle计数均0。

对这一个PSO的唯一GetCachedBlob调用S_OK，保存17530字节，SHA256ba49630dcff715e1fbed1701a4495faad9865f4c2dbee3e76ed5b09230c0c4c5。初次离线解析错误地要求DXIL ProgramSize与part大小严格相等，误报没有容器；旧失败报告保留为.containers.json.initial-rejected.json。微软格式验证允许part保留额外尾部空间。修正边界检查后直接复用同一份RAW，未再启动游戏，找到两份完整容器：

| 阶段 | 缓存内偏移／容器长度 | SHA256 |
| --- | --- | --- |
| VS 6.0 | 954／2132 | 7574568978a7d8c845aea694093e145b35fc8c773e52acd332b93e612e54c822 |
| PS 6.0 | 3086／7680 | 6411515e9fb9305d5ebe1b5605e2890303e7ac6720a7553d782e0bfbdf2de1fd |

已有Windows SDK的dxc.exe -dumpbin对两份容器均退出0并输出实际DXIL指令，验证回执为analysis/runtime/deathpso98-dxc-validation.json。没有安装工具，没有猜测厂商私有偏移；容器校验和未另行独立计算。PS实际声明7个纹理／采样器和cb0、cb2，因此旧四槽观察不能当作全部输入。

格式依据：[Microsoft DxilContainer.h](https://github.com/microsoft/DirectXShaderCompiler/blob/main/include/dxc/DxilContainer/DxilContainer.h)，IsValidDxilProgramHeader按给定length容纳program和bitcode检查，并不要求part与program大小相等。后续分析仅针对这对已关联到两眼死亡绘制的实际Shader，不重复启动读取缓存。

该轮另一个同票据HDR／最终图配对在4秒内未取得匹配，监视器明确报告TimeoutError('No exact HDR final pixel match')，未执行后续检查点动作。不能称本轮完成同帧像素验证；文档111／112的两次成功配对保持独立。PSO身份和缓存调用证据不以该配对成功为前提，也不等于Shader语义已还原。

正常停止后46个入口的独立字节校验全部匹配，原生清理、XR退出及普通关闭游戏通过；四份存档恢复并核验。总体测试监视器仍有上述失败，不改写成全通过。

原始缓存、结构报告、状态和GPU资料位于ignored analysis/runtime/hdrpixels98-*、上述会话文件及tools/native-capture/build-hdrpixels57/death-pso-cache-29064-337-0-745265632724.raw*，不打包发布。

下一步沿这对实际Shader检查纹理坐标和颜色输出依赖，再决定最小参数对照，尚未宣布具体根因。此前补充的115字节历史1c6340汇编仅确认R8 owner+0x48和R9 owner+0x50传入绑定缓存，不是字节码指针；当前已从SDK接口取得Shader，不必继续为这个目的追猜RHI内存布局。
