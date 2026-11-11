from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-161014';D=I/'actions/menu-firstperson-7440-161039';T=A/'runtime-7440-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
def decoded(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
p=A/'runtime-7440-first-failure-analysis.json';v=read(p)
assert v['actual_trajectory_count']==16 and v['trajectory_consistent'] and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');pg=decoded(T/'playground-7440.log')
for prefix in ('hammerburst_source_loss','hammerburst_service_loss','native_lancer_r2','hammerburst_history_origin','hammerburst_tick_cost_summary','hammerburst_tick_cost','hammerburst_first_tick_summary'):
 v[prefix+'_rows']=rows(prefix,pg)
loss=next(r for r in v['hammerburst_source_loss_rows'] if r.get('expired_reader')=='1');assert loss['withdrawal_frame']=='286' and loss['expired_commit_age_ns']=='19509200' and loss['expired_since_commit_ns']=='81409400'
cost=next(r for r in v['hammerburst_tick_cost_rows'] if r['ordinal']=='133');next_cost=next(r for r in v['hammerburst_tick_cost_rows'] if r['ordinal']=='134')
expired=int(loss['expired_read_ns']);sync=int(cost['last_input_sync_ns']);commit=expired-int(loss['expired_since_commit_ns']);begin=int(cost['begin_ns']);captured=int(cost['captured_ns'])
assert expired-sync==100918600 and expired-begin==77107900 and begin-commit==4301500 and int(cost['end_ns'])-expired==12900
input_row=next(r for r in v['native_lancer_r2_rows'] if r['frame']=='286');assert input_row['preparation_budget']=='0' and input_row['copied_input_age_ns']=='85107800'
service=v['hammerburst_service_loss_rows'][0];assert service['gate']=='4' and service['count']=='4'
v['timing_decomposition']=dict(first_expired_read=loss,gt_row=cost,input_row=input_row,commit_check_ns=commit,commit_to_gt_begin_ns=begin-commit,gt_begin_to_expired_read_ns=expired-begin,gt_end_after_expired_read_ns=int(cost['end_ns'])-expired,event_age_at_last_input_admission_ns=int(cost['last_input_observed_ns'])-captured,event_age_at_next_gt_begin_ns=int(next_cost['begin_ns'])-captured)
assert v['timing_decomposition']['event_age_at_next_gt_begin_ns']>250000000
native=decoded(T/'capture-status.txt');fields=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native));assert fields['weapon_hand_input_loss_total']=='0'
v['native_input_loss']=fields;v['native_age_summary']=rows('stereo_hand_loss_summary',native)
v['interpretation']='16 consistent rays: core6,one drain6,then4/6 explicitly cancelled by servicegate4 original250ms pending-event expiry. First actual expired-reader observation captured: Frame286/seq4043 commit source age19.5092ms, commit->GTstart4.3015ms, GTstart->first stale copy77.1079ms, total100.9186ms. Matching GT total77.1208ms; R2 copy already85.1078ms/preparationbudget0. Pendingevent17 capture169492545956300 still237.5792ms at last input admission, nextGTbegin308.4072ms. This shows concrete consumer latency; no evidence of Native input loss or mutex failure. History-origin branch not reached.'
v['next_bounded_source_question']='Inspect moving ONLY an already captured HB continuation/refire with previously certified released input to the existing fresh pre-original held-muzzle stage, before hand/visual/equipment observation cost. Keep same event/capture, original250ms/100ms/20ms, actual original six counter, identity/switch/cancel/release/tracking and once-only claim. New presses/first-event/equipment acquisition retain original post-Input path. Prove with exact caller and transaction tests before any game; no unchanged rerun. Missing4629 origin question remains open.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 pages personally inspected; oversized near-eye gun, ghost/separated hands and white geometry remain. Full fit/headset unaccepted.');write(p,v)
p=A/'runtime-7440-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-7440-first-failure-analysis.json',latest_runtime_result='runtime-7440-first-failure-analysis.json',requirements_complete=False,current_candidate='Private PGaa02381d/Nativeb9ab7a82 actual7440:16 consistent rays, third group4/6 pendingevent250ms expiry. First stale copy19.5092ms commit age+81.4094ms later; GT77.1208ms accounts77.1079ms of delay, no Native inputloss. New history-origin branch not reached. All13 retain full scope.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=7440,runner_last_poll='91144 terminal1; stop/launcher/Native/PG/XR normal0; owned game+children gone; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际7440：首次过期读取的执行耗时分解\n\nPGaa02381d/Nativeb9ab7a82实际16条一致射线（core6、drain6、下一组4/6），原servicegate4 pendingevent250ms到期拒绝，未到开盖/新origin观察。首次expired_reader1实录286/4043：commit source19.5092ms，commit→GTbegin4.3015ms，GTbegin→首个stale copy77.1079ms，合100.9186ms；GT完整77.1208ms，首拒绝距GTend12.9us。原R2读取已85.1078ms/preparation_budget0，pendingevent17在最后input admission年龄237.5792ms，下一GTbegin已308.4072ms，原250ms拒绝，不能延长。Nativeinputloss0/Gate27均age-only保留。下一步检查只将已捕获且已有真实UP认证的HB续发交给现有pre-original fresh muzzle阶段，避免等待后段手/视觉/装备观察；首发/新按下/装备仍走原Input次序，必须保留同event/原capture/20ms/100ms/250ms/原六发及一次claim所有身份取消切枪保护，先实际caller+transaction测试，不无变化重跑。4629缺失原因仍开。正常清理全0无forced、182帧错误空/编码0，3页48对亲阅视觉仍失败；13项保留。\n')
print(json.dumps(dict(actual_rays=16,completed=False,commit_age_ms=19.5092,gt_delay_ms=77.1079,terminal=True)))
