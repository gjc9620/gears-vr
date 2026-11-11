from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-163604';D=I/'actions/menu-firstperson-20924-163627';T=A/'runtime-20924-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
def decoded(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
p=A/'runtime-20924-full-empty-reload.json';v=read(p)
assert v['actual_trajectory_count']==84 and v['trajectory_consistent'] and v['operator_exit']==0 and v['stop_exit']==0 and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',D/'ordinary_empty_replacement/hammerburst-replacement.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');v['replacement']=read(D/'ordinary_empty_replacement/hammerburst-replacement.json')
e=v['empty'];r=v['replacement'];assert all(e['checks'].values()) and all(r['checks'].values()) and e['gameplay_setters'] is False and e['reserve_exhaustion_attempted'] is False
assert len(e['drain_loaded_only']['burst_observations'])==12 and all(b['actual_spent']==6 for b in e['drain_loaded_only']['burst_observations'])
assert [(r[k]['loaded'],r[k]['reserve']) for k in ('before','after','after_return_fire')]==[(0,78),(78,0),(72,0)] and len(r['shots'])==6 and not e['empty_trigger_shots']
pg=decoded(T/'playground-20924.log')
for prefix in ('hammerburst_pre_original_refire','hammerburst_service_loss','hammerburst_history_origin','hammerburst_latch_history','hammerburst_latch_rejected','hammerburst_extraction_history','hammerburst_reload_transition','hammerburst_reload_complete','hammerburst_tick_cost_summary','hammerburst_first_tick_summary'):
 v[prefix+'_rows']=rows(prefix,pg)
early=v['hammerburst_pre_original_refire_rows'];assert len(early)==12
assert all(int(x['after_count'])==int(x['before_count'])+1 and x['pending_after']=='0' and x['once_claim']=='1' for x in early)
assert len({x['event'] for x in early})==12 and not v['hammerburst_service_loss_rows'] and not v['hammerburst_latch_rejected_rows'] and not v['hammerburst_history_origin_rows']
assert len(v['hammerburst_reload_complete_rows'])==1 and v['hammerburst_reload_complete_rows'][0]['loaded']=='78'
native=decoded(T/'capture-status.txt');v['native_input_loss']=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native));assert v['native_input_loss']['weapon_hand_input_loss_total']=='0';v['native_age_summary']=rows('stereo_hand_loss_summary',native)
v['interpretation']='Full single-game ordinary empty reload/return-fire PASS: core6+12drain6=78 actual consistent rays, actual empty0/78 no-shot veto, original opening1->2->3->4/extraction/newmag/seat/close/complete78/0, one return6 actual rays and typed72/0. Total84 consistent rays, arithmetic84spent+72remaining=156initial. New earlier original released-refire path actually recorded12 distinct captured events, eachcount+1 andpending0; bounded logger cap12 prevents claiming total early uses. No original service/latch rejection; Nativeinputloss0. Current pickup direct_event, so repaired delayed-completion reader not exercised live. Original latch history bridge accepted3 actual112ms intervals; no rejection-origin/extraction-gap/pose-gap-causal proof. One game pass does not resolve historical missing4629/mutex/first-press variants or visual/headset/full13.'
v['mechanical_slice_passed']=True;v['whole_weapon_accepted']=False
v['next_bounded_work']='Preserve full pass; no unchanged full-flow rerun. Validate requested foregrip4 states in current private fixture and current source, then retained original magazine/perfect-cancel/remaining interruptions/zero-reserve as separate unmet requirements. Inspect actual hand/weapon/part correspondence and visible contact; current images fail fit. Keep original historical failures and public baseline.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 pages/48 paired full-span views personally inspected. Gun frequently lies low or outside view, arms/hands ghost or separate; visible replacement near eye around84.8s and large white geometry in some firing views. Cannot accept overall weapon/hand/part fitting. Pause menu appears111/112s then world resumes; no full interruption/headset claim.');write(p,v)
p=A/'runtime-20924-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
evidence=dict(path=str(A/'runtime-20924-full-empty-reload.json'),sha256=hashlib.sha256((A/'runtime-20924-full-empty-reload.json').read_bytes()).hexdigest())
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_result='runtime-20924-full-empty-reload.json',latest_full_empty_reload_pass='runtime-20924-full-empty-reload.json',requirements_complete=False,current_candidate='PG325200c3/Nativeb9ab7a82 actual20924 full ordinary empty reload+return PASS;84consistent rays/0-78veto/78-0reload/72-0return. Actual earlier released refire12events count+1/pending0. Single game only; other13 scope remains, fitting unaccepted.')
for requirement in a['requirements']:
 if requirement['id']=='empty_reload':
  requirement.update(status='current_full_ordinary_empty_reload_and_return_fire_passed_single_game',limit='Actual20924 complete0/78 empty veto->78/0 reload->72/0 after6rays. Single game only; perfect/cancel/retainedoriginal/zero-reserve exhaustion and visual/headset remain separate.')
 if requirement['id']=='finite_fire':
  requirement.update(status='current_14_original_six_bursts_84_consistent_rays_single_game',limit='Actual20924 core6+12drain6+return6=84, matching typed counts;12 early released-refire events count+1. Does not establish indefinite hold/reacquisition/all source-loss/zero-reserve cases.')
 if requirement['id'] in ('finite_fire','empty_reload','replacement'):requirement.setdefault('evidence',[]).append(evidence)
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=20924,runner_last_poll='31958 terminal0; stop/launcher/Native/PG/XR normal0; owned game+children gone; noforced;encode0',next=v['next_bounded_work']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际20924：完整普通空仓换弹与回枪六发通过\n\nPG325200c3/Nativeb9ab7a82原完整流程单局通过：取枪core6、12组drain6共78条一致射线，空仓0/78真实按下禁射；开盖1→2→3→4、原旧mag提取、新mag取放座入、合盖完成78/0；回枪实际6条射线后typed72/0。总84一致射线，84已用+72剩余=156初始总量；test gameplay_setters=false，reserve_exhaustion_attempted=false，不算零弹药耗尽测试。早续发路径实际12个不同已捕获原event，每条count+1/pending0，受logger cap12，不能称总共只走12次或其他未留存事件都走它；service/latch rejection0，Nativeinputloss0/Gate150 age-only均保留逻辑登记。当前取枪direct_event，修正延迟held读取没有实际命中；旧缺4629/锁失败/首发变体不宣称全修复。3次原latch history接受112ms分段，没有history-origin失败/提取gap修复因果证明。正常清理全0无forced；756帧错误空/编码0，3页48对全时段亲阅，枪经常低于/离开视野、手臂手重影或分离、84.8s近眼换弹与部分白几何仍不合格。可见暂停卡后回world仅本轮观察，不扩成全中断或物理验收。下一步当前fixture四态护木/原弹匣/完美取消等剩余要求与真实接触，不能再无变化重复完整流程；13项全部保留/public未变。\n')
print(json.dumps(dict(full_mechanical_slice_passed=True,total_actual_rays=84,final_loaded=72,final_reserve=0,early_logged_events=len(early),whole_weapon_accepted=False,terminal=True)))
