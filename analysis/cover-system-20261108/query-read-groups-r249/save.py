from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
cases={k:json.loads((h/(k+'-result.json')).read_text()) for k in ('geometry','groups_engine','owned','overlap','path','off','session','engine')}
for k,c in cases.items():
 assert c['build_exit']==c['test_exit']==0,(k,c)
 for p,d in c['source_sha256'].items():assert sha(r/p)==d,(k,p)
n=json.loads((h/'native-build-v3-result.json').read_text());m=json.loads((h/'native-source-v3-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for name in m['overlay']:assert sha(r/'tools/native-capture'/name)==m['files']['tools/native-capture/'+name]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v3'/p)==d
assert sha(h/'native-build-v3/GearsCapture.dll')==n['dll_sha256']
old=json.loads((h/'before_owned-result.json').read_text());bad=json.loads((h/'initial-owned-result.json').read_text());assert old['build_exit']==bad['build_exit']==0 and old['test_exit']==bad['test_exit']==1 and old['output']==bad['output'] and 'failed=22' in old['output']
baseline=json.loads((h.parent/'asset-bounds-r244/native-source-v3-manifest.json').read_text())['files'];assert sha(h/'before-owned/cover_physx_query.h')==baseline['tools/native-capture/cover_physx_query.h']
verification=dict(native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,results={k:c['output'] for k,c in cases.items()},fixture_failure_reproduced_before_and_after=True,
 scope='Only explicit synchronous no-dispatch source/owner read groups borrow typed layout proofs. Each group has full source/owner checks before and after; full component proof closes the group. begin/end and all original dispatch/callback boundaries retain full guards. Original timestamps and deadline unchanged. No cross-call proof, partial result or movement permission.',game_started=False,installed=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25)
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);previous=doc.read_bytes();assert previous==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r249' not in previous
add='''
## r249：原query之外的同步sources/owner读取分组，完整关闭与调用guard保持（2026-11-12）

针对r248实际36.7614ms/42次physics+collider重复/91次owner的关闭年龄拒绝，QueryEngine collect_all只在两个明确无dispatch的段落新增可选read_group：原调用之前asset/collider/components读取，及返回后shape-owner归属与asset/collider/components关闭读取。普通begin/end、原调用、callback均在分组之外；没有将asset专用借用guard包住query。无此方法的旧caller保留原行为，所有原始raw/typed匹配、原native两遍读取/代码/归属关闭保持。

SourceGuard的组入口与出口均执行完整physics/collider typed recheck和原owner/cover/pose/time，另完整component recheck关闭；内部read_source_group仍由完整owner前后括住。QueryReadGuard只在同步组内匹配入口typed的query-source/asset/collider/components，并每次检查stopped、三workspace read-ready及metadata/generation/cache相同、同线程/Session busy/context/key/special-move/metadata。关闭失败不发布、不会将proof跨越原调用、不续原source时间、不延长deadline。所有派生guard在原调用前已销毁；C0新增完整recheck/借用read guard计数供下一实际运行核对。

QueryReadGroups1046检查0：真实Session+typed/raw asset/collider/component读者，组内64重复匹配，正常Host dispatch1；78个完整owner逐个失败、BodyIndex/OwnerComponent/属性offset、component owner/member、collider Body变化，组内过期/metadata generation变化/drain均保持旧output；组内失败时Host调用0，模拟原调用中root变化仍由普通guard拒绝。ReadGroupEngine44检查0：实际QueryEngine collect_all分别sweep与overlap枚举两read组，Host原调用仅发生在组外；第一/第二组拒绝分别原调用0/1，均无结果发布/重派/悬空owner。原OwnedSweep146、Overlap87、CapsulePathSession391、AssetShapeSession202、AssetEngine1744、OFF65通过。

测试初遇OwnedSweep旧夹具仍将Shape+d0写成Body且不填+148，与r231已经由原constructor/实际读取证明的public Actor/+148 optional Body相反。以r244原q头与本轮q头分别编译同一旧夹具，两者均22失败、输出完全一致；只修夹具d0/148后146通过，新分组Engine原2个失败亦随之通过，未为此改产品。原before-owned三源码/初始build与test logs保留。另初次编译W4/WX拒绝lambda名owner_reads与测试global同名，改局部名owner_read_group，保留初始build失败证据。

Native冻结r244最终树仅C0/Session/QueryEngine三文件overlay；425实际依赖逐SHA、flags不变、无外部工作树，DLL SHA{dll}。仅编译，尚未实际运行或安装。下一步新实例定向复核路径原调用前后全部关闭、原时效与实际recheck计数；不声称已有完整路径/世界fence/body-view-hit-muzzle或遮挡修复。工程25%、CV24/AC22与头显未完成，publicv71/固定入口/Quest保持。证据query-read-groups-r249。
'''.format(dll=n['dll_sha256'])
new=previous+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),all_cases_pass=True,documents_equal=doc.read_bytes()==mirror.read_bytes())))
