from pathlib import Path
import json,hashlib,datetime,shutil
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r247';root=r/'analysis/instances/cover-1112-r247-cloak-path-units';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());assert v['operator_exit']==1 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['original_started']==v['original_returned']==0 and v['sphere']['cover_path_probe_attempted']=='0'
files=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(files)==1
shutil.copyfile(files[0],o/'cover-boundary-original.json');shutil.copyfile(root/'cover-output/operator.log',o/'operator-original.log');shutil.copyfile(root/'cover-output/projection/recording.json',o/'recording-original.json')
failure=json.loads(files[0].read_text())['candidate_closing_rejection'];d=failure['difference'];assert d['kind']=='registry' and d['before_header']['data']==d['after_header']['data'] and d['before_header']['capacity']==d['after_header']['capacity'] and d['after_header']['count']==d['before_header']['count']+1
(h/'verification.json').write_text(json.dumps(dict(path_attempted=False,original_calls=0,operator_exit=1,normal_cleanup_verified=True,remaining_owned={},candidate_closing_rejection=failure,projection_frames=133,projection_errors=[],personally_reviewed_frames=False,gameplay_accepted=False,headset_accepted=False,scope='Strict pre-input candidate closure rejected one registry append. Not a path-query failure and no retry inside the ended instance.'),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r247' not in old
add='''
## r247：路径观察在原A前候选关闭因registry新增一项而拒绝，未到query（2026-11-12）

新cover-1112-r247-cloak-path-units，准备1f3969ef，r244 Native66a96c12…97f4。原ordinary approach在候选CoverLink读取q.finish时触发原严格World-state metadata/registry changed at closing boundary。已有r180拒绝见证保存确切差异：registry地址5398218544，data372178944/capacity225272保持，count218210→218211。不是identity/字段变更，也不把新增一项当作允许旧样本通过；原读取正确拒绝，未调用原A候选操作与物理path。path attempted0、原query0/0，不能记作path query失败或原source时效失败。

operator1、133实际投影/errors0，正常Native/entry/XR/game清理true、自有remaining空；anchor/reader未变。原run/native、候选拒绝JSON、operator traceback、recording清单完整保留；本轮未亲看帧/未编码，不作完整视觉验证。下一步仅一次新实例完整新鲜读取复核该前置阶段，保持所有registry/identity/metadata/值关闭条件，不开启append-only放宽、不复活旧样本或重复未知原调用。若再失败则围绕新确切拒绝处理，禁止无变化循环。

该前置拒绝不撤销r246实际sphere查询通过，也不证明full self/world/body/view。工程25%，CV24/AC22及头显仍未完成。证据path-units-runtime-r247。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(rejection=d,normal_cleanup=True)))
