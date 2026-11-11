from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/query-units-r218'
names=['verified','catalog','catalog-fixture','observer','authored-geometry','primitive','box']
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in names}
assert all(r['build_exit']==r['test_exit']==0 for r in results.values())
before=json.loads((folder/'before-result.json').read_text());after=json.loads((folder/'after-result.json').read_text())
assert before['build_exit']==0 and before['test_exit']==1 and 'failures=2' in before['output']
assert after['build_exit']==after['test_exit']==0 and 'checks=3 failures=0' in after['output']
for n,r in results.items():
 for path,digest in r.get('source_sha256',{}).items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,(n,path)
old=json.loads((folder.parent/'c0-target-r99/verification.json').read_text())['fixtures']
actual={n:hashlib.sha256((folder.parent/'61492-cover-frame'/n).read_bytes()).hexdigest() for n in old};assert actual==old
sources=['tools/native-capture/cover_query_units.h','tools/native-capture/cover_primitive_query.h','tools/native-capture/cover_body_query_catalog.h',
 'tools/native-capture/cover_body_query_observer.h','tools/native-capture/cover_body_query_observer_test.cpp','tools/native-capture/cover_body_query_units_test.cpp','tools/native-capture/cover_physx_query.h']
certificate=root/'analysis/cover-system-20261108/physx-transform-r74/original-transform-certificate.json'
verification=dict(results={n:r['output'] for n,r in results.items()},sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
 original_transform_certificate_sha256=hashlib.sha256(certificate.read_bytes()).hexdigest(),fixture_sha256=actual,
 counterexample=dict(before=before['output'],after=after['output'],unchanged_counter_source_sha256=hashlib.sha256((folder/'unit-boundary-counter.cpp').read_bytes()).hexdigest()),
 fix='Body Geometry is in original game units; native query ABI consumes PhysX units directly. Catalog now calls explicit query_units conversion with original float0.02 (bits3ca3d70a), exactly once. Generic primitive_query packing stays in supplied units and is unspecified; catalog/rows mark PhysX explicitly and raw observer rejects unspecified units before first dispatch.',
 enclosure='Scale double centres/endpoints with FMA product residual bound; scale float dimensions exactly in double, round upward and include centre/endpoint double error. Reuse existing bounded primitive converter, then account total radius/half expansion against exact scaled original dimensions. Input budget remains game units; bound converted downward to PhysX units; output expansion PhysX units. Near rounding only, no shrinking/clamping.',
 tests='8333 unit/enclosure checks0: same sphere10,20,30/radius2 counterfixture had2 failures before and0 after; marked units, once-only conversion, strict budget/output preservation,128 sets each sphere/capsule and rotated box corners in inverse/transpose interpretations, rounding-mode refusal. Catalog16/C0catalog269/observer124(plus110 Session)/authored geometry377/generic primitive2134/box27140 all0. Original9 C0 files unchanged. Host dispatch only, no original/native query.',
 remaining='Does not establish active authored correspondence, original native internal box interpretation, capsule creation-axis correction, flags/eligibility, current query-visible world fence, production capture integration/build/game/headset or full CV/AC completion. Prior runtime timeouts not attributed to this previously offline-only path.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old_bytes=doc.read_bytes()
assert old_bytes==mirror.read_bytes() and hashlib.sha256(old_bytes).hexdigest()==guard['development_copy']['sha256']
addition='''
## r218：完整身体查询目录漏接原game→PhysX单位换算已修正（2026-11-12）

本轮从尺寸/轴基核对转入实际发现的单位边界缺口：body_geometry输出原游戏单位，而query ABI将Sphere/Box/Capsule直接交原PhysX，不做换算；此前catalog.prepare仅调用同单位primitive_query打包。以sphere中心10/20/30、radius2和明确0.001游戏单位预算的同一3检查夹具，原代码编译0/执行1，位置与radius两项失败；修正后同一夹具3检查0失败，原counter源码与前后日志保留。不将此离线路径缺口归因于r194既有超时或声称游戏已修。

新增cover_query_units.h，在catalog.prepare边界明确应用原F2610已认证float0.02（bits3ca3d70a）一次；不是HMD米/游戏校准。double中心/端点乘该精确float常量，用FMA residual界住double乘法误差；float尺寸乘float在double中精确表示，向上float舍入并加入中心/端点误差，再复用既有bounded converter。最终radius/half总外扩相对精确scaled原尺寸核对；输入预算仍为原游戏单位，换成PhysX上限时向下取界，输出expansion为PhysX单位。不缩形、不clamp预算、不通过重排救回异常值；非nearest模式拒绝。

generic primitive_query继续只打包输入同单位值，新增Units::unspecified/physx；catalog与每行明确physx。body_query_observer在第一次dispatch前拒绝目录或任一行单位未指定；手工Host fixture明确native单位。原q ABI增加单位注释，不在raw层再次换算，防止双乘。

新单位/包络8333检查0失败：单次换算与单位标记、零/小预算拒绝保持输出，128组球/胶囊端点及旋转盒全部角点逆坐标/转置投影包络、FP模式拒绝。受影响回归catalog16、原真实C0底座catalog269、observer124（含原Session110）、完整authored geometry377、generic primitive2134、box27140全0；原9 C0夹具SHA保持。对目录/单行unknown单位的拒绝均在Host query计数增长前。完整依赖SHA当前一致；原换算certificate/反例/源码/回执query-units-r218。

实际authored↔active逐项对应、native内部OBB语义、胶囊创建轴修正、flags/参与规则及current query-visible world闭合仍需完成；没有生产Native构建/安装/游戏/原查询/头显。本次是Source与Host原分派seam验证，完整CV24/AC22、body/view/hit/muzzle同版事务及视图遮挡继续待完成，工程25%。
'''
data=old_bytes+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'unit_checks':8333,'failures':0,'regression_variants':6,'fixture_files_unchanged':9,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
