"""Reconcile individual requirement entries with current actual failures."""
from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent
p=A/'runtime-20672-first-failure-analysis.json';d=json.loads(p.read_text())
assert (A.parents[1]/'analysis/instances/hb-cloak-1111-013055/actions/menu-firstperson-20672-013122/stereo-demo.mp4').exists()
d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
updates={
 'finite_fire':('partial_intermittent_original_event_expiry',
  'Actual54452 core6+12 drain6 reached0/R78 with78 consistent rays. Actual20672 core6+5 drain6 then1 of sixth, service gate4 original pending-event expiry;37 consistent rays. Current finite-fire stability and two-hand acceptance remain unproven.',[54452,20672]),
 'replacement':('partial_current_seating_failed',
  'Historical56252 replacement/closure observed. Current54452 both exact-source latch/supply handoffs exercised, zero support withdrawal/identity-lost; stage6 did not seat within8s. Endpoint/kernel/gap cause unknown.20672 failed before reload and provides no seating evidence.',[54452,20672]),
 'empty_reload':('partial_current_seating_failed',
  'Current54452 actual drain to0/R78, empty-trigger veto, original begin/latch/extraction and replacement acquisition passed; no seating/closure/return-fire. Historical56252 six return rays lacked final typed72/0. Full empty flow remains failed.',[54452]),
 'acquire_stow':('partial',
  'Actual54452 and20672 fresh shoulder acquisition reached original firing. Complete stow and current restore chain still need matching live evidence.',[54452,20672]),
 'part_fitting':('unaccepted',
  'Reviewed54452:48 paired full-span samples+4 originals;20672:48 paired full-span samples. Near-face obstruction/overlap/white geometry persist; no all-angle or five-part surface/contact acceptance. Preserve owner order logic then Snap/Volume then GrabPose.',[54452,20672])}
for r in d['requirements']:
 if r['id'] not in updates:continue
 r['status'],r['limit'],games=updates[r['id']]
 existing={e['path'] for e in r['evidence']}
 for game in games:
  f=A/f'runtime-{game}-first-failure-analysis.json'
  if str(f) not in existing:r['evidence'].append(dict(path=str(f),sha256=hashlib.sha256(f.read_bytes()).hexdigest()))
assert len(d['requirements'])==13 and not d['completion_proven']
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
print('Updated5 requirement entries from actual54452/20672; all13 requirements preserved and completion remains false')
