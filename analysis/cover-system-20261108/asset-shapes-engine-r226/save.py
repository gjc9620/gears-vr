from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/asset-shapes-engine-r226';sha=lambda b:hashlib.sha256(b).hexdigest()
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in ['engine','session','off','construction']}
for n,r in results.items():
 assert r['build_exit']==r['test_exit']==0,(n,r)
 for p,d in r['source_sha256'].items():assert sha((root/p).read_bytes())==d,(n,p)
native=json.loads((folder/'native-build-result.json').read_text());manifest=json.loads((folder/'native-source-manifest.json').read_text())
assert native['flags_unchanged'] and native['build_only'] and not native['staged'] and not native['game_started'] and not native['query_called']
assert not native['external_worktree_dependencies']
for name in manifest['overlay']:
 p='tools/native-capture/'+name;assert sha((root/p).read_bytes())==manifest['files'][p],p
for p,d in native['project_dependencies'].items():assert sha((folder/'native-source'/p).read_bytes())==d,p
dll=folder/'native-build/GearsCapture.dll';assert sha(dll.read_bytes())==native['dll_sha256']
sources=['tools/native-capture/cover_physx_query.h','tools/native-capture/cover_physx_pose_math.h',
 'tools/native-capture/cover_physics_query_live.inl','tools/native-capture/cover_physics_c0_live.h','tools/native-capture/cover_physics_c0_live.inl',
 'tools/native-capture/cover_physx_asset_shapes_test.cpp','tools/native-capture/cover_physx_asset_shapes_engine_test.cpp',
 'tools/native-capture/cover_asset_shapes_session_test.cpp','tools/native-capture/cover_physics_c0_off_test.cpp']
verification=dict(results={n:r['output'] for n,r in results.items()},sources={p:sha((root/p).read_bytes()) for p in sources},
 native=dict(dll=str(dll),sha256=native['dll_sha256'],dependencies=len(native['project_dependencies']),flags_unchanged=True,
             baseline='exact r193 frozen source plus explicit22-file overlay,14 actual changed/new files; inherited r200 nullable body fixes included',
             changed=manifest['changed'],external_worktree_dependencies=[],installed=False,game_started=False),
 change='Purpose-built read-only asset_shapes transaction now uses actual QueryEngine pinned image, begin/end source/code guards, same active/uncertain/cleanup ownership, and actual asset shape observer. NoCalls type compiles, so the read cannot instantiate original dispatch. Session ownership/metadata/closing transaction supports it with member-owned large result; C0 entry uses existing current entry/cover/time/metadata checks and rejects scratch aliases to C0 state. Caller supplies heap/TLS scratch. No new runtime trigger enabled.',
 tests='Engine1723 checks0 includes asset1714 and actual QueryEngine/HostImage source transaction with original DLL only read as PE data; 4 fixture shapes, mutation rejection/output preservation, recursive busy, alias, normal close and no loaded DLL. Session125 includes prior110: typed owner checks with intentionally empty Host read packet, owner replacement/drain rejection and zero query dispatch; no claim that empty Host packet is actual shape evidence. Feature OFF52 preserves previous output and no module/game/query. Construction33041 regression0 after actor->actor_pose parameter-only rename required by header exposure; lambda constexpr type test changed to direct type trait for MSVC compilation, no guard relaxed.',
 remaining='New C0 read API compiled but not invoked in a game. Need controlled one-shot trigger, verify existing eye sphere/path probe game-to-PhysX units before using their runtime results, actual runtime timing/ownership evidence and cache-to-active correspondence. No movement/world fence/fullCV24/AC22/game/headset acceptance.',
 game_started=False,full_native_built=True,installed=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r226：完整actual asset形状读取接入原QueryEngine／Session／C0接口，隔离Native编译通过（2026-11-12）

不再只留独立离线observer：QueryEngine新增observe_asset_shapes，复用实际固定Image、begin/end source/code/typed guard及active/uncertain/cleanup；调用r217完整asset shape observer，全部关闭后才发布输出。外部heap/TLS AssetShapeWorkspace持有candidate，失败不覆盖旧输出。接口在NoCalls空类型下可编译，原query dispatch不能被实例化；Code::queried在这个接口表示只读事务完成，不是原生查询发生。

Session同一序列化execute事务新增无geometry的AssetReadRequest，角色采集、所有者/metadata/源时效及最后关闭检查保留。大Snapshot作为Session成员，避免在GT回调栈新建大结果。cover_physics_c0_live::observe_asset_shapes通过既有当前Entry/C0/cover/metadata/time guard及query admission，额外拒绝scratch与C0 state/Entry/source/output别名；OFF分支unavailable且不覆盖。只新增接口，没有增加默认触发、原查询、body写入或新移动权限。

离线Engine1723累计0（含原asset1714）：实际QueryEngine/HostImage用原DLL作为PE数据，4个模拟原布局形状，跨actor变化拒绝/不覆盖、重入、别名、正常关闭、原DLL未加载。Session125累计0（原110+新15），实际Session/typed roles加明确空Host packet，仅验owner替换、drain及零query dispatch，空包不当成actual形状证据。OFF52累计0，构造数学33041回归0。首次Session编译暴露公共头pose_math参数actor与旧fixture全局actor的/WX shadow，改参数名actor_pose且数学不变；lambda内if constexpr改直接type trait解决MSVC捕获常量问题，未弱化guard或关闭warning，失败log保留。

完整私有Native在精确r193冻结树上做明确22文件overlay（实际14个changed/new，含此前已证r200 nullable Body修复），原全部GEARS布尔flag保持，独立source/build目录、420个项目依赖逐SHA、无外部工作树混入；GearsCapture.dll SHA50b76bd0f0bec9d518ecbef70a26b381b809741c46764d7d537e9af160abc750。未安装/放到公共入口、未启动游戏/XR、未分派原query。编译不等于C0实际调用或玩法验收。

后续先核实/修正既有eye sphere/path探测入口的game→PhysX单位与一次性触发，再做新的受控真实读取；不因接口编译通过重跑旧计时案例或放宽原来源时间。actual template/shape身份关联、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务、视图遮挡、完整CV24/AC22及头显继续未完成，工程25%。证据asset-shapes-engine-r226，原publicv71和固定手动入口不变。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(engine_checks=1723,session_checks=125,off_checks=52,construction_checks=33041,failures=0,native_sha256=native['dll_sha256'],dependencies=len(native['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
