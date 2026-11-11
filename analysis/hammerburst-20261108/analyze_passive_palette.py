"""Offline original CPU render-consumer correlation; no game/input access."""
from pathlib import Path
import csv,json,sys,collections
import numpy as np
def main():
    csv_path,commit_path,output=map(Path,sys.argv[1:4])
    commits=json.loads(commit_path.read_text())['rows']
    by_compose={r['compose']:r for r in commits}
    assert len(by_compose)==len(commits),'Repeated compose in successful passive records'
    rows=[];identities=set()
    with csv_path.open(encoding='utf-8-sig') as stream:
        for r in csv.DictReader(stream):
            if r['bone'] not in ('11','35'):continue
            identities.add((r['game_pid'],r['native_started_qpc']))
            matrix=lambda name:np.array([float(r[f'{name}_{i}']) for i in range(16)]).reshape(4,4)
            component=matrix('component')
            selected=matrix('selected_space')@component
            palette=matrix('palette_space')@component
            item=dict(at_ns=int(r['at_ns']),selected_ns=int(r['selected_ns']),bone=int(r['bone']),
                native_compose=int(r['native_compose']),completed_compose=int(r['completed_compose']),
                source_bracket_consistent=int(r['source_bracket_consistent']),
                native_end_ns=int(r['native_end_ns']),native_commit_result=int(r['native_commit_result']),
                native_consumer_result=int(r['native_consumer_result']),native_finish_result=int(r['native_finish_result']),
                native_solved_mask=int(r['native_solved_mask']),native_original_called=int(r['native_original_called']),
                selected_world=selected.tolist(),palette_world=palette.tolist(),
                selected_palette_distance=float(np.linalg.norm(selected[3,:3]-palette[3,:3])),
                shader_valid=int(r['shader_valid']),gpu=r['gpu'],data=r['data'])
            c=by_compose.get(item['native_compose'])
            if c:
                solved=c['solved'] if item['bone']==11 else c['right']
                target=np.array(solved[:3]);item.update(passive_frame=c['frame'],passive_weight=c['weight'],
                    passive_current_sync_ns=c['current_sync_ns'],same_last_compose_record=True,
                    selected_C0_solved_distance=float(np.linalg.norm(selected[3,:3]-target)),
                    palette_C0_solved_distance=float(np.linalg.norm(palette[3,:3]-target)),
                    original_C0_solved_pose=solved)
            rows.append(item)
    assert len(identities)==1,identities
    matched=[r for r in rows if r.get('same_last_compose_record') and r['bone']==11]
    full=[r for r in matched if r['passive_weight']==1]
    summary=dict(identity=list(identities)[0],wrist_rows=len(rows),left_matched_last_compose=len(matched),
        left_full_weight=len(full),strict_payload_provenance=False,
        limits='CPU selected SpaceBases and actual queued palette reconstruction. Native compose is latest Update observation, not proof that this compose produced the queued payload. No hardware/everyDraw/anatomy/headset acceptance.')
    for name,group in (('all_matched',matched),('full_weight',full)):
        if group:summary[name]={key:dict(min=min(r[key] for r in group),max=max(r[key] for r in group)) for key in ('selected_palette_distance','selected_C0_solved_distance','palette_C0_solved_distance')}
    result=dict(summary=summary,rows=rows)
    output.write_text(json.dumps(result,indent=2));print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
