from pathlib import Path
import json,hashlib,shutil,math
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-180416'
D=I/'actions/menu-firstperson-58352-180444';T=A/'runtime-58352-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
p=A/'runtime-58352-fresh-four-state.json';v=read(p)
assert v['operator_exit']==0 and v['native_cleanup_verified'] and v['xr_exit_code']==0 and v['playground_cleanup']['passed']
assert not v['cleanup']['forced_termination'] and v['actual_trajectory_count']==0
for source in (D/'hammerburst-twohand.json',I/'hb-source-binding.json',I/'native/capture-status.txt',I/'native/session-status.txt',I/'native/capture.ini'):
    target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
    v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
f=read(D/'hammerburst-twohand.json');c=f['checks'];assert len(f['states'])==7 and f['four_state_only']
assert c['source_and_actual_mesh_flow_passed'] and c['neutral_socket_no_weapon_steering_degrees']<.75
for key in ('neutral_wrist_stays_with_gun','neutral_wrist_follows_actual_weapon','release_returns_to_neutral_wrist'):assert c[key]['passed']
assert math.degrees(c['off_hand_changed_actual_rifle_radians'])>3 and c['actual_palm_foregrip_error_raw']<2
assert c['release_rotation_error_degrees']<2
v['foregrip']=f;v['native_candidate']=read(A/'fresh-passive-native-r1/prepared.json')['native_sha256']
commits=read(A/'passive-commit-58352.json');refusals=read(A/'passive-rejection-58352.json')
assert commits['records']==275 and commits['fresh_query_records']==275
applied=[r for r in commits['rows'] if r['applied']];assert len(applied)==273
matches=[]
for refusal in refusals['rows']:
    if refusal['target_gate']!=1:continue
    for row in applied:
        if row['fresh_query'] and row['current_sequence']==refusal['current_sequence']:
            age=(refusal['observed_ns']-row['current_sync_ns'])/1e6
            if 0<=age<=40:matches.append(dict(current_sequence=row['current_sequence'],fresh_query=row['fresh_query'],compose=row['compose'],old_world_request_age_ms=refusal['original_request_age_ms'],fresh_current_input_age_at_old_refusal_ms=age,weight=row['weight']))
assert matches
v['fresh_path']=dict(records=275,applied=273,blend_weight_zero_records=[r for r in commits['rows'] if not r['applied']],
    source=str(A/'passive-commit-58352.json'),original_world_rejections=str(A/'passive-rejection-58352.json'),
    same_current_sequence_old_expiry_and_fresh_applied=matches,
    limits='Original refusal row lacks Compose id; correlation is exact current sequence and clock bound, not a fabricated same-Compose assertion. Two actual successful C0 records have weight0/appliedfalse; do not call all visual gaps fixed.')
v['interpretation']='Fresh paired PG5fde320a/Nativef0635832 completed the original seven-snapshot four-state checks.275 recorded passive commits carry real fresh BodyMount query ids;273 applied and2 blend-weight0. Multiple original old-world request expiries coexist with applied fresh-query goals on exact same current input sequence within40ms. Old Request100ms gate unchanged; no expired world pose replay. This exercises the new branch in game but does not certify all whole-C0 losses, continuous skin stability or surface fitting. Parent20924 full empty reload pass is historical for this newly rebuilt pair; next run must check full flow.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 paired pages personally reviewed.703 frames/error-free encode0. Near-eye dominant rifle, flat/separated hand, intermittent original/ghost/white geometry remain; all-angle surface fit unaccepted. Pause-card111.44-114.74s/world116.42s are retained images, not full interruption acceptance.')
v['next']='One full finite empty/reload/return-fire check on this exact new PG/Native ABI6 pair; no unchanged four-state rerun. Keep two weight0/whole-C0/fit issues and all13 requirements open.'
write(p,v)
p=A/'runtime-58352-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13
a.update(latest_runtime_result='runtime-58352-fresh-four-state.json',latest_foregrip_sampled_pass='runtime-58352-fresh-four-state.json',requirements_complete=False,current_candidate='PG5fde320a/Nativef0635832 fresh-passive ABI6.58352 seven-snapshot four-state PASS;275 actual fresh query commits/273 applied/2 weight0. Same-current-sequence fresh applied target while original old world request expired observed. No whole-C0/stability/fit acceptance.20924 full empty reload is preserved parent evidence, not runtime proof for new pair; all13 retained.')
for requirement in a['requirements']:
    if requirement['id']=='foregrip_states':
        requirement.update(status='fresh_query_path_exercised_275_current_four_state_pass_273_applied_two_weight0_stability_fit_open',limit='New pair58352 all four-state sampled checks pass, new branch exercised. Old expired world request remains rejected; fresh query prepares new relative target. Two weight0 and whole-C0/visible fitting issues remain. Prior failures retained.')
        requirement.setdefault('evidence',[]).append(dict(path=str(A/'runtime-58352-fresh-four-state.json'),sha256=hashlib.sha256((A/'runtime-58352-fresh-four-state.json').read_bytes()).hexdigest()))
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=58352,runner_last_poll='21173 terminal0; normal stop/launcher/Native/PG/XR0; noforced/owned game+children absent; encode0',next=v['next']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as stream:
    stream.write('\n\n## 2026-11-11 实际58352：新鲜查询吸附路径实际执行，四态七快照通过\n\nPG5fde320a/Nativef0635832已在game执行新路径：275条passive成功C0记录全部带真实fresh BodyMount query id，273 applied/2 weight0。原七快照四态全部通过：无Grip不驱动枪1.71e-6deg；枪实际移动1.237321raw/2.285162deg时腕相对枪.004226raw/.006668deg；Grip左手带枪7.298deg、palm误差.004179raw；松手回单手与snap.0007515raw。多条旧world请求过100ms的原拒绝，与相同当前input sequence、40ms内的新鲜query已应用目标同时存在；拒绝记录无Compose id，不伪称逐Compose一一对应。旧world有效期逐字节未改，当前目标由新query输入/几何重新组装。两条weight0、整段C0不准入和视觉贴合仍未解决，不能称全稳定修复。清理Native/PG/XR/stop/launcher全0、无forced/前台违规，703帧错误空/编码0、3页48对亲阅，近眼大枪/张平分离手/原动画重影白几何仍不合格，111.44-114.74s暂停卡与116.42s世界图像保留。20924完整空仓换弹属于父候选证据，新ABI6 pair下一次需完整空仓/换弹/回枪开火验证；所有13项继续。\n')
print(json.dumps(dict(four_state_pass=True,fresh_query_records=275,applied=273,expiry_replacement_sequence_matches=len(matches),whole_complete=False)))
