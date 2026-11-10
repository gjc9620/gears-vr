from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent
p=A/'runtime-56348-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,
 findings='Personally viewed all three full-span overview pages/48 paired samples. Actual game/free hands/shoulder acquisition/held gun visible. Near-camera obstruction, overlapping/white geometry and intermittent close-gun visibility remain. No reload reached; full fit/continuity/all-angle acceptance false.',
 limitation='174 recorded frames/no capture errors;48 full-span samples reviewed, not every frame/all angles or a physical headset.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-56348-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0)
d['derived_original_intent_deadline_ns']=118063577524800+250000000
d['deadline_basis']='Frozen weapon_lancer_r2.h Input::defer anchors pending_deadline to first deferred controllers.sync_ns; Frame195 input admission118063894200400 is later. This is not event renewal or full pre-dispatch timing attribution.'
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(latest_runtime_failure='runtime-56348-first-failure-analysis.json',
 candidate_runtime_status='feed-history-r1 actual56348 loaded; first core DOWN Frame193 deferred10, Frame19420ms reserve fails at83.8485ms source age, Frame195 blocked8 after original intent deadline. Zero rays, no reload/history branch. Normal cleanup. Feed fix compiled43 local scenarios but not game exercised.',
 immediate_next='Inspect and reduce HB first-press pre-dispatch preparation delay using exact56348 evidence; preserve20ms reserve/source100ms/original intent250ms, no retry of blocked edge. Then validate actual feed-history adoption/seating/closure/typed conservation/return-fire and remaining13 requirements. Do not rerun unchanged candidate just to seek a pass.')
for r in d['requirements']:
 if r['id']=='finite_fire':
  r.update(status='partial_first_press_deferral_and_tail_expiry_open',
    limit='Actual54452 core+12drain6 passed.20672 pending original tail event expired after1/6.56348 new candidate first DOWN deferred then20ms reserve not available at83.8485ms; original intent expired, zero rays. Need reduce preparation delay while retaining all deadlines; whole stability/two-hand unaccepted.')
  f=A/'runtime-56348-first-failure-analysis.json';r['evidence'].append(dict(path=str(f),sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
assert len(d['requirements'])==13 and not d['completion_proven'];p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
