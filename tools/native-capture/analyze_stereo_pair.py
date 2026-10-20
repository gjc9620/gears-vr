"""Offline sparse stereo matching for an existing side-by-side window image.

Uses NumPy/Pillow only. This estimates image disparity, not depth or headset
correctness. Regions are manually chosen image coordinates in the LEFT half.
Example: --region foreground:30,60,100,470 --region distant:295,175,328,280
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


def match(image, template, x, y, radius, horizontal, vertical):
    """Exhaustive zero-mean NCC, with an unconstrained disparity sign."""
    height, width = image.shape
    x0, x1 = max(radius, x - horizontal), min(width - radius - 1, x + horizontal)
    y0, y1 = max(radius, y - vertical), min(height - radius - 1, y + vertical)
    candidates = np.lib.stride_tricks.sliding_window_view(
        image[y0-radius:y1+radius+1, x0-radius:x1+radius+1], template.shape)
    centered = template - template.mean()
    sums = candidates.sum(axis=(-2, -1))
    variances = np.maximum((candidates*candidates).sum(axis=(-2, -1)) - sums*sums/template.size, 0)
    denominator = np.sqrt(variances * (centered*centered).sum())
    scores = np.einsum('ijab,ab->ij', candidates, centered, optimize=False) / np.maximum(denominator, 1e-9)
    iy, ix = np.unravel_index(np.argmax(scores), scores.shape)
    score = float(scores[iy, ix])
    other = scores.copy()
    # Adjacent pixels are the same NCC peak; ambiguity is a separate peak.
    other[max(0, iy-1):iy+2, max(0, ix-2):ix+3] = -1
    margin = score - float(other.max())
    subpixel = 0.0
    if 0 < ix < scores.shape[1]-1:
        a, b, c = (float(v) for v in scores[iy, ix-1:ix+2])
        curvature = a - 2*b + c
        if curvature < -1e-9:
            subpixel = float(np.clip(.5*(a-c)/curvature, -.5, .5))
    return x0+int(ix), y0+int(iy), subpixel, score, margin


def summarize(rows):
    if not rows:
        return {'count': 0}
    disparity = np.array([r['disparity'] for r in rows])
    vertical = np.array([r['vertical'] for r in rows])
    return {'count': len(rows), 'disparity_median': float(np.median(disparity)),
            'disparity_q25_q75': np.percentile(disparity, [25, 75]).tolist(),
            'disparity_mad': float(np.median(np.abs(disparity-np.median(disparity)))),
            'vertical_median': float(np.median(vertical)),
            'vertical_abs_q95': float(np.percentile(np.abs(vertical), 95)),
            'ncc_median': float(np.median([r['ncc'] for r in rows])),
            'ambiguity_margin_median': float(np.median([r['margin'] for r in rows]))}


def parse_region(value):
    name, coordinates = value.split(':', 1)
    bounds = tuple(int(v) for v in coordinates.split(','))
    if len(bounds) != 4 or bounds[0] >= bounds[2] or bounds[1] >= bounds[3]:
        raise argparse.ArgumentTypeError('Region must be name:x0,y0,x1,y1')
    return name, bounds


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path, help='JSON output; matches written beside it as CSV')
    parser.add_argument('--region', action='append', type=parse_region, default=[])
    parser.add_argument('--radius', type=int, default=9)
    parser.add_argument('--step', type=int, default=12)
    parser.add_argument('--horizontal', type=int, default=20)
    parser.add_argument('--vertical', type=int, default=3)
    parser.add_argument('--minimum-ncc', type=float, default=.90)
    parser.add_argument('--minimum-margin', type=float, default=.02)
    parser.add_argument('--minimum-std', type=float, default=4)
    args = parser.parse_args()
    if args.radius < 2 or args.step < 1 or args.horizontal < 3 or args.vertical < 1:
        parser.error('Positive patch/grid/search settings required')
    image = np.asarray(Image.open(args.image).convert('RGB'), dtype=np.float64)
    if image.shape[1] % 2:
        parser.error('Side-by-side image must have even width')
    gray = image @ np.array([.2126, .7152, .0722])
    width = gray.shape[1]//2
    left, right = gray[:, :width], gray[:, width:]
    radius = args.radius
    accepted, rejected = [], {'low_texture': 0, 'low_ncc': 0, 'ambiguous': 0, 'reciprocal': 0}
    attempted = 0
    for y in range(radius+args.vertical, left.shape[0]-radius-args.vertical, args.step):
        # match() clips the candidate search at image edges. Large asymmetric
        # eye FOV offsets must not silently remove the requested image regions.
        for x in range(radius, width-radius, args.step):
            attempted += 1
            patch = left[y-radius:y+radius+1, x-radius:x+radius+1]
            std = float(patch.std())
            if std < args.minimum_std:
                rejected['low_texture'] += 1
                continue
            rx, ry, subpixel, score, margin = match(right, patch, x, y, radius, args.horizontal, args.vertical)
            if score < args.minimum_ncc:
                rejected['low_ncc'] += 1
                continue
            if margin < args.minimum_margin:
                rejected['ambiguous'] += 1
                continue
            reverse_patch = right[ry-radius:ry+radius+1, rx-radius:rx+radius+1]
            lx, ly, _, reverse_score, reverse_margin = match(left, reverse_patch, rx, ry, radius, args.horizontal, args.vertical)
            if abs(lx-x) > 1 or abs(ly-y) > 1 or reverse_score < args.minimum_ncc or reverse_margin < args.minimum_margin:
                rejected['reciprocal'] += 1
                continue
            accepted.append({'x_left': x, 'y_left': y, 'x_right_integer': rx, 'y_right': ry,
                             'disparity': x-rx-subpixel, 'vertical': y-ry,
                             'ncc': score, 'margin': margin, 'patch_std': std})
    regions = {}
    for name, (x0, y0, x1, y1) in args.region:
        if not (0 <= x0 < x1 <= width and 0 <= y0 < y1 <= left.shape[0]):
            parser.error(f'Region {name} is outside the left image')
        # Require the complete patch to lie in the named region.
        selected = [r for r in accepted if x0+radius <= r['x_left'] < x1-radius and y0+radius <= r['y_left'] < y1-radius]
        regions[name] = {'bounds': [x0, y0, x1, y1], **summarize(selected)}
    report = {'image': str(args.image.resolve()), 'sha256': hashlib.sha256(args.image.read_bytes()).hexdigest(),
              'half_size': [width, left.shape[0]],
              'coordinate_units': 'pixels in saved image; disparity = x_left - x_right after splitting halves',
              'settings': {key: value for key, value in vars(args).items() if key not in ('image', 'output', 'region')},
              'method': 'raw luma zero-mean NCC; exhaustive horizontal/vertical search; texture/peak/reciprocal filters; horizontal parabolic peak refinement',
              'attempted': attempted, 'rejected': rejected, 'all_matches': summarize(accepted), 'regions': regions,
              'zero_shift_mae': float(np.abs(left-right).mean()),
              'half_luma': [{'mean': float(a.mean()), 'std': float(a.std())} for a in (left, right)],
              'limits': ['Regions are manually interpreted as nearer/farther, not measured depth.',
                         'JPEG, downsampling, repeated texture, occlusion, lighting and temporal effects can bias matches.',
                         'Accepted patches overlap and are not independent statistical samples.',
                         'Search is bounded and favors approximately horizontal correspondence; it is not a calibration test.',
                         'A single composite window snapshot does not prove GPU submission synchronization, calibrated IPD, correct headset projection or VR completion.',
                         'Subpixel values describe a correlation peak fit; accuracy finer than a saved pixel is not guaranteed.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    csv_path = args.output.with_suffix('.csv')
    with csv_path.open('w', newline='', encoding='utf-8') as stream:
        fields = ['x_left', 'y_left', 'x_right_integer', 'y_right', 'disparity', 'vertical', 'ncc', 'margin', 'patch_std']
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(accepted)
    print(json.dumps({'output': str(args.output), 'all_matches': report['all_matches'], 'regions': regions}, indent=2))


if __name__ == '__main__':
    main()
