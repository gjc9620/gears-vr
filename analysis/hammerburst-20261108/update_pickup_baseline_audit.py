from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(current_candidate='pickup-baseline-r1: full PG compiled,9 exact observer fixtures and11 driver receipt fixtures passed; actual shoulder UP required before one Grip, no game yet',
 prepared_sha256=hashlib.sha256((A/'pickup-baseline-r1/prepared.json').read_bytes()).hexdigest(),
 immediate_next='One fresh focused pickup/replacement test. Require actual Source1 shoulder released-baseline marker after UP command, one distinct Grip, matching actual held receipt before moving away. First source gap aborts, no retry or deadline renewal. Then reach actual early reload/history/seating/closure/typed return; all13 requirements open.')
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
