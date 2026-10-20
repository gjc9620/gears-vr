"""Offline parallel-eye math on recorded matrices; never connects to the game."""
import argparse
import csv
import json
import math
import pathlib


def transform(v,m):return [sum(v[k]*m[k*4+j] for k in range(4)) for j in range(4)]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    samples=0;maximum=0.;example=None
    # Synthetic game units only: no physical IPD or world-scale calibration.
    half_gap=1.;distance=1000.
    with args.capture.with_suffix('.csv').open(newline='') as stream:
        for r in csv.DictReader(stream):
            if r['kind']!='1' or int(r['valid'])&5!=5:continue
            all_m=[float(r[f'matrix{i}']) for i in range(32)]
            if not all(math.isfinite(x) for x in all_m):raise RuntimeError('Nonfinite input')
            v=all_m[:16];p=all_m[16:];camera=[float(r[k]) for k in ('x','y','z')]
            right=[v[i*4] for i in range(3)];forward=[v[i*4+2] for i in range(3)]
            point=[camera[i]+distance*forward[i] for i in range(3)]+[1.]
            eyes=[]
            for sign in (-1,1):
                eye=[camera[i]+sign*half_gap*right[i] for i in range(3)]+[1.]
                eye_v=v.copy();eye_v[12]-=sign*half_gap
                origin=transform(eye,eye_v)
                # Rounded CSV positions have small residuals; subtract the
                # same baseline residual instead of claiming zero exact error.
                base_origin=transform(camera+[1.],v)
                maximum=max(maximum,max(abs(origin[i]-base_origin[i]) for i in range(4)))
                clip=transform(transform(point,eye_v),p)
                if abs(clip[3])<1e-6:raise RuntimeError('Synthetic point has zero W')
                eyes.append(clip[0]/clip[3])
            expected=2*half_gap*p[0]/transform(point,v)[2]
            maximum=max(maximum,abs((eyes[0]-eyes[1])-expected))
            samples+=1
            if example is None:example={'left_ndc_x':eyes[0],'right_ndc_x':eyes[1],'expected_horizontal_disparity':expected}
    report={'passed':samples>=100 and maximum<1e-5,'samples':samples,'maximum_residual_error':maximum,
        'synthetic_eye_gap_game_units':2*half_gap,'synthetic_forward_distance_game_units':distance,'example':example,
        'limitations':'Offline arithmetic only. No game writes, second native view, render target or headset output. Uses parallel eyes and identical recorded projections; physical IPD, game-unit scale and OpenXR asymmetric frustums remain uncalibrated.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
