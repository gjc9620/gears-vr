"""Save the checked query-scoped physics optimization before another game run."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
p=H.parent/'isolated-r105/prepare.py';s=p.read_text(encoding='utf-8-sig').rstrip()+'\n'
compile(s,str(p),'exec');assert "parser.add_argument('--native-r175'" in s;p.write_text(s,encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r175' not in old
entry='''
## r175：同query物理角色复核复用已认证布局，动态字段与成员收尾补齐；已编译（2026-11-11）

本轮progress。从r174实际query29.1036ms、最旧stereo首次12.2946ms、七轮physics/collider复核和40ms拒绝继续。发现SourceGuard每次physics复核都调用sample清空布局并重推属性链，而collider已有同一execute内的recheck。因此physics新增同样有界入口：每个execute仍先sample并使旧ready失效，成功才保存精确Binding/Scene/special_move/完整pose metadata；只有同一次同步查询的SourceGuard改用recheck。每轮进入先清ready，保留两个真实values读取、各自与initial比较、完整prior proofs在偏移前复核和原closing；任何已进入的失败不能复活旧certificate，重新sample才恢复。不同线程/Scene/metadata、锁失败、owner拒绝及异常均有定向检查，不跨Tick/execute缓存，不续原始时间/放宽40ms，不增加writer、input、original call或grant。

动态字段还进入已有Proof的双读和最终current：mesh PhysicsAsset/PhysicsAssetInstance两指针、PAA Owner/Bodies data/count/capacity/Root共七项，以及每个Body的真实数组成员、BodyIndex、OwnerComponent、BodyData和BoneSpring五项。原完整数组读取、身份、空值、重复/容量检查都保持，未截短64项。repeat只在完整instance/body proofs已重读、fresh owner identity等于initial、mesh类/引用精确一致时复用先前owner/mesh类关系推导，所有路径身份/父链仍由原完整Proof复核；首次sample独立重建。既有字段改写后即使对象地址相同也不能延用。

实际生产reader前/后对照530/562项均0，包含原physics282项；旧版有12种第二次读后动态变更未被closing覆盖，新版全部拒绝：两个OwnerComponent、Root、Bodies指针、Mesh instance、BodyData、BodyIndex、PAA Owner、Mesh asset、capacity/count及非root数组成员。BoneSpring刺激的fixture回调在copy前执行，原版已能拒绝这一项，保持其拒绝、不伪称旧漏洞。每次五轮复核，2 Body读数31565→22945（-27.31%）、16 Body34085→27355（-19.74%）、64 Body42725→42475（-0.585%）；最大场景收益较小，原因是补齐每个成员的收尾检查，未为性能删除它。读次数不是游戏时间，不称已消除实际年龄拒绝。相关collider219/component207/guard23/query105/path391/overlap53/OFF51全部0。

早期结果保留：checks-v1功能0但64 Body读取增加1.10%，未放宽比较；v2在重证后复用类关系推导改善。v3扩大动态反例时把BoneSpring当前读误标成late漏洞，before一项失败，按真实回调时相改回原拒绝；v4全530/562及相关检查通过。又补Bodies逐成员Proof后完整重编，旧Native/source/build/manifest保存为-v1历史，最终receipt绑定当前文件，不冒领旧构建。完整源码/计数/失败/依赖/delta/verify在physics-recheck-r175。

最终Native1e037608fc9b3722f0bc80bd0afbeac1f4499d85033c69b1954fa337f8491dfb编译链接0，exact r173冻结树仅cover_physics_roles_live.inl及cover_physics_query_live.inl两文件delta，413项目依赖、外部worktree0、GEARS flags保持，其余冻结源逐项相同。r173真实NpActor内嵌拥有者修复保持。prepare新增--native-r175，仍校验精确delta/原依赖和build-only状态。本条尚未准备或启动新游戏，不将离线速度外推实际query通过。

为持续验证空间，按既有结束实例缓存整理授权，仅对本chat的r170/r172/r174逐项正常cleanup、精确game/owner/XR/worker不在再次核验后，将315个PSOCache文件共2655147954字节复制到J:/gearsVr/analysis/cover-cache-archive-20261111-r175；全部目标/source SHA复核一致、再次核验进程不在才逐文件移除C盘缓存。只移除这三处缓存文件，目录/源码/文本数值证据/媒体/存档/配置/seed及其他实例均保持；cache-archive.json记录全部哈希与路径。没有删除其他owner或活动实例文件。

下一步Git保存后以新实例短查重复physics实际耗时、40ms入口余量、原查询started/returned和第一个实际失败；不原样重跑未改候选，也不以较好运气的一轮代替修复。真实年龄、NpActor修复实跑、严重自身模型遮挡、完整CV01–24/AC01–22及实体头显仍未完成，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],checks_before=530,checks_after=562,build_only=True,game_started=False)))
