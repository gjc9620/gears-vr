from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-24648-media-review/prepared.json';m=json.loads(p.read_text())
assert m['frames']==149 and m['sample_count']==48 and not m['capture_errors']
m.update(overview_reviewed=True,findings='Personally reviewed all3 full-span overview pages/48 paired samples. Actual scene throughout overview; hands/shoulder draw/held HB visible. Repeated near-eye oversized gun, intermittent gun visibility and hand/arm overlap remain; no firing/reload pixel acceptance, all-angle fit or headset proof.',limitation='48 sampled pairs of149 frames, not every frame or all contact angles.')
p.write_text(json.dumps(m,indent=2))
p=A/'runtime-24648-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==0 and d['native_cleanup_verified'] and d['xr_exit_code']==0 and d['stop_exit']==0
assert not d['foreground_violations'] and not d['cloak_violations']
pickup=d['core']['pickup_input'];assert pickup['baseline']['frame']=='130' and pickup['held']['frame']=='132' and 'first_source_gap' not in pickup
pg=ROOT/'tools/playground/build-hb-cloak-1111-034331/playground-24648.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
r2=[r for r in rows('native_lancer_r2') if 223<=int(r.get('frame','0'))<=226]
first=next(r for r in r2 if r['frame']=='224');next_frame=next(r for r in r2 if r['frame']=='225');fresh=next(r for r in r2 if r['frame']=='226')
deadline=int(first['input_sync_ns'])+250000000
assert first['status']=='10' and next_frame['status']=='8' and fresh['owned']=='0'
assert len(d['hammerburst_press_timing_rows'])==1 and d['hammerburst_press_timing_rows'][0]['invoked']=='0'
assert not rows('hammerburst_release_handoff') and not rows('native_hammerburst_first_captured')
d.update(first_press_frame_rows=r2,pickup_driver_live_exercised=True,
 first_press_failure=dict(original_edge_sync_ns=int(first['input_sync_ns']),original_edge_deadline_ns=deadline,
 next_admission_ns=int(next_frame['input_admission_ns']),next_admission_after_deadline_ns=int(next_frame['input_admission_ns'])-deadline,
 next_source_age_at_observation_ns=int(next_frame['observed_ns'])-int(next_frame['input_sync_ns']),
 scope='224 entry source76.302ms plus entry2.317/asset6.3101/contract3.5035/schema.0796ms cannot meet final20ms reserve. Original same press defer;225 admitted25.1956ms after original250ms edge expiry, blocks.226 fresher frame cannot replay. No original ButtonPress, first capture or ray.'),
 release_handoff_live_exercised=False,reload_live_exercised=False,
 next_source_scope='Inspect duplicate HB press asset reflection against existing exact-current MuzzleSample.asset/ready_for live checks. Do not simply remove fresh type/selector/hidden/attachment/identity or contract checks; prove equivalence or implement validated same-frame preparation. No widening/retiming/replay.')
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-24648-first-failure-analysis.json',candidate_runtime_status='release-handoff-r1 actual24648 pickupUP130/held132 succeeds; press224 reserve failure then same-edge defer expiry225, zero rays. New release handoff not reached; neither proved nor disproved live. Normal cleanup.',immediate_next=d['next_source_scope'])
e=A/'runtime-24648-first-failure-analysis.json'
for r in audit['requirements']:
 if r['id'] in ('acquire_stow','finite_fire'):r['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
 if r['id']=='finite_fire':r.update(status='partial_current_first_press_preparation_expiry',limit='Actual24648 first224 final20ms reserve failed before ButtonPress;225 same-edge250ms intent expired, zero rays. Actual22896 first1/tail failure separate. New exact real-UP handoff430 fake checks, not exercised game. Full finite6/two-hand/stability unaccepted.')
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
print('Actual24648 terminal evidence saved: first preparation expiry; handoff NOT reached; full objective remains open')
