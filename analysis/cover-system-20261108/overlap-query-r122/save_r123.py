from pathlib import Path
import json,hashlib,datetime,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r123'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf8'))
root=R/'analysis/instances/cover-1110-r123-cloak-source-stage'
result=load(root/'cover-output/run-result.json');session=result['terminal_session'];index=result['terminal_state']
assert result['normal_cleanup_verified'] and result['operator_exit']==0 and not result['remaining_owned']
assert result['projection_frames']==235 and not result['projection_errors'] and result['projection_thread_joined']
assert result['cloak_observations']['samples']==585 and not result['cloak_observations']['failures']
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
mute=index['test_audio_guard']['mute_verification'];assert mute['verified_muted_observations']==241 and not mute['verify_failed'] and not mute['target_shared_rejected']
probe=load(out/'probe.json');assert probe['first_rejection_layer']=='typed/physics_roles/layout' and probe['original_started']==probe['original_returned']==0 and probe['collision_clear'] is None
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl()
for key in ('game_identity','owner_identity','xr_identity'):assert control.identity(session[key]['pid'])!=session[key]
source_report=Path(index['session_report']);saved=out/'session-original.json';assert not saved.exists();shutil.copyfile(source_report,saved)
media=load(out/'media-review.json');assert media['encode_exit']==0 and len(media['pages'])==12 and len(media['contacts'])==5
media.update(full_span_personally_reviewed=True,all_contacts_personally_reviewed=True,visual_result='FAIL: self head/shoulder/torso obstruction in cover persists in all five phase contacts; initial hand gaps also visible',headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
record=dict(classification='progress',game=session['game_identity'],xr=session['xr_identity'],native_sha256='34c15590a10a1fa9f2f380ed0b2285afbd3d5889939e1c30f3f46542076dd1e9',
    original_report=str(source_report),original_report_sha256=sha(saved),run_result_sha256=sha(root/'cover-output/run-result.json'),
    input_and_phase_sequence=True,query_success=False,first_failure='physics_roles/layout before QueryEngine dispatch',
    original_query_started=0,original_query_returned=0,collision_clear=None,movement_authorized=False,
    specific_layout_field='unknown: schema/owner class/array root/body layout/declared membership still need bounded witness',
    new_capsule_enumeration_executed=False,normal_cleanup_verified=True,unified_weapons_cleanup=session['unified_weapons_cleanup'],
    cloak=result['cloak_observations'],mute_verification=mute,media_review_sha256=sha(out/'media-review.json'),
    remaining=['specific physics layout rejection','actual original query execution','head/shoulder obstruction','endpoint enumeration/path/body C0 integration','CV01-24 AC01-22 and headset'])
(out/'closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r123：实际掩体闭环恢复，source4定位到typed布局层（2026-11-10）

在保存r122后，用当前CLOAK／静音入口重新prepare本方独立实例 cover-1110-r123-cloak-source-stage。Native34c15590…d1e9、PGf8e6f3c2…807、XR16840b27…a7fc，0.4。game19480／creation134387893621457706，owner50296／creation134387893545464491，XR55936／creation134387893666015908。实际Source1读取通过，原版接近与CT2进入、五段中立／固定位置转头／回正／实际追踪上移0.10m／恢复全部执行，operator0；不是物理头显验收，也未做抓边／虚拟身体移动。

本次一厘米原始Sphere诊断 requested/attempted/finished均1，外层code4。outer returned6、session_entered1、Session physics_roles2／detail layout5、Engine idle0；original_started与returned前后均0。解释器按实际indexed session路径及本次Native SHA核对，first_rejection_layer=typed/physics_roles/layout；本次诊断尚未进入原版查询，raw_hit0仍是未知而非无障碍。Scene553／Compose33395／sequence2990／thread49740；读括号100616283951500→100616284724400ns。World2574680320/gen1、Pawn95491588352、Link2205697776/slot0，原始球(-3917.90698,-188.254379,621.074341,r1)保持原观察范围，米／单位几何尚非完整认证。

这是本次运行的具体失败层，不追溯冒称r102也是同一内部原因。typed sample已越过请求／初始对象读取，但layout仍包含schema、Owner类型、数组／root及body布局／成员检查，哪个字段拒绝尚不明；不靠放宽layout、选择任意Actor或把pending当空来修。下一步在现有读取路径记录有界布局见证并修实际不符项，原版调用、完整终点、世界覆盖和身体提交仍缺证。

235张实际双眼投影，无录制错误、thread joined；12张全跨度overview和5张原始阶段contact已全部亲自查看，VFR编码exit0。五个阶段均有自身头肩／躯干遮挡，视觉FAIL保持；前段也存在手部不显示帧，保留而不归因为新查询。阶段图只按事件墙钟区间选择（每段7张完整请求／观察落在区间内），不是nearest或同C0／GPU消费证明，不以数字读回替代画面。

正常stop与launcher0、Native恢复／入口恢复／手柄恢复true，fire_disable0／hand_stop5／drainedtrue，XR0，本方精确game／owner／XR均已不存在、未强杀，normal_cleanup_verified=true。此次正常关闭不改写早停实例的fire_disable2失败，也不称所有退出路径已修。CLOAK观察585次失败0；目标会话静音回读241次、initially_muted1、verify_failed0、target_shared_rejected0，guard正常结束，未动其他应用音量。

证据 overlap-query-r122/runtime-r123/{probe,closeout,media-review}.json 及 session-original.json 原报告副本，原报告SHA另存closeout。新Capsule枚举接口本次没有调用，仍只是编译／离线通过。公共v71不变，工程25%与全部CV-01–24／AC-01–22、头肩遮挡和物理头显未完成状态保持；无新增自动重跑请求，下一次必须是具体布局见证或修复的新候选。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup=True,query_success=False,first_layer=probe['first_rejection_layer'])))
