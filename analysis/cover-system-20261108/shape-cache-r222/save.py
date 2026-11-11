from pathlib import Path
import json,hashlib,struct,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-cache-r222';sha=lambda b:hashlib.sha256(b).hexdigest()
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in ['cache','live']}
for name,r in results.items():
 assert r['build_exit']==r['test_exit']==0,(name,r)
 for path,d in r['source_sha256'].items():assert sha((root/path).read_bytes())==d,(name,path)
raw=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin').read_bytes()
data=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_131b000.bin').read_bytes()
assert sha(raw)=='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
assert sha(data)=='c5deb08128dea890e2606b7bb6bf30eb3e1045c19b3f477e0bebf6c7fd15231c'
probe=json.loads((folder/'probe.json').read_text());near=probe['records'][0];assert near['entry']=='0x1402c5b5c'
for chunk in near['ranges']:
 a,b=int(chunk['start'],16),int(chunk['end_exclusive'],16);actual=raw[a-0x140001000:b-0x140001000];assert actual.hex()==chunk['bytes'] and sha(actual)==chunk['sha256']
assert sha((folder/'0x1402c5b5c-pseudocode.c').read_bytes())==near['pseudocode_sha256']
constant=data[0x1417070ac-0x14131b000:0x1417070ac-0x14131b000+4];assert constant.hex()=='17b7d138'
mask=data[0x141707090-0x14131b000:0x141707090-0x14131b000+16];assert mask.hex()=='ffffff7f'*4
decl=json.loads(next(line for line in (folder/'declarations.raw.log').read_text(encoding='utf-8-sig').splitlines() if line.startswith('{')))
summary=[]
for row in decl['arrays']:
 inner=row['members']['field:InnerProperty'];target=inner['members']['field:Struct']
 summary.append(dict(array=row['reference'],inner=inner['reference'],dimension=inner['members']['field:ArrayDim'],target=target['reference'],serialized_element_size=inner['members']['property:ElementSize']))
