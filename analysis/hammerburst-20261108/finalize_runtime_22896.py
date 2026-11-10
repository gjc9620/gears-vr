from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-22896-media-review/prepared.json';d=json.loads(p.read_text())
assert d['frames']==161 and not d['capture_errors'] and d['sample_count']==48
d.update(overview_reviewed=True,findings='Personally reviewed all3 full-span overview pages/48 paired samples. Actual scene throughout sampled overview, hands and shoulder acquisition, held HB and presentation motion visible; early arm/hand/gun overlap and near-camera right gun remain. Not all frames, all-angle five-part fitting, action-pixel or headset acceptance.',limitation='48 paired overview samples of161 recorded frames; complete fit remains unaccepted.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-22896-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==1 and d['trajectory_consistent'] and d['native_cleanup_verified'] and d['xr_exit_code']==0
assert d['operator_exit']==1 and d['stop_exit']==0 and not d['foreground_violations'] and not d['cloak_violations']
pickup=d['core']['pickup_input'];assert pickup['baseline']['frame']=='129' and pickup['held']['frame']=='131'
assert int(pickup['baseline']['sync_ns'])>=pickup['up_sent_ns'] and int(pickup['held']['input_sync_ns'])>=pickup['down_sent_ns']
assert 'first_source_gap' not in pickup
pg=ROOT/'tools/playground/build-hb-cloak-1111-033308/playground-22896.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
r2=[r for r in rows('native_lancer_r2') if 204<=int(r.get('frame','0'))<=215]
first=next(r for r in rows('native_hammerburst_original_delivered') if r['first']=='1')
assert first['frame']=='206' and first['original_count']=='1'
up=next(r for r in r2 if r['frame']=='207');assert up['owned']=='0' and up['trigger_value']=='0.000' and up['trigger_released']=='1'
loss=d['hammerburst_service_loss_rows'];assert len(loss)==1 and loss[0]['gate']=='4' and loss[0]['count']=='1'
d.update(first_delivery_rows=rows('native_hammerburst_original_delivered'),first_and_release_frame_rows=r2,pickup_driver_live_exercised=True,
 first_event_observation='Actual original first206 succeeded. Existing owned HB first qualifies for postponed L2; original Input gates precede delivery. Single non-matched run is not causal/stability proof.',
 tail_failure='Real UP207 consumed; service gate4 pending expiry at count1, no further rays. Source withdrawal208 age101.3219ms observed separately. Pending refire capture clock/validation at207 not logged, so exact cause unproven.',
 next_source_evidence='Actual frozen adapter322 checks reproduce service-before-UP validated state being cleared on genuine release; no reservice in current after_tick/deliver path. Same pending event with full reservice before expiry succeeds, after250ms cancels without ray. Fakes do not prove game207 pending identity/timing.')
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-22896-first-failure-analysis.json',candidate_runtime_status='first-priority-r1 actual22896: freshUP129/singleGrip/held131, originalfirst206 and one consistent ray; realUP207, pending tail expired at count1 gate4. No reload reached. Normal cleanup. Not full first stability or finite6 acceptance.',immediate_next='Repair exact successful real-UP handoff: fully revalidate existing pending HB refire after Input release and before original delivery, with no new capture/time/source/permit. First add actual adapter regression including UP/new-edge/cancel/cohort/expiry/no-double cases. Preserve original release checks; build/save before fresh runtime. Record original pending capture clock/serial at handoff to resolve actual timing cause. All13 requirements remain.')
e=A/'runtime-22896-first-failure-analysis.json'
for r in audit['requirements']:
 if r['id'] in ('acquire_stow','finite_fire'):
  r['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
 if r['id']=='acquire_stow':r.update(status='current_fresh_baseline_pickup_observed_stow_open',limit='Actual13492 and22896 fresh UP baseline followed by one matching Grip/held before moving. Stow/restore/full stability remain open.')
 if r['id']=='finite_fire':r.update(status='partial_current_first1_tail_expiry',limit='Actual22896 originalfirst206 one consistent ray; realUP207 then remaining finite burst expired at count1 gate4. Current first stability/two-hand/finite6 unaccepted. Offline322 exact adapter checks identify release validation loss path but not sole game cause.')
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
print('Actual22896 evidence finalized:1 ray, finite6 failed, normal cleanup,13 requirements retained')
