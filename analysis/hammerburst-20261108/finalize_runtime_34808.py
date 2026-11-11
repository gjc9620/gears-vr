from pathlib import Path
import json,hashlib,shutil,math
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-173534'
D=I/'actions/menu-firstperson-34808-173558';T=A/'runtime-34808-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,d):p.write_text(json.dumps(d,indent=2),encoding='utf-8')
p=A/'runtime-34808-four-state-pass.json';v=read(p)
assert v['operator_exit']==0 and v['native_cleanup_verified'] and v['xr_exit_code']==0 and v['playground_cleanup']['passed']
assert not v['cleanup']['forced_termination'] and v['actual_trajectory_count']==0
for source in (D/'hammerburst-twohand.json',I/'hb-source-binding.json',I/'native/capture-status.txt',I/'native/session-status.txt',I/'native/capture.ini'):
    target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
    v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
ini_raw=(T/'capture.ini').read_bytes();ini=ini_raw.decode('utf-16' if ini_raw[:2]==b'\xff\xfe' else 'utf-8-sig')
assert 'diagnostic_first_failure=0' in ini and 'diagnostic_render_pose_trace=1' not in ini
f=read(D/'hammerburst-twohand.json');c=f['checks'];assert len(f['states'])==7 and f['four_state_only']
assert c['source_and_actual_mesh_flow_passed'] and c['neutral_socket_no_weapon_steering_degrees']<.75
for key in ('neutral_wrist_stays_with_gun','neutral_wrist_follows_actual_weapon','release_returns_to_neutral_wrist'):assert c[key]['passed']
assert math.degrees(c['off_hand_changed_actual_rifle_radians'])>3 and c['actual_palm_foregrip_error_raw']<2
assert c['release_rotation_error_degrees']<2
v['foregrip']=f;v['native_candidate']=read(A/'native-passive-rejection-r1/prepared.json')['native_sha256']
rejections=read(A/'passive-rejection-34808.json');commits=read(A/'passive-commit-34808.json')
assert len(rejections['rows'])==64 and commits['records']==294
end=f['states'][-1]['ns'];begin=commits['rows'][0]['current_sync_ns']
action_rejections=[r for r in rejections['rows'] if begin<=r['observed_ns']<=end]
v['original_passive_refusal']=dict(summary=rejections['summary'],action_rejections=action_rejections,
    after_last_snapshot_excluded=len(rejections['rows'])-len(action_rejections),cause_of_previous_game_4312_proved=False)
