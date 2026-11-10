"""Checkpoint the fully checked schema-proof reuse, without gameplay claims."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r158：第二遍重复schema查找改为完整既有证明重读，已编译未运行（2026-11-11）

r157仍在collider_read期间body年龄拒绝且原query0，故继续实际physics/collider复核路径。两个typed reader第一遍仍完整选择/认证原属性与类型；第二遍保留fresh对象identity、原始动态字段及数值条件，但不重新搜索第一遍已认证的schema，而先复核与第一遍完全相同的对象身份/metadata代次，并用原binding::current完整重读对应Proof全部identity与source字段，再消费缓存offset。collider按Pawn/Component/Body三段分别在其动态读取前复核；第一遍无BodyInstance而第二遍出现对象时直接changed，不读取未建立body字段。每次sample原清空证明仍在，不跨sample/帧/query复用。最终members.current与全部Proof关闭检查原文不变，样本入口/出口owner guard事务除last-pass选择参数外原文不变，所有原动态读取调用及physics动态尾部保持；没有改任何年龄、输入或移动写入门。

actual冻结r156对照与新两文件覆盖均通过同一focused738检查（包含原physics282+collider219，不称738全新项），覆盖两遍中途offset、kind、reference、父类、成员Next、inner宽度、metadata变化，以及body空/非空双向转换与失败保持原输出。8个相关consumer/guard/path/overlap/OFF程序分别282/219/207/23/105/391/53/51全0。所有检查树的运行头来自冻结树，测试cpp单独复制，实际依赖无外部worktree。这些是合成数据/Host seam，不是原引擎调用或完整物理证明。

相对r156该层sample的safe_copy次数：physics2/16/64部件分别7076→6313、7580→6817、9308→8545（10.78%/10.07%/8.20%）；collider有body的两种body_data状态均7455→6005（19.45%），无body5407→4571（15.46%）。该计数不是实际query毫秒，不能把不同层收益叠加当真实全链加速，更不能据此称年龄拒绝已修复。

Native58c2fe1d9fa2a108010f33008278060f9ad3a2ccde53d3d87f3e27c829680c7f编译链接0；exact r156冻结树仅cover_physics_roles_live.inl和cover_collider_roles_live.inl两运行文件delta，409项目依赖、外部worktree0、GEARS flags保持。delta.patch/verification.json核对完整关闭和动态调用；prepare新增--native-r158认证精确delta/flags/构建状态，后续启动来源含launch_reserve在内11项绑定。源、对照、测试依赖与构建记录在schema-repeat-r158。本条仅源码/离线/编译，未准备或启动新实例，不替换publicv71和固定入口。

下一步Git保存后准备新私有实例，做一次原CT2/同query实际拒绝或原调用及画面检查，保留r155/r157年龄失败；不刷新旧样本、不放宽年龄、不原样重跑r157。完整CV01–24/AC01–22仍开放，工程25%、实体头显未验收，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['dll_sha256'],build_only=True,game_started=False)))
