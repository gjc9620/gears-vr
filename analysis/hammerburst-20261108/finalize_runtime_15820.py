from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-155817';D=I/'actions/menu-firstperson-15820-155900';T=A/'runtime-15820-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
def decoded(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
p=A/'runtime-15820-first-failure-analysis.json';v=read(p)
assert v['actual_trajectory_count']==78 and v['trajectory_consistent'] and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',D/'ordinary_empty_replacement/hammerburst-replacement.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');v['replacement']=read(D/'ordinary_empty_replacement/hammerburst-replacement.json');assert v['empty']['checks']['empty_trigger_preserves_L0_R78_no_shot']
pg=decoded(T/'playground-15820.log')
for prefix in ('hammerburst_source_loss','hammerburst_identity_lost','hammerburst_history_rejection','hammerburst_history_raw','hammerburst_history_hand','hammerburst_latch_history','hammerburst_latch_rejected','hammerburst_reload_cleanup','hammerburst_tick_cost_summary','hammerburst_first_tick_summary'):
 v[prefix+'_rows']=rows(prefix,pg)
replay=read(A/'history-replay-15820/result.json');assert replay['passed'] and replay['exact_rejection_reproduced'];v['actual_history_replay']=replay
failure=v['hammerburst_latch_rejected_rows'][0];assert failure['sequence']=='4633' and failure['reason']=='5' and int(failure['sample_ms'])-int(failure['previous_ms'])==111
native=decoded(T/'capture-status.txt');fields=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native));assert fields['weapon_hand_input_loss_total']=='0'
mutex=[dict(index=int(n),attempted=int(a),result=int(b),contended=int(c)) for n,a,b,c in re.findall(r'shared_grip_handoff_mutex_(\d+)=(\d+),(\d+),(\d+)',native)]
assert len(mutex)==7 and all(r['attempted']==1 and r['result']==0 and r['contended']==0 for r in mutex)
v['native_input_loss']=fields;v['native_age_summary']=rows('stereo_hand_loss_summary',native);v['actual_mutex_observations']=mutex;v['native_sha256']=read(A/'native-mutex-r1/prepared.json')['native_sha256']
v['interpretation']='78 actual consistent rays/emptyveto passed; no mutex failure/inputloss reproduced. New Native original mutex results exported7 successful reads (no failed-result live claim). Opening505/4633 first raw history observer hit; previous4628/current4633, retained history4630..4633. Actual original C++ kernel replay reproduces build=false solely at previous4628->first4630 strict consecutive sequence, all recorded shapes/projections/held counters and retained adjacent pairs pass. Delta5 is within original capacity8; no invented4629. Latch gap111ms reason5, angle.166937848. Full reload remains failed.'
v['next_bounded_source_question']='Determine why original history snapshot lacks4629: preserve original packet.latest_sequence and loop stop reason/index/sequence during the same existing read, not a new ring query. Distinguish eviction versus missing/bad raw pair before repair. Native mutex failure not reproduced; keep strict negative outcomes and no unchanged rerun.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 pages personally inspected; oversized near-eye gun, ghost/separated hands and white geometry remain. No complete contact/headset acceptance.');write(p,v)
p=A/'runtime-15820-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-15820-first-failure-analysis.json',latest_runtime_result='runtime-15820-first-failure-analysis.json',requirements_complete=False,current_candidate='Nativeb9ab7a82/PG31f7d324 actual15820:78 consistent rays/emptyveto pass. Opening history missing4629, original pure kernel actual replay reproduces strict rejection4628->4630; not capacity>8 or bad geometry. Native7 successful mutex receipts/inputloss0; no mutex failure reproduced. Full13 remain incomplete.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=15820,runner_last_poll='9914 terminal1; stop/launcher/Native/PG/XR normal0; owned game+children gone; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际15820：原始历史拒绝与实际数据内核重放\n\nNativeb9ab7a82/PG31f7d324完整78条一致射线、空仓0/78禁射通过。Native7条原锁结果成功0/inputloss0，未复现mutex失败，不称修复。开盖505/4633首次采到原始history失败：上次4628，本次4633，history4630..4633共4，缺4629；gap111ms原reason5在angle.166937848拒绝。用12条双手原pose/flags/counters及6条原raw重建，运行实际未修改controller_relative_motion内核，所有shape/project/held计数及已留相邻motion_pair均通过，唯一首失败4628→4630非连续；delta5在原capacity8内。未补造4629/放宽gap。尚不能分辨从未发布/读取前覆盖/原pair无效；下一步保留同一次原packet.latest及history loop停止原因，不增读。完整换弹仍失败。正常清理全0无forced、439帧错误空/编码0，3页48对亲阅视觉仍失败；13项保留。\n')
print(json.dumps(dict(actual_rays=78,empty_veto=True,actual_history_replayed=True,missing_sequence=4629,mutex_failures=0,terminal=True)))
