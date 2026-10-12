"""Collect a completed capture, group call data, and report its limits."""
import argparse
import collections
import csv
import json
import math
import pathlib
import shutil
import statistics
import time


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--timeout',type=float,default=180)
    parser.add_argument('--build-dir',type=pathlib.Path,default=pathlib.Path(__file__).with_name('build'))
    parser.add_argument('--summary',action='store_true',help='Print compact counts; keep the full report on disk')
    parser.add_argument('--pid',type=int,help='Require completed status from this process, ignoring older runs')
    args=parser.parse_args()
    build=args.build_dir.resolve()
    deadline=time.monotonic()+args.timeout
    while True:
        try:
            status=(build/'capture-status.txt').read_text()
        except (PermissionError,FileNotFoundError):
            status=''
        if args.pid and f'capturing pid={args.pid} ' not in status:status=''
        if 'stopped disable_status=' in status:break
        if 'rejected:' in status or 'hook installation failed' in status:raise RuntimeError(status)
        if time.monotonic()>=deadline:raise RuntimeError('Capture not confirmed stopped; inspect game/session before proceeding')
        time.sleep(.5)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(build/'capture.csv',args.output.with_suffix('.csv'))
    args.output.with_suffix('.status.txt').write_text(status)
    frequency=int(next(line.split('=')[1] for line in status.splitlines() if line.startswith('qpc_frequency=')))
    rows=list(csv.DictReader(args.output.with_suffix('.csv').open()))
    groups=collections.defaultdict(list)
    for row in rows:
        key=(int(row['kind']),row['object'],int(row['thread']),int(row['focused']))
        groups[key].append(row)
    summaries=[]
    labels={1:'scene_view',2:'player_view_event',3:'present',4:'present1',5:'execute_command_lists',7:'family_submission_candidate',8:'render_view_copy_candidate'}
    labels[9]='renderer_constructor_candidate'
    labels[10]='renderer_consume_candidate'
    labels[11]='renderer_destroy_candidate'
    labels[12]='renderer_prepare_candidate'
    for (kind,obj,thread,focus),items in groups.items():
        items.sort(key=lambda r:int(r['qpc']))
        intervals=[(int(items[i]['qpc'])-int(items[i-1]['qpc']))*1000/frequency for i in range(1,len(items))]
        valid=[r for r in items if int(r['valid'])&3==3]
        summary=dict(kind=labels[kind],object=obj,thread=thread,focused=bool(focus),calls=len(items),
            interval_median_ms=statistics.median(intervals) if intervals else None,
            largest_interval_ms=max(intervals) if intervals else None,
            duration_median_ms=statistics.median(int(r['duration_qpc'])*1000/frequency for r in items),
            matching_expected_player_calls=sum(bool(int(r['valid'])&16) for r in items),
            callers=dict(collections.Counter(r['caller_rva'] for r in items)))
        if valid:
            summary['unique_positions']=len({(r['x'],r['y'],r['z']) for r in valid})
            summary['unique_rotations']=len({(r['pitch'],r['yaw'],r['roll']) for r in valid})
        if kind==1:
            matrices=[r for r in items if int(r['valid'])&4]
            summary['matrix_samples']=len(matrices)
            summary['finite_matrix_samples']=sum(all(math.isfinite(float(r[f'matrix{i}'])) for i in range(32)) for r in matrices)
            summary['unique_matrices']=len({tuple(r[f'matrix{i}'] for i in range(32)) for r in matrices})
            summary['unique_camera_timestamps']=len({r['camera_timestamp'] for r in items if int(r['valid'])&8})
        if kind in (3,4):
            summary['non_test_present_calls']=sum(bool(int(r['valid'])&32) for r in items)
            summary['failed_present_calls']=sum(int(r['result'],16)>=0x80000000 for r in items)
        if kind in (3,4,5) and 'gpu_device' in items[0]:
            keys=['gpu_device','queue_type','command_count'] if kind==5 else ['gpu_device','gpu_buffer','gpu_window','buffer_index','buffer_count','width','height','format','swap_flags']
            summary['gpu_metadata']={key:dict(collections.Counter(r[key] for r in items)) for key in keys}
        summaries.append(summary)
    test_mode='gpu_diagnostics' if 'gpu_diagnostics=1' in status else ('live_head' if 'live_head_enabled=1' in status else ('six_axis_sweep' if 'sweep_enabled=1' in status else ('yaw_wobble' if 'wobble_enabled=1' in status else 'read_only')))
    if 'family_diagnostics=1 ' in status:test_mode='family_diagnostics'
    report=dict(status=status.strip(),samples=len(rows),groups=summaries,test_mode=test_mode,
        caveat='Present and Present1 may nest; do not add their counts. Focus gaps and menu pauses are not VM stalls. View edits depend on the recorded test_mode.')
    if rows and 'yaw_delta' in rows[0]:
        applied=[r for r in rows if int(r['valid'])&64]
        events=[r for r in rows if r['kind']=='2' and int(r['valid'])&64]
        views=[r for r in rows if r['kind']=='1' and int(r['valid'])&64]
        other_events=[r for r in rows if r['kind']=='2' and r['caller_rva']!='0x15d0ca']
        report['wobble']=dict(applied_event_calls=len(events),applied_scene_calls=len(views),
            applied_event_callers=dict(collections.Counter(r['caller_rva'] for r in events)),
            affected_other_event_calls=sum(bool(int(r['valid'])&64) for r in other_events),
            modified_unfocused_calls=sum(r['focused']=='0' for r in applied),
            delta_range_ru=[min(int(r['yaw_delta']) for r in applied),max(int(r['yaw_delta']) for r in applied)] if applied else None,
            unique_source_yaws=len({r['source_yaw'] for r in events}),
            unique_cache_yaws_in_modified_views=len({r['cache_yaw'] for r in views}),
            unique_modified_view_matrices=len({tuple(r[f'matrix{i}'] for i in range(32)) for r in views}),
            event_delta_mismatches=sum(int(r['yaw'])-int(r['source_yaw'])!=int(r['yaw_delta']) for r in events))
        if 'wobble_enabled=1' in status:
            report['caveat']='Render-only yaw outputs were deliberately modified. No player-controller/cache rotation writes were requested; cache values require correlation. Present counts are not one-to-one frame proof.'
    args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'output':str(args.output),'samples':len(rows),'test_mode':test_mode} if args.summary else report,indent=2))


if __name__=='__main__':main()
