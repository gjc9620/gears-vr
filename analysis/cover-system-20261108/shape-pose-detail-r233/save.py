from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();rs={k:json.loads((h/(k+'-result.json')).read_text()) for k in ['engine','off']}
for k,v in rs.items():
 assert v['build_exit']==v['test_exit']==0
 for p,d in v['source_sha256'].items():assert sha(r/p)==d,(k,p)
n=json.loads((h/'native-build-v2-result.json').read_text());m=json.loads((h/'native-source-v2-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(r/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v2'/p)==d
assert sha(h/'native-build-v2/GearsCapture.dll')==n['dll_sha256']
(h/'verification.json').write_text(json.dumps(dict(results={k:v['output'] for k,v in rs.items()},native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,game_started=False,installed=False,change='World read diagnostic scratch stores named first/last stages and already-read shape/body raw pose. No new memory reads or altered quaternion/rigid thresholds; complete output still atomic. Explicit stored/body invalid quaternion fault cases verify exact failed input preserved only in scratch.',remaining='Need actual failed pose input from new candidate; full CV24/AC22, self/world/template/body/view/hit/muzzle/headset open.',engineering_percent=25),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r233' not in old
add='''
## r233：world pose拒绝保留已有原始输入与阶段，数学规则不变（2026-11-12）

针对r232 actual world::pose，world Workspace增加first/last Diagnostic，分参数/shape slot/stored pose/body header/class/values/body pose/compose/complete；保留已有读取的shape stored位置/四元数、Body table/位置/四元数。只在owned scratch记录，仍仅原全部关闭成功发布正式Snapshot；summary在一次finished后输出明确pose_unpublished=1，不增加game读、不执行原getter，不改scalar/normal/rigid/norm阈值或归一化。

新增两处定向检查分别损坏stored与Body的w为2，验证拒绝、正式输出保持、正确阶段以及精确原输入在scratch保留。Engine1743累计0（world1708/asset1734），原owner read_budget228、parameter_reads102、world_reads140保持，OFF65通过；说明诊断没有新增memory reads。完整Native在r231冻结树仅world与C0两个文件overlay，422依赖逐SHA、flags不变、无外部工作树混入，DLL SHA{dll}。此版尚无游戏内pose输入证据，下一次明确新版本读取后再判断具体布局/数学问题。

完整CV24/AC22、full self/world fence、active/template、body/view/hit/muzzle、视图遮挡与头显仍未完成，工程25%，publicv71/固定手动入口/Quest不变。证据shape-pose-detail-r233。
'''.format(dll=n['dll_sha256']);new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
