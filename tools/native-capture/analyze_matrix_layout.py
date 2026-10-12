"""Test a row-vector view-matrix hypothesis against recorded camera positions."""
import argparse
import csv
import json
import math
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=pathlib.Path,required=True)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    samples=0;max_basis_error=0.;max_position_error=0.;variants=set();aspect=[];fov=[]
    with args.capture.with_suffix('.csv').open(newline='') as stream:
        for r in csv.DictReader(stream):
            if r['kind']!='1' or int(r['valid'])&5!=5:continue
            m=[float(r[f'matrix{i}']) for i in range(32)]
            if not all(math.isfinite(x) for x in m):raise RuntimeError('Nonfinite input')
            samples+=1;variants.add(tuple(m[:16]))
            rotation=[m[i:i+3] for i in (0,4,8)]
            for i in range(3):
                for j in range(3):
                    dot=sum(rotation[i][k]*rotation[j][k] for k in range(3))
                    max_basis_error=max(max_basis_error,abs(dot-int(i==j)))
            # Row-vector model: translation = -camera_position * rotation.
            recovered=[-sum(m[12+k]*rotation[i][k] for k in range(3)) for i in range(3)]
            measured=[float(r[k]) for k in ('x','y','z')]
            max_position_error=max(max_position_error,max(abs(a-b) for a,b in zip(recovered,measured)))
            max_basis_error=max(max_basis_error,abs(m[3]),abs(m[7]),abs(m[11]),abs(m[15]-1))
            if m[16]>0 and m[21]>0:
                aspect.append(m[21]/m[16]);fov.append(math.degrees(2*math.atan(1/m[16])))
    report={'passed':samples>=100 and max_basis_error<1e-4 and max_position_error<.05,
        'samples':samples,'view_variants':len(variants),'max_orthonormal_affine_error':max_basis_error,
        'max_recovered_camera_position_error_units':max_position_error,
        'projection_scale_ratio_range':[min(aspect),max(aspect)] if aspect else None,
        'horizontal_fov_hypothesis_degrees_range':[min(fov),max(fov)] if fov else None,
        'limitations':'Offline hypothesis against recorded scene samples, no game writes. Does not establish GPU shader packing, projection handedness/depth mapping, all camera modes or stereo.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2))
    print(json.dumps(report,indent=2));return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
