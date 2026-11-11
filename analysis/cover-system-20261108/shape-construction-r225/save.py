from pathlib import Path
import json,hashlib,struct,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-construction-r225';sha=lambda b:hashlib.sha256(b).hexdigest()
r=json.loads((folder/'construction-result.json').read_text());assert r['build_exit']==r['test_exit']==0
for p,d in r['source_sha256'].items():assert sha((root/p).read_bytes())==d,p
evidence=folder/'original-evidence.json';bundle=json.loads(evidence.read_text())
records={x['normalized_result']['procedure']['address']:x for x in bundle['records'] if x['operation']=='analyze_function'}
binary=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
data=binary.read_bytes();assert sha(data)=='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
pe=struct.unpack_from('<I',data,0x3c)[0];sections=struct.unpack_from('<H',data,pe+6)[0];optional=struct.unpack_from('<H',data,pe+20)[0]
def image(rva,size):
 for n in range(sections):
  at=pe+24+optional+40*n;virtual_size,virtual_at,raw_size,raw_at=struct.unpack_from('<IIII',data,at+8)
  if virtual_at<=rva and rva+size<=virtual_at+raw_size:return data[raw_at+rva-virtual_at:raw_at+rva-virtual_at+size]
 raise AssertionError((rva,size))
functions=[]
for addr in ['0x1801336b0','0x180120780','0x180124660','0x18009ef30','0x1800104b0','0x1800b0790']:
 record=records[addr];body=record['normalized_result']['procedure']['body'];assert body['available']
 chunks=[]
 for extent in body['ranges']:
  a,b=int(extent['start'],16),int(extent['end'],16);blob=image(a-0x180000000,b-a+1)
  chunks.append(dict(start=hex(a),end_inclusive=hex(b),size=len(blob),sha256=sha(blob)))
 assert sum(x['size'] for x in chunks)==body['total_bytes']
 functions.append(dict(entry=addr,evidence_id=record['evidence_id'],chunks=chunks,complete_body_bytes=body['total_bytes']))
assert struct.unpack('<Q',image(0x22ccb8+0x70,8))[0]==0x18009ef30
assert struct.unpack('<Q',image(0x2330a8+0x88,8))[0]==0x1801336b0
certificate=dict(artifact=dict(path=str(binary),sha256=sha(data),bytes=len(data)),evidence=dict(path=str(evidence.relative_to(root)),sha256=sha(evidence.read_bytes()),records=len(bundle['records'])),functions=functions,
 facts=[
 'Original NpActor vtable2330a8 slot88 ->1336b0 createShape; it passesNpActor+28 toActor120780 ->124660. Actor body member+18 is therefore publicNpActor+40, same optional Body field used by current ownership observations.',
 'Actor124660 builds internal shape creation structure. Dynamic Body present: descriptor Matrix34 translation30/34/38 copied unchanged and rotation converted to quaternion. Body null: Actor matrix atActor+48 (NpActor+70) is composed with descriptor pose before conversion, so stored pose is world-space.',
 'Float quaternion conversion uses trace branch and largest diagonal branches, no normalization. Dynamic trace grouping=(m11+m00)+m22; static=(m22+m11)+m00. Static composition translation row0 Y+X+Z+T; rows1/2 X+Z+Y+T.',
 'Public capsule radius80 and full length84 are copied as one qword to internal78/7c. Scene vtable22ccb8 slot70 ->9ef30 factory, case3 ->104b0. Capsule ctor copies internal78 radius to1b0 and multiplies internal7c by0.5 to half_segment1b4.',
 'Common B0790 copies internal position40/44/48 into stored88/8c/90 and quaternion4c/50/54/58 into stored78/7c/80/84 without normalization. Additional capsule backend rotation fieldsb4..c0 do not replace those stored quaternion fields.',
 'Bridge is pure owned supported positive-normal geometry math. It does not prove initial actor loading uses every observed add-shape path, current template/active identity, weld mapping or filter participation. No constructor/getter executed.' ],
 implementation='Explicit dynamic_local/static_world placement; caller supplies authenticated static actor pose. Float operation grouping preserved in written math; fixed capsule half conversion, no unit/margin conversion repeated. Nearest rounding only; nonrigid/nonfinite/unsupported/alias and subnormal length/half reject without publishing.',
 limitations='Static analysis and CPU reconstruction, not bit-identical proof for every native floating-point case or original code execution. Full module/lifetime/world scope remains external; native value-flow limits in full evidence remain retained.')
