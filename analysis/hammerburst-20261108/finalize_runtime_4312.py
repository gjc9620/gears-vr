from pathlib import Path
import json,hashlib,shutil,csv,collections
A=Path(__file__).resolve().parent;R=A.parents[1]
I=R/'analysis/instances/hb-cloak-1111-172045';D=I/'actions/menu-firstperson-4312-172130';T=A/'runtime-4312-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,d):p.write_text(json.dumps(d,indent=2),encoding='utf-8')
p=A/'runtime-4312-passive-palette.json';v=read(p)
assert not v['cleanup']['forced_termination'] and v['native_cleanup_verified'] and v['xr_exit_code']==0
assert v['playground_cleanup']['passed'] and v['actual_trajectory_count']==0
for source in (D/'hammerburst-twohand.json',I/'hb-source-binding.json',I/'native/capture-status.txt',I/'native/session-status.txt',I/'native/capture.ini',I/'native/head-palette-observations.csv',I/'native/body-hands-commits.bin'):
    target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest()
    assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
    v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
raw_ini=(T/'capture.ini').read_bytes();ini=raw_ini.decode('utf-16' if raw_ini[:2]==b'\xff\xfe' else 'utf-8-sig')
assert 'diagnostic_render_pose_trace=1' in ini and 'diagnostic_first_failure=1' in ini
assert 'record_base_entry=1' not in ini
v['foregrip']=read(D/'hammerburst-twohand.json');checks=v['foregrip']['checks']
assert checks['neutral_socket_no_weapon_steering_degrees']<.75 and not checks['neutral_wrist_stays_with_gun']['passed']
assert len(v['foregrip']['states'])==2
commits=read(A/'passive-commit-4312.json');palette=read(A/'passive-palette-4312.json')
assert commits['records']==119 and palette['summary']['left_matched_last_compose']==119
assert palette['summary']['full_weight']['palette_C0_solved_distance']['max']<.00005
v['C0_and_render_observation']=dict(commits=str(A/'passive-commit-4312.json'),palette=str(A/'passive-palette-4312.json'),summary=palette['summary'],strict_queued_payload_origin_proved=False,hardware_or_every_draw_proved=False)
lo=commits['rows'][0]['current_sync_ns'];hi=commits['rows'][-1]['current_sync_ns']
window=[r for r in csv.DictReader((T/'head-palette-observations.csv').open()) if r['bone']=='11' and lo<=int(r['at_ns'])<=hi]
v['passive_window_render_stages']=dict(rows=len(window),counts={k:dict(collections.Counter(r[k] for r in window)) for k in ('native_begin_result','native_apply_gate','native_commit_result','native_consumer_result','native_finish_result','native_hand_status','native_solved_mask','composed','render_observation_reason')})
left=[r for r in palette['rows'] if r['bone']==11];near=[]
for state in v['foregrip']['states']:
    stamp=state['body']['sample_qpc_ns'][-1];sample=state['body']['samples'][-1]['11']['world_xyz']
    nearest=sorted(left,key=lambda r:abs(r['at_ns']-stamp))[:3]
    near.append(dict(label=state['label'],sample_ns=stamp,async_SpaceBases_world=sample,
        nearby_render=[dict(dt_ms=(r['at_ns']-stamp)/1e6,native_compose=r['native_compose'],native_commit_result=r['native_commit_result'],passive_weight=r.get('passive_weight'),palette_world=r['palette_world'][3][:3],C0_distance=r.get('palette_C0_solved_distance')) for r in nearest]))
