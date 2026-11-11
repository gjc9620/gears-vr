from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-153841';D=I/'actions/menu-firstperson-52000-153900';T=A/'runtime-52000-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
p=A/'runtime-52000-first-failure-analysis.json';v=read(p)
assert v['actual_trajectory_count']==36 and v['trajectory_consistent'] and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json')
raw=(T/'playground-52000.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
for prefix in ('hammerburst_input_consumed','hammerburst_source_loss','hammerburst_history_rejection','hammerburst_tick_cost_summary','hammerburst_first_tick_summary','hammerburst_tick_cost'):
 v[prefix+'_rows']=rows(prefix,pg)
inputs=v['hammerburst_input_consumed_rows'];selected=[next(r for r in inputs if r['frame']==str(f)) for f in (345,355,361,386)]
assert [(r['status'],r['owned']) for r in selected]==[('1','0'),('1','0'),('2','0'),('8','0')]
loss=next(r for r in v['hammerburst_source_loss_rows'] if r.get('withdrawal_frame')=='347')
assert loss['withdrawal_age_ns']=='100134800' and loss['withdrawal_failure']=='5'
assert len(v['empty']['drain_loaded_only']['burst_observations'])==5
cost=v['hammerburst_tick_cost_rows'];assert len(cost)==128
for r in cost:assert int(r['total_ns'])==sum(int(r[k]) for k in ('pre_original_ns','original_ns','service_ns','equipment_ns','follow_ns'))
v['failure_transition']=dict(up_baseline=selected[0],first_withdrawal=loss,recovered_held_baseline=selected[1],later_invalid=selected[2],blocked_held=selected[3],down_attempt=v['empty']['drain_loaded_only']['confirmed_inputs'][-1],nearby_cost=[r for r in cost if 196<=int(r['ordinal'])<=206])
v['interpretation']='Core6 and five drain6 cohorts produced36 consistent rays; loaded42 remains. New press7/release6 never owned, no shots. Playground client withdrawal347/seq3550 stale_before100.1348ms, PG no-current seals Input; fresh355/seq3557 already held becomes BASELINE1, later386 BLOCKED8. This is anti-replay refusal after source loss, distinct from29012 missed owned2 receipt. No unchanged rerun or expiry relaxation. Raw history observer and protected first-debt not reached.'
v['next_bounded_source_question']='Use preserved exact source to locate frame347 stale-before calculation/publication and quantify producer-to-Native versus Native-to-PG delay. Existing tail measures GT phases but does not contain every source timestamp; preserve unknowns. Change only a demonstrated scheduling/read cost, never reuse held DOWN after reset or renew source timestamps.'
raw=(T/'capture-status.txt').read_bytes();native=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace');v['native_age_summary']=rows('stereo_hand_loss_summary',native);v['native_input_loss']=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native))
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 pages personally reviewed. Oversized near-eye gun and separated/ghost hands/white geometry remain; no full contact/headset acceptance.');write(p,v)
p=A/'runtime-52000-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-52000-first-failure-analysis.json',latest_runtime_result='runtime-52000-first-failure-analysis.json',requirements_complete=False,current_candidate='PGaf4dac2b/Native71f280b0, helper16 checks.52000 actual36 consistent rays/core6+five drain6; next DOWN after stale-before347/3550 at100.1348ms never owned, held recovery BASELINE then BLOCKED; no empty/reload reached. Distinct from29012 missed owned receipt. All13 remain.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=52000,runner_last_poll='57756 terminal1; stop/launcher/Native/PG/XR normal0; owned game+children absent; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际52000：读取修正后遇到真实源撤回\n\n原candidate PGaf4dac2b/Native71f280b0不变，测试修正fde58046。core6与五组drain6实际36条一致射线、剩42；下一次p7/r6没有持有或射线。源347/seq3550 stale_before100.1348ms撤回，PG无当前Frame关闭输入；恢复355/3557扳机已按下，BASELINE1未持有，后386 BLOCKED8。符合原防重放，不是29012漏认owned2，也不允许补按重试。tail292/128分段和核对，firstseen0，历史观察未到达；Native inputloss0/Gate76均age-only保留逻辑登记，不等于允许输入。下一步只定位原publication stale-before时刻及源到Native/Native到PG延迟，现有GT耗时不证明全链原因，不改期限/续时/旧按下。正常清理全0、无forced，211帧错误空/编码0，3页48对亲阅，视觉仍失败；13项完整目标保留。\n')
print(json.dumps(dict(actual_rays=36,finite_cohorts=6,loaded_remaining=42,full_pass=False,terminal=True)))
