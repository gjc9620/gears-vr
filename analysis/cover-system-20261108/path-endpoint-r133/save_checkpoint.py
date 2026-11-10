"""Save compiled endpoint-enumeration work independently from game acceptance."""
from pathlib import Path
import datetime, hashlib, json, re
H=Path(__file__).resolve().parent
R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checks=load(H/'checks-v1/results.json')
assert len(checks)==4 and all(c['build_exit']==c['test_exit']==0 for c in checks)
counts={}
for c in checks:
    text=(H/'checks-v1'/(c['name']+'-test.log')).read_text()
    found=re.search(r'(?:checks=|: )(\d+)(?: failed=0| checks, 0 failures)',text)
    assert found,text
    counts[c['name']]=int(found[1])
assert sum(counts.values())==587
build=load(H/'native-build-result.json')
assert build['flags_unchanged'] and not build['external_worktree_dependencies']
assert len(build['project_dependencies'])==406
source_names=['cover_physics_query_live.inl','cover_physx_capsule_path_observation.h']
assert set(build['changed'])=={'tools/native-capture/'+name for name in source_names}
source_manifest=load(H/'native-source-manifest.json')['files']
for name in source_names:
    assert sha(R/'tools/native-capture'/name)==source_manifest['tools/native-capture/'+name]
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
record=dict(classification='progress: complete endpoint packet connected to serialized capsule path',
    source_sha256={name:sha(R/'tools/native-capture'/name) for name in source_names},
    test_sha256=sha(R/'tools/native-capture/cover_physics_capsule_path_live_test.cpp'),
    checks=counts,total_checks=587,failures=0,native_sha256=build['dll_sha256'],
    project_dependencies=406,flags_unchanged=True,external_worktree_dependencies=[],
    endpoint='original capsule overlap_all stream: raw shape order, duplicates, owners and positive self provenance retained',
    budget='segments and endpoint share max_hits/max_reports; no endpoint call with exhausted remaining capacity',
    closure='endpoint asset bodies, collider and component snapshots must match path; final Session closing unchanged',
    storage='large endpoint packet lives in serialized Session RawPath; cleared in place without aggregate stack temporary',
    offline_scope='actual typed reader and Session; module/dispatch Host fixtures, not original runtime calls',
    staged=False,game_started=False,query_called=False,world_coverage_verified=False,
    occupancy_authorized=False,movement_authorized=False,public_changed=False,headset_accepted=False,
    remaining=['certify actual body path and endpoint query behavior and complete current collision world coverage',
        'connect current occupancy and same-pose C0 body/view/hit/muzzle commit',
        'repair own head/shoulder obstruction and separately inspect Core measurement refusal',
        'complete all CV01-24 and AC01-22 including physical headset acceptance'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry=f'''
## r133：完整路径终点改接原版形状枚举与归属闭合（2026-11-10，未运行游戏）

上轮r132已按2bfd8918保存原版Sphere执行1/返回1及双眼遮挡FAIL。本轮沿CV-16完整路径/终点要求继续实现：RawPath终点由Overlap布尔值改为AllOverlapOwners，Session在最后一段后调用现有原版capsule overlap_all接口，保留每个原始shape顺序、重复项、owner及正向self来源。它与路径扫掠共用同一max_hits/max_reports剩余额度；额度已耗尽时拒绝终点查询，返回数量/报告超预算或raw_count不一致时拒绝。终点asset bodies/collider/component清单与路径任一不一致，整次操作changed，最终Session身份/输入/元数据closing保持。

较大的终点包仍置于既有序列化Session的RawPath中，原地清零，不引入游戏栈大型临时对象、新writer、hook或输入。空枚举仍不签发完整世界覆盖、自身排除、当前占用或身体移动许可；这些证据与实际消费者尚未接完。

四组实际typed-reader/Session离线检查共587项0失败：path391、Session105、overlapSession53、C0关闭38。覆盖原始形状/归属保留、空返回、跨所有查询共享预算、终点数量与报告超限、三类来源清单不一致、终点调用中身份变化/故障/停止/输入突变/重入及未完整操作不改调用者输出。原查询模块/dispatch使用Host fixture，不是游戏执行。

在r131冻结Native源上仅叠cover_physics_query_live.inl与cover_physx_capsule_path_observation.h，完整编译链接通过；Native SHA256 {build['dll_sha256']}，406项项目依赖均来自冻结源，外部工作树依赖0，原GEARS开关不变。未stage、未启动游戏、未调用原版路径/终点枚举。证据path-endpoint-r133/{{verification,native-build-result,native-source-manifest}}.json与checks-v1/。

下一步认证实际身体体积路径和终点枚举、完整当前碰撞世界覆盖，并接唯一C0提交及同版视图/命中/枪口；自身头肩遮挡、Core测量拒绝和完整抓边/保持/重抓/战斗/恢复仍开放。工程25%、公共v71、固定入口和CV01–24/AC01–22总范围保持，目标active。
'''
assert '## r133：' not in doc.read_text(encoding='utf8')
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8')
doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc)
g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,checks=587,native=build['dll_sha256'],game_started=False)))
