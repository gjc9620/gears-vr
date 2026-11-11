from pathlib import Path
import json,hashlib,struct,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/capsule-creation-r220'
sha=lambda b:hashlib.sha256(b).hexdigest()
result=json.loads((folder/'creation-result.json').read_text());assert result['build_exit']==result['test_exit']==0
for p,d in result['source_sha256'].items():assert sha((root/p).read_bytes())==d,p
raw_path=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin');raw=raw_path.read_bytes()
data_path=raw_path.with_name('section_131b000.bin');data=data_path.read_bytes()
assert sha(raw)=='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
assert sha(data)=='c5deb08128dea890e2606b7bb6bf30eb3e1045c19b3f477e0bebf6c7fd15231c'
records=json.loads((folder/'probe.json').read_text())['records']
for r in records:
 for x in r['ranges']:
  a,b=int(x['start'],16),int(x['end_exclusive'],16);blob=raw[a-0x140001000:b-0x140001000]
  assert blob.hex()==x['bytes'] and sha(blob)==x['sha256']
 assert sha((folder/(r['entry']+'-pseudocode.c')).read_bytes())==r['pseudocode_sha256']
constants=[]
for a,bits in [(0x1417070c4,'0ad7a33c'),(0x1417070e4,'0000003f'),(0x14170778c,'cdcccc3c'),(0x141707080,'00000080')]:
 blob=data[a-0x14131b000:a-0x14131b000+4];assert blob.hex()==bits
 constants.append(dict(address=hex(a),bytes=bits,value=struct.unpack('<f',blob)[0]))
certificate=dict(code_source=dict(path=str(raw_path),sha256=sha(raw)),data_source=dict(path=str(data_path),sha256=sha(data)),
 records=records,constants=constants,address_mapping='IDA raw DB base0 corresponds original140001000; pseudocode symbol/data numbers need that bias, not140000000.',
 claims=[
 '1402c3efc passes BodySetup+108 aggregate to1402cfa6c; full caller archive transform-callers.json and caller pseudocode retained, source records SHA checked separately.',
 '1402cfa6c iterates Sphere offset0/count8, Box10/count18, Sphyl20/count28, stride80; skips NoRBCollision bit0 at44/4c/48 respectively.',
 '1402cfa6c computes per-axis PhysX scale in float32 as float.02 * incoming scale. Translation multiplies respective axis. Dimensions all use resulting X scale.',
 'Sphere radius=scaledX*radius+.025; box half=(scaledX*dimension)*.5+.025; capsule radius=scaledX*radius+.025 and descriptor full segment length=scaledX*length, without length margin.',
 'Capsule temporary UE rows are -row0,row2,row1; translation row3 copied. 1402cf478 transposes3x3 into PhysX row-major and copies translation. New PhysX localY is original UE localZ, no endpoint sign reversal.',
 'Creation scale producer is not certified by this record. Do not substitute existing point-query scale proof or infer welded body/member correspondence.' ],
 limitation='Historical captured original code/static interpretation. No present-process freshness, original call, world fence, shape membership join or permission. IDA tool bridge failed missing ida_nexus; direct local IDA script used copied DB, no original DB save requested; qexit0. First probe used wrong VA base and found no refs; corrected base explicitly recorded. Native data addresses recovered from pseudocode must include1000 section bias.',
 target_executed=False)
(folder/'original-creation-certificate.json').write_text(json.dumps(certificate,indent=2)+'\n',encoding='utf-8')
verification=dict(test=result['output'],sources={p:sha((root/p).read_bytes()) for p in ['tools/native-capture/cover_physx_shape_creation.h','tools/native-capture/cover_physx_shape_creation_test.cpp']},
 certificate_sha256=sha((folder/'original-creation-certificate.json').read_bytes()),
 change='Owned descriptor builder with original .02 axis scale, fixed .025 margin and capsule [-X,Z,Y] basis; retains full segment length as descriptor does. Explicit creation-scale input, no inference from existing point-scale collector. Invalid/disabled/unsupported/alias preserve output.',
 remaining='Creation-scale live producer, authored-to-active identity, flag eligibility, welds, internal capsule constructor full-to-half route, production collection/integration/query fence, full game/headset CV/AC acceptance.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r220：找到原aggregate创建路径，胶囊Z→Y及固定margin来源已明确（2026-11-12）

沿F2610十四个caller定位1402c3efc的BodySetup+108 aggregate参数，再到1402cfa6c：依次Sphere/Box/Sphyl原数组，stride80，各自NoRBCollision bit0（44/4c/48）跳过。原胶囊临时UE矩阵行改为[-row0,row2,row1]，translation原样复制；1402cf478将其3x3转置为PhysX行主矩阵。因此PhysX局部Y精确对应原authored局部Z，不能按通用SDK假定简单X轴或任意90度补丁。

同一创建流程的另一个关键差异：原float.02先乘incoming scale各轴；平移各轴再乘该axis scale，而所有尺寸使用scaledX。sphere radius=scaledX*radius+.025；box half=(scaledX*dimension)*.5+.025；capsule radius=scaledX*radius+.025，descriptor full segment length=scaledX*length，无长度margin。原常量PhysX.025而非零，因此不能拿r218未加creation margin的查询几何直接断言与active尺寸相等；不修改既有point语义来掩盖这种差异。

新增cover_physx_shape_creation.h纯owned descriptor转换，显式要求该创建调用的scale；不把既有point-scale proof冒充creation-scale来源。保留full segment length字段，内部构造full→half后续另证；按原规则处理非均匀正scale的尺寸X/平移XYZ，保留异常/disabled/unsupported/alias拒绝不发布。1554检查0失败，包括XYZ各128旋转下新Y列等于原Z行、原尺寸float分组与margin、非均匀平移、零段和拒绝保护；MSVC/W4/WX构建0。

原工具桥接因ida_nexus缺失失败，未安装/升级；改用已安装本地IDA的只读脚本和独立DB副本，最终qexit0，无游戏/目标运行。首次相对地址基准误用导致无xref，纠正为raw0→140001000后定位；原数据常量地址必须加这一1000偏置（.02=1417070c4、.5=1417070e4、.025=14170778c、sign=141707080），证书逐原字节核对完整chunk与SHA，保留远尾块1408f2a3c。证据capsule-creation-r220，原DB未主动保存。

仍需creation-scale当前来源闭合、逐active/authored身份关联、weld/flags参与规则、完整world fence及生产接入；未生产Native构建/安装/游戏/XR/头显，完整CV24/AC22和body/view/hit/muzzle同版事务、视图遮挡继续未完成，工程25%。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=1554,failures=0,original_functions=len(records),constant_values=constants,documents_equal=doc.read_bytes()==mirror.read_bytes())))
