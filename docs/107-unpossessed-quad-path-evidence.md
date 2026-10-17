# 死亡右眼场景的逐票据诊断

2026-10-17。默认仍Reticle47。独立Unpossessed50仅观察，不是可交付修复。

源提交 `e00b1b2bfed2af38bf6ff7683699ae5b9d6ee85f`，冻结目录 `analysis/native-source-unpossessed50`，DLL SHA256 `61bb8f7d8a4de553b2a0ce8bcc1ced20f2414b0a44cde502575aa455de50b51d`。新增默认关闭 `GEARS_UNPOSSESSED_QUAD_OBSERVER`，冻结内部FrameTicket相机模式，在已有consume/quad/Draw/Execute观察点选前3及第49个不同无占有票据；每票据上限64种调用形状，记录重复计数、丢弃数。当前native view与final-quad眼别分开，未知读取保留位；资源仅为已绑定引擎对象提示，Execute为调用前CPU关联，不证明GPU完成。无新增Hook、GPU复制、放行或参数修改。13项冻结CPU测试通过。

实际会话 `continuous-stereo-39268-20261017-110122-8c758782`：票据666、667、668、714各54行、无丢弃、均有拦截到的Draw；源GPU票据691、722均在512条容量的明确无占有提交记录中。右眼近黑像素分别97.10%和97.81%，左眼有死亡场景；前后正常游玩票据289/982双眼非黑。因此新路径的新图问题已重复，不是仅凭首末区间推断模式。

四票据均出现 `d80a7b` 死亡处理分支，之后 `2b11a0` 最终输出。已有普通 `d83cbf` 路径消失的关联进一步缩小到该分支，但不能仅凭caller或UV猜根因。部分局部中间纹理使用0原点可能是合理复用；必须对照具体输入纹理、矩形及静态调用上下文。记录的native眼别表明两眼都执行了分支，不代表两眼GPU场景正确。

一次检查点world1→2，恢复观察4907ms，包含加载。正常原生清理、游戏正常关闭、四份存档恢复并核验通过。菜单名称分块修正连续两轮自动导航成功。尚不提升候选或重打发行包；真实头显、全部CG/载具及UI回归仍待完成。

证据均留在忽略目录：`analysis/runtime/unpossessed89-{audit,normal-close,gpu-audit,save-restored}.json`，会话`.native-status.txt`、`.native-status.unpossessed-quads.json`、stdout和四张GPU图。相关历史原生函数先查知识库，保留artifact1快照身份；不能仅凭RVA把静态伪代码当本轮运行字节。