(folder/'original-construction-certificate.json').write_text(json.dumps(certificate,indent=2)+'\n',encoding='utf-8')
sources=['tools/native-capture/cover_physx_shape_construction.h','tools/native-capture/cover_physx_shape_construction_test.cpp']
verification=dict(test=r['output'],sources={p:sha((root/p).read_bytes()) for p in sources},certificate_sha256=sha((folder/'original-construction-certificate.json').read_bytes()),
 tests='33041 checks0:128 rotations about eachXYZ, two placements and all3 kinds; four points checked with independent Rodrigues and quaternion cross-product oracles against local or Actor-composed world transforms; dimensions/half length retained, zero segment, source invalid/nonfinite/subnormal/min-normal length-to-subnormal half, nonrigid local/static actor, dynamic unused actor, unsupported/alias and nearest-mode refusal/output preservation.',
 remaining='No current active actor/template association or complete body match, static actor source sampling, welds, flags/full self/world fence, production integration/game/headset/fullCV24/AC22.',
 game_started=False,original_constructor_executed=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r225：原descriptor→实际stored形状表示路线与dynamic/static区别明确（2026-11-12）

对原PhysXCore同SHA22051aa7…只读REA：NpActor vtable2330a8 slot88→1336b0 createShape，NpActor+28交Actor120780→124660；Actor+18正是publicNpActor+40可选Body。124660把公开descriptor local Matrix34转换为内部position/quaternion：动态Body存在时保留local；Body为null静态时先与Actor+48（NpActor+70）矩阵合成world，再转换。不能用动态local规则套静态，或把静态stored world再乘一次Actor。

胶囊公开radius80/full length84作为qword复制至内部78/7c；Scene vtable22ccb8 slot70→9ef30 case3→104b0构造，内部78 radius直接入1b0，7c full length乘原0.5后入1b4 half_segment。共同B0790把内部40/44/48位置写stored88/8c/90，4c/50/54/58 quaternion写78/7c/80/84，不normalize。胶囊另外的b4..c0 backend旋转不覆盖这些stored quat。完整11份Evidence导出/REA正常关闭，六个完整函数体与两个vtable slot逐PE字节SHA核对，未运行原constructor/getter。

新增cover_physx_shape_construction.h纯owned表示桥：显式dynamic_local/static_world，静态Actor pose须调用者认证，动态不消费未使用Actor矩阵；保留原float矩阵合成及trace/最大对角分支四元数转换分组，dynamic trace=(m11+m00)+m22、static=(m22+m11)+m00；静态translation row0 Y+X+Z+T、rows1/2 X+Z+Y+T。sphere radius/box half原样，capsule full length转half，不再乘单位或margin。不normalize/clamp，要求nearest；异常、非rigid、subnormal/半段下溢、unsupported/alias拒绝保持输出。

33041检查0：XYZ各128旋转，两种placement和三种kind，每例四个独立点用Rodrigues及quat cross-product oracle核对local/Actor合成world，另核尺寸、零段、min-normal长度除半成subnormal拒绝、NaN/无穷/负数/非rigid、动态忽略无效unused Actor、unsupported/alias/FP mode拒绝。所有依赖SHA当前一致。此为原静态证据和CPU数学重建，不称全部浮点逐位相同、所有初次load路径已证或实际active形状验收；当前actor/template身份、staticActor当前读取、weld和flags/full self/world fence及生产接入仍待完成。无游戏/XR/生产Native构建/安装，完整CV24/AC22、body/view/hit/muzzle事务及视图遮挡/头显未完成，工程25%。证据shape-construction-r225。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=33041,failures=0,original_functions=len(functions),documents_equal=doc.read_bytes()==mirror.read_bytes())))
