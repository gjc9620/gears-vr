from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(current_candidate='feed-history-r1: original producer intermediate motion ingestion; full PG compiled;21 real-kernel +22 exact consumer local fixtures passed, no game yet',
 prepared_sha256=hashlib.sha256((A/'feed-history-r1/prepared.json').read_bytes()).hexdigest(),
 immediate_next='One fresh focused loaded replacement on feed-history-r1. Require actual used history marker and original seating/closure/typed conservation/return-fire. Preserve all guards; retain original pending-event expiry and full empty flow as open requirements.')
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
