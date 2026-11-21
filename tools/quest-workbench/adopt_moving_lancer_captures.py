"""0.3 moving-part compatibility wrapper; leaves the frozen legacy adopter unchanged."""
from __future__ import annotations
import argparse
import copy
import json
import math
from pathlib import Path
import numpy as np
import adopt_lancer_captures as legacy

_convert_legacy = legacy.convert_selected


def convert_selected(project, model_hash, pivot):
    meta = project['capture']
    hand = next(h for h in project['parts'][meta['part']]['hands'] if h['side'] == 'left')
    if not hand.get('part_delta_known', False):
        return _convert_legacy(project, model_hash, pivot)
    delta = hand['part_delta_in_object']
    dr = legacy.rotation(delta['rotation_xyzw'])
    dp = np.asarray(delta['position_metres'], dtype=float)
    if dp.shape != (3,) or not np.isfinite(dp).all() or np.linalg.norm(dp) > 2:
        raise ValueError('Invalid captured part delta')
    theta, travel = hand['starter_angle_radians'], hand['charging_distance_metres']
    if not math.isfinite(theta) or not 0 <= theta <= math.pi/2+1e-6 or not math.isfinite(travel) or not 0 <= travel <= .231:
        raise ValueError('Invalid captured mechanical state')
    wrist = hand['wrist_in_object']
    r = legacy.rotation(legacy.t.multiply(wrist['rotation_xyzw'], legacy.t.correction(hand['wrist_correction_degrees'])))
    p = np.asarray(wrist['position_metres'], dtype=float) + r@np.asarray(hand['wrist_offset_metres'], dtype=float)
    if p.shape != (3,) or not np.isfinite(p).all() or np.linalg.norm(p) > 2:
        raise ValueError('Invalid object-local wrist')
    p, r = dr.T@(p-dp), dr.T@r
    neutral = copy.deepcopy(project)
    h = next(h for h in neutral['parts'][meta['part']]['hands'] if h['side'] == 'left')
    h['wrist_in_object'] = dict(position_metres=p.tolist(), rotation_xyzw=legacy.quaternion(r))
    h['wrist_correction_degrees'] = h['wrist_offset_metres'] = [0., 0., 0.]
    h['starter_angle_radians'] = h['charging_distance_metres'] = 0.
    result = _convert_legacy(neutral, model_hash, pivot)
    result.update(captured_angle=theta, captured_travel=travel, captured_part_delta=delta)
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--captures', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--apply', action='store_true')
    args = p.parse_args()
    legacy.convert_selected = convert_selected
    result = legacy.adopt(args.root, args.captures, args.output, args.apply)
    print(json.dumps(dict(parts=list(result['captures']), applied=result['applied'], right_hand_changed=False)))