v['failed_sample_render_neighbours']=near
v['interpretation']='No-drive PASS; neutral-left-move wrist-in-gun2.210612raw FAIL.119 latest-compose-correlated actual queued CPU palette rows agree with C0 world wrist within .00004312raw;112 full weight. Thus successful passive target reaches observed render consumer. At failing SpaceBases sample, nearby actual palette(-38.211ms) matches the displaced2.21raw wrist; next(+43.6221ms) returns to original snap wrist. Intermittent pose withdrawal is real at the observed CPU consumer, not merely a stale external reader. New diagnostic mode may affect timing; do not claim exact withdrawal cause, hardware/everyDraw or final fit. Right-move/Grab/release not reached.'
v['next_bounded_source_question']='Identify original passive target refusal/absence at the matching successful non-passive/release-blend C0 commit and separate whole C0 non-admission. Existing active-support first-rejection observer excludes passive. Add bounded original-return reasons for passive target, including exact broker source/frozen frame association from the existing admission call; no extra game/clock reads or relaxed admission. Preserve anti-replay, identity, freshness, fixed lengths and1cm/2deg tests. Do not rerun wide palette observer unchanged.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 paired overview pages reviewed. Near-eye rifle, flat/separated left hand, intermittent original/ghost/white geometry remain.393 frames, no capture errors; sparse54.33/61.32/69.74s samples retained, not called smooth motion. Full fitting unaccepted.')
write(p,v)
p=A/'runtime-4312-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13
a.update(latest_runtime_result='runtime-4312-passive-palette.json',latest_runtime_failure='runtime-4312-passive-palette.json',requirements_complete=False,current_candidate='PG325200c3/Nativee4e3155d; wide existing render witness ON this run only.4312 no-drive PASS/neutral snap2.210612raw FAIL.119 actual queued CPU palette rows match successful C0 wrist; intermittent displaced/original pose also observed. Withdrawal reason unproved, right-follow/Grab/release not reached. Full ordinary empty reload20924 PASS preserved; all13 retained.')
for requirement in a['requirements']:
    if requirement['id']=='foregrip_states':
        requirement.update(status='no_drive_passed_intermittent_passive_pose_withdrawal_seen_at_CPU_render_consumer',limit='4312 actual queued CPU consumer shows both correct C0 snap and displaced/original poses. Exact refusal cause unknown; do not dismiss test as async sampling only. Current Grab/release and actual surface fitting incomplete.')
        requirement.setdefault('evidence',[]).append(dict(path=str(A/'runtime-4312-passive-palette.json'),sha256=hashlib.sha256((A/'runtime-4312-passive-palette.json').read_bytes()).hexdigest()))
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=4312,runner_last_poll='23356 terminal1; normal stop/launcher/Native/PG/XR0; noforced/owned game+children absent; encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:
    f.write('\n\n## 2026-11-11 实际4312：成功C0姿势到达显示消费者，但被动姿势间歇撤回\n\n完全相同PG325200c3/Nativee4e3155d，本次仅既有render-pose/first-failure witness ON，无base-entry observer。未按Grip不驱动枪通过，左手移动后腕相对枪2.210612raw失败，右手跟随/Grab/释放未到。119份latest-compose相关CPU queued palette的左腕与成功C0最终腕最大差.00004312raw，112份weight1，证明这些成功姿势确实到观察到的显示消费者；不称确切queued来源/硬件/每Draw。失败SpaceBases采样前38.211ms的实际palette也处于偏移位置，后43.6221ms回到正确snap；因此真实间歇撤回仍失败，不能将测试改判为异步误报。窗口163显示行中28行solved_mask0，另有成功C0未使用passive目标/释放混合；具体拒绝原因未证，diagnostic模式可影响时序。下一步只记录原passive target拒绝及原broker source/frozen frame关联，不额外读取/时钟/放宽规则，不重复宽观察。正常清理全0无forced/前台违规，393帧错误空/编码0，3页48对亲阅，近眼大枪/张平分离左手/过渡原动画白几何仍不合格，稀疏采样54.33/61.32/69.74s原样保留。20924完整空仓换弹/回枪六发通过与全部13项保留。\n')
print(json.dumps(dict(terminal=True,correct_C0_reaches_CPU_consumer=True,intermittent_withdrawal=True,exact_cause_proved=False,whole_complete=False)))
