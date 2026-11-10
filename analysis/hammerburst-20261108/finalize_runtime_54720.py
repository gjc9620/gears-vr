from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-54720-media-review/prepared.json';m=json.loads(p.read_text())
assert m['frames']==152 and m['sample_count']==48 and not m['capture_errors']
m.update(overview_reviewed=True,findings='Personally reviewed all3 full-span overview pages/48 paired samples. Actual scene in every sampled pair including first; shoulder draw and held HB visible. Large near-right-eye gun, intermittent missing/ghost/overlap remain. No shot/reload or all-angle/five-part/headset acceptance.',limitation='48 sampled pairs of152 frames; not all frames/all angles. Startup gate and exact pickup receipt accepted only at their narrow observed scope.')
p.write_text(json.dumps(m,indent=2))
p=A/'runtime-54720-first-failure-analysis.json';d=json.loads(p.read_text())
assert d['actual_trajectory_count']==0 and d['native_cleanup_verified'] and d['xr_exit_code']==0 and d['stop_exit']==0
assert not d['foreground_violations'] and not d['cloak_violations']
pickup=d['core']['pickup_input'];assert pickup['baseline']['frame']=='148' and pickup['held']['frame']=='151' and 'first_source_gap' not in pickup
assert pickup['shoulder_endpoint']['x']==.23 and pickup['shoulder_endpoint']['z']==-.17
assert int(pickup['baseline']['sync_ns'])>=pickup['up_sent_ns'] and int(pickup['held']['input_sync_ns'])>=pickup['down_sent_ns']
action=ROOT/'analysis/instances/hb-cloak-1111-043643/actions/menu-firstperson-54720-044259';record=json.loads((action/'recording.json').read_text())
events=[e for e in record['events'] if e.get('label') in ('native_hand_connection_ready','HB_real_projection_ready','HB_first_action_capture_ready')]
assert [e['label'] for e in events]==['native_hand_connection_ready','HB_real_projection_ready','HB_first_action_capture_ready']
pg=ROOT/'tools/playground/build-hb-cloak-1111-043643/playground-54720.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
down=next(r for r in rows('hammerburst_input_consumed') if r['frame']=='239')
assert down['current']=='0' and down['active_at_admission']=='0' and down['owned']=='0'
r2=[r for r in rows('native_lancer_r2') if 240<=int(r.get('frame','0'))<=241]
assert r2[0]['status']=='1' and r2[1]['status']=='8' and not d['hammerburst_press_timing_rows']
d.update(real_projection_gate_live_exercised=True,startup_events=events,full_endpoint_pickup_live_exercised=True,
 first_down_failure=dict(row=down,source_age_at_input_admission_ns=int(down['admitted_ns'])-int(down['input_sync_ns']),next_fresh_rows=r2,
  interpretation='239 actualDOWN source104.8766ms at Input admission/current0: correctly sealed.240 fresh frame only baseline,241 blocked8 cannot replay. No ButtonPress/preparation timing or event capture reached. Prior helper mislabeled later blocked242 as consumedDOWN and waited for nonexistent shot.'),
 driver_change='UP-only test readiness additionally requires original producer<=60ms at observation, preparation_budget1, and admission>=producer. Exactly one realDOWN; blocked8 receipt immediately fails/releases without retry. Production100ms/20ms/250ms and all source/anti-replay checks unchanged; no full firing stability claim.',
 press_asset_timing_live_exercised=False,release_handoff_live_exercised=False,reload_live_exercised=False)
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text())
audit.update(latest_runtime_failure='runtime-54720-first-failure-analysis.json',candidate_runtime_status='endpoint-baseline-r1 actual54720 bound real-projection gate +full endpointUP148/singleGrip/held151 exercised. ActualDOWN239 stale104.8766ms seals;240 fresh baseline/241blocked cannot replay. Zero first/rays/timing/handoff/reload. Normal cleanup.',immediate_next='Save UP-only stricter60ms/budget test setup and explicit blockedDOWN failure (9 helper tests), then fresh instance with same compiled candidate. First resolve workspace disk margin using only verified-terminal own cache archive with SHA/junction safeguards. Preserve all13 requirements.')
e=A/'runtime-54720-first-failure-analysis.json'
for r in audit['requirements']:
 if r['id'] in ('acquire_stow','finite_fire','delivery_headset'):r['evidence'].append(dict(path=str(e),sha256=hashlib.sha256(e.read_bytes()).hexdigest()))
 if r['id']=='acquire_stow':r.update(status='current_full_endpoint_pickup_observed_stow_open',limit='Actual54720 full original commanded endpoint +freshUP148 +oneGrip +matching held151 before moving away. No source-gap retry. Single run only; stow/restore/full stability remain open.')
 if r['id']=='finite_fire':r.update(status='current_first_down_source_expiry',limit='Actual54720 DOWN239 source104.8766ms/current0 correctly sealed before ButtonPress; fresh240 baseline and241 blocked cannot replay. Test UP baseline stricter60ms/budget now9 synthetic tests only. Actual first/tail/reload unaccepted.')
assert len(audit['requirements'])==13 and not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
p=A/'active-runtime-wait.json';w=json.loads(p.read_text());w.update(terminal=True,game_pid=54720,runner_last_poll='5500 terminal exit1, normal cleanup, no restart',next=audit['immediate_next']);p.write_text(json.dumps(w,indent=2))
print('Actual54720 startup and endpoint pickup live, firstDOWN stale failure preserved, no goal completion')
