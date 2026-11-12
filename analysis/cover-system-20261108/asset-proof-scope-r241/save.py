from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;runtime=h.parent/'pose-contract-runtime-r240/runtime-r240';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();rs={k:json.loads((h/(k+'-result.json')).read_text()) for k in ['session','engine','off']}
for k,v in rs.items():
 assert v['build_exit']==v['test_exit']==0
 for p,d in v['source_sha256'].items():assert sha(r/p)==d,(k,p)
n=json.loads((h/'native-build-v3-result.json').read_text());m=json.loads((h/'native-source-v3-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(r/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v3'/p)==d
assert sha(h/'native-build-v3/GearsCapture.dll')==n['dll_sha256']
(h/'verification.json').write_text(json.dumps(dict(results={k:v['output'] for k,v in rs.items()},native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,game_started=False,installed=False,change='Borrow typed layout proof only within synchronous no-dispatch asset read; native Source/asset/actor/shape/world full value closes unchanged. Initial roles sample and final full SourceGuard roles recheck, owner/cover/pose/original freshness checks still required before public output. Metadata cache/generation/read-ready/current context checked by each borrowed guard. Ordinary query branches unchanged.',tests='Session202: real supplied2-actor observer packet and cached typed match, one complete physics recheck plus initial sample, every full owner reject, expiry/drain/root replacement, six persistent body/property mutations, wrong actor packet; output stays private on all failure. Host still has no shape/world/original dispatch. Engine1743 separate actual QueryEngine fixture; OFF65.',remaining='Need new actual game read within original source deadline; r240 internal observer complete but whole read failed original age after98.8438ms. Full self/world/template/geometry/body/view/hit/muzzle/headset remain open.',engineering_percent=25),indent=2)+'\n')
v=json.loads((runtime/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
media=json.loads((runtime/'media-review.json').read_text());media['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in media['contacts']];media['full_span_personally_reviewed']=False;media['visual_acceptance']=False;(runtime/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r240' not in old and b'## r241' not in old
actual=json.loads((runtime/'run-result-original.json').read_text())['terminal_session']['game_identity'];add='''
## r240：内部完整清单两遍通过，98.8438ms后全owner关闭因原时效拒绝（2026-11-12）

新cover-1112-r240-cloak-pose-contract使用r239 Native828e7e24…82c6，准备/导入/身份guard14通过，准备fc4d6e25；actual game{pid}/creation{creation}。完整asset observer detail0，failed_actor/member均unset，最后shape/world两pass完成，内部清单及姿态已全部比较一致；但整个事务98.8438ms，原full owner5/failed5 context source_age_expired extra6，Session1172/failed1172 owner_end，q4 source拒绝。原query调用0、正式bodies/actors/shapes0，不能把内部成功称作整批已发布或时效通过。

266实际投影/errors0、14总览生成、5阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，精确CLOAK/前台计数见closeout，anchor/reader前后相同。证据pose-contract-runtime-r240。

## r241：同一无分派只读事务借用入口字段证明，完整关闭前重新校验（2026-11-12）

实际r240的1172个SourceGuard重复完整角色字段回读是主要剩余工作量；本轮只对asset Snapshot的同步NO-DISPATCH分组新增AssetReadGuard。初始roles::sample仍两pass证明全部类/字段/引用/数组/body动态值；组内每个guard确认read-ready、当前metadata cache/generation、相同线程/busy/context/key/metadata，并用原matches_query_source/matches_asset_actors核对每个原生来源快照。原生Source/asset/actor/member/shape/pose双pass与全部代码/归属闭合保持。完整typed roles::recheck移到最终SourceGuard关闭（两遍值、初始比较、完整前后Proof）；原full owner/cover/pose/source时间在分组前后和关闭仍执行。只有全部关闭才发布私有结果，没有跨execute的证明借用，也不将此guard用于真实query分派。

Session202检查0：从原空Host packet推进到实际asset actor observer读取合成当前typed内存中的2body/2actor，错误raw actor packet拒绝；原始shape/world/original模块执行仍是Host seam，无这些玩法证据。初始sample之后完整physics_rechecks=1，64重复typed来源匹配、full owner6；每个full拒绝、expired即使私有结果已写仍不发布、root替换/drain以及六种持久修改（BodyIndex/OwnerComponent/BoneSpring、属性offset/width/reference class）均拒绝输出保持。初测两失败是mutation写到fixture未使用位置，依据既有property helper改正确94/d0，仅改测试目标，失败log/result及说明保留，不当成产品修复。Engine1743、OFF65通过。

C0 summary新增完整回读/借用guard计数，候选计数明确requires_close，成功时额外导出Body实例ordinal→唯一Actor index及地址/scene/internal映射；只读取已完成owned scratch，不增加game读。完整Native在r239最终冻结树仅Session/C0两文件overlay，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHA{dll}。此版尚未游戏运行，下一步新实例检查完整读取、原deadline关闭和真实proof计数。

完整CV24/AC22、active/template/full self/world fence、非strict数值包络、body/view/hit/muzzle、视图遮挡及头显仍未完成，工程25%，publicv71/固定入口/Quest不变。证据asset-proof-scope-r241。
'''.format(pid=actual['pid'],creation=actual['creation_time'],dll=n['dll_sha256'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),actual_game=actual['pid'],documents_equal=doc.read_bytes()==mirror.read_bytes())))
