from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;runtime=h.parent/'asset-bracket-runtime-r238/runtime-r238';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rs={k:json.loads((h/(k+'-result.json')).read_text()) for k in ['before','span','math','geometry','engine','session','off']}
for k,v in rs.items():
 assert v['build_exit']==0 and v['test_exit']==(1 if k=='before' else 0)
 v['source_sha256']={p.replace(chr(92),'/'):d for p,d in v['source_sha256'].items()}
 if k!='before':
  for p,d in v['source_sha256'].items():assert sha(r/p)==d,(k,p)
key='tools/native-capture/cover_physx_pose_span_test.cpp';assert rs['before']['source_sha256'][key]==rs['span']['source_sha256'][key]
for name in ['cover_physx_observed_pose.h','cover_physx_shape_geometry.h']:assert sha(h/('before-'+name))==rs['before']['source_sha256']['tools/native-capture/'+name]
n=json.loads((h/'native-build-v3-result.json').read_text());m=json.loads((h/'native-source-v3-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(r/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v3'/p)==d
assert sha(h/'native-build-v3/GearsCapture.dll')==n['dll_sha256'];geometry_native='tools/native-capture/cover_physx_shape_geometry.h' in n['project_dependencies']
(h/'verification.json').write_text(json.dumps(dict(results={k:v['output'] for k,v in rs.items()},same_recorded_replay=True,native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),geometry_is_native_dependency=geometry_native,flags_unchanged=True,game_started=False,installed=False,change='Observed read qualifies finite nonzero raw quaternion through original float matrix with proper orientation and explicit Gram spectral upper bound G<1. No fixed quaternion norm cutoff in read; no normalization. Strict Geometry now separately rebuilds strict unit-quaternion and derived matrix from raw source and requires exact match. Original strict pose/query math unchanged. Float intermediate and compose translation range checked before overflow.',remaining='New observed contract not game-run; complete actual asset close, numerical enclosure into actual geometry/query, full self/world/template/body/view/hit/muzzle/headset pending.',engineering_percent=25),indent=2)+'\n')
v=json.loads((runtime/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
media=json.loads((runtime/'media-review.json').read_text());media['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in media['contacts']];media['full_span_personally_reviewed']=False;media['visual_acceptance']=False;(runtime/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r238' not in old and b'## r239' not in old
actual=json.loads((runtime/'run-result-original.json').read_text())['terminal_session']['game_identity'];add='''
## r238：只读分组实际推进至actor11，21.1594ms后旧norm cutoff拒绝（2026-11-12）

新cover-1112-r238-cloak-read-group使用r237 Nativec5e3d0f2…73d8，准备423依赖/导入/身份guard14通过，准备c10a13a1；actual game{pid}/creation{creation}。一次读取至actor index11/member0，21.1594ms，owner4无失败、typed Session guard250无失败，q10 owner_read/asset8 world/world8 pose、first world Stage7 Body pose、parameters0，原query调用0。尚未group-end或全asset关闭，不能把未观察到deadline拒绝称为最终时效通过。

Body q=.401522607,.304909706,-.0472320467,.862281024，norm平方.9999497638780792；stored q=-.517145097,-.482245654,-.482245654,.517145157，norm平方.9999999062283447。旧观测器沿用3e-5 unit cutoff在Body处拒绝，未发布整批。此数据说明不能逐个把常量调大来取得全清单；须使原数据读取与严格query资格分开。

259实际投影/errors0、13总览、5阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK545失败0、前台367无owned激活，anchor/reader前后相同。证据asset-bracket-runtime-r238。

## r239：观测矩阵资格与严格几何资格分离，不逐次调大unit容差（2026-11-12）

observed_pose保留finite/nonzero原q、原float getter矩阵、可表示运算范围及完整Gram误差，不再以unit quaternion norm固定3e-5截断只读观测。资格改为矩阵有限、det>0及Gram谱误差上界G<1；此条件给出奇异值sqrt(1-G)..sqrt(1+G)和非退化proper polar rotation，是显式可传递数值性质，不是查询精度预算。compose继续传播并检查误差和proper/nonsingular条件，不归一化或clamp。新增float中间量/translation求和的double范围预检，有限极端输入在运算前拒绝，未先触发float overflow。

原pose_math的3e-5 unit/rigid规则、原query ABI资格保持。strict shape_geometry现在额外从raw stored/Body q+p重建严格姿态，检查Body presence与owner pointer一致，再要求派生world矩阵逐字节匹配；即使非unit q.w=2/xyz0的原getter矩阵为identity，也不能直接进入strict geometry。旧owned-only几何fixture补齐其原raw q/p，使严格资格有可追溯数据。该库不授予world/movement权限，非strict原数据后续仍须数值包络和实际query事务。

相同r238记录replay旧3检查1失败，新5检查0，raw q/p与误差保留、strict geometry仍拒绝。Math4369检查0：包含norm范围±.1与近unit的未归一化Rodrigues点oracle、矩阵资格/误差、反射/退化/下报误差、非nearest、零/NaN/Inf/denormal、极端有限值与overflow flag、identity但非unit的严格gate拒绝。Geometry13840、Engine1743、Session165、OFF65均0；Session full owner6/64重复typed checks保持。源SHA逐一当前。

最终完整Native v3在r237冻结树overlay observed_pose/shape_geometry，423依赖逐SHA、flags不变、无外部工作树混入，DLL SHA{dll}；geometry是否Native实际依赖={geo}，其严格转换通过独立模块测试，仍不称已接入玩法查询。中间v2构建保留；v3包含追加的可表示范围预检。此版尚未游戏运行，下一步新版本一次读取，检查全asset和原deadline关闭。

完整CV24/AC22、active/template/full self/world fence、非strict数值包络、body/view/hit/muzzle、视图遮挡及头显继续未完成，工程25%，publicv71/固定入口/Quest不变。证据pose-read-contract-r239。
'''.format(pid=actual['pid'],creation=actual['creation_time'],dll=n['dll_sha256'],geo=geometry_native)
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),geometry_native=geometry_native,actual_game=actual['pid'],documents_equal=doc.read_bytes()==mirror.read_bytes())))
