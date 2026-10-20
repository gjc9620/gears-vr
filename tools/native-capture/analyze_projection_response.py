"""Image-derived horizontal projection response; offline NumPy/Pillow only.

Regions are static scene coordinates in each source eye. Fit is independent of
the expected transform. This does not validate native matrices or headset VR.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image
from analyze_stereo_pair import parse_region


def sample(image, x, y):
    x, y = np.broadcast_arrays(x, y)
    valid = (x >= 0) & (x < image.shape[1]-1) & (y >= 0) & (y < image.shape[0]-1)
    ix = np.clip(x.astype(int), 0, image.shape[1]-2)
    iy = np.clip(y.astype(int), 0, image.shape[0]-2)
    fx, fy = x-ix, y-iy
    values = ((1-fx)*(1-fy)*image[iy, ix] + fx*(1-fy)*image[iy, ix+1]
              + (1-fx)*fy*image[iy+1, ix] + fx*fy*image[iy+1, ix+1])
    return values, valid


def ncc(a, b):
    a = a-a.mean(axis=-1, keepdims=True)
    b = b-b.mean(axis=-1, keepdims=True)
    return (a*b).sum(axis=-1)/np.maximum(np.sqrt((a*a).sum(axis=-1)*(b*b).sum(axis=-1)), 1e-9)


def measure(source, target, regions, expected):
    """Bounded coarse/refined model competition followed by patch consistency."""
    height, width = source.shape
    center = width/2
    dy, dx = np.mgrid[-5:6, -5:6]
    dx, dy = dx.ravel(), dy.ravel()
    points = []
    for _, (x0, y0, x1, y1) in regions:
        for y in range(y0+6, y1-6, 24):
            for x in range(x0+6, x1-6, 24):
                patch, valid = sample(source, x+dx, y+dy)
                if valid.all() and patch.std() >= 4:
                    points.append((x, y, patch))
    # Deterministic spatial subsampling keeps runtime bounded.
    if len(points) > 80:
        points = [points[i] for i in np.linspace(0, len(points)-1, 80).astype(int)]
    if not points:
        return {'reliable': False, 'reason': 'no textured source patches', 'source_patches': 0}
    xs = np.array([p[0] for p in points])[:, None]
    ys = np.array([p[1] for p in points])[:, None]
    patches = np.array([p[2] for p in points])

    def scores(scale, shift, vertical=0):
        values, valid = sample(target, center+scale*(xs-center+dx)+shift, ys+dy+vertical)
        result = ncc(patches, values)
        result[~valid.all(axis=1)] = np.nan
        return result

    def metric(scale, shift, vertical=0):
        values = scores(scale, shift, vertical)
        values = values[np.isfinite(values)]
        # Out-of-frame hypotheses must not win from just one lucky patch.
        quality = float(np.median(values)) if len(values) >= max(12, len(points)//3) else -1.
        return {'scale': float(scale), 'shift_pixels': float(shift), 'vertical_pixels': float(vertical),
                'valid_patches': len(values), 'median_ncc': quality,
                'high_ncc_patches': int((values >= .80).sum())}

    candidates = [metric(s, t) for s in (.75, 1., 1.25, 1.5, 1.75, 2., 2.25)
                  for t in range(-48, 49, 4)]
    best = max(candidates, key=lambda m: m['median_ncc'])
    refined = [metric(s, t, v)
               for s in np.arange(max(.5, best['scale']-.125), best['scale']+.126, .025)
               for t in np.arange(best['shift_pixels']-3, best['shift_pixels']+3.1, 1.)
               for v in (-1., 0., 1.)]
    best = max(refined, key=lambda m: m['median_ncc'])
    identity, expected_metric = metric(1., 0.), metric(*expected)
    fitted = scores(best['scale'], best['shift_pixels'], best['vertical_pixels'])
    # Check that individual patches prefer the global prediction to distinct
    # neighboring positions, rejecting repeated textures/ambiguous peaks.
    alternatives = np.array([scores(best['scale'], best['shift_pixels']+t,
                                    best['vertical_pixels']+v)
                             for t, v in ((-4, 0), (4, 0), (0, -3), (0, 3))])
    alternatives = np.where(np.isfinite(alternatives), alternatives, -1.)
    consistent = np.isfinite(fitted) & (fitted >= .80) & (fitted-alternatives.max(axis=0) >= .02)
    reliable = bool(consistent.sum() >= 12 and best['median_ncc'] >= .75)
    expected_close = abs(best['scale']-expected[0]) <= .075 and abs(best['shift_pixels']-expected[1]) <= 3
    distinct = expected != (1., 0.)
    supported = reliable and expected_close and abs(best['vertical_pixels']) <= 1 and (
        not distinct or expected_metric['median_ncc']-identity['median_ncc'] >= .05)
    return {'source_patches': len(points), 'consistent_unambiguous_patches': int(consistent.sum()),
            'reliable': reliable, 'expected_response_supported': bool(supported),
            'observed_fit': best, 'identity_candidate': identity, 'expected_candidate': expected_metric,
            'expected_minus_identity_ncc': expected_metric['median_ncc']-identity['median_ncc'],
            'limits': 'Bounded horizontal affine search; static ROI interpretation and patch consistency, not calibration.'}


def read(path):
    rgb = np.asarray(Image.open(path).convert('RGB'), dtype=float)
    if rgb.shape[1] % 2:
        raise ValueError('Side-by-side image width must be even')
    gray = rgb @ np.array([.2126, .7152, .0722])
    return np.split(gray, 2, axis=1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for phase in ('a1', 'b', 'a2', 'c', 'a3'):
        parser.add_argument('--'+phase, required=True, type=Path)
    parser.add_argument('--region', action='append', type=parse_region, required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    images = {p: read(getattr(args, p)) for p in ('a1', 'b', 'a2', 'c', 'a3')}
    shape = images['a1'][0].shape
    if any(eye.shape != shape for pair in images.values() for eye in pair):
        parser.error('All image dimensions must match')
    for name, (x0, y0, x1, y1) in args.region:
        if not (0 <= x0 < x1 <= shape[1] and 0 <= y0 < y1 <= shape[0]):
            parser.error(f'Region {name} outside eye bounds')
    comparisons = {}
    for source, target, expected in (('a1', 'b', (2., 0.)), ('a2', 'c', (1., .05*shape[1])),
                                     ('a1', 'a2', (1., 0.)), ('a2', 'a3', (1., 0.)), ('a1', 'a3', (1., 0.))):
        comparisons[source+'_'+target] = {eye: measure(images[source][i], images[target][i], args.region, expected)
                                           for i, eye in enumerate(('left', 'right'))}
    report = {'evidence_kind': 'image-derived', 'half_size': [shape[1], shape[0]],
              'regions': dict(args.region), 'comparisons': comparisons,
              'settings': {'patch_radius': 5, 'source_grid_step': 24, 'maximum_source_patches': 80,
                           'minimum_patch_std': 4, 'coarse_scales': [.75, 1., 1.25, 1.5, 1.75, 2., 2.25],
                           'coarse_shift_bounds': [-48, 48], 'coarse_shift_step': 4,
                           'refined_scale_step': .025, 'refined_shift_step': 1,
                           'refined_vertical_bounds': [-1, 1], 'minimum_consistent_patches': 12,
                           'consistent_ncc': .80, 'ambiguity_margin': .02,
                           'minimum_global_median_ncc': .75, 'scale_tolerance': .075,
                           'shift_tolerance_pixels': 3, 'distinct_expected_ncc_advantage': .05},
              'inputs': {p: {'path': str(getattr(args, p).resolve()),
                              'sha256': hashlib.sha256(getattr(args, p).read_bytes()).hexdigest()} for p in images},
              'limits': ['Animation, lighting, occlusion, JPEG and downsampling can obscure the response.',
                         'Overlapping source patches are not independent samples; insufficient texture or ambiguity yields no reliable result.',
                         'P00 response assumes a centered viewport and unchanged camera; P8 sign is an explicit hypothesis.',
                         'A correlation fit does not prove matrix writes, depth preservation, frame synchronization, IPD or headset VR.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'output': str(args.output), 'comparisons': {k: {e: {'reliable': v['reliable'],
                         'expected_response_supported': v.get('expected_response_supported', False),
                         'observed_fit': v.get('observed_fit')} for e, v in eyes.items()} for k, eyes in comparisons.items()}}))


if __name__ == '__main__':
    main()
