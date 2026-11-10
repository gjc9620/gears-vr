from pathlib import Path
import datetime,hashlib,importlib.util,json,re,shutil,sys
from inspect_pose_probe import inspect as inspect_pose
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r130'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
root=R/'analysis/instances/cover-1110-r130-cloak-pose-witness';result=load(root/'cover-output/run-result.json')
index=load(root/'launcher-session.json');session=result['terminal_session']
assert result['normal_cleanup_verified'] and not result['remaining_owned'] and result['projection_thread_joined']
assert result['operator_exit']==0 and not result.get('error') and not result['projection_errors'] and result['projection_frames']==308
assert result['cloak_observations']==dict(samples=744,failures=[])
assert session['unified_weapons_cleanup']==dict(fire_disable_code=0,hand_stop_code=5,drained=True)
mute=index['test_audio_guard']['mute_verification'];assert mute['verified_muted_observations']==317 and not mute['verify_failed'] and not mute['target_shared_rejected']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl()
for key in ('game_identity','owner_identity','xr_identity'):assert c.identity(session[key]['pid'])!=session[key]
source_report=Path(index['session_report']);saved=out/'session-original.json'
if saved.exists():assert sha(source_report)==sha(saved)
else:shutil.copyfile(source_report,saved)
spec=importlib.util.spec_from_file_location('cover_query_inspect',H.parent/'source-stage-r104/inspect_probe.py');parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
native=load(H/'native-build-result.json')['dll_sha256'];probe=parser.inspect(load(saved),native);pose=inspect_pose(load(saved))
assert probe['first_rejection_layer']=='typed/physics_roles/layout' and probe['original_started']==probe['original_returned']==0
assert probe['raw']['layout_step']=='4' and probe['raw']['layout_pass']=='1' and probe['raw']['layout_mesh_identities']=='128'
assert probe['raw']['layout_offsets']=='584,0,0,0,0'
assert len(pose['rows'])==2 and not pose['missing']
measure=next(r for r in pose['rows'] if r['name']=='measurement');complete=next(r for r in pose['rows'] if r['name']=='complete')
assert measure['count']==243 and measure['measurement_status']=='core' and complete['count']==1
assert complete['scene']==410 and complete['compose']==25204 and measure['scene']==411 and measure['compose']==25266
(out/'query-probe.json').write_text(json.dumps(probe,indent=2)+'\n');(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
media=load(out/'media-review.json');assert media['encode_exit']==0 and len(media['pages'])==16 and len(media['contacts'])==5
media.update(full_span_personally_reviewed=True,all_contacts_personally_reviewed=True,
    visual_result='FAIL: own head/shoulder/torso obscures both eyes in all five cover phases; intermittent initial hand disappearance retained; first roughly20s static start retained',headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
boundary=load(next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json')))
assert len(boundary['eye_observation'])==5 and all(p['observation']['cover_state_raw']['CoverType']==2 for p in boundary['eye_observation'])
reacquisitions=[s['registry_reacquisition'] for s in boundary['steps'] if s.get('registry_reacquisition')]
record=dict(classification='progress: actual precise membership rejection observed and separate later measurement failures retained',
    game=session['game_identity'],owner=session['owner_identity'],xr=session['xr_identity'],native_sha256=native,
    original_report=str(source_report),original_report_sha256=sha(saved),input_and_phase_sequence=True,
    original_query_started=0,original_query_returned=0,collision_clear=None,movement_authorized=False,
    first_failure='PhysicsRoles mesh_asset_property role membership, pass1; property offset0x248 populated, mesh identity witness count128/128',
    capacity_inference='128 identity capacity is the leading mechanism; exact individual membership rejection is not separately instrumented, so do not declare every schema/root cause resolved',
    separate_measurement=measure,probe_admission=complete,same_C0_across_failures=False,
    observer_sequence_passed=True,registry_reacquisition_branch_observed=bool(reacquisitions),
    normal_cleanup_verified=True,unified_weapons_cleanup=session['unified_weapons_cleanup'],cloak=result['cloak_observations'],mute=mute,
    new_capsule_enumeration_executed=False,remaining=['bounded complete member witness capacity without global Proof/stack growth',
        'later Core measurement rejection','original query and full body path endpoint world coverage C0 commit','head shoulder obstruction','CV01-24 AC01-22 headset'])
(out/'closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
# Preserve the distinct pre-birth disk refusal, not a gameplay or cleanup fail.
old=R/'analysis/instances/cover-1110-r129-cloak-disk-guard';refused=load(old/'cover-output/run-result.json')
assert refused['start_exit']==1 and not (old/'launcher-session.json').exists() and not (old/'isolated-worker.json').exists()
match=re.search(r'RuntimeError: (\{"check": "disk_admission".*?\}); insufficient',refused['start_stderr']);assert match
disk=json.loads(match.group(1));assert not disk['admitted'] and disk['free_bytes']==3880353792
(out/'r129-prebirth-refusal.json').write_text(json.dumps(dict(disk=disk,game_started=False,worker_created=False,
    original_result_sha256=sha(old/'cover-output/run-result.json'),normal_cleanup_required=False),indent=2)+'\n')
archives=[]
for name in ('cover-1110-r112-cloak-source-stage','cover-1110-r113-cloak-source-stage'):
    original=R/'analysis/instances'/name;target=Path('J:/gearsVr/analysis/cover-disk-recovery-20261110-r128/instances')/name
    assert original.is_junction() and original.resolve()==target.resolve()
    receipt=target.with_name(name+'-hashes.json');archives.append(dict(original=str(original),target=str(target),manifest=str(receipt),sha256=sha(receipt)))
(out/'additional-owned-archive.json').write_text(json.dumps(archives,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry=f'''
## r129/r130：磁盘准入真实拒绝；实际PhysicsAsset成员检查达到128身份见证上限（2026-11-10）

上轮e136b287为实际ENOSPC记录和本方空间恢复进展。本轮先复核r112/r113旧实例stopped及精确game/owner/XR/worker全不在，仅将这两组各约914MB归档J盘、全文件前后SHA一致并保留C路径junction。r129使用最新共享来源9f350220…345a准备，但真正资源准入时free3880353792<4294967296，require_disk明确拒绝；无worker/index/game，独立于玩法失败，未改旧票hash重用。自身runner新增claim前disk检查，原共享start/worker仍再查。改动已在当前Git历史c5bc7f54保存，运行r130实际加载的旧runner没有disk_before字段，不追称其已实跑新前置检查。

空间回升后，新票cover-1110-r130-cloak-pose-witness通过准备与冻结adapter真实导入；两实例资源占用时持续核对精确worker等待，资源释放后同一未消费票只启动一次。Native d7cb104d…c2e5、PG/XR冻结版、0.4；game58348/creation134387957377725571，owner58484/creation134387957292749552，XR55864/creation134387957447722033。Source1输入、原版接近/A进入CT2、五段neutral/turned/restored/raised/restored均执行，operator0。未做抓边/虚拟身体位移。

首次Sphere requested/attempted/finished均1，外层source4，Session physics_roles2/detail layout5，Engine idle0，original_started/returned前后均0。LayoutStep4=mesh_asset_property，pass1，mesh sources132/identities128，instance sources2/identities3，offsets584(0x248),0,0,0,0，body尚未采样。代码清零字段后，binding::property成功才写offset，因此已越过该属性自身解析，失败在role_property的完整成员闭合。身份数已达Proof固定128容量，容量耗尽是当前首要机制；本次未记录membership内部究竟哪个条件拒绝，仍不排除其他同时存在的不符项，不直接叫所有根因修复。

对应首次测量complete1的Scene410/Compose25204/sequence4508与Sphere同C0；之后measurement拒绝243次，原Status2=core，首份Scene411/Compose25266/sequence4509，是另一C0。不能拼成一次失败或据此单独断言时限/字段根因。Sphere读括号107011982808600→107011983642300ns，World2186117376/gen1、Pawn16800153856、Link2320974592/slot0，原球(-3930.43384,-190.230759,614.0177,r1)。无原版调用/完整空碰撞/移动许可。

308张实际双眼投影，错误0/线程joined，VFR编码0；16张全跨度overview与5张原始phase图全部亲自查看。五段仍严重自身头肩/躯干遮挡，visualFAIL；开场约20秒静止和间歇手部缺失保留。phase只按事件区间内完整请求/观察括号选取（7/3/7/6/7可选帧），不称同C0/GPU消费。新adapter完整操作序列通过，但registry重建分支本次是否实际触发为{bool(reacquisitions)}，不将正常序列当该特殊分支已证。

本次正常stop，Native/entry/pad恢复true、fire_disable0/hand_stop5/drainedtrue、XR0/launcher0、精确game/owner/XR均不在，无强杀；CLOAK744次失败0，目标静音读回317/initially_muted1/verify_failed0/target_shared_rejected0。r128缺失的恢复回执不因此补签。原报告原样复制并按Native SHA解释，证据pose-probe-r126/runtime-r130/{{query-probe,pose-probe,closeout,media-review}}.json。

下一步为typed完整成员见证提供有界分页/独立存储，维持全部兄弟、身份/字段/声明及closing验证，避免全局Proof扩容引起其他writer/栈尺寸变化；先离线复现跨128成员与末端突变，再编译定向修复。后续Core测量拒绝单独处理；完整当前占用/世界覆盖、原查询、唯一C0身体/视图/命中/枪口、抓边联合战斗和头显仍未完成。工程25%、公共v71与全部CV01–24/AC01–22状态保持，目标active。
'''
assert '## r129/r130：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,normal_cleanup=True,layout_step=4,identity_count=128,measurement_failures=243,registry_reacquisition=bool(reacquisitions))))
