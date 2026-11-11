from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/asset-setup-r211'
result=json.loads((folder/'verified-result.json').read_text())
assert result['build_exit']==result['test_exit']==0
sources=['tools/native-capture/cover_asset_setup_live.inl','tools/native-capture/cover_asset_setup_live_test.cpp',
 'tools/native-capture/cover_body_setup_live.inl','tools/native-capture/cover_body_setup_live_test.cpp']
verification=dict(result=result['output'],sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
 association='Certified current mesh PhysicsAsset0x248/PhysicsAssetInstance0x250, their exact class references; PhysicsAsset.BodySetup ArrayProperty16 at0x68, own inner ObjectProperty(dim1,width8,offset0) -> RB_BodySetup; instance Bodies header/count/order match supplied typed roles; unique authored setup identities and positive BoneName FNames at0xb4 retained in Proof.',
 bridge='Selected Source body_index/setup identity/bone must match membership. BodySetup->array->raw capture guard closes membership Proof and external typed Source guard at each boundary. Caller still closes current roles/C0 lifetime.',
 tests='219 checks0 including earlier135. All3 actual reflection/raw transfer implementations with synthetic memory and initial real roles-reader sample; mutations during payload: setup member, count, mesh asset, instance body member; bad ordinal/layout/target/owner/duplicate/count/offset/bone/template. Full64 authored-member bound on an explicitly constructed typed body snapshot, not a64-member live roles sample.',
 failures_retained='Initial /WX global-name collisions and signed narrowing fixed. First64 fixture used name-entry regions overlapping actor/class memory: failed roles observation and access violation are retained; moved synthetic name entries to a disjoint arena region and made fixture abort on failed observation. No product acceptance claim from failures.',
 remaining='Production capture integration, same current C0/time/world/roles guard, original receiver scale and native code/stride proof, active PhysX shape mapping and full body queries/transaction. Current module adds no movement authority.',
 game_started=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r211：当前物理资产BodySetup成员/顺序与形状读取关联已实现（2026-11-12）

新增cover_asset_setup_live.inl，以现有typed roles Snapshot为caller持有的当前上下文，认证当前mesh的PhysicsAsset0x248/PhysicsAssetInstance0x250指向及class引用、PhysicsAsset.BodySetup ArrayProperty16 offset0x68、own inner ObjectProperty dimension1/width8/offset0和准确RB_BodySetup Class。读取完整1–64 setup成员、无重复、各自继承关系及BoneName FName，instance Bodies header/count/指针顺序同时匹配typed roles；全部值与身份纳入Proof闭合。capture桥接要求Source.body_index、setup身份和骨名对应成员，所有raw读取guard边界都闭合membership Proof及caller typed owner guard。它不替代caller当前roles/C0时间闭合。

合计219离线检查0失败（含原135），三类完整读取使用现有roles reader的初始采样及实际反射/Proof/transport实现，注册表/内存均synthetic。payload中改成员、数量、mesh资产或instance成员均拒绝并保持输出；错误ordinal/重复/count/inner宽度目标owner/offset/骨名及默认模板均拒绝。64成员边界通过，其typed body列表明确为构造值，并非64身体roles实采。顺带修正r210 standalone入口拒绝Default__及非正骨名索引。

首次编译全局同名遮蔽/有符号窄化修正；初版64夹具的name-entry区覆盖既有actor/class，导致roles采样失败及后续访问异常，日志保留。改用互不重叠的夹具区域并使初始采样失败立即中止后，最终219项通过；不能把夹具修复称为游戏修复。

完整实时接入、原receiver scale/consumer stride代码证明、活跃PhysX形状对应和同C0世界闭合、全身体查询/事务仍待完成；无游戏/原函数调用/完整Native构建。源码与回执asset-setup-r211，完整CV/AC/头显仍未完成，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':219,'failures':0,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
