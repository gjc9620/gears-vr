from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent
p=A/'runtime-20284-media-review/prepared.json';m=json.loads(p.read_text())
assert m['frames']==54 and m['sample_count']==48 and not m['capture_errors']
m.update(overview_reviewed=True,findings='Personally reviewed all3 full-span pages/48 paired samples. Initial00000..00003 waiting cards,00016 black, later actual scene and shoulder hand. No successful held HB, firing/reload pixel evidence. Arm intersection remains; not full fit/all angles/headset.',limitation='48 sampled pairs of54 recorded frames; early cards/black remain, no full action acceptance.')
p.write_text(json.dumps(m,indent=2))
p=A/'runtime-20284-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==0 and d['native_cleanup_verified'] and d['xr_exit_code']==0 and d['stop_exit']==0
assert not d['foreground_violations'] and not d['cloak_violations']
pickup=d['core']['pickup_input'];baseline=pickup['baseline']
assert int(baseline['sync_ns'])>pickup['up_sent_ns'] and baseline['frame']=='499' and 'held' not in pickup
assert 0<pickup['down_sent_ns']-int(baseline['sync_ns'])<100000000
assert not d['native_equipment_input_gap_rows'] and not d['hammerburst_press_timing_rows']
d.update(pickup_failure=dict(actual_up=pickup,source_gap_rows=0,matching_held=False,first_failure='One Grip after actual freshUP499, no matching held within2s; actual failureUP, no repeat press.',
 source_review='Actual driver source/fixture confirms callback stopped original reach as soon as40mm observer-region UP appeared. This is not proof of reaching exact requested endpoint. Actual early-stop pose not archived, so sole game failure cause unproven.'),
 driver_repair='Preserve every original gesture sample to exact endpoint, observe without early return, assert/record exact commanded endpoint beforeGrip; skip only250ms settling tail. Private observation region narrowed40mm->5mm; actual equipment/action geometry unchanged.',
 release_handoff_live_exercised=False,press_asset_timing_live_exercised=False)
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-20284-first-failure-analysis.json',current_candidate='endpoint-baseline-r1 full PG compiled,10 observer fixtures and17 driver cases passed, no game yet; full original reach preserved, exact endpoint required, private UP observation5mm only',prepared_sha256=hashlib.sha256((A/'endpoint-baseline-r1/prepared.json').read_bytes()).hexdigest(),candidate_runtime_status='Previous press-assets-r1 actual20284: fresh realUP499 and oneGrip but no matching held; no source-gap rows/no first/no timing/handoff/reload. Normal cleanup. Early-stop driver flaw corrected, sole actual failure cause unproven.',immediate_next='Save new source then one fresh endpoint-baseline-r1 instance. Confirm commanded endpoint+freshUP+singleGrip+matching held before first split timing/finite6. Do not reuse20284 or43764, both terminal. Preserve all13 requirements.')
e=A/'runtime-20284-first-failure-analysis.json'
for r in audit['requirements']:
 if r['id']=='acquire_stow':
  r.update(status='current_driver_full_endpoint_repair_not_live',limit='Actual20284 freshUP and singleGrip yielded no matching held; premature observer-region stopping in driver source corrected. New full endpoint/5mm observer10+driver17 only offline, no runtime yet. Prior actual pickups retained; stow/restore/full stability open.')
  r['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
p=A/'active-runtime-wait.json';w=json.loads(p.read_text());w.update(runner_last_poll='43764 terminal exit1 with normal owned cleanup; no new runtime started',terminal=True,game_pid=20284,gameplay_result_available=True,next=audit['immediate_next']);p.write_text(json.dumps(w,indent=2))
print('20284 terminal and failure preserved; endpoint-baseline-r1 compiled/offline only, all13 requirements remain')
