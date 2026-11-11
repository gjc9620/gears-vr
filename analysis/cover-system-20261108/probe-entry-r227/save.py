from pathlib import Path
import hashlib,json,datetime
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in ['before','units','off','catalog']}
for n,r in results.items():
 assert r['build_exit']==0 and r['test_exit']==(1 if n=='before' else 0),(n,r)
 if n!='before':
  for p,d in r['source_sha256'].items():assert sha(root/p)==d,(n,p)
native=json.loads((folder/'native-build-v2-result.json').read_text());manifest=json.loads((folder/'native-source-v2-manifest.json').read_text())
assert native['flags_unchanged'] and not native['external_worktree_dependencies'] and not native['game_started'] and not native['staged']
for n in manifest['overlay']:assert sha(root/'tools/native-capture'/n)==manifest['files']['tools/native-capture/'+n],n
for p,d in native['project_dependencies'].items():assert sha(folder/'native-source-v2'/p)==d,p
dll=folder/'native-build-v2/GearsCapture.dll';assert sha(dll)==native['dll_sha256']
v=dict(results={n:r['output'] for n,r in results.items()},native=dict(sha256=sha(dll),dependencies=len(native['project_dependencies']),flags_unchanged=True,installed=False,game_started=False),
 change='Legacy sphere/path eye probes now convert game units to original .02 PhysX units once, including capsule endpoints, radius, delta and epsilon. Pure helper rejects bad values, aliases and non-nearest rounding without overwriting output. Lightweight units constant shared with unchanged transform math. Complete asset shape read adds opt-in one-shot C0 trigger, mutually exclusive sphere/path/shape modes, simulator/observe-only restriction, same typed source/time/session guard. Logs current raw shapes and guards, no world/movement permission.',
 failure_preserved='Former probe expressions:3 checks/3 failures. Initial full Native failed because first cfenv standard header inclusion inside anonymous namespace introduced nested std; replaced helper rounding check with C floating control API, test cfenv included globally. Failed source/build/log retained. No guards weakened; full v2 rebuild uses same frozen baseline flags.',
 remaining='No actual new C0 game read yet; full active/template correspondence, static/weld/flags/self/world fence, body/view/hit/muzzle transaction, view obstruction, CV01-24/AC01-22 and physical-headset acceptance remain open.',engineering_percent=25,game_started=False)
(folder/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=root/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=root/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'];assert b'## r227' not in old
addition='''
## r227：旧sphere/path探测补原game→PhysX单位；完整形状一次性读取入口已编译（2026-11-12）

实际C0 measurement入口的eye是game units；旧sphere/path直接传eye及units_per_metre*.01给native ABI，遗漏原F2610固定float .02。本轮新增cover_probe_geometry.h统一转换centre、胶囊两端、radius、delta和path epsilon，仅一次game→PhysX转换；cover_physx_units.h与既有transform共享原常量，transform数学不改。旧三个构造表达式用同测试复现3检查3失败；修复28检查0，含已知正负坐标、50/100单位标定、无效值、溢出/退化胶囊、别名、非nearest浮点模式及失败不覆盖。不是头部/body collider，也不把旧r194原calls0超时归因于单位。

通过已有observe_sphere_once入口新增GEARSVR_COVER_SHAPE_SNAPSHOT/INI cover_shape_snapshot显式一次性只读；sphere/path/shape互斥，shape仅现有simulator_operator、continuous_session和observe-only场景允许，默认关闭。读取调用r226实际C0→Session→QueryEngine全形状接口，原时间/角色/owner/metadata/关闭检查不改；大workspace/packet为静态独占一次性存储，回调不持私有锁读游戏。拒绝也算本次终止不重试；预置detail=request避免尚未进入Engine的拒绝被标成observed。只成功才导出完整shape行、stored pose/dims/world matrix及原始调用计数，保留source/guard失败。未授予movement/world权限。

功能关闭65检查0，查询目录16回归0。首次完整构建暴露cfenv首次在anonymous namespace内引入nested std；改用C浮点控制接口，测试在全局引用cfenv，保留失败build/source/log。v2在r226精确冻结Native上仅5文件overlay，原GEARS flags不变、独立源和构建、无外部工作树混入；完整构建通过，DLL SHA为{dll}，项目依赖{deps}逐SHA验证。未安装、未启动游戏/XR，尚无新C0实际形状读取证据。

后续进行新的一次受控真实shape读取，保留原source freshness；继续闭合active/template身份、static/weld/flags/full self/world fence及body/view/hit/muzzle同版事务。完整CV24/AC22、视图遮挡及头显仍未完成，工程25%；publicv71与固定手动入口不变。证据probe-entry-r227。
'''.format(dll=native['dll_sha256'],deps=len(native['project_dependencies']))
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(dll=sha(dll),dependencies=len(native['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
