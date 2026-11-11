from pathlib import Path
import json,math,re,sys
def distance(a,b):return math.sqrt(sum((a[i]-b[i])**2 for i in range(3)))
def angle(a,b):
    dot=sum(a[i]*b[i] for i in range(3,7))
    norm=math.sqrt(sum(x*x for x in a[3:])*sum(x*x for x in b[3:]))
    return 2*math.acos(min(1,abs(dot)/norm))*180/math.pi
def main():
    p=Path(sys.argv[1]);raw=p.read_bytes();s=raw.decode('utf-16' if raw[:2] in (b'\xff\xfe',b'\xfe\xff') else 'utf-8-sig')
    rows=[]
    for m in re.finditer(r'\bbody_passive_commit_(\d+)=([^\s]+)',s):
        i=int(m[1]);v=[float(x) for x in m[2].split(',')]
        poses=[]
        for n in range(5):
            match=re.search(r'\bbody_passive_commit_'+str(i)+'_pose_'+str(n)+r'=([^\s]+)',s)
            assert match,(i,n)
            poses.append([float(x) for x in match[1].split(',')])
        rows.append(dict(index=i,tick=int(v[0]),compose=int(v[1]),frame=int(v[2]),request_sequence=int(v[3]),current_sequence=int(v[4]),request_sync_ns=int(v[5]),current_sync_ns=int(v[6]),applied=int(v[7]),fitted=int(v[8]),solved_mask=int(v[9]),kind=int(v[10]),weight=v[11],grip=poses[0],physical=poses[1],adjusted=poses[2],solved=poses[3],right=poses[4],target_solve_distance=distance(poses[2],poses[3]),target_solve_degrees=angle(poses[2],poses[3])))
    for row in rows:
        fresh=re.search(r'\bbody_passive_commit_'+str(row['index'])+r'_fresh_query=(\d+)',s)
        row['fresh_query']=int(fresh[1]) if fresh else 0
    out=dict(source=str(p),records=len(rows),rows=rows,pose_order=['grip','physical','adjusted','solved','right'],units='world game units; degrees',same_successful_C0_commit=True,render_upload_or_visible_fit_proved=False,fresh_query_records=sum(r['fresh_query']!=0 for r in rows))
    if rows:
        out['summary']=dict(frame_range=[rows[0]['frame'],rows[-1]['frame']],all_applied=all(r['applied'] for r in rows),position_fitted=sum(r['fitted'] for r in rows),weight_range=[min(r['weight'] for r in rows),max(r['weight'] for r in rows)],max_target_solve_distance=max(r['target_solve_distance'] for r in rows),max_target_solve_degrees=max(r['target_solve_degrees'] for r in rows))
        for key in ('grip','physical','adjusted','solved','right'):
            out['summary'][key+'_maximum_from_first']=dict(distance=max(distance(r[key],rows[0][key]) for r in rows),degrees=max(angle(r[key],rows[0][key]) for r in rows))
    destination=Path(sys.argv[2]);destination.write_text(json.dumps(out,indent=2));print(json.dumps(out.get('summary',out),indent=2))
if __name__=='__main__':main()
