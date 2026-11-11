from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-154738';D=I/'actions/menu-firstperson-35716-154802';T=A/'runtime-35716-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
def decoded(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
p=A/'runtime-35716-first-failure-analysis.json';v=read(p)
assert v['actual_trajectory_count']==78 and v['trajectory_consistent'] and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',D/'ordinary_empty_replacement/hammerburst-replacement.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');v['replacement']=read(D/'ordinary_empty_replacement/hammerburst-replacement.json');assert v['empty']['checks']['empty_trigger_preserves_L0_R78_no_shot']
pg=decoded(T/'playground-35716.log')
for prefix in ('hammerburst_source_loss','hammerburst_identity_lost','hammerburst_history_rejection','hammerburst_latch_history','hammerburst_latch_rejected','hammerburst_latch_seek_unavailable','hammerburst_pose_gap_incumbent','hammerburst_reload_cleanup','hammerburst_tick_cost_summary','hammerburst_first_tick_summary','hammerburst_tick_cost'):
 v[prefix+'_rows']=rows(prefix,pg)
assert not v['hammerburst_history_rejection_rows'] and not v['hammerburst_latch_rejected_rows']
identity=v['hammerburst_identity_lost_rows'][0];assert identity['frame']=='585' and identity['difference']=='64' and identity['old_lifecycle']=='4' and identity['now_lifecycle']=='5'
native=decoded(T/'capture-status.txt');fields=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native));assert fields['weapon_hand_input_loss_total']=='2' and fields['weapon_hand_input_loss_1_frame']=='583' and fields['weapon_hand_input_loss_1_stage']=='1'
now=int(fields['weapon_hand_input_loss_1_now_ns']);stereo=int(fields['weapon_hand_input_loss_1_stereo_ns']);assert now-stereo==17771000
v['native_input_loss']=fields;v['native_age_summary']=rows('stereo_hand_loss_summary',native)
v['failure_chain']=dict(first_native_prepare=dict(frame=583,sequence=0,flags=13,stage=1,stereo_age_ns=now-stereo,controller_sync_ns=0),subsequent_client_game_difference=next(r for r in v['hammerburst_source_loss_rows'] if r['withdrawal_frame']=='584'),identity_lost=identity,limits='Input-matched/controller absent while stereo17.771ms; not stereo expiry. Existing stage1 does not say why original controller reader returned absent. game_difference8 is broad game/mount/held change, not a named causal field. Lifecycle change is proven; do not weaken it or attribute concurrency.')
v['interpretation']='78 consistent actual rays and empty0/78 veto passed. Opening1->2 reached, angle0.452992159 at cleanup; Native prepare583 has absent matched controller seq0/flags13 despite stereo17.771ms, later client584 game_difference8,585 lifecycle4->5 causes strict identity cleanup. No latch motion rejection/history observer/expired-reader hit. Full reload/return-fire fails; no causal freshness repair claim.'
v['next_bounded_source_question']='Inspect original Native controller acquisition at prepare stage1 frame583: determine whether transport read, coherent snapshot, freshness or stereo/controller cohort matching rejected it. Preserve exact first failure with original status/timestamps, no requery/input/changed freshness/lifecycle rules. No unchanged rerun.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 overview pages personally inspected: near-eye oversized gun, ghost/separated hands and white geometry; contact/headset still unaccepted.');write(p,v)
p=A/'runtime-35716-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-35716-first-failure-analysis.json',latest_runtime_result='runtime-35716-first-failure-analysis.json',requirements_complete=False,current_candidate='Private PG31f7d324/Native71f280b0 actual35716:78 consistent rays/emptyveto passed, opening failed Native583 absent matched controller despite stereo17.771ms; lifecycle4->5 cleanup585 atangle.452992159. No new expired-reader/history hits; all13 incomplete.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=35716,runner_last_poll='56925 terminal1; stop/launcher/Native/PG/XR normal0; owned game+children gone; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际35716：打空通过，开盖时原控制器匹配缺失\n\nPG31f7d324/Native71f280b0完整78条一致射线、空仓0/78禁射通过，开盖1→2后角度0.452992159结束；Native583 prepare stage1 seq0/flags13，Stereo仅17.771ms，controller sync缺失，并非Stereo100ms超期。后584消费game_difference8（广义game/mount/held变化，不命名具体字段），585身份difference64 lifecycle4→5按原保护清理；开盖未完成、无取旧弹匣/换弹后射击。原输入缺失总2（另133为早期），Gate39 age-only全保留。新expired-reader/history观察均未命中，不称修复。下一步定位原Native controller读取/一致性/新鲜度/cohort匹配哪个返回缺失，不能放宽保护或无变化重跑。正常清理全0无forced；481帧错误空/编码0，3页48对亲阅视觉仍失败。13项保留。\n')
print(json.dumps(dict(actual_rays=78,empty_veto=True,opening_completed=False,stereo_age_ns=now-stereo,terminal=True)))
