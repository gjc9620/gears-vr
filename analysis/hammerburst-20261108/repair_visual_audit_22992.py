"""Attach already reviewed visual evidence to the real requirement IDs.

Do not rerun the historical runtime finalizer: it would overwrite the newer
prelaunch-failure status. No runtime, game input, new fit acceptance or score.
"""
from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;ROOT=A.parents[1];P=A/'endpoint-baseline-r1/source/tools'
result=A/'runtime-22992-focused-result.json';review=A/'runtime-22992-media-review/prepared.json'
r=json.loads(result.read_text());m=json.loads(review.read_text())
assert r['focused_actions_passed'] and not r['full_goal_complete'] and m['overview_reviewed']
assert len(m['contact_images_reviewed'])==6 and m['sample_count']==48
for p in m['contact_images_reviewed']:assert hashlib.sha256(Path(p['path']).read_bytes()).hexdigest()==p['sha256']
files=('vr-common/weapon_support_socket_mode.h','vr-common/weapon_hammerburst_foregrip.h','playground/weapon_lancer_grasp.inl','playground/weapon_native_lancer_follow.inl')
source={f:(P/f).read_text() for f in files}
mode=source[files[0]];grasp=source[files[2]];follow=source[files[3]]
assert 'Snap never grants a lease' in mode and 'd.lease.id==f.press_token' in mode
assert 'gears_weapon_hammerburst_support::mode' in grasp and 'Mode::snap' in grasp
assert 'q.kind=api::Kind::support;q.proximity=1;' in grasp
assert 'decision.lease.kind==hand::LeaseKind::support' in grasp
assert 'snapshot_known&&snapshot.grip==hand::Grip::two' in follow
report=dict(scope='Static PG source boundary review only; no new Native/game/headset evidence',source_sha256={f:hashlib.sha256((P/f).read_bytes()).hexdigest() for f in files},findings=[
 'HB passive branch requires neutral snap mode and publishes proximity1, not an active lease.',
 'Active support branch requires current valid matching support lease and original held control.',
 'PG fallback two-hand steering requires current snapshot Grip::two; neutral source alone does not enter it.',
 'HB foregrip surface remains explicitly provisional. No proof of exact Native C0 skin consumer/complete visual contact or current combined four-state test follows from this source review.'
],runtime_tests_rerun=False,production_changed=False,grasp_or_finger_tuning_performed=False,old_visual_evidence_scope='48 overview paired samples plus6 original contacts previously personally viewed; no new all-angle proof')
report_path=A/'foregrip-source-boundary-review-20261111.json';report_path.write_text(json.dumps(report,indent=2))
p=A/'full-requirements-audit-20261110.json';audit=json.loads(p.read_text());index={x['id']:x for x in audit['requirements']}
assert len(index)==13 and 'part_fitting' in index and 'guides' in index and 'fivepartfitting' not in index
ev=dict(path=str(result),sha256=hashlib.sha256(result.read_bytes()).hexdigest())
for name in ('part_fitting','guides'):
 target=index[name]
 if not any(x.get('path')==ev['path'] for x in target['evidence']):target['evidence'].append(ev)
index['part_fitting'].update(status='unaccepted',limit='Current22992 mechanically successful loaded chain still shows near-right-eye oversized gun, ghost/intersection and open/separated offhand.48 paired overview samples+6 contact images reviewed; not full five-part skin/surface/all-angle/headset acceptance. Preserve logic→Snap/Volume→GrabPose order; no finger tuning begun here.')
index['guides'].update(status='unaccepted_visual',limit='Current22992 sampled frames show guide wire geometry/labels during reload, but small text readability and complete step-by-step/action alignment remain unaccepted. Mechanical success is not visual guide acceptance.')
ev2=dict(path=str(report_path),sha256=hashlib.sha256(report_path.read_bytes()).hexdigest())
if not any(x.get('path')==ev2['path'] for x in index['foregrip_states']['evidence']):index['foregrip_states']['evidence'].append(ev2)
audit['visual_audit_recording_repair']='Corrected finalizer wrong ID fivepartfitting→part_fitting, attached reviewed22992 evidence also to guides, asserted target IDs exist. No requirement/score/acceptance change and no overwrite of current052600 runtime blocker.'
assert not audit['completion_proven'];p.write_text(json.dumps(audit,indent=2,ensure_ascii=False))
print('Correct requirement IDs now reference current reviewed visual failures; source boundary review adds no new runtime acceptance')
