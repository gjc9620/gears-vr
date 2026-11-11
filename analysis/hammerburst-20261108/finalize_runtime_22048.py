"""Preserve actual empty-drain success and pre-seat authority failure."""
from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1]
action=R/'analysis/instances/hb-cloak-1111-115944/actions/menu-firstperson-22048-120016'
p=A/'runtime-22048-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==78 and d['trajectory_consistent']
assert d['operator_exit']==1 and d['stop_exit']==0 and d['native_cleanup_verified']
assert d['xr_exit_code']==0 and d['cleanup']['owned_game_absent']
for name,source in [('empty_reload',action/'hammerburst-empty-reload.json'),('replacement',action/'ordinary_empty_replacement/hammerburst-replacement.json')]:
 d[name]=json.loads(source.read_text());dest=A/'runtime-22048-text'/source.name
 shutil.copy2(source,dest);h=hashlib.sha256(source.read_bytes()).hexdigest()
 assert h==hashlib.sha256(dest.read_bytes()).hexdigest()
 d['verified_archive'].append(dict(source=str(source),archive=str(dest),sha256=h))
e=d['empty_reload'];assert e['drain_loaded_only']['genuine_press_release_bursts']==12
assert e['checks']['empty_trigger_preserves_L0_R78_no_shot']
assert d['replacement']['before']['loaded']==0 and d['replacement']['before']['reserve']==78
b=(R/'tools/playground/build-hb-cloak-1111-115944/playground-22048.log').read_bytes()
lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(pre):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(pre+' ')]
assert [x['to'] for x in rows('hammerburst_reload_transition')]==['2','3','4','5','6']
withdraw=rows('native_equipment_support_withdrawal');moves=rows('hammerburst_feed_move_rejected')
assert withdraw[0]['frame']=='698' and moves[0]['frame']=='709'
ci=e['drain_loaded_only']['confirmed_inputs']
d.update(full_goal_complete=False,focused_actions_passed=False,
 verified_partial=dict(core6_and_drain72=True,empty_trigger_veto=True,original_empty_begin=True,replacement_acquired=True,seating=False,return_fire=False),
 first_seating_failure=dict(first_withdrawal=withdraw[0],lease_lost=withdraw[3],first_move_rejection=moves[0],last_admitted_feed=rows('hammerburst_feed_outcome')[-1],
 interpretation='Token2 source expiry698,706/707, decision12/no lease708, no grant709 onward. Last admitted insertion706 depth21.913565mm. Later805 lifecycle difference64 is not first cause. Upstream publication/latency cause unproven; no relaxed freshness or grant replay.'),
 drain_reader_diagnostics=dict(runtime_exercised=True,accepted_source_age_ns=[q['first_fire_input']['baseline_wait']['minimum_source_age']['source_age_ns'] for q in ci],max_read_ns=max(s['read_ns'] for q in ci for s in q['first_fire_input']['baseline_wait']['samples'])),
 next_source_question='Trace exact native selected producer through support decision/consumer698->709. Compare same-Scene source publication against frozen806dcbd6; repair proven selection/latency only, never renew timestamps or retain stale control.')
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
mp=A/'runtime-22048-media-review/prepared.json';m=json.loads(mp.read_text())
assert m['frames']==699 and m['sample_count']==48 and not m['capture_errors']
m.update(overview_reviewed=True,findings='Personally reviewed3 overview pages48 paired samples. Draw/fire/reload visible; near-eye occlusion, ghost/open detached offhand and white geometry remain. Not complete guide/hand/all-angle/headset acceptance; dense contacts pending.')
mp.write_text(json.dumps(m,indent=2))
ap=A/'full-requirements-audit-20261110.json';audit=json.loads(ap.read_text(encoding='utf-8'))
audit.update(latest_runtime_failure=p.name,latest_runtime_result=p.name,candidate_runtime_status='Actual22048 full78 drain and empty veto0/78 passed. Reload1->6/replacement acquired; source expiry698 then lease loss708/709 beforeseat. No close/return; normal cleanup.',immediate_next=d['next_source_question'])
ev=dict(path=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest())
for req in audit['requirements']:
 if req['id'] in ('acquire_stow','finite_fire','empty_reload','replacement','part_fitting','delivery_headset'):req['evidence'].append(ev)
 if req['id']=='empty_reload':req.update(status='current_full_drain_veto_pass_seating_source_lease_failure',limit='Actual22048 78 consistent rays/typed0R78 and empty veto passed; original reload1->6, no seat/close/return. Token2 consumer source expiry698 then lease loss708/709. Upstream cause unproven; historical56252 finaltyped72/0 missing.')
assert len(audit['requirements'])==13 and not audit['completion_proven']
ap.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
wp=A/'active-runtime-wait.json';w=json.loads(wp.read_text());w.update(terminal=True,game_pid=22048,runner_last_poll='35043 terminal1; operator1 and all own runtime cleanup normal0',next=d['next_source_question']);wp.write_text(json.dumps(w,indent=2))
cp=A/'first-fire-reader-diagnostics-check.json';c=json.loads(cp.read_text())
c['source_sha256']['check_confirmed_first_fire.py']=hashlib.sha256((A/'check_confirmed_first_fire.py').read_bytes()).hexdigest()
c.update(runtime_exercised_game=22048,limit='12 accepted UP setups recorded; prior15016 timeout cause not reproduced.')
cp.write_text(json.dumps(c,indent=2))
print('Saved22048 full drain/empty veto and exact pre-seat authority failure; full goal incomplete')