assert [r['target'] for r in summary]==["ScriptStruct'Core.Object.Pointer'","ScriptStruct'Core.Object.Vector'"]
(folder/'declaration-summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
package=Path('J:/gearsVr/analysis/packages/decompressed/Engine.u')
with package.open('rb') as f:package_sha=hashlib.file_digest(f,'sha256').hexdigest()
assert package_sha=='95b0c8da438a5acaf1ebfcdd0d94ac4d42c06422e717ce1ffe3f095e8ce90510'
caller_path=folder.parent/'creation-scale-r221/probe.json';caller=json.loads(caller_path.read_text())
caller=next(x for x in caller['records'] if x['entry']=='0x1402c3efc')
for chunk in caller['ranges']:
 a,b=int(chunk['start'],16),int(chunk['end_exclusive'],16);v=raw[a-0x140001000:b-0x140001000];assert v.hex()==chunk['bytes'] and sha(v)==chunk['sha256']
certificate=dict(raw_sha256=sha(raw),data_sha256=sha(data),near_zero_function=near,
 caller_evidence=dict(path=str(caller_path.relative_to(root)),sha256=sha(caller_path.read_bytes()),entry=caller['entry']),
 epsilon=dict(address='0x1417070ac',bytes=constant.hex(),value=struct.unpack('<f',constant)[0]),abs_mask=dict(address='0x141707090',bytes=mask.hex()),
 facts=['BodySetup+cc CollisionGeom pointer array (8 stride), +dc CollisionGeomScale3D vector array (12 stride), loop count+e4; index associates scale with opaque descriptor at pointers[index].',
 '2c3efc subtracts cached scale minus request in float32.2c5b5c clears sign bit and JB/JNB implements strict each abs(delta)<epsilon; finite-positive path only admitted here.',
 'Original cache loop continues after a match and overwrites selected pointer, so last matching entry wins. Duplicate keys/pointers are preserved, not rejected.',
 'Package declarations associate these arrays with Core.Object.Pointer and Core.Object.Vector. Serialized ElementSize0 is not native extent; live reflection separately requires8/12 and exactXYZ FloatProperty.'],
 limitation='Original static evidence; not a current original call. NaN/unordered behavior is excluded by validated finite-positive keys, no permission inferred from a selected cache entry.')
(folder/'original-cache-certificate.json').write_text(json.dumps(certificate,indent=2)+'\n',encoding='utf-8')
sources=['tools/native-capture/cover_physx_shape_cache.h','tools/native-capture/cover_physx_shape_cache_test.cpp','tools/native-capture/cover_shape_cache_live.inl','tools/native-capture/cover_shape_cache_live_test.cpp']
verification=dict(results={n:r['output'] for n,r in results.items()},sources={p:sha((root/p).read_bytes()) for p in sources},
 package_sha256=package_sha,parser_sha256=sha((folder/'Eliot.UELib.dll').read_bytes()),parser_helper_source_sha256=sha((folder/'DescribeShapeCache.cs').read_bytes()),parser_helper_exe_sha256=sha((folder/'DescribeShapeCache.exe').read_bytes()),
 certificate_sha256=sha((folder/'original-cache-certificate.json').read_bytes()),
 change='Complete paired opaque cache observation,64 entries maximum no truncation; both headers close per pass, two complete passes, typed guard before/between/after. Ordered finite-positive float32 selection uses strict1e-4 and last matching index. Reflected adapter binds same BodySetup/BoneName and exact packed+cc/+dc ArrayProperty16, inner StructProperty dimension1 offset0 width8/12, Pointer/Vector ScriptStruct extent8/12, XYZ float fields.',
 tests='Raw129 checks0: empty/1/3/64, all20 read faults at3entries, header/pointer/scale mutations, mismatch/65 rejection, invalid values, three guard phases, request/busy/alias, duplicates and last match, strict threshold/nextafter on allXYZ, invalid lookup. Typed332 includes point292; three shapes: normal typed snapshot, inner metadata mutation during payload read guard/output preservation, wrong offsets/stride/target/bone/vector extent rejection. Initial builds required atomic include and shadow name fixes; no warning suppression or product relaxation.',
 remaining='Opaque selected descriptor only. No descriptor shape list read, cache-to-active identity or authored correspondence, full self set/world fence/production integration, game/headset or fullCV24/AC22 acceptance.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r222：BodySetup配对cache完整读取、原近似匹配和typed字段认证（2026-11-12）

原2c3efc使用BodySetup+cc CollisionGeom指针数组与+dc CollisionGeomScale3D向量数组，index配对，count+e4；helper2c5b5c对cachedScale-request float32差值清sign bit，JB/JNB严格逐XYZ小于float1e-4（1417070ac=17b7d138），匹配后循环不break，最后匹配项覆盖前者。原完整helper与caller全部chunks/指令、abs mask和epsilon字节SHA已核。原Engine.u同SHA95b0c8da…只读UELib声明解析确认数组inner为Core.Object.Pointer/Vector，序列化ElementSize0不当作native extent。

新增cover_physx_shape_cache.h：最多64全部配对、不截断；packed4头与Pointer8/Vector4 payload对齐分别处理，count一致、capacity边界和正normal scale/pointer验证；每pass关闭两个header、两次完整pass比较，typed source guard前/中/后，busy/alias/exception及失败不发布。保留重复pointer/key与原顺序。select按原有限正scale路径float差值及严格epsilon返回最后匹配index和opaque pointer，miss/invalid保留旧输出；不声称复刻NaN/unordered路径，不选择active shape。

新增cover_shape_cache_live.inl：同BodySetup身份/BoneName，CollisionGeom与CollisionGeomScale3D必须ArrayProperty16且offsetcc/dc；inner StructProperty dim1/offset0/stride8或12，目标Pointer/Vector ScriptStruct extent8或12及XYZ FloatProperty4布局；producer Proof贯穿完整raw cache采集，外层仍认证原route、当前asset/body/C0关联。没有在持binding mutex时调用外层guard。

Raw129检查0失败：空/1/3/64全量、3条的20处读失败、header/scale/pointer跨pass变化、两数组count不符/65越界、无效值、三次guard、request/busy/alias、重复项最后匹配、XYZ严格阈值及nextafter边界。typed332累计（含point292）0：三种shape当前反射采集，payload时inner metadata变更触发guard并保持输出，offset/stride/target/BoneName/Vector extent错误拒绝。编译初次补显式atomic include与局部shadow名称；保留失败log，未放宽/WX。C#首次路径斜杠参数被csc误解，改绝对Windows路径，解析完成；旧API弃用warning原样保留。

cache条目目前仍是opaque descriptor；尚未读取其shape vector或证明它与active shape逐项对应。缓存读取/原lookup不提供world或movement权限。所有依赖SHA保存时当前一致，证据shape-cache-r222；无游戏/XR/原getter/生产Native构建/安装，完整CV24/AC22及body/view/hit/muzzle事务、视图遮挡与头显继续未完成，工程25%。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(raw_checks=129,typed_checks=332,failures=0,package_arrays=len(summary),documents_equal=doc.read_bytes()==mirror.read_bytes())))
