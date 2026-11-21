"""Adopt selected real LEFT Lancer captures, including their component-local wrists.

Offline only. Does not install, launch, contact a device or edit the right hand.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import numpy as np
import transfer as t

A = np.array([[-1., 0., 0.], [0., 0., 1.], [0., -1., 0.]])
S = np.diag([-1., 1., 1.])
PARTS = ('support', 'magazine', 'charging', 'starter')


def rotation(q):
    x, y, z, w = t.normalized(q)
    return np.array([[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
                     [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
                     [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]])


def quaternion(r):
    # Proper matrix -> quaternion, with an independent matrix round-trip.
    if not np.isfinite(r).all() or np.max(abs(r.T@r-np.eye(3))) > 1e-7 or abs(np.linalg.det(r)-1) > 1e-7:
        raise ValueError('Non-rigid or reflected wrist rotation')
    tr = np.trace(r)
    if tr > 0:
        k = math.sqrt(1+tr)*2
        q = [(r[2, 1]-r[1, 2])/k, (r[0, 2]-r[2, 0])/k, (r[1, 0]-r[0, 1])/k, k/4]
    else:
        i = int(np.argmax(np.diag(r))); j, z = (i+1) % 3, (i+2) % 3
        k = math.sqrt(1+r[i, i]-r[j, j]-r[z, z])*2
        q = [0., 0., 0., 0.]; q[i] = k/4
        q[j] = (r[j, i]+r[i, j])/k; q[z] = (r[z, i]+r[i, z])/k
        q[3] = (r[z, j]-r[j, z])/k
    q = t.normalized(q)
    if np.max(abs(rotation(q)-r)) > 1e-7:
        raise ValueError('Wrist quaternion round-trip failed')
    return q


def convert_selected(project, model_hash, pivot):
    if project.get('schema') != 1 or project.get('kind') != 'quest_weapon_workbench' or project.get('rig_signature') != t.SIGNATURE:
        raise ValueError('Wrong project/rig')
    if project.get('model') != 'Lancer.glb' or str(project.get('model_content_fnv64')) != str(model_hash) or project.get('rig_joint_basis') != 'ue_wrist_reflect_x':
        raise ValueError('Wrong model or wrist basis')
    meta = project['capture']
    if meta['side'] != 0 or not project['calibration'][0]['known']:
        raise ValueError('Only calibrated selected left-hand captures are adopted')
    part = project['parts'][meta['part']]; key = part['id']
    if key not in PARTS:
        raise ValueError('This operation cannot change main/right grip')
    hand = next(h for h in part['hands'] if h['side'] == 'left')
    raw = hand['raw_joints_in_object']
    if not hand['retargeted'] or not hand['fully_tracked'] or hand['hand_data_source'] != 1 or hand['source_time_ns'] <= 0 or len(raw) != 26 or any(j['flags'] & 15 != 15 for j in raw):
        raise ValueError('Incomplete/inferred/obstructed hand capture')
    if len(hand['local_fingers_xyzw']) != 15 or len(hand['finger_correction_degrees']) != 15:
        raise ValueError('Incomplete fingers')
    fingers = []
    for q, correction in zip(hand['local_fingers_xyzw'], hand['finger_correction_degrees']):
        q = t.normalized(t.multiply(t.normalized(q), t.correction(correction)))
        fingers.append([q[0], -q[1], -q[2], q[3]])
    wrist = hand['wrist_in_object']
    xr_r = rotation(t.multiply(wrist['rotation_xyzw'], t.correction(hand['wrist_correction_degrees'])))
    xr_p = np.array(wrist['position_metres']) + xr_r@np.array(hand['wrist_offset_metres'])
    if not np.isfinite(xr_p).all() or np.linalg.norm(xr_p) > 2:
        raise ValueError('Invalid object-local wrist')
    # Object uses reflected (-X,Z,-Y); rig-local hand uses reflected (-X,Y,Z).
    # Both reflections are necessary. A rotation-only axis swap reverses hands.
    p, r = A.T@xr_p*100, A.T@xr_r@S
    original_p, original_r = p.copy(), r.copy()
    theta, travel = hand['starter_angle_radians'], hand['charging_distance_metres']
    if not math.isfinite(theta) or not 0 <= theta <= math.pi/2+1e-6 or not math.isfinite(travel) or not 0 <= travel <= .231:
        raise ValueError('Invalid captured mechanical state')
    if key == 'starter':
        dx = rotation([0., -math.sin(theta/2), 0., math.cos(theta/2)])
        delta = A.T@dx@A
        p, r = pivot + delta.T@(p-pivot), delta.T@r
        recovered_p, recovered_r = pivot+delta@(p-pivot), delta@r
    elif key == 'charging':
        offset = A.T@np.array([0., 0., travel])*100
        p = p-offset
        recovered_p, recovered_r = p+offset, r
    elif key == 'magazine':
        basis = np.array([[0., 1., 0.], [1., 0., 0.], [0., 0., -1.]])
        anchor = np.array([.00796632598, 28.14602802187, 4.39606437790])
        p, r = basis.T@(p-anchor), basis.T@r
        recovered_p, recovered_r = anchor+basis@p, basis@r
    else:
        recovered_p, recovered_r = p, r
    if np.max(abs(recovered_p-original_p)) > 1e-8 or np.max(abs(recovered_r-original_r)) > 1e-8:
        raise ValueError('Moving component conversion did not round-trip')
    return dict(part=key, capture_id=meta['id'], name=meta['name'], fingers=fingers,
                position_raw=p.tolist(), rotation_xyzw=quaternion(r),
                captured_angle=theta, captured_travel=travel,
                roundtrip_position_raw=float(np.max(abs(recovered_p-original_p))),
                roundtrip_rotation=float(np.max(abs(recovered_r-original_r))))


def values(xs):
    return ','.join(format(float(x), '.17g') for x in xs)


def header(rows, pivot):
    text = '''#pragma once
#include "pose_math.h"

// Owner's four real Quest LEFT snapshots, 2026-11-12. Generated by
// tools/quest-workbench/adopt_lancer_captures.py. Original raw archives retained.
// Static authoring only: never input, contact, tracking or action permission.
namespace gears_lancer_owner_grabposes {
'''
    names = {'support': 'support_wrist_in_gun', 'magazine': 'magazine_wrist_in_item',
             'charging': 'loading_wrist_in_gun', 'starter': 'starter_wrist_closed'}
    for key in PARTS:
        r = rows[key]
        text += '// '+r['capture_id']+'; captured C='+format(r['captured_angle'], '.17g')+'; E='+str(r['captured_travel'])+'m\n'
        text += 'inline constexpr gears_vr::Pose '+names[key]+'{{'+values(r['position_raw'])+'},{'+values(r['rotation_xyzw'])+'}};\n'
    text += 'inline constexpr gears_vr::Vector starter_pivot{'+values(pivot)+'};\n'
    text += '''inline bool world_wrist(gears_vr::Pose const& gun,double scale,
    gears_vr::Pose const& local,gears_vr::Pose& out) noexcept {
    const auto q=gun.orientation;const double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    if(!gears_vr::finite(gun.position)||!std::isfinite(scale)||scale<=0||
       !std::isfinite(norm)||std::abs(norm-1)>1e-6)return false;
    const auto d=gears_vr::transform(gears_vr::rotation_matrix(q),
        {local.position.x*scale,local.position.y*scale,local.position.z*scale});
    out={{gun.position.x+d.x,gun.position.y+d.y,gun.position.z+d.z},
         gears_vr::multiply(q,local.orientation)};return gears_vr::finite(out.position);
}
inline bool starter(double angle,double pull,gears_vr::Pose& out) noexcept {
    if(!std::isfinite(angle)||angle<0||angle>1.570796326794897||
       !std::isfinite(pull)||pull<0||pull>8)return false;
    const gears_vr::Quaternion q{0,0,std::sin(angle*.5),std::cos(angle*.5)};
    const auto& p=starter_wrist_closed.position;const auto& a=starter_pivot;
    const auto d=gears_vr::transform(gears_vr::rotation_matrix(q),{p.x-a.x,p.y-a.y,p.z-a.z});
    out={{a.x+d.x,a.y+d.y+pull,a.z+d.z},gears_vr::multiply(q,starter_wrist_closed.orientation)};
    return true;
}
}
'''
    return text


def adopt(root, captures, output, apply=False):
    output.mkdir(parents=True, exist_ok=False)
    baseline = t.baselines(root)
    pivot_xr = json.loads((root/'tools/quest-workbench/assets/Lancer.parts.json').read_text())['starter_pivot_metres']
    pivot = A.T@np.array(pivot_xr)*100
    rows = {}
    for path in sorted(captures.glob('*.json')):
        d = json.loads(path.read_text('utf-8')); r = convert_selected(d, baseline['model_content_fnv64'], pivot)
        if r['capture_id'] != path.stem or r['part'] in rows:
            raise ValueError('Ambiguous or duplicated selected capture')
        r['source'] = str(path); r['source_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest(); rows[r['part']] = r
    if set(rows) != set(PARTS):
        raise ValueError('All four left captures are required')
    common = root/'tools/vr-common'
    updates = {common/'weapon_lancer_owner_grabposes.h': header(rows, pivot)}
    p = common/'weapon_lancer_natural_hand.h'; text = p.read_text('utf-8-sig')
    table = '\n'.join('    {'+values(q)+'},' for q in rows['support']['fingers'])
    text, n = re.subn(r'(support_closed\[15\]\s*=\s*\{).*?(\n\};)', lambda m: m[1]+'\n'+table+m[2], text, count=1, flags=re.S)
    if n != 1: raise ValueError('Support source table missing')
    updates[p] = text
    p = common/'weapon_lancer_part_hand.h'; text = p.read_text('utf-8-sig')
    for key, symbol in [('magazine','magazine'), ('charging','loading'), ('starter','starter')]:
        table = '\n'.join('    {'+values(q)+'},' for q in rows[key]['fingers'])
        text, n = re.subn(r'(api::Joints '+symbol+r'\{\{).*?(\n\}\};)', lambda m: m[1]+'\n'+table+m[2], text, count=1, flags=re.S)
        if n != 1: raise ValueError('Part source table missing: '+symbol)
    updates[p] = text
    right = common/'weapon_lancer_main_hand.h'; right_before = right.read_bytes()
    for path, text in updates.items():
        if path.exists():
            (output/(path.name+'.before')).write_bytes(path.read_bytes())
        (output/path.name).write_text(text, 'utf-8')
        if apply: path.write_text(text, 'utf-8')
    if right.read_bytes() != right_before: raise AssertionError('Right hand changed')
    if apply:
        export_baseline = root/'tools/quest-workbench/mod-baselines.json'
        if export_baseline.exists():
            (output/'mod-baselines.json.before').write_bytes(export_baseline.read_bytes())
        export_baseline.write_text(json.dumps(t.baselines(root), indent=2), 'utf-8')
    report = dict(schema=1, captures=rows, applied=apply, right_hand_sha256=hashlib.sha256(right_before).hexdigest(),
                  right_hand_changed=False, wrist_and_fingers_converted=True,
                  game_run=False, full_skin_or_headset_accepted=False)
    (output/'review.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), 'utf-8')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--captures', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--apply', action='store_true')
    args = p.parse_args(); result = adopt(args.root, args.captures, args.output, args.apply)
    print(json.dumps(dict(parts=list(result['captures']), applied=result['applied'], right_hand_changed=False)))
