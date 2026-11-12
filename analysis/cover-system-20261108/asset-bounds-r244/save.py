from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
cases={k:json.loads((h/(k+'-result.json')).read_text()) for k in ('geometry','engine','off')}
for k,c in cases.items():
 assert c['build_exit']==c['test_exit']==0
 for p,d in c['source_sha256'].items():assert sha(r/p)==d,(k,p)
n=json.loads((h/'native-build-v3-result.json').read_text());m=json.loads((h/'native-source-v3-manifest.json').read_text())
assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for name in m['overlay']:assert sha(r/'tools/native-capture'/name)==m['files']['tools/native-capture/'+name]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v3'/p)==d
assert sha(h/'native-build-v3/GearsCapture.dll')==n['dll_sha256']
v={'results':{k:c['output'] for k,c in cases.items()},'native_sha256':n['dll_sha256'],'dependencies':len(n['project_dependencies']),'flags_unchanged':True,'native_integrated':True,'game_started':False,'installed':False,'scope':'All-or-none mathematical broad-phase bounds derived from a closed owned asset packet. Derivation is after close; source timestamps/proofs are not renewed. No original dispatch, query/movement permission or full-self certificate.','engineering_percent':25,'gameplay_accepted':False,'headset_accepted':False}
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r244' not in old
add='''
## r244：完整asset范围集合全有或全无，接入Native只读输出（2026-11-12）

新增cover_physx_asset_bounds.h：校验body→actor映射（共享引用显式保留、null BodyData允许）、每个actor必须被引用、唯一actor、连续完整member组、row actor/member ordinal、shape/internal唯一和member归属。容量body/actor64、per-actor shape64、总shape256；逐行执行r243 owned Snapshot原getter/Gram复核，任何一项失败保留调用者旧output；全部成功才一次提交rows/aggregate，空集合has_shapes=false，没有虚构原点碰撞体。

AssetBounds2391检查0，覆盖64actor/256shape上限、空actor组、共享/null、数量超限、漏项/重复/错误归属、末行矩阵/半径失败与alias。现有QueryEngine供给的实际observer四形状合成内存packet关闭后接新转换器通过，Engine1744；OFF65通过。这里Engine执行的是实际QueryEngine与原始PE只读数据、合成对象内存，非游戏形状证据。

C0仅在完整observe_asset_shapes返回queried之后，才对owned packet计算整组范围；单独记录bounds begin/end与code，明确derived_after_close=1、mathematical_only=1、query/movement_permission=0。失败不输出任意bounds row；成功逐行带actor/member/shape保留映射。此后计算不会续原source时钟、复活借用proof、分派原query或授予移动权限。原关闭guard/借用范围保持。Native基于r241最终冻结树仅C0及两个新头overlay，425编译依赖逐SHA、flags不变、无外部工作树依赖，DLL SHA{dll}。尚未实际游戏运行/安装；下一步新实例验证36实际形状能否整组输出及计算耗时。

数学AABB不是PhysX窄相位浮点误差界，也不是world fence/full player self；模板对应、实时query/body/view/hit/muzzle与自身遮挡仍未完成。CV24/AC22与头显不宣称通过，工程25%。publicv71、固定入口、Quest保持。证据asset-bounds-r244。
'''.format(dll=n['dll_sha256'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(v))
