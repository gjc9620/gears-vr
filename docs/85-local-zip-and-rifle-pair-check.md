# V1本地测试ZIP与新步枪配对检查

2026-10-17。第一版仍未完成；本地可试候选不等于正式发布或真机验收。

## 可交付的本地测试压缩包

新增开发工具tools/vr-launcher/package_portable.py，只按不可变stage inventory列出的文件及inventory.json创建ZIP，不递归收集试运行后的目录。校验路径、核心文件完整性、重复项、大小／哈希，排除analysis／游戏／快照／资料库等路径，拒绝重解析点；写入临时文件后重新打开核验每个成员与清单，成功后无覆盖发布。11项测试通过，覆盖运行时新增日志排除、篡改／缺失payload、路径／链接、已有输出保持及失败清理。

实际输入为已完成独立包运行回归的caption38-rebind5-20261017，700项payload全部仍符合原清单。生成analysis/release-zips/GearsVR-V1-development-Caption38-20261017.zip，共701成员（含inventory.json），43,420,731字节payload，压缩包13,634,630字节；SHA256 c7204e0ae2625d56b011fd81dbc66c835e86c3fec66356f2ab25506200669d2d。运行后stage中额外的原始GPU图像、游戏绝对路径回执及日志未包含。

校验ZIP哈希后重新解压到新的caption38-zip6-extracted-20261017；包内Python隔离预检通过，没有启动游戏或GUI。原stage的实际游戏回归见docs82，解压副本未重复启动游戏。此包保留development candidate／release_ready=false说明；未上传或公开发布。

使用方式已在包内中英文README：完整解压、打开GearsVR Launcher.exe、选择GOWDE-Steam.exe；Quest／VD用户先连接VD并选择VDXR，再启动VR。游戏内设置分辨率，DLSS／DLAA保持锁定关闭。此包仍默认镜头拉近0、HUD65°，不是本机45°／0.3米调试值。

原始回执analysis/runtime/portable50-zip-receipt.json、zip-tests.log、extracted-preflight.json保留。压缩包工具属于开发流程，不作为玩家运行依赖。

## 等待实际双目就绪后的步枪样本

新会话continuous-stereo-14348-20261017-064240-b072663e，Caption38／Borderless1。先检查至少10个epoch内提交、发布图像与首次实际projection记录，再开始动作。只读确认实例Weap_AssaultRifle。先LT两秒，之后另一个LT／RT三秒请求；每个普通控制器请求最终归中，后台实体手柄保持关闭。

子采样器有94次有效采样，AmmoUsedCount从4到35、SpareAmmoCount600，之后因RegistryHeaderChanged停止。协调器的动作流程error=null不能覆盖子采样器错误，故不记完整8秒稳定通过。该拒绝保留，没有为了通过测试放宽注册表校验。原生停止、入口恢复、XR退出与游戏正常关闭通过。

六份4K收据中，ticket117／138在开火请求之前；174／184约在请求后0.30／0.58秒，203约1.06秒，263约2.64秒。实际图像有前期开镜、短暂退镜再进入开镜的过程；与两个请求之间的归中／重开镜时序重合，不把缺少可见准星的早期帧强行配成内外圈。源图／缩略图和动作时刻保留。

原生准星considered／planned／applied均390，居中、几何、投影拒绝0，有限点应用390、方向回退0。保留ticket92／93／94／95各有同一root／list／target的16顶点4范围与4顶点1范围配对，每眼viewport／scissor逐字一致，命令完成／内容匹配标记存在。它证明这些已记录范围应用同一偏移，不能和不同票据的截图混成同一帧，也不证明纹理alpha、独立可见中心点、子弹命中或物理融合。

证据analysis/runtime/rifle50-audit.json、ammo.json、pair-audit.json、before-fire-centres.png、normal-close.json，以及会话原始状态／GPU／日志。UI问题仍开放；没有仅凭390次通过计数宣布用户反馈已解决。
