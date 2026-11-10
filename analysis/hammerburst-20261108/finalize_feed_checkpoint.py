"""Record personally inspected media and compiled local parity evidence."""
from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
p=A/'runtime-54452-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,raw_contacts_reviewed=['frame-00326.jpg','frame-00468.jpg','frame-00524.jpg','frame-00595.jpg'],
 findings='Personally inspected all three full-span overview pages/48 paired samples and four original paired contact images. Actual game, held gun, latch/extraction/replacement and guide visible. Near-face obstruction, overlapping/white geometry remain; complete contact, continuity and all-angle fitting not accepted.',
 limitation='48 full-span samples and four original frames, not every frame/all angles. Recording has667 frames and no capture errors; no physical-headset acceptance.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-54452-first-failure-analysis.json';d=json.loads(p.read_text());d['media']['encoded']=True
d['media']['encode_exit_code']=0;p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(current_candidate='feed-observe-r1: existing insertion outcome, sample-gap rearm and early/move gates; compiled; exact before/after8 scenarios84 paired samples using real FeedInsertionStroke',
 prepared_sha256=hashlib.sha256((A/'feed-observe-r1/prepared.json').read_bytes()).hexdigest(),
 latest_runtime_failure='runtime-54452-first-failure-analysis.json',
 candidate_runtime_status='Actual54452 part-frame-r1: core6+12six-shot drain to0/R78,78 consistent rays, empty veto; both handoff markers exercised, zero support withdrawal/identity-lost. Stage6 seating failed within8s; endpoint cause unknown. New observation candidate not yet game executed.',
 immediate_next='One fresh feed-observe-r1 attempt through canonical isolated runtime. Preserve original source100ms/event250ms, geometry and all ownership guards. Use actual insertion result/gap/move witnesses to choose repair; no unchanged part-frame rerun.')
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
print('Recorded54452 media review and full-scope audit; feed observation remains unvalidated in game')
