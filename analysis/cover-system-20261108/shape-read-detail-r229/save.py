from pathlib import Path
import json,hashlib,datetime
root=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
results={n:json.loads((h/(n+'-result.json')).read_text()) for n in ['engine','off']}
for n,v in results.items():
 assert v['build_exit']==v['test_exit']==0
 for p,d in v['source_sha256'].items():assert sha(root/p)==d,(n,p)
n=json.loads((h/'native-build-v2-result.json').read_text());m=json.loads((h/'native-source-v2-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(root/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v2'/p)==d
assert sha(h/'native-build-v2/GearsCapture.dll')==n['dll_sha256']
v=dict(results={k:x['output'] for k,x in results.items()},native_sha256=n['dll_sha256'],project_dependencies=len(n['project_dependencies']),flags_unchanged=True,
change='Preserve exact actor-shape reads and accept/reject policy; store named read progress/member index per pass. One-shot summary exports nested asset/shape/world codes, failed actor/member and partial scratch headers/arrays after transaction finished, clearly unpublished. No extra game reads during logging.',
remaining='r228 actual failure nested stage unknown; need new controlled r229 shape-mode observation. No active/template identity/world/self/body/view/hit/muzzle/headset acceptance.',game_started=False,installed=False,engineering_percent=25)
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=root/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=root/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r229' not in old
add='''
## r229：针对r228真实拒绝补形状清单分阶段进度，完整Native通过（2026-11-12）

cover_physx_actor_shapes新增Stage/Progress，分别保留first/last两pass实际进行到的读取/检查阶段及member序号；原read调用、顺序、布局、guard及接受/拒绝条件不变。阶段包含actor header/links/slots、Body pointer、backlink、root header/slot/kind、array读取/count、member/internal header/slot、parent/body/public归属及type table、重复和完成。不是新增诊断查询或重试；最终Snapshot只在原双pass关闭成功时发布。

一次性形状summary在finished后输出原workspace已有asset_code/shape_code/world_code、failed_actor/member、两pass progress及first scratch中已读取的actor/root/type/count/arrays；显式partial_unpublished=1，不能把scratch当作成功清单。不在summary内新增game读或原函数分派，保持原私有锁与只读事务边界。

Engine1723累计检查0，含owner519/parameters687/world1688/asset1714与实际QueryEngine/HostImage；原DLL只作为PE数据，无原查询执行。OFF65检查0。独立完整Native在r227冻结树上仅两文件overlay，flags保持，{deps}项目依赖逐SHA、无外部工作树混入，DLL SHA{dll}。该版本尚未游戏运行；下一次新shape观察用于定位r228未导出的nested失败，不能用本次编译声称失败已修。

完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle、视图遮挡和头显仍未完成，工程25%，publicv71/固定入口不变。证据shape-read-detail-r229。
'''.format(deps=len(n['project_dependencies']),dll=n['dll_sha256'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
