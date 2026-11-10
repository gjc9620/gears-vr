from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent
p=A/'runtime-53560-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,raw_contacts_reviewed=['frame-00398.jpg','frame-00445.jpg','frame-00491.jpg'],
 findings='Personally viewed three full-span overview pages/48 paired samples and three original contact frames. Actual gun/latch/magazine/guide visible. Near-face obstruction, white/overlapping geometry and intermittent close gun visibility remain; complete five-part/all-angle fitting not accepted.',
 limitation='551 recorded frames without capture errors; reviewed48 samples plus3 originals, not every frame/all angles or a physical headset.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-53560-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0)
p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text());proof=A/'runtime-53560-first-failure-analysis.json'
d.update(latest_runtime_failure=proof.name,
 candidate_runtime_status='Actual53560 focused loaded replacement: core6, original72/R78 reload, latch/extract/supply to6. Observed kernel approaching; Frame787 gap110.739ms resets stroke at41.923mm below45mm approach. Endpoint depth.011326mm/lateral.005797mm/orientation1 rejected. No move/early/support/identity reject. Normal cleanup; not empty-flow validation.',
 immediate_next='Implement/test authenticated original producer intermediate-motion ingestion for HB insertion, preserving current Frame/grant/identity and all per-sample100ms/geometry/monotonic checks. Prove exact current endpoint, same held controls/mount/publisher, no fabricated poses/timestamps, and atomic rejection of missing/changed history. Do not rerun unchanged feed-observe candidate. Original event expiry and full13 requirements remain open.')
for r in d['requirements']:
 if r['id']=='replacement':
  r.update(status='current_seating_sample_gap_failure_proven',
    limit='Actual53560 loaded replacement reached6; approaching before787 then110.739ms gap rearmed below45mm, endpoint aligned at.011326mm depth rejected. No move/early/support/identity reject. Need authenticated real intermediate-motion ingestion; no geometry or deadline widening. Historical56252 remains separate.')
  r['evidence'].append(dict(path=str(proof),sha256=hashlib.sha256(proof.read_bytes()).hexdigest()))
assert len(d['requirements'])==13 and not d['completion_proven'];p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
