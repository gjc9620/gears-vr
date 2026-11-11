from pathlib import Path
import json,hashlib,struct,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/cached-shapes-r223';sha=lambda b:hashlib.sha256(b).hexdigest()
r=json.loads((folder/'shapes-result.json').read_text());before=json.loads((folder/'before-result.json').read_text())
for result in (r,before):result['source_sha256']={p.replace('\\','/'):v for p,v in result['source_sha256'].items()}
assert r['build_exit']==r['test_exit']==0 and before['build_exit']==0 and before['test_exit']==1 and 'failures=1 ' in before['output']
for p,d in r['source_sha256'].items():assert sha((root/p).read_bytes())==d,p
test_path='tools/native-capture/cover_physx_cached_shapes_test.cpp';assert r['source_sha256'][test_path]==before['source_sha256'][test_path]
old_header=folder/'before-cover_physx_cached_shapes.h';assert sha(old_header.read_bytes())==before['source_sha256']['tools/native-capture/cover_physx_cached_shapes.h']
original_path=folder.parent/'capsule-creation-r220/probe.json';original=json.loads(original_path.read_text());creator=next(x for x in original['records'] if x['entry']=='0x1402cfa6c')
raw=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin').read_bytes();data=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_131b000.bin').read_bytes()
assert sha(raw)=='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
assert sha(data)=='c5deb08128dea890e2606b7bb6bf30eb3e1045c19b3f477e0bebf6c7fd15231c'
for x in creator['ranges']:
 a,b=int(x['start'],16),int(x['end_exclusive'],16);v=raw[a-0x140001000:b-0x140001000];assert v.hex()==x['bytes'] and sha(v)==x['sha256']
tables=[]
for tag,rva,size in [(1,0x1347dc0,136),(2,0x13486f0,144),(3,0x1347dd8,144)]:
 blob=data[rva-0x131b000:rva-0x131b000+24]
 tables.append(dict(tag=tag,table_rva=hex(rva),descriptor_bytes=size,original_table_bytes=blob.hex(),original_slot_values=[hex(v) for v in struct.unpack('<QQQ',blob)]))
certificate=dict(original_creator=dict(path=str(original_path.relative_to(root)),sha256=sha(original_path.read_bytes()),entry=creator['entry'],ranges=creator['ranges']),
 code_sha256=sha(raw),data_sha256=sha(data),tables=tables,
 layout=dict(actor_descriptor_bytes=136,shape_vector_begin_offset=104,shape_vector_end_offset=112,shape_vector_limit_offset=120,member_pointer_stride=8,
             shape_type_offset=8,shape_pose_offset=12,shape_pose_bytes=48,flags_offset=60,dimensions_offset=128),
 claims=['Original actor descriptor allocation136, vector begin/end/limit104/112/120; append writes8-byte shape descriptor pointers and increments end8.',
 'Original Sphere/Box/Capsule descriptor alloc136/144/144, type1/2/3, original game vtable RVAs1347dc0/13486f0/1347dd8 (raw DB bias1000 accounted).',
 'Local row-major PhysX Matrix34 is copied to descriptor+12; flags word+60 is retained opaque. sphere radius128, box half128/132/136, capsule radius128/full segment length132.',
 'No claim on original internal capsule constructor half conversion, active actor creation from a cache entry, flags participation or world movement permission. Caller must pin current original image/layout/route.' ],target_executed=False)
(folder/'original-descriptor-certificate.json').write_text(json.dumps(certificate,indent=2)+'\n',encoding='utf-8')
verification=dict(test=r['output'],sources={p:sha((root/p).read_bytes()) for p in ['tools/native-capture/cover_physx_cached_shapes.h',test_path]},
 certificate_sha256=sha((folder/'original-descriptor-certificate.json').read_bytes()),
 counter=dict(before=before['output'],after=r['output'],unchanged_test_source_sha256=r['source_sha256'][test_path],old_header_sha256=sha(old_header.read_bytes()),
              issue='In-place vector member replacement after second-pass member read escaped header-only closure. Now re-read each member at end of each pass before vector header close. Initial11 failures also included a test sentinel reset error; fixed test reset first, then same corrected test showed exactly1 failure before product fix.'),
 change='Complete selected cached descriptor shape inventory up to256, no truncation. Cache snapshot re-observed twice at both outer ends; selected scale/index original lookup; two whole shape passes, vector member/header close; exact class-table/tag, rigid pose, positive dimensions. Full raw descriptor bytes retained. Source image and full typed cache guard required, output atomic and reentry blocked.',
 tests='705 checks0: empty/1/3/256 all shapes,52 read points and9 guard failure phases, shape field/membership/header/cache mutation,257/invalid class/type/value/pose/alignment/member, reentry busy and alias. Same corrected counter source before699checks1failure and after705checks0 (6 extra closing reads produce6 additional fault cases). Mock memory only.',
 remaining='No cache-to-active identity, authored matching, current world fence, filter interpretation, complete self set, production integration/game/headset/fullCV24/AC22.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r223：选中cache的完整shape descriptor清单接通；原地member替换回读缺口修正（2026-11-12）

新增cover_physx_cached_shapes.h，将r222完整cache snapshot和明确requested scale交原lookup选中index，再读取原actor descriptor+104/+112/+120的std-vector形状清单。原1402cfa6c证据逐字节重新核对：actor descriptor136，成员指针stride8；sphere/box/capsule tag1/2/3、alloc136/144/144、game vtable RVA1347dc0/13486f0/1347dd8；local Matrix34在+12，flags+60原样保留，sphere radius128、box half128/132/136、capsule radius128/full segment length132。原rdata三个完整table片段保留；未调用其函数。外层必须pin当前原game image/route并认证typed cache布局/body/C0，不靠读取到相同地址冒充当前模块认证。

最多256全部descriptor，两次完整shape pass，选中cache清单在外层前后各用r222完整双pass回读并与原snapshot一致；逐类型table/tag一致、rigid pose/有限正尺寸、header/capacity/alignment、busy/alias/异常与输出不覆盖。保留完整raw descriptor字节和原列表顺序、重复pointer；未知/convex类整批停止，不丢shape继续。值保持PhysX单位，无半尺寸再除2/0.02再乘，不解读flags为参与权限。

新故障用例找到本轮初版真实缺口：vector header不变，second-pass某成员已读后原地替换该pointer，header-only closure未发现。新增每pass尾部逐member指针回读后再关闭vector header。初版11失败中另有测试sentinel未逐例复位导致连锁误报；先修测试复位后，固定同测试源码原header699检查只1失败；产品补member close后同测试705检查0（新增6处收尾读导致6个额外fault case）。旧header/前后依赖SHA/log都保留，不将测试修正冒称产品修正。

705检查0覆盖空/1/3/256清单、三类字段/flags原样、3条时52个读失败点、9个guard拒绝阶段、shape数据/membership/header/cache中途变化、257越界/错误类型table/NaN/非rigid/错对齐/null成员、reentry busy/alias。只有Mock内存原分派，无原getter/游戏/XR/生产Native构建/安装。active actor确由此cache创建的身份关联、authored逐项匹配、full self set/flags/world fence及生产接入仍未完成；完整CV24/AC22、body/view/hit/muzzle事务、视图遮挡和头显继续待完成，工程25%。证据cached-shapes-r223。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=705,failures=0,before_failures=1,descriptor_kinds=3,documents_equal=doc.read_bytes()==mirror.read_bytes())))
