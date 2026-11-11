from pathlib import Path
import json,hashlib,datetime
r=Path(__file__).resolve().parents[3];h=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();rs={k:json.loads((h/(k+'-result.json')).read_text()) for k in ['before','replay','math','engine','session','off']}
for k,v in rs.items():
 assert v['build_exit']==0 and v['test_exit']==(1 if k=='before' else 0)
 v['source_sha256']={p.replace(chr(92),'/'):d for p,d in v['source_sha256'].items()}
 if k!='before':
  for p,d in v['source_sha256'].items():assert sha(r/p)==d,(k,p)
key='tools/native-capture/cover_physx_observed_pose_test.cpp';assert rs['before']['source_sha256'][key]==rs['replay']['source_sha256'][key];assert sha(h/'before-cover_physx_shape_world.h')==rs['before']['source_sha256']['tools/native-capture/cover_physx_shape_world.h']
n=json.loads((h/'native-build-v2-result.json').read_text());m=json.loads((h/'native-source-v2-manifest.json').read_text());assert n['flags_unchanged'] and not n['external_worktree_dependencies'] and not n['game_started'] and not n['staged']
for f in m['overlay']:assert sha(r/'tools/native-capture'/f)==m['files']['tools/native-capture/'+f]
for p,d in n['project_dependencies'].items():assert sha(h/'native-source-v2'/p)==d
assert sha(h/'native-build-v2/GearsCapture.dll')==n['dll_sha256']
(h/'verification.json').write_text(json.dumps(dict(results={k:v['output'] for k,v in rs.items()},same_replay_source=True,native_sha256=n['dll_sha256'],dependencies=len(n['project_dependencies']),flags_unchanged=True,game_started=False,installed=False,change='Raw original quaternion arithmetic and explicit measured Gram spectral upper bound. Existing quaternion norm3e-5 retained; original getters not normalized. Strict rigid math and geometry/query gate unchanged. World snapshots retain actor/shape Gram errors and require full existing source/owner/close transaction. Same recorded replay old1fail/new0.',remaining='New observed-pose candidate not game-run; geometry/query enclosure of non-strict rigid poses and complete actual asset read still pending. Full CV24/AC22, self/world/template/body/view/hit/muzzle/headset open.',engineering_percent=25),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r235' not in old
add='''
## r235：按原getter保留未归一化观测矩阵及显式Gram误差；实际值replay通过（2026-11-12）

新增cover_physx_observed_pose.h，按原1209f0 float分组构造矩阵，不归一化、不clamp；已有quaternion norm容差3e-5保持。Observed Pose明确携带R^T R-I的Frobenius上界（亦为谱范数上界），double累加/sqrt舍入另加保守margin。原理：精确四元数矩阵Gram特征值1和1+4*(norm平方-1)*|xyz|平方，再计逐float算式operator rounding界；用此导出单Pose资格范围与compose传播界，不将近旋转冒充严格rigid。source/owner/metadata/时间/双pass/关闭guard不改。

world snapshot保留原actor/shape矩阵及各自gram_error，same也比较误差；summary成功行输出两误差并明确rigid_query_permission=0。既有pose_math::rigid、quaternion、compose及shape_geometry/query入口严格判断均不改。新的读取结果仍须后续带数值包络进入几何/查询事务；没有因为读到矩阵授予碰撞/移动权限。

以r234原float记录建立相同replay，旧代码3检查1失败，新代码5检查0（新增成功后断言被执行）：保留raw q/p逐float相同、原矩阵仍不满足strict rigid，不加载原DLL。独立math2060检查0：256组norm边缘±2.9e-5，以未归一化Rodrigues点旋转oracle核对两级变换，显式误差界、超norm/零/NaN或Inf/denormal、反射/低报误差、非nearest舍入拒绝及输出保持；实际原矩阵继续被严格geometry拒绝。Engine1743、Session125、OFF65均0，内存读取预算228/102/140不变。独立Python每op float-round原式及原getter evidence/PE来源SHA保存original-pose-arithmetic.json，原数据norm/Gram/det数值见r234。

完整Native在r233冻结树仅world/observed-pose/C0三文件overlay，423项目依赖逐SHA、flags不变、无外部工作树混入；DLL SHAc9ea318097be628ab96ac48b4c97f9ab6b545e91047bee3b7d8a5e03f4059e25。此修复尚未游戏运行，下一步新版本一次形状读取验证是否越过Body pose，再闭合全清单与非严格rigid数值包络；不称actual asset通过。

完整CV24/AC22、active/template/full self/world fence、body/view/hit/muzzle、视图遮挡及头显仍未完成，工程25%；publicv71/固定手动入口/Quest不变。证据observed-pose-r235。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(dll=n['dll_sha256'],dependencies=len(n['project_dependencies']),same_replay_source=True,documents_equal=doc.read_bytes()==mirror.read_bytes())))
