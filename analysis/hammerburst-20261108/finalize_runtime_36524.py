from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-141210';D=I/'actions/menu-firstperson-36524-141231';T=A/'runtime-36524-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
p=A/'runtime-36524-first-failure-analysis.json';v=read(p)
for source in (D/'hammerburst-empty-reload.json',D/'ordinary_empty_replacement/hammerburst-replacement.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');v['replacement']=read(D/'ordinary_empty_replacement/hammerburst-replacement.json')
assert v['actual_trajectory_count']==78 and v['trajectory_consistent']
assert v['empty']['checks']['empty_trigger_preserves_L0_R78_no_shot']
assert not v['empty']['empty_trigger_shots'] and v['empty']['empty_after_trigger']['loaded']==0 and v['empty']['empty_after_trigger']['reserve']==78
assert v['replacement']['checks']['replacement_fills_and_conserves'] and v['replacement']['after']['loaded']==78 and v['replacement']['after']['reserve']==0
receipt=v['replacement']['first_fire_input'];assert receipt['remaining_before_down_ns']==846788200 and 'down_sent_ns' not in receipt
v['return_test_budget_failure']=dict(completion_test_deadline_seconds=8,remaining_before_down_ns=846788200,down_sent=False,correction='The original8s closure/completion wait was also reused for returned-hand gesture/freshUP/return burst. Preserve closure8s and separately anchor12s return-phase budget once before hand return; never renew during DOWN or partial count observation. The preliminary coordinator end-message said45s in error; actual source is8s.')
raw=(T/'capture-status.txt').read_bytes();s=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
v['native_age_summary']=rows('stereo_hand_loss_summary',s);v['native_age_rows']=rows('stereo_hand_loss',s)
fields=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',s));assert fields['weapon_hand_input_loss_total']=='1' and fields['weapon_hand_input_loss_0_stage']=='1'
v['native_input_loss']=fields;v['native_sha256']=read(A/'native-render-age-r1/prepared.json')['native_sha256']
v['native_input_loss_limit']='Frame958 flags13/controller sequence0: strict prepare rejected missing matched input. Original stereo age14.7082ms, NOT a proven stereo expiry. No borrowed timestamp or broad age-only extension.'
raw=(T/'playground-36524.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
for prefix in ('hammerburst_release_debt_wait','hammerburst_stale_debt_wait','hammerburst_pose_gap_incumbent','hammerburst_latch_seek_unavailable','hammerburst_extraction_observed','hammerburst_reload_complete','hammerburst_feed_outcome','hammerburst_release_handoff'):
 v[prefix+'_rows']=rows(prefix,pg)
v['interpretation']='One coordinated solo-load comparison. Core6+12drain bursts=78 consistent rays, typed0/78 and empty-trigger no-shot/veto consumption, ordinary replacement/close typed78/0 passed. No return DOWN sent because original8s completion test deadline had.8467882s left. Whole empty reload-and-return-fire remains FAILED. No single-run concurrency causal claim. Strict prepare missing-input rejection958 remains; new-mag incumbent/latch rollback/released-WAIT branch claims require actual markers.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='Oversized near-right-eye gun, ghost/separated hand, transient white geometry; reload hand contact/all-angle fitting unaccepted. No physical headset proof.');write(p,v)
p=A/'runtime-36524-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-36524-first-failure-analysis.json',latest_runtime_result='runtime-36524-first-failure-analysis.json',requirements_complete=False,current_candidate='Native71f280b0/PGb7dcb9ec controlledsolo36524: drain78/emptyveto/reload78R0 passed; no returnDOWN under8s inherited test deadline. Separate12s return test window repaired; no product changes or full acceptance.')
for req in a['requirements']:
 if req['id']=='empty_reload':
  req.update(status='current_empty_veto_and_reload78R0_observed_return_fire_missing',limit='36524 actual78 consistent rays, typed0/78, no-shot/veto and replacement close78/0. ReturnDOWN not sent under test budget; final72/0 absent, wholeflow not passed.')
a['solo_load_comparison']=dict(evidence='runtime-36524-first-failure-analysis.json',preflight='solo-load-comparison-preflight.json',scope='One finite flow, same Native/PG. Original strict guards remain; no causality from one differing outcome.',coordination_ended=True);write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=36524,runner_last_poll='55019 terminal1; stop/launcher/Native/PG/XR0; owned game+children gone, noforced; encode0; coordinator notified release',next='Save corrected independent12s return test phase and original8s completion wait. Next targeted normal-admission recheck after this concrete harness failure; no repeat solo window or product guard relaxation. All13 retained.');write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 单实例36524：空仓和换弹通过，回射窗口不足\n\n按用户原模拟器协调授权，四工作会话临时停止新增game、现有运行自然结束；14:11:59精确worker/game/XR/编码/构建均空闲后，单次原有限空仓流程。Native71f280b0/PGb7dcb9ec不变：78条一致射线、库存0/78、空扳机无射线和原veto消费、普通换弹关盖78/0通过；回射前原8s关盖完成测试窗口仅余0.8467882s，预算检查未发送DOWN，整流程仍失败。原8s被复用于返手/新鲜UP/回射，现仅私有适配器保留关盖8s，返手前一次性建立独立12s观察窗口，不在按下/部分射线后续期；产品100/20/250ms和输入次数不变，12项原库存回射检查及8模块编译通过。Native年龄31次保留、prepare958因未匹配input拒绝1次（Stereo14.7082ms，不得称stereo过期），单次对照不证明并发根因或修复。正常清理全0、无forced、637帧无捕获错误/编码0，3页48对全跨度亲阅，近眼巨枪/手分离/白几何仍不合格。已通知统筹解除让行，不再追加本次单实例测试。全部13项保留，回射72/0与整枪验收仍缺。\n')
print(json.dumps(dict(shots=78,empty_veto=True,reload_loaded=78,reload_reserve=0,return_down_sent=False,age=v['native_age_summary'],new_branch_counts={k:len(v[k+'_rows']) for k in ('hammerburst_release_debt_wait','hammerburst_pose_gap_incumbent','hammerburst_latch_seek_unavailable')})))
