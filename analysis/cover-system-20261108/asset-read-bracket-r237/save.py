from pathlib import Path
import json,hashlib,datetime,re
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;runtime=h.parent/'observed-pose-runtime-r236/runtime-r236';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rs={k:json.loads((h/(k+'-result.json')).read_text()) for k in ['before','session','engine','off']}
for k,v in rs.items():
 assert v['build_exit']==v['test_exit']==0
 v['source_sha256']={p.replace(chr(92),'/'):d for p,d in v['source_sha256'].items()}
 if k!='before':
  for p,d in v['source_sha256'].items():assert sha(r/p)==d,(k,p)
key='tools/native-capture/cover_asset_shapes_session_test.cpp';assert rs['before']['source_sha256'][key]==rs['session']['source_sha256'][key];assert sha(h/'before-cover_physics_query_live.inl')==rs['before']['source_sha256']['tools/native-capture/cover_physics_query_live.inl']
full={k:int(re.search(r'AssetReadFullOwner calls=(\d+)',rs[k]['output']).group(1)) for k in ['before','session']};assert full==dict(before=138,session=6)
n=json.loads((h/'native-build-v2-result.json').read_text());m=json.loads((h/'native-source-v2-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(r/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v2'/p)==d
assert sha(h/'native-build-v2/GearsCapture.dll')==n['dll_sha256']
(h/'verification.json').write_text(json.dumps(dict(results={k:v['output'] for k,v in rs.items()},same_session_fixture=True,full_owner_calls=full,repeated_typed_sourceguard_checks=64,native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,game_started=False,installed=False,change='Complete no-dispatch asset read uses existing synchronous read_source_group. Every typed SourceGuard recheck/value/metadata proof retained. Full external owner/cover/pose/original freshness checks before/after group and in final closing; all result stays private until close. Original query branches unchanged. No new timestamp or expanded lifetime.',remaining='New grouped candidate not yet game-run. r236 reached actor3 then original age expired. Need actual full read within unchanged time, then numerical geometry envelope and full self/world/template/body/view/hit/muzzle/headset.',engineering_percent=25),indent=2)+'\n')
v=json.loads((runtime/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
media=json.loads((runtime/'media-review.json').read_text());media['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in media['contacts']];media['full_span_personally_reviewed']=False;media['visual_acceptance']=False;(runtime/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r236' not in old and b'## r237' not in old
add='''
## r236：观测近旋转实际越过旧pose失败，actor3时原source deadline拒绝（2026-11-12）

新cover-1112-r236-cloak-observed-pose使用r235 Nativec9ea3180…9e25，准备423依赖/导入/身份guard14通过，准备c4fae7f2。actual game27884（精确身份见原run-result），一次形状读取34.6018ms；原Body pose失败已越过，后在actor index3的shape guard拒绝。owner147/failed147 Gate8 context/detail3 source_age_expired/extra6，Session72/failed72 owner_begin；source cause1，original calls0、整批未发布。当前world scratch两pass Stage9 complete，不能把这些局部结果当全asset通过。实际game身份与source原始值以run-result/native-session-status为准。

264实际投影/errors0，14总览已生成、5阶段contact逐张亲看仍自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK552失败0，前台372无owned激活，anchor/reader前后相同。未触碰其他会话。新失败指向同一只读事务的重复全owner开销；保留原source时间，不以更新时间或放宽期限修复。

## r237：无分派的完整asset读取复用同步只读分组，全owner138→6（2026-11-12）

Session只在asset Snapshot类型的engine执行分支复用既有read_source_group。整个完整资产observer是一个同步READ-ONLY、无原query分派的操作；每一次typed SourceGuard仍调用原roles::recheck、两遍dynamic values、全部prior/final Proof与初始结果比较。分组内部的owner变成已有read_current（相同线程、enabled/busy、scene/key/special move/metadata）；分组开始/结束仍执行原完整owner/cover/pose/原时效，execute closing还复核一遍。私有member result只有全部关闭通过才写外部输出；分组失败清除recheck资格。未将权限/缓存跨execute保留，未修改实际query分支、原时间戳或时效限额。

同一Session fixture用64个重复typed SourceGuard调用、相同源码前后比较：原full owner138次，新6次；old825/new165均失败0，检查数差异来自逐个full owner拒绝边界138→6，原110基础项保留。中途expired即使Host已写私有结果仍不发布，逐个现有full boundary拒绝、root替换、drain及无原调用通过。Host结果明确为空，仅验真实Session与typed role事务，不冒充游戏全形状。Engine1743与OFF65通过。首次MSVC编译拒绝nested lambda局部constexpr判定，改直接Output type trait，保留失败log，未改guard。

完整Native在r235冻结树仅cover_physics_query_live.inl单文件overlay，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHA{dll}。此版尚未游戏运行，下一步明确新版本一次asset读取，验证真实耗时及全清单能否在原deadline内关闭；不因离线full调用减少声称时效问题已修。

r236/r237均不改变publicv71/固定入口/Quest。完整CV24/AC22、active/template/full self/world fence、非strict rigid数值包络、body/view/hit/muzzle、视图遮挡与头显继续未完成，工程25%。证据observed-pose-runtime-r236、asset-read-bracket-r237。
'''.format(dll=n['dll_sha256'])
# Bind the narrative game ID to this authoritative run rather than memory.
actual=json.loads((runtime/'run-result-original.json').read_text())['terminal_session']['game_identity']['pid'];add=add.replace('actual game27884','actual game'+str(actual))
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),full_owner_calls=full,actual_game=actual,documents_equal=doc.read_bytes()==mirror.read_bytes())))
