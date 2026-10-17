# Reticle47：修复开场 CG 后的图像发布

2026-10-17。本地默认 Reticle47／Borderless1，命名清单 `manifest-reticle47-borderless-vd.json`；保留 Reticle46 及更早版本回退，用户 config 不覆盖。源码冻结 `9e2f073`，DLL SHA256 `a1e65907970fc2b8fd626bc8c8fef0383d4bac5dbfd0fa89ab34e6417b2f3e1d`。诊断调用边界观察器关闭。延续 Reticle46 的准星来源修复，统一命令列表跟踪容量，包含受控角色链接最终校验变化的启动重试。

## 实际回归

Prison81 在开场后双眼准备5535次却始终发布0帧，绑定表256项与关联表512项不一致。Prison82 同章节对照中绑定表实际高水位378，溢出0，成功发布2180帧，两眼快照均有关卡图像。完整证据、最早一张全黑过渡快照及结论边界见[发布中断诊断](101-prison-post-cinematic-publication.md)。这证明该关卡的发布阻断已修复，不能推广为全部间歇黑屏已解决。

11项聚焦可执行测试通过，FrameLinkTest为21078项检查；启动另执行必要自测。受控角色链接重试的59项Python测试通过，但本次真实准备未触发该新分支，不能称其运行时覆盖完成。

## 独立便携包

ZIP：`analysis/release-zips/GearsVR-V1-development-Reticle47-20261017.zip`，SHA256 `fd083cfda47bf618baef6a48f31f24a2e0206c1afdc185f78db16e3af1ea6567`。701成员，13,638,160字节；700个payload文件，共43,432,675字节。inventory SHA256 `34c6c646f202835a62c09b17eefacb399dfb6515889db02f30c714199d4546fd`。严格按允许清单打包，不包含游戏、模拟器或逆向资料。

全新解压目录的包内Python以-I/-B隔离启动，通过独立预检，再实际启动包外游戏。portable83 会话 `continuous-stereo-8908-20261017-095543-3906443f`，真实首投影及两个3840×2160 GPU快照通过，两眼均有非黑场景；停止后phase=stopped、passed=true、native_cleanup_verified=true，独立恢复和普通关闭游戏通过。运行后700个清单文件哈希均未改变。使用显式测试模拟器运行时，未改全局OpenXR注册表。

这是本机开发候选包，未公开发布，release_ready=false。干净玩家机器、GUI安装、真头显准星融合／字幕、真实机枪／载具、更多CG及长时间连续性仍待完成。启动及检查点仍可能秒级延迟。

后续仓库实测：三次检查点恢复及其连续性边界见[恢复与过渡缺口](103-reticle47-checkpoint-continuity.md)；过渡提示卡独立GPU上传和双眼合成检查见[GPU隔离实验](104-transition-card-gpu-isolation.md)。没有据此更换DLL或改写已生成ZIP，也未宣布全部黑屏解决。
