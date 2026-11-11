from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/body-setup-r210'
result=json.loads((folder/'verified-result.json').read_text())
assert result['build_exit']==result['test_exit']==0
verification=dict(result=result['output'],sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in [
 'tools/native-capture/cover_body_setup_live.inl','tools/native-capture/cover_body_setup_live_test.cpp','tools/native-capture/cover_array_layout_live_test.cpp']},
 association='Current certified RB_BodySetup class path; inherited KMeshProps AggGeom StructProperty72 -> complete ArrayLayout; exact BoneName NameProperty8 at original consumer offset0xb4; expected FName index and number, instance identity and values retained in Proof.',
 bridge='Aggregate base computed from reflected field offset with overflow/disjoint checks; all array guard boundaries include full Proof and external typed owner guard. No hardcoded aggregate offset.',
 tests='All3 kinds, reflected aggregate relocated0x60->0xc0, bone/index-number mismatch, wrong offset/target/inheritance/overlap/tail cycle, mid-payload BoneName/target/instance-identity mutation and denied outer guard; output preserved.',
 first_compile_failure='Additional fixture lambda shadowed existing global denied under /WX; renamed reject_owner. Original log retained.',
 remaining='Synthetic registry and instances only. PhysicsAsset membership/body ordinal, original receiver scale, native consumer/stride attestation, current C0/time/world and active PhysX correspondence remain external obligations. No movement permission.',
 game_started=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r210：BodySetup的AggGeom实例与骨名关联已接读取链（2026-11-12）

新增cover_body_setup_live.inl：当前setup身份及RB_BodySetup继承路径、KMeshProps声明的AggGeom StructProperty72与完整r209布局、BoneName NameProperty8及原consumer offset0xb4共同保留在同一Proof。骨名同时匹配FName index和number；aggregate实例地址以反射offset计算，检查溢出和与BoneName重叠，不固化夹具0x60。capture桥接把该实例、骨名及外层typed owner guard接回r209/r206，所有读取边界仍闭合完整Proof，失败保持公共输出。

合计135检查0失败（含原80）：三类读取、aggregate字段移到0xc0仍成功，错误骨名/number、offset/target/继承/重叠/尾链拒绝；payload中改骨名、AggGeom目标或setup自身身份及外层guard拒绝均不发布。补充夹具首次因lambda遮蔽既有全局denied触发/WX，改名后通过，失败日志保留。全部synthetic注册表/实例，无真实进程读取、游戏/原函数/完整Native构建。

PhysicsAsset成员与body ordinal、原receiver scale、原consumer/stride代码认证、当前C0身份时间/世界及活跃PhysX对应仍需接入；本关联不能授予移动。源码/依赖/回执body-setup-r210，完整CV/AC/头显仍未完成，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':135,'failures':0,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
