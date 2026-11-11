"""Checkpoint query-local collider rechecks and the reproduced closing gap."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r167' not in old
entry='''
## r167：同query碰撞体复核复用已完整重证的布局，并补动态字段关闭缺口；已编译未运行（2026-11-11）

依据r166五轮collider复核累计13.6629ms，本轮只将Session::SourceGuard内部collider调用改为recheck；每个新execute的初始sample仍清空并建立全新证明。sample仅首尾guard和关闭全部通过后登记sample_owner及ready；recheck核对完整owner key/Scene/special_move，同步作用域内才可使用。进入复核事务即清ready，任何该事务失败均不能重试复活旧证明；原SourceGuard对初始Snapshot的same比较仍保留，不新增写入或动作许可。跨query/帧不复用，下一execute必须sample。物理reader与其他模块未改。

recheck每轮仍两遍读取实际Pawn/Cylinder/BodyInstance数据，使用各阶段offset前由既有values(repeat=true)重读完整identity/source证明，最终metadata/member链/Pawn/component/body全部Proof关闭保持；首尾owner guard仍调用，不改来源时间、年龄、输入、释放或反重放。与旧sample差别是跳过已认证结构的重复查找，及在两遍之间逐次与最初snapshot核对。

新增fixture还复现了原scalar动态读的具体缺口：第二遍BodyData读取时才改变此前已读的OwnerComponent，旧sample+SourceGuard同值比较仍接受；首版复用候选也接受，checks-v1失败保留。最终将8项动态字段（Pawn两指针、component owner/body、body index/owner/data/spring）纳入各自既有Proof::source_field，实际双读/记账并在完整current关闭时重读，原失败在新候选拒绝且不更改输出。这是该离线时序缺口修复，不推断实际游戏已复现或所有并发变化均已覆盖。

checks-v2保留了原BoneSpring测试因读取方式改变而错触发的失败：原hook挂BodyData第2读，改为paired reader后仍属第一遍，发生在首次BoneSpring观察前。v3将hook改为BoneSpring自身第2观察，before/after都检查真实两次观察间变化；不删除该检查或放宽预期。最终before380/after407全0，包含既有collider219，不称407全新项；无可用证明状态、同owner多轮、三种body presence、布局/identity/metadata变更、两阶段动态变更、失败不复活、不同Scene、首尾guard、锁竞争/外线程及输出保持均有聚焦检查。旧baseline晚到owner接受在v3明确单独记录，新的拒绝与其对应，不把旧失败改成修复通过。相关collider219/component207/guard23/query105/path391/overlap53/OFF51全0；Host dispatch与合成内存不调用原引擎。

五次实际reader复核safe_copy计数：无BodyInstance22855→18375（19.60%），body存在且BodyData空/非空均30025→22255（25.88%）。该计数包含新增动态关闭读取；不是原query毫秒或全链提速，initial sample新增的动态读取开销也需实际测量。Native a41fadcf144fdde43a2573b1e5f52530e43ceca8eef256439ed505d0994b3ce8编译链接0；exact r165冻结树只改cover_collider_roles_live.inl和cover_physics_query_live.inl两运行文件，410项目依赖、外部worktree0、GEARS flags保持。verify核对原调用分支只换collider重查入口、两遍/首尾guard/原关闭均保留；prepare新增--native-r167并认证精确delta/构建状态。源、初失败、最终检查/对照/依赖/构建与核对在collider-recheck-r167。

下一步保存后以新实例验证原CT2与同query分段，确认重复collider实际成本、查询到达位置及画面；不原样重跑r166、不放宽时限、不刷新旧样本。此条仅源码/离线/编译，尚未准备或启动新游戏；完整CV01–24/AC01–22、身体来源年龄/自身遮挡/头显仍未完成，工程25%、publicv71/固定入口保持，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],build_only=True,game_started=False)))
