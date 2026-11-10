from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent
p=A/'runtime-57960-media-review/prepared.json';m=json.loads(p.read_text())
assert m['frames']==106 and m['sample_count']==48 and not m['capture_errors']
m.update(overview_reviewed=True,findings='Personally reviewed all3 full-span overview pages/48 paired samples. Actual scene and shoulder-reaching arm visible; initial ghost/overlap and hand arm intersection remain. No successful held HB, shot or reload accepted. Not all frames/all angles/headset.',limitation='48 paired overview samples of106 frames; no action acceptance from overview.')
p.write_text(json.dumps(m,indent=2))
p=A/'runtime-57960-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==0 and d['native_cleanup_verified'] and d['xr_exit_code']==0 and d['stop_exit']==0
up=d['core']['pickup_input']['up_sent_ns'];markers=d['hammerburst_pickup_up_rows']
assert len(markers)==16 and all(int(r['sync_ns'])<up and int(r['admitted_ns'])<up for r in markers)
assert set(d['core']['pickup_input'])=={'up_sent_ns'} and not d['hammerburst_press_timing_rows']
d.update(pickup_failure=dict(up_sent_ns=up,marker_count=16,last_marker=markers[-1],last_marker_admitted_before_command_ns=up-int(markers[-1]['admitted_ns']),cause='Native bounded marker budget exhausted before the driver sent its own UP, because driver reached and settled first. No fresh post-command UP marker can be emitted; no Grip sent.'),
 driver_repair='Send actualUP before existing finite reach; observe during gesture, stop remaining UP gesture samples on fresh Native marker, update current test-hand pose then oneGrip. Default later reaches preserve original full gesture and settle; no Native budget/age/geometry changes.',
 release_handoff_live_exercised=False,press_asset_timing_live_exercised=False)
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-57960-first-failure-analysis.json',candidate_runtime_status='press-assets-r1 actual57960:16 shoulder-UP marker rows all predate driverUP; noGrip sent, no acquisition/press/timing/handoff/reload. Driver now sendsUP before reach and consumes fresh marker during gesture;16 fixtures +8 adapters compile, not live yet.',immediate_next='One fresh instance with repaired driver and SAME compiled press-assets-r1; actual caller changed, not unchanged replay. Obtain successful currentUP/oneGrip then split press timing. Full13 requirements remain.')
e=A/'runtime-57960-first-failure-analysis.json'
for r in audit['requirements']:
 if r['id']=='acquire_stow':
  r.update(status='current_driver_marker_cap_failure_repaired_not_live',limit='Prior13492/22896/24648 real pickup success retained.57960 noGrip because16 UP rows all predate own command; actual driver now observes during reach,16 fixture checks only. Stow/restore/full stability remain open.')
  r['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
print('57960 actual marker-cap failure preserved; driver repair not yet game verified')
