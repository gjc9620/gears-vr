# 死亡 Shader 两组常量参数的实际排除

2026-10-17。默认候选仍Reticle47。观察实现提交e5d650e6b6a483d0933226f6f3d84030b65967ca；独立Hdrpixels56 DLL SHA256c2cbf500185d9efc513c8c692c3f50090dd4a5869de1ac2cf4fb576c3a03f3c5。完整DLL及17项冻结CPU检查通过；XR沿用Hdrmatched55。

## 有界观察与汇编依据

先检索KB再在独立IDA副本机械导出：setter1760b4共205字节、调用分支d80630共1104字节，与artifact1历史blob逐字节一致，哈希分别d7d20d3f5c510685b8b3ffb12d12d51ae583d4070e00cf11a39655c7e086c45b和cd8bb3b4a32cf5de1893c8cb9dcf85dabfbe5b3614d90d9ed9c1834afe548314。原库哈希未变，owned IDA lease已关闭。

Hex-Rays伪代码的参数位置不可靠；汇编为RDX描述符、R8数据、R9索引。仅增加一个默认关闭的只读setter Hook，运行时认证上述两个代码范围后安装。限定第49个无占有票据、左右眼各一次ratio（owner+0xB2，即十进制178）和transform（owner+0xAA，即十进制170），在现有d80a7b入口各记录一次比较。原参数和返回值不变；不在quad处重读已经复用的R8栈临时地址，不将highwater当分配容量。

## 实际96

会话continuous-stereo-32756-20261017-123126-4ba5868d，选中票据2352/world1。四个setter、两个quad均有记录；重复与拒绝均0，左右owner相同、各眼视图和矩形稳定。两个代码范围在实际进程匹配。

四个描述符在setter前后和quad处均为 **65535,65535,0**；调用的float4均为 **.5,-.5,.5,.5**，原setter返回0。认证汇编在descriptor[2]为0时直接返回，不读取TLS也不写常量。因此它们在本次variant中是未激活的参数；不是观察器未能读出有效上传，也没有证据支持“右眼漏更新这两组CPU常量”。“被编译器优化掉”是解释之一，未直接证明。

第二次同票据像素对照成功：2352/world1/copy_seq170，HDR右半均值.027793/.030564/.029827、有限通道100%；最终右眼97.5370%像素RGB≤8，左眼保留场景。重复确认了文档111的故障区间；没有确认Shader实际纹理采样的具体根因。

证据为ignored analysis/runtime/hdrpixels96-exact-pixel-audit.json、该会话.native-status.death-constants.json及原始状态、RAW和GPU图。不能将comparison_verified=0解释为GPU上传失败：这里描述符大小0，TLS读取明确不适用。

## 停止校验的独立问题与复核

实际96原生报告正常清理，XR正常退出，但退出校验脚本未收录新setter入口，拒绝将游戏模块地址当作图形系统模块校验，整会话如实标incomplete。游戏正常关闭、四份存档恢复通过；未声称当轮入口独立校验通过。

提交914e272为精确RVA1760b4添加固定205字节历史SHA256的独立停止后校验，没有接受日志中学习出的期望字节。实际97复用同一不可变Hdrpixels56，启动并观察新双目提交后停止；**44个入口全部匹配，包括setter完整205字节哈希**。phase=stopped、原生清理、XR退出及普通游戏关闭通过；四份存档恢复核验通过。这次短测只复核退出路径，不替代96的死亡像素对照。

下一步停止围绕这两组未激活参数试改，定向寻找实际死亡VS/PS的认证绑定／字节码来源，再检查HDR与小尺寸辅助纹理的坐标依赖。未证实指针布局前不从owner猜字节码地址，不增加通用PSO创建观察器。首版及真头显回归仍未完成。
