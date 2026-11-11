from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rs={n:json.loads((h/(n+'-result.json')).read_text()) for n in ['before','counter','engine','actor','session','off']}
for n,v in rs.items():
 v['source_sha256']={k.replace(chr(92),'/'):d for k,d in v['source_sha256'].items()}
 assert v['build_exit']==0 and v['test_exit']==(1 if n=='before' else 0)
 if n!='before':
  for p,d in v['source_sha256'].items():assert sha(r/p)==d,(n,p)
counter='tools/native-capture/cover_physx_shape_owner_fields_test.cpp';assert rs['before']['source_sha256'][counter]==rs['counter']['source_sha256'][counter]
for name in ['cover_physx_actor_shapes.h','cover_physx_shape_owner.h']:
 assert sha(h/('before-'+name))==rs['before']['source_sha256']['tools/native-capture/'+name]
n=json.loads((h/'native-build-v2-result.json').read_text());m=json.loads((h/'native-source-v2-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for name in m['overlay']:assert sha(r/'tools/native-capture'/name)==m['files']['tools/native-capture/'+name]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v2'/p)==d
assert sha(h/'native-build-v2/GearsCapture.dll')==n['dll_sha256']
v=dict(results={k:x['output'] for k,x in rs.items()},same_counter_source=True,original_static_evidence='shape-stage-runtime-r230/original-owner-construction.json',
 native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,game_started=False,installed=False,
 change='Shape+d0 public Actor must match public actor; Shape+148 optional Body must independently match actor+40. Correct both actor inventory and per-shape ownership readers, preserve all links/source/closure guards. Update older synthetic fixtures to original constructor layout. Same9-counter: before6fail after0; old code also wrongly accepted replacing public actor field with Body pointer.',
 remaining='Actual repaired game read not run yet. Full CV24/AC22, world/self boundary, template-active identity, body/view/hit/muzzle consistency, severe view obstruction and headset acceptance open.',engineering_percent=25)
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r231' not in old
add='''
## r231：修正Shape公开Actor／optional Body字段混淆，同counter旧6失败→新0（2026-11-12）

按r230实际Stage15与原124660→b0790构造证据，新增独立cover_physx_shape_owner_fields_test.cpp，明确原Shape+d0=public Actor、+148=optional Body、NpActor+40=Body；不从被测reader推expected字段。相同counter源码/实际原DLL仅作为PE数据：旧header9检查6失败，既错误拒绝正确dynamic/static，也错误接受把public owner字段替换成Body。旧两个product header与旧依赖/log保留；修复后同9检查0。

actor_shapes与shape_owner两处同时修正：d0独立等于当前public Actor，148独立等于NpActor+40 optional Body，最小内部shape pointer范围扩至150。保留已有public↔internal shape、actor backlink、scene、type code、membership双pass、owner/source/关闭比较；不是删除Body校验或将错归属当non-self。原Body为空static路径仍受两条分别资格检查。下游world的internal_actor仍指optional Body，语义不改。三个旧synthetic fixture改为原构造字段，counter自身源码不变。

Engine1741累计0（owner531、parameter702、world1706、asset1732），新增read引出额外故障点逐一拒绝；actor inventory79、Session125、OFF65均0。原DLL未加载/未分派query，Session空Host packet仅作owner事务回归。完整Native以r229冻结树仅两个product header overlay，flags不变，422依赖逐SHA无外部混入；DLL SHA3ca85b2f8642d8a3ef9e58f030ca58be18c94df586c24607c71d3ce36b157471。未安装/未游戏运行此修复，下一次新shape模式须验证实际失败能否越过此点，不提前称整批成功。

完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务、视图遮挡与头显仍未完成，工程25%；publicv71/固定手动入口/Quest不变。证据shape-owner-fields-r231。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),same_counter_source=True,documents_equal=doc.read_bytes()==mirror.read_bytes())))
