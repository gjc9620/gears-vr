"""Compare one owned SG_Chamber capture against both explicit key-time laws.

No process access or product permission. Original samples/decoded keys remain
under analysis; ambiguous or mismatched observations fail instead of choosing.
"""
import argparse,json,math
from pathlib import Path
from analyze_gnasher_runtime import rotation,transpose,multiply,angle_axis,summarize

def expected(keys,seconds,length,divisor):
    p=max(0.,min(len(keys)-1,seconds*divisor/length));a=int(p);b=min(a+1,len(keys)-1);t=p-a
    qa=keys[a]['quaternion_xyzw'];qb=keys[b]['quaternion_xyzw']
    if sum(x*y for x,y in zip(qa,qb))<0:qb=[-x for x in qb]
    q=[x*(1-t)+y*t for x,y in zip(qa,qb)];n=math.sqrt(sum(x*x for x in q));q=[x/n for x in q]
    return math.degrees(2*math.atan2(abs(q[0]),abs(q[3])))

def analyze(capture,static):
    summary=summarize(capture)
    sequence=next(s for s in static['sequences'] if s['sequence_name']=='SG_Chamber')
    keys=sequence['sampled_rotation']['samples'];length=sequence['sequence_length_seconds'];rows=capture['samples']
    base=multiply(rotation(rows[0]['lever']),transpose(rotation(rows[0]['main'])))
    actual=[angle_axis(multiply(transpose(base),multiply(rotation(r['lever']),transpose(rotation(r['main'])))))[0] for r in rows]
    timing_errors=[abs(r['observed_node_time_seconds']-r['position_seconds']) for r in rows]
    if any(not math.isfinite(x) for x in timing_errors):raise ValueError('Nonfinite original node time')
    scores={}
    for law,divisor in [('keys_minus_one',len(keys)-1),('keys',len(keys))]:
        error=[abs(a-expected(keys,r['observed_node_time_seconds'],length,divisor)) for a,r in zip(actual,rows)]
        scores[law]={'rms_degrees':math.sqrt(sum(x*x for x in error)/len(error)),'max_degrees':max(error)}
    matched=[law for law,v in scores.items() if v['rms_degrees']<=.2 and v['max_degrees']<=.5]
    axis=summary['peak_axis_in_main']
    valid=bool(len(matched)==1 and max(timing_errors)<=.0001 and summary['final_degrees_from_home']<=.5 and
               .4<math.radians(summary['peak_degrees'])<.8 and axis and axis[0]<-.98 and abs(axis[1])<.02 and abs(axis[2])<.02)
    return {'schema':1,'passed':valid,'scope':'Original stopped SG_Chamber interpolation only; no physical mechanism or visual fitting acceptance',
        'game_identity':capture.get('game_identity'),'original_sequence_sha256':sequence['export_sha256'],
        'launcher_cleanup_pending':summary['launcher_cleanup_pending'],
        'frame_count':len(rows),'decoded_keys':len(keys),'maximum_node_time_difference_seconds':max(timing_errors),
        'candidate_laws':scores,'matched_laws':matched,'verified_law':matched[0] if valid else None,'original_bone_summary':summary,
        'physical_headset_accepted':False}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--static',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=analyze(json.loads(a.capture.read_text(encoding='utf-8')),json.loads(a.static.read_text(encoding='utf-8')))
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ('passed','verified_law','candidate_laws')}))
    if not result['passed']:raise SystemExit(1)
if __name__=='__main__':main()
