from pathlib import Path
import json,hashlib,datetime,re
root=Path.cwd();folder=root/'analysis/cover-system-20261108/point-scale-r212'
result=json.loads((folder/'source-verified-result.json').read_text())
assert result['build_exit']==result['test_exit']==0
sources=['tools/native-capture/cover_point_scale_live.inl','tools/native-capture/cover_point_scale_live_test.cpp','tools/native-capture/cover_asset_setup_live_test.cpp']
originals=['analysis/cover-system-20261108/collection-static/objects.tsv',
 'analysis/cover-system-20261108/world-component-point-r62/component-route-evidence.json',
 'analysis/cover-system-20261108/world-component-point-r62/0x1403a6c34-pseudocode.c',
 'tools/native-capture/cover_primitive_metrics.h']
names=(root/originals[0]).read_text(encoding='utf-8-sig').splitlines()
declarations=[line for line in names if any(name in line for name in ["Engine.PrimitiveComponent.Scale'","Engine.PrimitiveComponent.Scale3D'","Engine.Actor.DrawScale'","Engine.Actor.DrawScale3D'"])]
assert len(declarations)==4
stack={}
for variant in ['stack-frame','source-verified']:
 path=folder/(variant+'.asm');lines=path.read_text(encoding='utf-8',errors='replace').splitlines();rows=[]
 for n,line in enumerate(lines):
  if re.match(r'(main\s+PROC|\?Prior(?:PointAsset|AssetBody|BodyArray|PrimitiveLayout)Main@@YAHXZ PROC|\?point_scale_cases@@YAHXZ PROC)',line):
   rows.append(dict(procedure=line.strip(),prologue=lines[n:n+28]))
 stack[variant]=dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(),prologues=rows)
(folder/'stack-frame-evidence.json').write_text(json.dumps(stack,indent=2)+'\n',encoding='utf-8')
verification=dict(result=result['output'],sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
 original_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in originals},declarations=declarations,
 association='Current mesh Scale/Scale3D exact reflected offsets1e8/1ec; Owner ObjectProperty78 -> Actor exact reference; optional owner DrawScale/DrawScale3D198/19c. Both vectors certify ScriptStruct Vector12 and X/Y/Z FloatProperty offsets0/4/8. Complete identity/field/value proofs close before publication.',
 bridge='Original point_scale operation order, exact Source.receiver_scale equals producer result, same mesh as membership inventory; all raw capture guard boundaries include producer, asset membership, shape proofs and caller typed Source guard. Uniform-normal restriction retained at raw transport.',
 tests='292checks0 includes prior219. All3 raw paths at receiver6; mid-payload component scalar/owner vector/owner pointer/Vector field mutation preserve output. Wrong offsets/name/vector extent, zero/negative/nonfinite reject. Null Owner leaves its denied/NaN scalar unread. Nonuniform measured but raw capture refuses.',
 corrections='Initial test C macro rejected template comma; alias fixed. Nested prior suite call frames exhausted default Windows test stack: confirmed emitted main154296+prior414104+127336+126136+125288 frames; moved new cases into noinline function invoked after prior suites return, no stack-limit increase. Fixture initially overwrote Actor member head; append to existing tail and take fresh fixture binding. Initial draft conflated component with Actor names; original package index proves Scale/Scale3D vs DrawScale/DrawScale3D, corrected and wrong-name regression added before final validation.',
 remaining='Only retained original component point-query producer. Current native code/dispatch attestation, live roles/C0 lifetime, complete raw inventory and active PhysX shape correspondence remain caller obligations. No game run/full Native build/movement permission.',
 game_started=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r212：原组件point-query缩放生产者已接资产/形状读取链（2026-11-12）

新增cover_point_scale_live.inl，以当前mesh认证Scale/Scale3D字段及原offset1e8/1ec，Owner ObjectProperty78及Actor引用；Owner存在时认证其DrawScale/DrawScale3D198/19c，不存在则不读其数值。两个Vector均认证ScriptStruct12及XYZ FloatProperty4/offset0/4/8，完整成员/身份/字段/值入Proof；复用r62 point_scale保持component scalar*axis、owner scalar*axis、owner product*component product的原float32分组。结果须与Source.receiver_scale精确相同且mesh与r211资产相同；所有raw guard共同闭合scale、membership、shape三Proof及caller typed guard。

原collection-static/objects.tsv确认PrimitiveComponent.Scale/Scale3D与Actor.DrawScale/DrawScale3D不同，初版将component也写成DrawScale，最终验证前纠正并增加错名拒绝。原r62完整代码/consumer证据及该包索引SHA保存在point-scale-r212，未以常见字段名替代原包依据。此生产者是保留的组件point-query路径，不能当作active PhysX缩放认证。

292检查0失败（含219）：三类以receiver6完整读取；payload中改component scalar、owner axis、Owner指针或Vector字段均拒绝并保持输出；错误offset/name/Vector extent、零/负/非有限拒绝。Owner缺失即使其scalar为NaN并禁止读取仍成功；非均匀缩放可记录来源值，但raw形状转换仍明确拒绝。

扩展夹具首次模板逗号与C宏冲突修复；随后默认Windows测试栈溢出，原汇编证明嵌套suite main栈帧154296+414104+127336+126136+125288字节叠加，再加内部调用越界。将新cases放独立noinline函数，在原suite返回后调用，未提高栈上限，最终通过。夹具追加Actor字段初版覆盖原WorldInfo/Instigator头链导致guard拒绝，改为尾部追加并在构造完成后重新取fixture binding，未削弱生产guard。原失败/汇编栈帧摘录保留。

尚未接完整实时库存/current C0世界闭合/原code分派认证及active PhysX对应，无游戏/原函数调用/完整Native构建；完整身体查询/事务、视图遮挡及全CV/AC/头显仍待完成，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':292,'failures':0,'documents_equal':doc.read_bytes()==mirror.read_bytes(),'original_declarations':declarations}))
