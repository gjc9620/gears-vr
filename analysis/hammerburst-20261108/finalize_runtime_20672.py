from pathlib import Path
import json
A=Path(__file__).resolve().parent
p=A/'runtime-20672-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,full_fit_accepted=False,
 limitation='Personally reviewed three full-span overview pages/48 paired samples, not every frame or all angles. No capture error; no headset acceptance.',
 findings='Actual held gun/fire visible, not waiting cards. Near-face obstruction/overlap and intermittent lowered/disappearing close gun appearances remain. No single geometry/renderer diagnosis; complete fitting and continuity unaccepted.')
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(latest_runtime_failure='runtime-20672-first-failure-analysis.json',
 candidate_runtime_status='feed-observe-r1 actual20672: core6+five drain6 then1 ray of sixth burst; service gate4 pending original event release_required, total37 consistent rays. Normal cleanup. No reload/feed observer reached. Original250ms/100ms retained. Prior54452 seating failure still open.',
 immediate_next='One separate focused loaded-replacement diagnostic through unchanged production code to obtain insertion endpoint/kernel/gap witnesses without twelve prerequisite drain bursts. This does not accept empty reload or close intermittent firing deadline failure. Full original requirements retained.')
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
