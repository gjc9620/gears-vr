from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-parameters-r215'
result=json.loads((folder/'verified-result.json').read_text());assert result['build_exit']==result['test_exit']==0
bundle_path=folder/'rea-evidence.json';bundle=json.loads(bundle_path.read_text());records=bundle['records']
certificate=json.loads((folder/'getter-certificate.json').read_text());artifact=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
digest=hashlib.sha256(artifact.read_bytes()).hexdigest();assert digest==certificate['artifact_sha256']
assert len(records)==45 and all(r['subject']['digest']['sha256']==digest for r in records)
def fact(procedure):
 record=next(r for r in records if r['operation']=='analyze_function' and r['parameters']['procedure']==procedure)
 return dict(evidence_id=record['evidence_id'],entry=record['normalized_result']['procedure']['address'],pseudocode=record['normalized_result']['pseudocode'],body=record['normalized_result']['procedure']['body'])
facts={p:fact(p) for p in ['0x18000c410','0x180012770','0x180012760','0x180012780','0x18000c460','0x1800b1350','0x1800b1370','0x1800bd210','0x1800bda8a']}
(folder/'focused-facts.json').write_text(json.dumps(facts,indent=2)+'\n',encoding='utf-8')
sources=['tools/native-capture/cover_physx_shape_parameters.h','tools/native-capture/cover_physx_shape_parameter_code.h','tools/native-capture/cover_physx_shape_parameters_test.cpp']
verification=dict(result=result['output'],sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},
 artifact_sha256=digest,rea_records=len(records),rea_bundle_sha256=hashlib.sha256(bundle_path.read_bytes()).hexdigest(),
 observations='Public box/capsule/sphere tables and exact internal tables226c80/2272d0/2303e8; sphere table is supported by original SphereShape constructor/destructor assignments and radius getter references. Current kind and full owner links are re-read. Stored position88/8c/90, quaternion78..84, flags180, box vector1b8..1c0, radius1b0 and capsule half1b4 read via original getter-backed fields.',
 code='Ten complete original getter/stub bodies210 bytes plus existing actor/shape getter byte checks and actual dispatch slots close before/after parameter reads. No original function dispatched.',
 interpretation='Internal box vector retained as raw box_dimensions, no unit/half-extents semantics claimed. Capsule public length getter doubles1b4; global capsule getter uses it on the cached axis to form endpoint offsets but can update cached pose. Stored quaternion/position are not universally a world pose. Raw flags not interpreted as eligibility or authored bNoRBCollision.',
 tests='687 checks0 includes519 current ownership checks. Original exact PE loaded only as fixture data, synthetic object memory, real production ownership reader as typed guard for all3 kinds.99 parameter-only read points each fail-injected with output preservation; code/slot/kind/value/owner/link mismatches, inter-pass mutation, all3 guards, final attribution mutation, reentry/alias, static null dynamic Body, exception and unsupported convex covered. DLL remains unloaded.',
 first_failure='Added constant-throw guard fixture made compiler prove following template code unreachable under /WX; changed fault injection to a volatile runtime condition. Product logic unchanged; failure retained.',
 remaining='Not yet joined to authored body/shape inventory, current actor-world pose and units/conventions, active query participation and full scene/world fencing. No production Native capture integration, game/XR, actual original query, stereo/headset acceptance or full cover completion.',
 native_session_closed=True,game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r215：原PhysX三类形状stored参数getter取证与只读观察器（2026-11-12）

原PhysXCore.dll SHA22051aa7保持，REA Ghidra新静态证据45记录完整导出并关闭本次数据库；未执行原DLL。读取三public table及box/capsule internal table完整相应前缀，由radius getter数据引用定位2303e8，再沿两个代码引用证明原SphereShape constructor/destructor确实赋该表，未以排列猜sphere表。原getter证明box vector在internal+1b8/1bc/1c0，radius在1b0、胶囊长度getter12760返回1b4+1b4；flags在180，stored位置88/8c/90、四元数78..84。胶囊12780以1b4形成轴两侧端点，但含缓存pose更新写入，本实现不调用它。

新增cover_physx_shape_parameters.h与自动生成cover_physx_shape_parameter_code.h，依托原owner观察和typed Guard(Observation)认证当前kind/member/Scene/C0/image归属，准确table和getter槽、10个完整原getter/stub共210bytes及原Actor/shape getter bytes前后匹配后，两个完整参数观察比较，最终再闭合归属/代码/guard才发布。无需任何原函数调用/缓存更新，失败保持输出、释放exclusive workspace；未知类型/convex明确拒绝。保留box_dimensions原向量、radius/half_segment及flags，未声称box单位/半轴语义、世界位姿、活动查询资格或bNoRBCollision等价关系。

687检查0失败，包含当前原owner519项；实际PE仅作为数据夹具，objects为synthetic。三类使用真正生产owner reader作为typed guard；99个parameter-only读取点逐项故障注入，slot/code/kind/value/owner/link、两轮间变化、三guard拒绝、最终输入attribution变化、重入/alias、静态null dynamic Body、异常和convex均保护输出；DLL保持未加载。异常负例初版constant throw令模板后续代码被/WX判不可达，改为volatile运行时故障条件后通过，未据此改产品逻辑，失败保留。

完整证据/范围/原指令在shape-parameters-r215/rea-evidence.json及focused-facts/getter-certificate/verification。仍缺authored库存逐项对应、当前Actor world pose及单位/轴基转换、原参与/过滤语义及全场景world/path/occupancy闭合，尚未生产Native接入/完整构建/游戏/XR/原查询/头显。完整CV24/AC22、视图遮挡和body/view/hit/muzzle同版事务继续待完成，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':687,'failures':0,'original_artifact_sha256':digest,'rea_records':45,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
