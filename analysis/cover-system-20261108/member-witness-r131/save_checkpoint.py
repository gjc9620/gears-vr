from pathlib import Path
import datetime,hashlib,json,re
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
before=load(H/'before-v1/results.json');assert len(before)==1 and before[0]['build_exit']==before[0]['test_exit']==0
tests={x['name']:x for x in load(H/'after-v1/results.json')}
tests.update({x['name']:x for x in load(H/'after-v2/results.json')})
assert len(tests)==8 and all(x['build_exit']==x['test_exit']==0 for x in tests.values())
counts={'cover_member_witness_live':50,'cover_physics_roles_live':282,'cover_collider_roles_live':219,'cover_component_roles_live':207,
        'cover_physics_query_live':105,'cover_physics_overlap_query_live':53,'cover_physics_capsule_path_live':191,'cover_physics_c0_off':38}
assert sum(counts.values())==1145
for name,row in tests.items():assert sha(R/'tools/native-capture'/(name+'_test.cpp'))==row['source_sha256']
receipt=load(H/'native-build-result.json');assert receipt['flags_unchanged'] and not receipt['external_worktree_dependencies']
assert len(receipt['changed'])==5 and len(receipt['project_dependencies'])==406
assert sha(H/'native-build/GearsCapture.dll')==receipt['dll_sha256']
for p in receipt['changed']:assert sha(R/p)==sha(H/'native-source'/p)
common=H/'native-source/tools/native-capture/weapon_hand_scene_binding.inl'
assert sha(common)==sha(H.parent/'pose-probe-r126/native-source/tools/native-capture/weapon_hand_scene_binding.inl')
assert 'std::array<gears_anchor::Identity,128> identities' in common.read_text()
store=(R/'tools/native-capture/cover_member_witness_live.inl').read_text()
assert 'using Store=Storage<4096>' in store and 'count=0;exhausted=false;index.fill(0)' in store
record=dict(classification='progress',actual_trigger='r130 PhysicsAsset member closure saturated shared Proof128',
    before=dict(checks=8,scope='actual typed reader with192 appended synthetic members rejects at legacy128',test=before[0]),
    after=dict(checks=1145,counts=counts,tests=list(tests.values()),
        scope='actual typed readers and Session; memory/dispatch Host fixtures, no game or original query'),
    production_sources={p:sha(R/p) for p in receipt['changed']},test_source_sha256=sha(R/'tools/native-capture/cover_member_witness_live_test.cpp'),
    native_sha256=receipt['dll_sha256'],project_dependencies=406,flags_unchanged=True,global_binding_Proof_unchanged=True,
    independent_member_limit=4096,lookup_slots=8192,allocation='fixed Store in existing serialized Workspace; clear active count/hash index, no giant aggregate temporary',
    closure='full identity and original Next for every sibling; duplicate/cycle/outer checks retained; full current re-read; no truncation or unknown-as-empty',
    capacity_scope='unique members across a complete reader sample; exhaustion maps to capacity and preserves caller output',
    superseded='native-v1 passed, but final component closure checks metadata before member reads; after-v2 and final Native rebuilt',
    game_started=False,staged=False,query_called=False,movement_authorized=False,public_changed=False,headset_accepted=False,
    remaining=['actual typed roles and original query under new candidate','later Core measurement rejection','complete body path endpoint world coverage and C0 commit','head shoulder obstruction','CV01-24 AC01-22 headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry=f'''
## r131：独立有界成员见证修复128项上限，源码/定向检查/私有编译（2026-11-10）

上轮b85d3480已实际定位PhysicsAsset完整成员检查且shared Proof身份数128。本轮先在真实typed roles/registry代码中追加192个合法合成mesh sibling，before-v1严格复现offset0x248已解析、mesh_asset_property/layout5、identity128和调用方输出不变（8检查0失败，验证的是原缺陷）。未改旧游戏记录或把该before结果算玩法通过。

新增Cover专用MemberStore，固定最多4096个唯一成员及8192查找槽，保存完整Identity和原Next；先按原规则核对身份/declaring owner/唯一选名，再双读链指针、记录与原样比较。重复遍历复用同地址见证，内容不符拒绝；完整链仍保留4096 visited/cycle边界。结束时在同一metadata/锁/当前来源保护下逐成员重新读取身份与Next。存储满明确capacity、无截断成功；clear只重置活动长度/索引，旧scratch不参与输出，不在游戏栈创建整个大Store临时对象。

PhysicsRoles、ColliderRoles、ComponentRoles三个已有读取Workspace各拥有独立存储，所有生产role_property/pointer_property及body_layout都传入并在末尾闭合，支持选中属性之后的兄弟成员。公共weapon_hand_scene_binding::Proof保持128、布局与源码在冻结树逐SHA不变；不触碰其他writer或共享在研binding修改。没有新输入、hook、原调用、身体/骨骼/相机写者，也不从这些源见证签移动许可。Sphere摘要追加member_count/exhausted以便实际验证。

同一192成员fixture修复后完整通过，记录201成员且普通Proof不到128。50项新增检查保留public legacy路径128拒绝，覆盖末端循环/重复选名/错误owner、第二遍之后类型/Next/注册表变化、拒读、碰撞/存储耗尽/clear恢复。实际8程序合计1145项0失败（Member50/Physics282/Collider219/Component207/Session105/OverlapSession53/Path191/OFF38），MSVC W4/WX/O2/UNDEBUG；派发和内存为明确Host缝，不是游戏/原查询证据。首次检查目录父路径遗漏在编译前拒绝，创建本方目录后复现；未放宽警告。初版Native成功但审查调整component结束复核先metadata后成员，final重编与受影响4程序after-v2通过。

最终Native以r126冻结树仅叠5文件，406项目依赖、外部工作树依赖0、开关不变，编译链接0；SHA256 {receipt['dll_sha256']}。当前尚未stage或启动新game/XR；公共v71/固定手动入口保持。证据member-witness-r131/{{verification,native-build-result}}.json、before-v1与after-v1/v2。下一步保存本源码后新票定向进入，检查是否越过具体layout阻断再按实际下一拒绝修复；r130后续Core拒绝、完整当前占用/世界覆盖/唯一C0身体及视图/命中/枪口、抓边联合战斗与头显均未通过，工程25%及全部CV01–24/AC01–22保持。
'''
assert '## r131：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native=receipt['dll_sha256'],checks=1145,game_started=False)))