grip_start=next(phase['ns'] for phase in f['phases'] if phase['label']=='new_Grip_on_actual_foregrip')
neutral_rejections=[r for r in action_rejections if r['observed_ns']<grip_start]
assert len(action_rejections)==6 and len(neutral_rejections)==4 and all(r['target_gate']==1 for r in neutral_rejections)
assert sum(r['target_gate']==7 for r in action_rejections)==1 and not rejections['summary']['socket_gates']
v['same_commit_observation']=dict(path=str(A/'passive-commit-34808.json'),summary=commits['summary'],records=294)
v['original_passive_refusal']['neutral_UP_rejections']=neutral_rejections
v['interpretation']='All requested four-state sampled actual-mesh/wrist checks PASS in one current game, seven snapshots: neutral no-drive8.45e-6deg; passive follow .004185raw/.006659deg under actual1.237318raw/2.285162deg gun movement; Grip steers7.298deg, palm error.004311raw; release returns baseline and snap .0009524raw. Native changes are observation-only, so do not claim causal repair/stability/full fitting. Original passive target refused four neutral-UP requests by existing100ms age (106.524,136.433,103.757,101.118ms), then one by actual Grip DOWN and one age161.185ms after DOWN. Remaining58 bounded refusal rows occur after last snapshot while original request is left to expire, not58 action defects. No socket-frame refusal captured; offline N/N+1 result is NOT live cause. Earlier4312/58780/53872 failures retained.'
v['next_bounded_source_question']='Implement a fresh-input passive socket path through the existing sole C0 solver: first inspect current BodyMount/hand geometry contract to distinguish reusable certified socket geometry from per-frame requested world pose. Do not extend old Request100ms or replay expired world target; current identity/held attachment/input/contact/no-incumbent checks must certify any newly assembled target. Add regression for valid current input after old world request expiry, real Grip transition and release/cancel/loss. Do not infer all whole-C0 losses fixed or change1cm/2deg thresholds. No unchanged game run.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 paired overview pages reviewed. Near-eye dominant rifle, flat or separated left hand and transient ghost/white geometry persist. Grip/release visible sequence is reached; pixel contact/all-angle fitting unaccepted. Pause card102.09-105.36s and world107.01s retained as images, not full interruption acceptance.')
write(p,v)
p=A/'runtime-34808-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13
a.update(latest_runtime_result='runtime-34808-four-state-pass.json',latest_foregrip_sampled_pass='runtime-34808-four-state-pass.json',requirements_complete=False,current_candidate='PG325200c3/Native4691fdf2; narrow refusal observer only/wide OFF.34808 seven-snapshot four-state PASS once. Four original>100ms passive request rejections still captured during action, one actual DOWN; prior intermittent failures preserved. No causal repair or full fit acceptance. Full empty reload20924 PASS preserved; all13 retained.')
for requirement in a['requirements']:
    if requirement['id']=='foregrip_states':
        requirement.update(status='current_seven_snapshot_four_state_pass_once_prior_intermittent_failure_and_expired_requests_open',limit='34808 current no-drive/follow/Grip steering/release checks pass. Observation-only Native delta cannot certify stability repair; four original100ms target expiries captured and prior4312/58780/53872 failures remain. Visual/all-angle fit incomplete.')
        requirement.setdefault('evidence',[]).append(dict(path=str(A/'runtime-34808-four-state-pass.json'),sha256=hashlib.sha256((A/'runtime-34808-four-state-pass.json').read_bytes()).hexdigest()))
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=34808,runner_last_poll='65662 terminal0; stop/launcher/Native/PG/XR0; noforced/owned game+children absent; encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as stream:
    stream.write('\n\n## 2026-11-11 实际34808：当前七快照四态通过一次，原请求过期仍存在\n\nPG325200c3/Native4691fdf2，仅窄拒绝记录，宽render witness OFF。无Grip不驱动枪8.45e-6deg；右手带枪1.237318raw/2.285162deg时腕相对枪.004185raw/.006659deg；Grip后左手带枪7.298deg、palm误差.004311raw；松手回单手baseline并回snap.0009524raw，七快照原阈值全部PASS。Native只改观察，不能称稳定性修复；实际动作窗捕获4次原100ms target expiry，年龄106.524/136.433/103.757/101.118ms，另1次真Grip DOWN及随后161.185ms旧请求过期。其余58拒绝在末快照后旧请求自然过期，不能混作58动作缺陷。没有捕获socket帧不一致拒绝；离线新旧帧结果未成为live原因证据，4312/58780/53872失败保留。下一步将可重用认证握点几何与逐帧world目标分开，调查现有BodyMount/当前输入接口并在唯一C0求解点用当下认证输入组装新目标，绝不延长旧请求100ms或重放过期world姿势；Grip/part竞争/输入身份/新鲜度/固定骨长保护和原阈值保留。正常清理全0无forced/前台违规，703帧错误空/编码0、3页48对亲阅，近眼枪体/张平分离左手/过渡白几何仍未通过贴合。102.09-105.36s暂停卡和107.01s世界图像保留，不扩称全中断通过。20924完整空仓换弹/回枪六发与全13项继续。\n')
print(json.dumps(dict(terminal=True,sampled_four_state_pass=True,stability_fixed=False,action_expiries=4,whole_complete=False)))

