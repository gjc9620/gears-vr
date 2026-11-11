from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/authored-inventory-r213'
result=json.loads((folder/'packed-header-after-result.json').read_text());regression=json.loads((folder/'array-regression-result.json').read_text())
before=json.loads((folder/'packed-header-before-result.json').read_text())
assert result['build_exit']==result['test_exit']==regression['build_exit']==regression['test_exit']==0
assert before['build_exit']==0 and before['test_exit']==1 and 'failures=14' in before['output']
sources=['tools/native-capture/cover_authored_inventory_live.inl','tools/native-capture/cover_authored_inventory_live_test.cpp',
 'tools/native-capture/cover_authored_array.h','tools/native-capture/cover_point_scale_live_test.cpp']
originals=['tools/game-playtest/cover_physics_shapes.py',
 'analysis/cover-system-20261108/world-component-point-r62/0x1403a6c34-pseudocode.c',
 'analysis/cover-system-20261108/world-component-point-r62/component-route-evidence.json']
verification=dict(result=result['output'],array_regression=regression['output'],
 sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
 original_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in originals},
 association='Two complete body/kind/array-ordinal passes under current metadata/GT binding lock. Reuses asset membership, scale and each BodySetup all3 primitive proofs; exact aggregate5-member schema including opaque RenderInfo Pointer8 and ConvexElems inner StructProperty -> KConvexElem. Retained original point route requires reflected AggGeom offset0x6c.',
 completeness='All declared bodies/kinds; all raw80 and collision-disabled rows preserved. Counts per body/kind and headers retained. Nonempty convex -> unsupported; above256 rows -> capacity; no dropped rows. Empty inventory captured as observation only, never an occupancy/geometry pass.',
 closure='Typed roles guard outside mutex before/after complete two passes. Each array capture closes producer/membership/shape proofs; all body proofs close after each whole pass and finally. Entire passes must agree including raw bytes. Exclusive caller heap/TLS workspace with reentry rejection; failure preserves public output.',
 tests='328 checks0 includes292. Synthetic2 bodies/7 shapes/all3 kinds and1 collision-disabled row. Mutating early payload during late body is caught by full pass comparison. Schema/membership changes, nonempty convex, malformed convex inner/RenderInfo/extra member, rejected guards, thrown guard/read, alias/reentry and exact256/257 capacity, empty arrays covered. No actual process memory or game.',
 packed_header_fix='Authored TArray headers at original BodySetup+0x6c are4-aligned, not8. Separate source.header predicate changed from PhysX pointer8 to header4 plus unchanged numeric span; payload/PhysX pointer8 unchanged. Full native-offset fixture compiled and failed14 before fix; after328 checks0 includes2 added alignment checks and7 per-row checks now reached (before319 checks). This is a source/fixture counterexample, not a game repair claim.',
 first_compile_failures='New local names pointer_type and throw_read shadowed nested legacy fixture globals under /WX; renamed. Logs retained.',
 remaining='No production capture integration/full Native build/current code-dispatch attestation, active PhysX correspondence, world/path/occupancy safety, same-pose body/view/hit/muzzle transaction or headset proof. All original CV24/AC22 remain. Disabled rows retained, query eligibility still consumer work.',
 game_started=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r213：完整authored身体库存采集器及原0x6c packed header修正（2026-11-12）

新增cover_authored_inventory_live.inl，复用r211资产成员/r212缩放/每BodySetup三primitive布局和r206 raw传输，按body/kind/原array ordinal整批采集，保留每组header/count、原raw80及collision-disabled条目，不静默丢弃。aggregate需恰好五字段：三primitive数组、ConvexElems和RenderInfo Pointer8；opaque RenderInfo不读值/不解引用，空Convex仍核inner dimension/stride/目标ScriptStruct/owner/extent及header。非空Convex返回unsupported，超过256形状返回capacity，均不发布部分结果。

调用者heap/TLS工作区保存每body Proof和两完整库存，禁止work/output/input别名与同work重入；持当前binding锁两轮完整body采集，各array仍闭合三类Proof，整轮及最后再闭合所有body Proof，完整两轮raw/header/count/成员/缩放须一致。外层Guard(typed roles Snapshot)只在锁外调用，成功后再次匹配原typed输入才发布。无整套C0/世界许可由此产生，空库存只是观察，不当身体几何/占用通过。

核原r62 3a6c34 consumer确认setup+0x6c交aggregate，完整采集器要求反射AggGeom offset精确0x6c。此前r206把Source.header误套PhysX pointer8 predicate，而该packed TArray头实际4字节对齐。将完整夹具改为原0x6c，编译成功、319检查14失败复现；仅对authored header改为4对齐+原span，payload/PhysX pointer8和身份guard均未放宽。修正后328检查0失败，含新增2个header对齐检查及之前未进入的7个逐行检查；原独立AuthoredArray23回归0。它是离线来源拒绝修正，未声称游戏已修。

完整fixture为synthetic两body/7条三类型形状并保留1条collision-disabled；读取末body时改首body payload由全轮比较拒绝，schema/成员变化、convex存在、inner/RenderInfo/额外aggregate成员、首尾guard拒绝、read/guard异常、alias/重入均保护输出。总256条成功、257条明确拒绝，空数组观察成功。首次两个局部名遮蔽旧fixture全局触发/WX，改名后通过；原编译与packed失败日志保留。

原code分派认证/实时capture接入、active PhysX形状对应、collision-disabled查询参与策略、完整身体几何/路径/占用、同版body/view/hit/muzzle及头显仍待完成。源码/依赖/原consumer SHA/回执authored-inventory-r213，无游戏/原函数调用/完整Native构建；全CV24/AC22保留，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':328,'array_regression_checks':23,'failures':0,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
