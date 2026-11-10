from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-13492-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,
 findings='Personally reviewed all3 full-span overview pages/48 paired samples. Actual hands, shoulder acquisition and held HB visible. Near-camera overlap/white geometry/intermittent gun visibility remain; no firing/reload accepted, no full contact/all-angle/headset proof.',
 limitation='168 recorded frames/errors empty,48 paired samples reviewed, not every frame/all angles.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-13492-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0)
pickup=d['core']['pickup_input'];assert pickup['baseline']['frame']=='110' and pickup['held']['frame']=='112'
assert int(pickup['baseline']['sync_ns'])>=pickup['up_sent_ns'] and int(pickup['held']['input_sync_ns'])>=pickup['down_sent_ns']
assert int(pickup['held']['press'])==int(pickup['baseline']['right_pressed'])+1
assert 'first_source_gap' not in pickup and d['actual_trajectory_count']==0
d['pickup_driver_live_exercised']=True
pg=ROOT/'tools/playground/build-hb-cloak-1111-030749/playground-13492.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
rows=[dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith('native_lancer_r2 ') and ('frame=180 ' in l or 'frame=181 ' in l or 'frame=182 ' in l)]
d['first_event_frame_rows']=rows
f181=next(r for r in rows if r['frame']=='181')
sync=int(f181['input_sync_ns']);observation=int(f181['observed_ns']);admission=int(f181['input_admission_ns'])
waits=d['native_hammerburst_first_wait_rows'];delivered=next(r for r in waits if r.get('phase')=='deliver' and r.get('delivery')=='8')
expiry=next(r for r in waits if r.get('phase')=='service' and r.get('cancel')=='1' and int(r.get('event_captured_ns','0'))>0)
d['first_event_failure']=dict(frame=181,source_sync_ns=sync,after_game_observation_age_ns=observation-sync,
 input_admission_age_ns=admission-sync,delivery_gate8_age_ns=int(delivered['observed_ns'])-sync,
 event_captured_ns=int(delivered['event_captured_ns']),expiry_observed_age_ns=int(expiry['observed_ns'])-int(expiry['event_captured_ns']),
 source_read_and_later_work='After game observation79.9266ms -> input admission82.4843ms -> delivery gate8 at83.9367ms. Original20ms reserve still required. Original event expired at254.3478ms at next service.',
 scope='Latest withdrawal frame177 predates this fresh frame181 and is not the causal181 source-expiry receipt. No first shot delivered; no input retry.')
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-13492-first-failure-analysis.json',
 candidate_runtime_status='pickup-baseline-r1 actual13492: fresh UP110 +one Grip+matching held112 before move succeeded. First press invoked180; pending original first event unable to deliver at83.9367ms source age on181 (20ms reserve), expired next service254.3478ms old, zero rays. No reload priority/history reached. Normal cleanup.',
 immediate_next='Implement early scheduling of ONLY already-owned/validated pending HB FIRST event on same original held trigger/counters/cohort, immediately after current muzzle preparation before read_game/aim/input overhead. Keep existing deliver/native/current/20ms/100ms/250ms/claim checks; at most one delivery attempt per Frame. Test actual adapter UP/new-edge/cohort/cancel/expiry/no double dispatch, build, save, then one fresh focused run. All13 requirements retained.')
for r in audit['requirements']:
 if r['id']=='acquire_stow':
  r.update(status='current_fresh_baseline_pickup_observed_stow_open',limit='Actual13492 required released Native baseline110, issued one Grip, observed matching held112 before moving away. No source-gap retry. Core fire then failed independently; whole stability/stow/restore remain open.')
  f=A/'runtime-13492-first-failure-analysis.json';r['evidence'].append(dict(path=str(f),sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
 if r['id']=='finite_fire':
  r.update(status='partial_current_first_event_expiry',limit='Actual13492 original ButtonPress invoked180 but actual queued first event could not meet20ms reserve at delivery181/source83.9367ms and expired at next service254.3478ms; zero rays. Need earlier existing-first scheduling, no source/expiry widening. Previous full bursts remain separate; two-hand/full stability unaccepted.')
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
