"""Summarize bounded same-tick retail Lancer blade bones without copying matrices.

The input receipt stays in ignored analysis/. Bone pivots describe a candidate
blade segment; they do not establish the rendered edge or enemy contact.
"""
import argparse
import json
import math
from pathlib import Path
import statistics


def local_point(world, gun):
    origin = gun[12:15]
    delta = [world[i] - origin[i] for i in range(3)]
    axes = [gun[4 * i:4 * i + 3] for i in range(3)]
    lengths = [math.sqrt(sum(value * value for value in axis)) for axis in axes]
    if min(lengths) <= 0 or max(lengths) - min(lengths) > .02 * max(lengths):
        raise ValueError('Gun mesh axes have invalid or nonuniform scale')
    for i in range(3):
        for j in range(i + 1, 3):
            cosine = sum(axes[i][k] * axes[j][k] for k in range(3)) / (
                lengths[i] * lengths[j])
            if abs(cosine) > .001:
                raise ValueError('Gun mesh axes are not orthogonal')
    return [sum(delta[j] * axis[j] for j in range(3)) / (lengths[i] ** 2)
            for i, axis in enumerate(axes)]


def median_vector(vectors):
    return [statistics.median(vector[i] for vector in vectors) for i in range(3)]


def distance(a, b):
    return math.dist(a, b)


def summarize(receipt):
    if receipt.get('passed') is not True or receipt.get('mode') not in (
            'chainsaw_bones', 'chainsaw_bones_held'):
        raise ValueError('Expected a passed original Lancer blade-bone receipt')
    rows = receipt['blade_bone_samples']
    if len(rows) != 16 or [row['sample'] for row in rows] != list(range(16)):
        raise ValueError('Blade-bone sample count or sequence changed')
    roots, tips, spans = [], [], []
    for row in rows:
        gun, root, tip = (row[name] for name in ('gun', 'root', 'tip'))
        if any(len(value) != 16 or not all(math.isfinite(item) for item in value)
               for value in (gun, root, tip)):
            raise ValueError('Blade-bone matrices are invalid')
        root_local = local_point(root[12:15], gun)
        tip_local = local_point(tip[12:15], gun)
        roots.append(root_local)
        tips.append(tip_local)
        spans.append(distance(root_local, tip_local))
    root_mid, tip_mid = median_vector(roots), median_vector(tips)
    return {
        'game_pid': receipt['game_pid'],
        'samples': len(rows),
        'root_local_median_raw': root_mid,
        'tip_local_median_raw': tip_mid,
        'root_local_max_drift_raw': max(distance(row, root_mid) for row in roots),
        'tip_local_max_drift_raw': max(distance(row, tip_mid) for row in tips),
        'pivot_span_median_raw': statistics.median(spans),
        'pivot_span_range_raw': [min(spans), max(spans)],
        'mode': receipt['mode'],
        'limit': 'original bone pivots only; no rendered-edge or collision proof',
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('receipt', type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize(json.loads(args.receipt.read_text(encoding='utf-8'))),
                     indent=2))


if __name__ == '__main__':
    main()
