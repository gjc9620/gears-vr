from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1];NAME='hb-cloak-1111-024442';instance=ROOT/'analysis/instances'/NAME
p=A/'runtime-57324-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,
 findings='Personally viewed all3 full-span overview pages/48 paired samples. Initial sampled00000/00002 are startup cards and00009 black; later actual game/hands/near-right gun visible. No authoritative acquisition, shots or reload established. No full fitting/continuity/headset acceptance.',
 limitation='86 recorded frames/errors empty;48 overview samples, not every frame/all angles. No claim every recorded image is actual gameplay; retain startup/black samples.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-57324-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0)
gaps=d['native_equipment_input_gap_rows'];assert len(gaps)==1;g=gaps[0]
assert g['frame']=='331' and g['reason']=='producer_sync_gap'
sync_gap=int(g['sync_ns'])-int(g['previous_sync_ns']);edge_age=int(g['observed_ns'])-int(g['right_press_sync_ns'])
assert sync_gap==159375500 and edge_age==142958800
down=[r for r in d['hammerburst_inventory_timing_rows'] if r['right_down']=='1']
assert len(down)==4 and all(r['phase']=='4' and r['stage']=='ok' for r in down)
d['first_acquisition_cause']=dict(frame=331,producer_sync_gap_ns=sync_gap,recorded_original_press_age_ns=edge_age,
 inventory_down_successes=4,inventory_elapsed_ns=[int(r['elapsed_ns']) for r in down],
 inventory_exit_age_ns=[int(r['exit_age_ns']) for r in down],
 scope='57324 only; not retroactive attribution for56032 current_after. Inventory passed in actual Grip frames; Input rejects original edge older than100ms across producer gap.')
receipts=[]
for name in ('isolated-start-request.json','isolated-worker.json'):
 source=instance/name;target=A/'runtime-57324-text'/name;target.write_bytes(source.read_bytes())
 receipts.append(dict(source=str(source),archive=str(target),sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
d['first_admission_receipts']=receipts;d['canonical_first_request_pid']=4856
d['canonical_wait_observation']='Same first request4856 was positively observed alive with exactcreation134388098990567222 before worker/session existed; subsequently same fresh instance admitted once and normal game cleanup. No worker restart/old-name reuse. One live request observation, not universal contention acceptance.'
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(current_candidate='inventory-timing-r1: compiled+6 observer checks, actual57324 inventory timing reached; reload priority/history still not game exercised',
 prepared_sha256=hashlib.sha256((A/'inventory-timing-r1/prepared.json').read_bytes()).hexdigest(),latest_runtime_failure='runtime-57324-first-failure-analysis.json',
 candidate_runtime_status='Actual57324 initial Grip Frame331 rejected producer_sync_gap159.3755ms; exact original edge142.9588ms old. Down inventory checks all passed in4.5333-10.5288ms, exit ages54.4694-75.6599ms. Zero authoritative acquisition/rays/reload. Normal cleanup. Does not establish56032 cause.',
 immediate_next='Improve isolated pickup driver to require observed fresh released Native baseline at actual shoulder before ONE new Grip, then await actual matching held receipt before moving away. Preserve original100ms edge/source and all product gates; no silent blocked-edge retry or deadline expansion. Then validate priority/history/seating/closure/return and full13 requirements.')
for r in audit['requirements']:
 if r['id']=='acquire_stow':
  r.update(status='partial_latest_grip_expired_before_admission',limit='Actual57324 inventory passed, but right recorded Grip edge142.9588ms old across producer sync gap159.3755ms, correctly rejected by original100ms bound. No held receipt. Need fresh released Native baseline before new physical test intent; prior successful acquisitions remain separate, full stow/restore open.')
  f=A/'runtime-57324-first-failure-analysis.json';r['evidence'].append(dict(path=str(f),sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
