"""Focused loaded replacement pass; explicitly not whole-weapon completion."""
from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-22992-media-review/prepared.json';m=json.loads(p.read_text());assert m['frames']==582 and m['sample_count']==48 and not m['capture_errors']
action=ROOT/'analysis/instances/hb-cloak-1111-051514/actions/menu-firstperson-22992-051857'
contacts=[action/f'frame-{n:05d}.jpg' for n in (185,297,334,420,470,494)]
m.update(overview_reviewed=True,contact_images_reviewed=[dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in contacts],findings='Personally reviewed all3 full-span overview pages/48 paired samples plus6 actual contact frames. Actual shoulder draw, held HB, old/new magazine/latch hand motion and pause/B-resume visible. Gun very near right eye, ghosting/intersections and offhand separation/open pose persist; yellow guides visible but fine text not accepted. No complete five-part surface/all-angle/headset fitting acceptance.',limitation='48 sampled pairs of582 plus6 contacts, not every frame/all angles. Pause menu pages retained as genuine action UI, not waiting cards.')
p.write_text(json.dumps(m,indent=2))
p=A/'runtime-22992-focused-result.json';d=json.loads(p.read_text())
assert d['operator_exit']==d['stop_exit']==d['cleanup']['launcher_exit']==0 and d['native_cleanup_verified'] and d['xr_exit_code']==0
assert d['actual_trajectory_count']==12 and d['trajectory_consistent'] and not d['foreground_violations'] and not d['cloak_violations']
assert all(d['core']['checks'].values()) and all(d['replacement']['checks'].values()) and 'error' not in d['core'] and 'error' not in d['replacement']
r=d['replacement'];assert (r['before']['loaded'],r['before']['reserve'])==(72,78)
assert (r['after']['loaded'],r['after']['reserve'])==(78,72)
assert (r['after_return_fire']['loaded'],r['after_return_fire']['reserve'])==(72,72)
assert r['return_fire_observations'][-1]['exported_rays']==6
assert len(d['core']['shots'])==len(r['shots'])==6
pg=ROOT/'tools/playground/build-hb-cloak-1111-051514/playground-22992.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
handoffs=rows('hammerburst_release_handoff');assert len(handoffs)==2 and all(x['validated']=='1' and x['pending_same']=='1' and x['no_dispatch']=='1' for x in handoffs)
assert [x['frame'] for x in handoffs]==['274','769']
history=d['hammerburst_feed_history_rows'];bridged=[x for x in history if x['used']=='1' and x['result']=='1'];assert bridged and all(int(x['points'])==6 for x in bridged)
assert not d['hammerburst_identity_lost_rows'] and len(d['native_equipment_support_withdrawal_rows'])==6
assert d['hammerburst_early_reload_rows'][0]['frame']=='369' and d['hammerburst_supply_frame_rows'][0]['frame']=='500'
assert all(x['invoked']=='1' for x in d['hammerburst_press_timing_rows'])
action_result=json.loads((action/'result.json').read_text());assert action_result['checks']['pause_B_resume']
setup_ages=[obj['first_fire_input']['down_sent_ns']-int(obj['first_fire_input']['baseline']['input_sync_ns']) for obj in (d['core'],r)]
d.update(focused_actions_passed=True,full_goal_complete=False,release_handoff_rows=handoffs,
 verified_branches=dict(full_endpoint_pickup=True,first_fire_setup_down_age_from_baseline_ns=setup_ages,release_revalidation_two_real_events=True,early_reload_frame369=True,latch_same_frame369=True,supply_same_frame500=True,feed_history_original6points=bridged,loaded_replacement_and_return_typed_counts=True,pause_B_resume=True),
 limits='Focused loaded-magazine replacement only: no empty drain/zero reserve, retained-original reseat, two-hand/foregrip four states, perfect cancellation, tracking/switch/stow restore, five-part/all-angle fit or headset acceptance. Six original support-withdrawal rows after seat/closure retained, not globally zero withdrawal. One passing batch is not all-load stability or unique causal proof. Ordinary startup success does not prove WinError5 retry branch executed.')
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_result='runtime-22992-focused-result.json',latest_focused_pass='runtime-22992-focused-result.json',candidate_runtime_status='endpoint-baseline-r1 actual22992 focused loaded replacement PASS: two real original6 bursts, realUP pending handoff274/769 revalidated, typed72/78->78/72->72/72; original6-point history bridge live, pause/Bresume. Normal cleanup. Full weapon/empty/retained/fit/headset incomplete.',immediate_next='Save focused result; one fresh FULL empty path (omit --focused-loaded-replacement) using same compiled candidate and stricter9-fixture driver. Verify actual78 drain +zero veto +empty replacement78/0 +return6 with final typed72/0; stop first failure. Then retained original/four-state/two-hand/interruptions and fitting remain. Preserve all13 requirements; no unchanged focused rerun.')
e=A/'runtime-22992-focused-result.json'
for req in audit['requirements']:
 if req['id'] in ('acquire_stow','finite_fire','replacement','interruptions','fivepartfitting','delivery_headset'):
  req['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
 if req['id']=='finite_fire':req.update(status='current_two_original6_bursts_released_tail_confirmed_partial',limit='Actual22992 two real original6 bursts; both actualUP handoffs revalidate same pending event then tail.12 consistent rays; full two-hand/continuous-load/empty/zero-reserve acceptance remains open.')
 if req['id']=='replacement':req.update(status='current_loaded_replace_and_return_counts_confirmed',limit='Actual22992 loaded replacement typed72/78->78/72 and return6->72/72 passed, live6-point history/early priority/latch/supply. Empty/retained/all-flow stability and visual surface fit remain unaccepted.')
 if req['id']=='interruptions':req.update(status='current_pause_B_resume_only_other_interruptions_open',limit='Actual22992 pause/B resume passed. Tracking loss, switch, stow restore and all anti-replay interruption chains not covered by this focused batch.')
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
p=A/'active-runtime-wait.json';w=json.loads(p.read_text());w.update(terminal=True,game_pid=22992,runner_last_poll='29637 terminal exit0, all own runtime cleanup0; focused actions pass only',next=audit['immediate_next']);p.write_text(json.dumps(w,indent=2))
print(json.dumps(dict(focused_pass=True,actual_rays=12,typed_counts=[[72,78],[78,72],[72,72]],real_release_handoffs=2,history_bridges=len(bridged),full_goal_complete=False)))
