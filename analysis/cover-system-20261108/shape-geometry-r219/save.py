from pathlib import Path
import json,hashlib,struct,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-geometry-r219'
sha=lambda data:hashlib.sha256(data).hexdigest()
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in ['geometry','world']}
for n,r in results.items():
 assert r['build_exit']==r['test_exit']==0,(n,r)
 for p,digest in r['source_sha256'].items():assert sha((root/p).read_bytes())==digest,(n,p)
evidence_path=folder/'original-evidence.json';bundle=json.loads(evidence_path.read_text())
records={r['parameters'].get('procedure'):r for r in bundle['records'] if r['operation']=='analyze_function'}
binary=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
data=binary.read_bytes();assert sha(data)=='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
pe=struct.unpack_from('<I',data,0x3c)[0];sections=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0]
section_at=pe+24+optional
def image_bytes(rva,size):
 for n in range(sections):
  at=section_at+n*40;virtual_size,virtual_at,raw_size,raw_at=struct.unpack_from('<IIII',data,at+8)
  if virtual_at<=rva and rva+size<=virtual_at+raw_size:
   return data[raw_at+rva-virtual_at:raw_at+rva-virtual_at+size]
 raise AssertionError((rva,size))
facts=[]
for procedure,claim in [
 ('0x18000bd10','BoxShape initialization negates 1b8/1bc/1c0 to min and keeps positive max, then builds eight corners; values are local half extents.'),
 ('0x1801aaac0','Corner helper copies all eight combinations of supplied min/max XYZ, no scaling.'),
 ('0x180012780','Capsule endpoint getter uses cached matrix column Y (18/24/30) times half_segment1b4, minus/plus centre38/3c/40, radius1b0. It may update cache; never dispatched here.')]:
 r=records[procedure];body=r['normalized_result']['procedure']['body'];assert body['available'] and len(body['ranges'])==1
 a,b=body['ranges'][0]['start'],body['ranges'][0]['end'];size=int(b,16)-int(a,16)+1
 assert size==body['total_bytes'];blob=image_bytes(int(a,16)-0x180000000,size)
 facts.append(dict(procedure=procedure,claim=claim,evidence_id=r['evidence_id'],start=a,end=b,size=size,sha256=sha(blob)))
verification=dict(results={n:r['output'] for n,r in results.items()},
 sources={p:sha((root/p).read_bytes()) for p in ['tools/native-capture/cover_physx_shape_geometry.h','tools/native-capture/cover_physx_shape_geometry_test.cpp','tools/native-capture/cover_physx_shape_parameters.h']},
 artifact=dict(path=str(binary),sha256=sha(data),bytes=len(data)),evidence=dict(path=str(evidence_path.relative_to(root)),sha256=sha(evidence_path.read_bytes()),records=len(bundle['records'])),
 original_semantics=facts,
 implementation='Pure owned Snapshot interpretation, native PhysX units throughout. Box half extents retained without dividing by2 or multiplying .02. Capsule endpoints use shape_world columnY in double. Sphere centre/radius, zero-segment capsule retained explicitly; unsupported/invalid/alias preserve output. No memory reads or original calls.',
 testing='13840 checks0 across128 rotations about each XYZ axis; independent Rodrigues oracle for every box corner and Y capsule endpoints, symmetry, sphere/reset, zero segment, unsupported shape, invalid/subnormal/nonfinite dimension/radius/half, nonrigid pose and alias. Original world1688/parameter687/owner519 regression0, PE opened as data only.',
 remaining='Authored Z to active Y creation basis and per-shape correspondence unproven. No selection/filter flags, current world fence, query packing or permission. Pure decoder does not attest current module/freshness. Numerical endpoint values describe geometric input, not a bit-identical replay of float32 cache getter; no capsule sweep enabled. Original native OBB query internal convention remains unproven.',
 game_started=False,original_getter_dispatched=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r219：原active盒体半尺寸／胶囊Y轴语义与owned几何解释接通（2026-11-12）

原PhysXCore完整SHA22051aa7…ddd48静态核对：BoxShape构造9100把descriptor78/7c/80直接存入1b8/1bc/1c0；bd10取其负值作为min、正值作为max，交1aaac0生成八种XYZ角点组合。因此这三个原getter值是局部半尺寸，不再仅称未知raw dimensions。原胶囊12780使用缓存矩阵18/24/30（矩阵14起始的Y列）乘1b4 half_segment，再减/加38/3c/40 centre，radius1b0；该getter可写缓存，未调用。完整13份REA证据导出并正常关闭，三个关键完整函数体原字节范围/SHA已记录，无目标执行。

新增cover_physx_shape_geometry.h纯owned Snapshot解释层：sphere centre/radius，box half/orientation，capsule Y方向两端点与radius，全部保持原PhysX单位，不再乘0.02、不将半尺寸除2。端点double几何计算，不冒称逐位复刻原float缓存getter；零半段胶囊明确保留两端重合，无自动转球/启用sweep。无内存读取、原函数分派、世界权限或authored匹配。

13840检查0失败：XYZ各轴128组旋转，独立Rodrigues检查全部盒角与胶囊Y端点、端点对称、sphere清空其他字段、零半段、负/非有限/subnormal尺寸、非rigid pose、未知kind和alias，拒绝保持输出。原world1688（含parameter687/owner519）重新编译与PE只读夹具回归0。所有编译依赖SHA在保存时当前一致。

authored胶囊Z轴→activeY轴创建修正和逐shape对应仍未证明；不按actor/member序号猜骨骼。flags/参与规则、current query-visible world闭合、native OBB查询内部矩阵解释及生产接入仍待完成。未运行游戏/XR/原getter、未生产Native构建/安装；body/view/hit/muzzle同版事务、视图遮挡与完整CV24/AC22、头显继续未完成，工程25%。证据shape-geometry-r219。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(geometry_checks=13840,world_checks=1688,failures=0,semantics=len(facts),documents_equal=doc.read_bytes()==mirror.read_bytes())))
