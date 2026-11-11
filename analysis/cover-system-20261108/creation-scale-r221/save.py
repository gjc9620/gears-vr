from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/creation-scale-r221';sha=lambda v:hashlib.sha256(v).hexdigest()
r=json.loads((folder/'scale-result.json').read_text());assert r['build_exit']==r['test_exit']==0
for p,d in r['source_sha256'].items():assert sha((root/p).read_bytes())==d,p
source=Path('J:/gearsVr/analysis/ghidra/render-16812-verified/section_1000.bin').read_bytes();assert sha(source)=='911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6'
probe=json.loads((folder/'probe.json').read_text());entries={x['entry']:x for x in probe['records']}
for x in entries.values():
 for c in x['ranges']:
  a,b=int(c['start'],16),int(c['end_exclusive'],16);raw=source[a-0x140001000:b-0x140001000]
  assert raw.hex()==c['bytes'] and sha(raw)==c['sha256']
 assert sha((folder/(x['entry']+'-pseudocode.c')).read_bytes())==x['pseudocode_sha256']
assert '0x140211900' in entries and '0x1402c3efc' in entries
certificate=dict(source_sha256=sha(source),probe_sha256=sha((folder/'probe.json').read_bytes()),
 route=['0x140211900','0x1402c3efc','0x1402cfa6c'],
 facts=[
 '211900: RCX PhysicsAssetInstance ->RSI, RDX SkeletalMeshComponent ->RDI; instance60 is assigned mesh78 Actor pointer before scale use.',
 '211a02..211a2e: component1e8 scalar multiplied component1ec/1f0/1f4 axes; stores vector var190/18c/188.',
 '211a34..211a76: optional Actor from instance60; actor198 times19c/1a0/1a4, then that result times each component product, original float32 grouping.',
 '211b69..211b7d passes scale stack vector asR9 to2c3efc. 2c3f44 keepsR9 inRDI; 2c47b8 passes sameRDI asRDX to2cfa6c; RCX isBodySetup+6c.',
 '2c3efc can reuse scale-keyed cached descriptors from BodySetup cc/dc arrays; current field equality cannot by itself date active shape construction.',
 'Other direct callers2d1eb0 and1016350 retained; this producer covers the skeletal PhysicsAssetInstance211900 route, not all creation callers.' ],
 input_scope='Historical code and original source bytes only. No target execution or current creation-call capture. Reuses r220 owned raw IDA database copy, address bias140001000.',
 records={k:dict(ranges=v['ranges'],pseudocode_sha256=v['pseudocode_sha256']) for k,v in entries.items()})
(folder/'original-scale-certificate.json').write_text(json.dumps(certificate,indent=2)+'\n',encoding='utf-8')
verification=dict(test=r['output'],sources={p:sha((root/p).read_bytes()) for p in ['tools/native-capture/cover_creation_scale_live.inl','tools/native-capture/cover_creation_scale_live_test.cpp']},
 certificate_sha256=sha((folder/'original-scale-certificate.json').read_bytes()),
 change='Creation-route-specific reflected producer reuses verified scalar/vector selection but additionally proves current mesh250 PhysicsAssetInstance identity, instance60 MeshPointer Owner schema and same Actor as mesh78 and typed roles. Captured authored source must carry same mesh/instance and exact scale, producer Proof closes through payload read.',
 tests='356 total checks0 including existing point292. Three kinds: instance/actor/mesh link failures preserve output, proof expires on actor or instance owner changes, nonuniform input observed, authored capture succeeds, owner mutation during payload read returns guard and preserves output; mismatched inventory instance rejected before capture. Initial capture test build failed missing namespace alias; fixed test alias, no product relaxation.',
 remaining='Current fields are not historical construction/cache identity proof. Need active/authored shape join, cached scale binding, flags/weld behavior, world fence, production integration and game/headset fullCV24/AC22.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r221：skeletal碰撞体创建scale原调用来源与实例Owner关联已接通（2026-11-12）

原211900保留RCX PhysicsAssetInstance于RSI、RDX SkeletalMeshComponent于RDI；先把mesh78 Actor赋到instance60。211a02…211a2e用component1e8乘1ec/1f0/1f4形成XYZ；211a34…211a76可选Actor198乘19c/1a0/1a4，再乘component各自乘积，原float分组与point route相同。211b69把该stack vector作R9传2c3efc，2c3f44留在RDI，2c47b8同RDI作为RDX传2cfa6c，RCX则BodySetup+6c。四个完整原函数/所有chunks、pseudocode、caller和指令证据creation-scale-r221逐字节SHA与原911ab9bc…快照核对。其余2d1eb0／1016350直接caller保留，不把这个skeletal来源冒充所有创建路径。

新增cover_creation_scale_live.inl：复用已有scalar/vector反射检查，但以独立creation语义核对mesh250实例引用和PhysicsAssetInstance身份、instance60 Owner MeshPointer的schema/reference class，并要求其Actor与mesh78和typed roles Owner身份相同。这里明确要求外层认证211900创建route及当前C0/roles；不只因point公式相同就跳过实例关联。capture还要求同inventory mesh/instance和精确receiver scale，producer Proof贯穿raw payload前后；不在binding mutex内调用外层guard。

356累计检查0失败（含原point292）：三种shape分别验证正常producer/capture，instance Owner或mesh实例引用/typed Owner不一致拒绝且保持输出；Actor缩放/instance Owner更改使旧Proof失效；非均匀输入如12/6/6按原值观察；读取payload时改变instance Owner返回guard且不覆盖旧输出，错误inventory实例在采集前拒绝。新增capture测试首次编译漏局部namespace别名，保留失败log；修正测试别名后build/test0，无产品放宽。

原2c3efc还可从BodySetup cc/dc数组复用按scale索引的cached descriptor，因此“今天字段scale正确”仍不等于“当前active shape当时按它创建”。新producer只证明当前输入与所选原route字段对应；历史cache/active关联仍待闭合，不授予世界或移动权限。原完整源码/证书与记录同步，未游戏/XR/原调用/生产Native构建/安装；完整CV24/AC22、body/view/hit/muzzle事务和视图遮挡继续未完成，工程25%。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(checks=356,failures=0,original_functions=len(entries),documents_equal=doc.read_bytes()==mirror.read_bytes())))
