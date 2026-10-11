"""Cross-check completed native-camera and OpenXR pose logs; no game writes."""
import argparse
import collections
import csv
import json
import math
import pathlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture',type=pathlib.Path)
    parser.add_argument('poses',type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    with args.capture.open(newline='') as f: rows=list(csv.DictReader(f))
    with args.poses.open(newline='') as f: poses={int(r['frame']):r for r in csv.DictReader(f)}
    views=[r for r in rows if r['kind']=='1']
    events=[r for r in rows if r['kind']=='2' and r['caller_rva']=='0x15d0ca']
    modified=[r for r in rows if int(r['valid'])&64]
    applied=[r for r in events if r['pose_status']=='7']
    valid_views=[r for r in views if int(r['valid'])&4]
    rotation_keys=['delta_pitch','delta_rot_yaw','delta_roll']
    lengths=[math.sqrt(sum(float(r[k])**2 for k in ['delta_x','delta_y','delta_z'])) for r in modified]
    def valid_sample(row):
        pose=poses.get(int(row['pose_sequence']))
        return bool(pose and int(pose['head_flags'])&15==15 and pose['session_state']=='5' and
                    all(math.isfinite(float(pose[k])) for k in ['head_x','head_y','head_z','qx','qy','qz','qw']))
    checks={
        'actual_head_samples_applied':bool(applied) and len({r['pose_sequence'] for r in applied})>10,
        'applied_samples_match_tracked_focused_openxr_log':bool(applied) and all(valid_sample(r) for r in applied),
        'only_known_event_caller_modified':not any(r['kind']=='2' and r['caller_rva']!='0x15d0ca' for r in modified),
        'no_unfocused_modifications':not any(r['focused']=='0' for r in modified),
        'translation_limit_with_float_rounding':bool(lengths) and max(lengths)<=2.01,
        'rotation_limits':bool(applied) and all(abs(int(r[k]))<=911 for r in applied for k in rotation_keys),
        'all_view_matrices_finite':bool(valid_views) and all(math.isfinite(float(r[f'matrix{i}'])) for r in valid_views for i in range(32)),
        'view_matrices_changed':len({tuple(r[f'matrix{i}'] for i in range(16)) for r in valid_views})>10,
        'no_failed_present':not any(int(r['result'],16)>=0x80000000 for r in rows if r['kind'] in ['3','4']),
    }
    report={'checks':checks,'all_checks_passed':all(checks.values()),
        'scene_calls':len(views),'matching_events':len(events),
        'status_counts':dict(collections.Counter(r['pose_status'] for r in events)),
        'applied_events':len(applied),'applied_views':sum(bool(int(r['valid'])&64) for r in views),
        'unique_applied_pose_sequences':len({r['pose_sequence'] for r in applied}),
        'max_translation_units':max(lengths,default=0),
        'rotation_delta_ranges':{k:[min((int(r[k]) for r in applied),default=0),max((int(r[k]) for r in applied),default=0)] for k in rotation_keys},
        'unique_source_event_positions':len({tuple(r[k] for k in ['source_x','source_y','source_z']) for r in events}),
        'unique_source_event_rotations':len({tuple(r[k] for k in ['source_pitch','source_rot_yaw','source_roll']) for r in events}),
        'unique_cache_yaws_in_applied_views':len({r['cache_yaw'] for r in views if int(r['valid'])&64}),
        'unique_projection_matrices':len({tuple(r[f'matrix{i}'] for i in range(16,32)) for r in valid_views}),
        'unfocused_view_timestamp_count':len({r['camera_timestamp'] for r in views if r['focused']=='0'}),
        'capture_csv':str(args.capture),'pose_csv':str(args.poses),
        'limits':'Bounded game-camera integration only; no game image in headset, no stereo scene rendering. Hook restoration must be checked separately.'}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    return 0 if report['all_checks_passed'] else 1


if __name__=='__main__':raise SystemExit(main())
