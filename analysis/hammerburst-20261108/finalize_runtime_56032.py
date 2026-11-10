from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-56032-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,
 findings='Personally viewed all3 full-span overview pages/48 paired samples. Actual game/hands and brief near-right gun appearance visible; no matching completed HB acquisition receipt, shooting or reload proven. Close-gun visibility intermittent; full fit/continuity/headset false.',
 limitation='163 recorded frames/no capture errors;48 paired samples, not every frame/all angles; transient gun pixels are not authoritative held-state evidence.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-56032-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0)
pg=ROOT/'tools/playground/build-hb-cloak-1111-021724/playground-56032.log';b=pg.read_bytes();lines=b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf8',errors='replace').splitlines()
for prefix in ('native_equipment','native_equipment_frame_read','native_equipment_input_gap'):
 d[prefix+'_rows']=[dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in lines if l.startswith(prefix+' ')]
d['cause_limit']='current_after means the existing current(nullptr,frame) check rejected after inventory snapshot. This record alone does not isolate stale input versus game/identity invalidation or total inventory-read cost.'
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(latest_runtime_failure='runtime-56032-first-failure-analysis.json',
 candidate_runtime_status='reload-priority-r1 actual56032 reached game but no matching initial HB acquisition receipt. Inventory rows include current_after rejection; exact cause unproven. Zero rays/reload/early-priority/history rows. Candidate43 feed+9 schedule local checks remain separate. Normal cleanup.',
 immediate_next='Inspect first acquisition inventory current_after and original right-shoulder source/edge/receipt timing from56032. Any targeted observation must retain original current/identity/freshness/selection checks. Do not rerun unchanged candidate, retry blocked edge, or call priority/history exercised. Full13 requirements remain open.')
for r in d['requirements']:
 if r['id']=='acquire_stow':
  r.update(status='partial_latest_acquisition_unverified',limit='Earlier54452/53152 acquisition and original fire observed. Latest56032 no matching HB held receipt; inventory current_after rejection, cause unproven. Brief gun pixels do not prove acquisition. Full stow/restore unverified.')
  f=A/'runtime-56032-first-failure-analysis.json';r['evidence'].append(dict(path=str(f),sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
assert len(d['requirements'])==13 and not d['completion_proven'];p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
