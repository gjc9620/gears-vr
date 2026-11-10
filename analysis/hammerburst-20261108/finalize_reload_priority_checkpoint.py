from pathlib import Path
import difflib,hashlib,json
import prepare_reload_priority as prepare
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
patch=''
for rel in ('tools/playground/weapon_equipment_backend.inl','tools/playground/weapon_hammerburst_runtime.inl'):
 before=(prepare.BASE/'source'/rel).read_text();after=(prepare.P/'source'/rel).read_text()
 patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
(ROOT/'docs/hammerburst-reload-priority.patch').write_text(patch)
p=A/'runtime-53152-media-review/prepared.json';d=json.loads(p.read_text())
d.update(overview_reviewed=True,
 findings='Personally reviewed all3 full-span paired overview pages/48 samples. Actual scene/acquisition/six-shot firing and attempted latch visible. Near-face obstruction/white overlap/intermittent close-gun visibility remain. No opening/insertion action reached. Full fit/continuity/headset false.',
 limitation='376 frames/no capture errors;48 paired samples, not every frame/all angles.')
p.write_text(json.dumps(d,indent=2))
p=A/'runtime-53152-first-failure-analysis.json';d=json.loads(p.read_text());d['media'].update(encoded=True,encode_exit_code=0);p.write_text(json.dumps(d,indent=2))
p=A/'full-requirements-audit-20261110.json';d=json.loads(p.read_text())
d.update(current_candidate='reload-priority-r1: exact HB reload once before follow visuals; compiled+9 local schedule cases; retains feed-history43 local checks; no game yet',
 prepared_sha256=hashlib.sha256((prepare.P/'prepared.json').read_bytes()).hexdigest(),latest_runtime_failure='runtime-53152-first-failure-analysis.json',
 candidate_runtime_status='Actual53152 press-timing-r1 core6 passed, entry age60.337ms and stage durations recorded. Initial Native latch magazine grant Frame280; source stale101.0049ms before opening. No feed history reached; first-press stability unaccepted.',
 immediate_next='One fresh focused replacement with HB once-before-visual scheduling. Require actual early reload marker, first latch grant, history adoption/seating/closure/typed conservation/return. Preserve all deadlines, no same-Frame retry. Full13 requirements remain open.')
p.write_text(json.dumps(d,indent=2,ensure_ascii=False))
