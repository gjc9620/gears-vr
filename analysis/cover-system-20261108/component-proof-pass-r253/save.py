from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
cases={k:json.loads((h/(k+'-result.json')).read_text()) for k in ('geometry','cache','components','groups','path','session','engine','off')}
for name,c in cases.items():
 assert c['build_exit']==c['test_exit']==0
 for p,d in c['source_sha256'].items():assert sha(r/p)==d,(name,p)
n=json.loads((h/'native-build-v3-result.json').read_text());m=json.loads((h/'native-source-v3-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for name in m['overlay']:assert sha(r/'tools/native-capture'/name)==m['files']['tools/native-capture/'+name]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v3'/p)==d
assert sha(h/'native-build-v3/GearsCapture.dll')==n['dll_sha256']
v=dict(native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,results={k:c['output'] for k,c in cases.items()},scope='One synchronous no-dispatch component proof pass uses one fresh actual read for each exact identity address or (address,size) byte window. All expectations compare to that observed value, never another expectation. Every independent barrier is cold; all dynamic values passes/full guards remain. Bounded cache saturation/collisions reread normally. Shared binding::current and other consumers are unchanged.',game_started=False,installed=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25)
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r253' not in old
add='''
## r253：component单次proof pass共享相同实际读值，三独立barrier仍全部重新读（2026-11-12）

依据r252组件4次recheck合11.2947ms，先对真实typed合成夹具统计：7份完整Proof+24members，每pass身份期望140次/独立地址64，字段期望283次/独立(address,size)207。此为合成规格，不把它当成实际游戏重复比例或性能结论。

新增cover_proof_pass.h，组件current()每次begin新epoch、结束关闭，不跨另一次current()/动态值pass/原query保留有效缓存。每个独立身份仍执行原binding::identity的完整双观察/registry/name/slot关闭；每个不同字节窗口仍用原safe_copy精确size读取。不合并重叠但size不同窗口，不扩大读区。相同身份/窗口的重复期望与本pass真实读出的完整Identity/bytes逐一比较，不能只比较两份期望或把冲突期望当成命中。缓存原子busy拒绝嵌套、结束后不能读缓存；容量半满或64次hash冲突后直接执行普通新读取，不裁剪/因新容量拒绝原本合法proof。epoch回绕清私有索引；无分配/游戏写/时钟更新或权限。

组件完整current保留EVERY member identity/next、roots/layouts/assets/body layouts的EVERY Proof identity/source期望；只在同一无dispatch检查内部形成共同的读取快照。current入口与出口metadata/generation/cache一致；sample/recheck的所有独立current调用保持，recheck仍prior/after-first/after-second三个完整barrier、两遍动态值与原owner前后关闭。共享binding::current和MemberStore类型及其他消费者未改。组件Workspace堆内私有scratch，Session/C0只新增pass/期望/真实读/fallback计数，以便实际验证。

ProofPass601检查0：重复一致/矛盾期望、每次barrier冷读、失效后旧值不复活、容量1/密集hash冲突fallback、同地址不同size、失读/非法窗口、epoch回绕。真实typed组件夹具实际读64/140身份和207/283字段，三barrier独立；逐项修改全部207独立字段与64身份，下一pass均拒绝、逐项恢复后均重新通过。Component207、QueryReadGroups1046、CapsulePath391、AssetSession202、AssetEngine1744、OFF65通过；游戏原模块只作为固定PE字节数据，全部这些检查不是实际游戏/原query运行。

Native从r251最终v4冻结树仅组件/Session/C0/新头四overlay，427编译依赖逐SHA、flags不变、无外部工作树，DLL SHA{dll}。尚未游戏运行/安装；下一步新实例定向检查真实读取去重比例、完整路径原调用/关闭与原年龄，不调整30ms准入或40/50ms上限、不重跑旧版凑成功。完整CV24/AC22、active-authored/full self/world fence/body-view-hit-muzzle/遮挡/头显未完成，工程25%，publicv71/固定入口/Quest保持。证据component-proof-pass-r253。
'''.format(dll=n['dll_sha256'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),documents_equal=doc.read_bytes()==mirror.read_bytes())))
