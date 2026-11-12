from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
cases={k:json.loads((h/(k+'-result.json')).read_text()) for k in ('geometry','off')}
for name,c in cases.items():
 assert c['build_exit']==c['test_exit']==0
 for p,d in c['source_sha256'].items():assert sha(r/p)==d,(name,p)
n=json.loads((h/'native-build-v4-result.json').read_text());m=json.loads((h/'native-source-v4-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for name in m['overlay']:assert sha(r/'tools/native-capture'/name)==m['files']['tools/native-capture/'+name]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v4'/p)==d
assert sha(h/'native-build-v4/GearsCapture.dll')==n['dll_sha256']
v=dict(native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,results={k:c['output'] for k,c in cases.items()},reserve_ns=30000000,source_times_renewed=False,original_deadlines_unchanged=True,scope='Scheduling preflight before the first diagnostic path attempt only. All original source/identity/cover/metadata/query checks still apply. No frozen candidate, original redispatch, proof lease, result or movement permission is created by admission.',game_started=False,installed=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25)
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r251' not in old
add='''
## r251：仅首次path尝试前按原始时钟预留工作余量，不续期或重派（2026-11-12）

新增cover_query_headroom.h纯整数计算：读取原born/max_age窗口，先拒绝null/空/过多窗口、非正/未来时间、无效reserve；按无溢出的unsigned年龄减法求最少remaining，与工作reserve比较。原生40ms tracking/50ms parent限制保持；Report不改输入、不授予来源或完成lease，ready之后若过期仍由原完整guard拒绝。2048检查0，含r250真实25.9772ms旧样本只余14.0228ms、重复检查不变新、边界相等/一纳秒、过期/未来/缺失、uint64溢出与ready后过期。review补充极大maximum_age且age0时应初始化首个limiting clock诊断，v3中间编译不运行，最终v4完整重编译2048检查通过；不称实际运行故障修复。

C0只对one-shot diagnostic path加首次工作准入：独立BodyTickSource取metadata_begin（原50ms）、tracking begin/end/stereo/controller/sync（原40ms），如有parent equipment同时取原equipment起点与原上限；legacy TickInput另按原parent窗口，Core mode仅取原实际存在的controller窗口。30ms reserve是可调调度初值，尚未证明足够完整path，不是新age上限/保证，也不覆盖所有非时钟身份条件。余量不足返回当前C0，common.attempted仍false；下一次读取的是新C0自身完整current source/pose，绝不保存旧pose/goal或修改时间。首次开始后原attempted闭锁继续，已尝试、返回后拒绝或未知原调用都不自动重试。

准入在owner GT、当前cover measurement之后；原query wrapper所有identity/cover/metadata/完整typed/release/source年龄条件保持。记录checks/deferred/invalid/mode及first/last/admitted原now/remaining/limiting/born；明确timestamps_renewed0、work_admission_only1、query/movement_permission0。summary与GT用现有mutex读写这些标量，不跨原调用持锁。OFF65通过。

最终Native基于r249最终冻结树仅C0与新headroom头overlay，426依赖逐SHA、flags不变、无外部工作树，DLL SHA{dll}。候选为native-build-v4，不选中间v3；尚未游戏运行/安装。下一步新实例验证是否等到有足够原始余量、仅一次attempt与实际原查询全部关闭；若仍拒绝按原失败处理，不宣称完整path/碰撞/世界fence通过。完整CV24/AC22与身体-view-hit-muzzle/遮挡/头显继续未完成，工程25%，publicv71/固定入口/Quest保持。证据query-headroom-r251。
'''.format(dll=n['dll_sha256'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
