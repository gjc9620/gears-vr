from pathlib import Path
import json,hashlib,datetime,subprocess
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent;sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=r/'analysis/instances/cover-1112-r283-pending-observation/cover-output';run=json.loads((b/'run-result.json').read_text())
p=r/'analysis/runtime/continuous-stereo-45472-20261112-153730-1e7c5a73.json';runtime=json.loads(p.read_text());f=runtime['native_status']
assert run['normal_cleanup_verified'] and not run['forced_termination'] and run['remaining_owned']=={}
assert f['entries_restored']=='1' and f['body_native_pending']=='0'
boundary=next(b.glob('menu*/cover-boundary.json'));phases=json.loads(boundary.read_text())['eye_observation'];assert len(phases)==5
summary=dict(result='bounded body/view check passed; historical stall unresolved',runner_exit=0,operator_exit=run['operator_exit'],normal_cleanup_verified=True,forced_termination=False,remaining_owned={},native_cleanup_verified=runtime['native_cleanup_verified'],entries_restored=f['entries_restored'],xr_exit=runtime['xr_exit_code'],cover_consumed=f['body_goal_cover_consumed'],cover_view_used=f['body_goal_cover_view_used'],cover_heading_used=f['body_goal_cover_heading_used'],cover_heading_seeded=f['body_goal_cover_heading_seeded'],native_pending=f['body_native_pending'],cover_pending_ready=f['cover_pending_ready'],phases=[dict(label=x['label'],consumed=x['native']['body_goal_cover_consumed'],view=x['native']['body_goal_cover_view_used']) for x in phases],runtime_report=str(p),runtime_report_sha256=sha(p),run_result_sha256=sha(b/'run-result.json'),grip_tested=False,headset_accepted=False)
(o/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
(o/'terminal-status-selected.json').write_text(json.dumps({k:v for k,v in f.items() if k.startswith(('body_native_','body_goal_cover_','cover_pending_','cover_consume_','body_cover_pose_','atom_overwrite_','fresh_pose_'))},indent=2)+'\n')
(o/'visual-review.json').write_text(json.dumps(dict(all_frames=271,overview_pages_personally_reviewed=list(range(1,7)),dense_frames_personally_reviewed=[187,206,225,244,263],observed=['Both submitted projection eyes remain world-visible throughout the reviewed timeline; no sustained black.','Turn and 0.10m rise visibly change framing and restoration returns to initial framing.','Entry still shows own arms/shoulder; large lower foreground body remains throughout cover; complete fit not accepted.'],full_hand_body_fit_accepted=False,headset_accepted=False,grip_tested=False,historical_stall_fixed=False),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text());a=r/'docs/cover-system-development-20261108.md';bcopy=Path(g['development_copy']['path']);assert a.read_bytes()==bcopy.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r283 观察记录版本实际连续通过，历史挂起未重现/未证实修复

r283 runner2602 exit0、operator0；r282 Native只增加观察器，私有entry启用既有first_failure和record_base_entry，不变更写入/恢复规则。cover applied/consumed237、body_cover_pose accepted237/rejected0、cover_view_used236、heading读取238/seeded0；五阶段consumed/view为34/34、76/76、118/118、161/161、204/204，均新增。pending终态0，cover_pending_ready0，无不可变挂起档案是本局未连续积累32样本，不是记录器游戏实证通过。不能把一次通过归因为观察器修复了r280/r281；二者失败与内部恢复失败继续保留。

normal_cleanup_verified=true，Native cleanup/独立entries恢复/XR exit0、所属进程为空、forced_termination=false。271份实际projection双眼图完整6页及近五阶段187/206/225/244/263亲自审阅：世界持续可见，无持续黑，转头/抬高和恢复可见；进入时自身臂/肩与cover期间下半部大面积自身身体仍明显，完整身体/手部拟合不通过，遮挡按owner后置，不扩展新遮挡系统。阶段图按保存wall/perf与QPC近似配对，相差约16..47ms，不称逐GPU帧绑定。Grip尚未测试。

下一步进行左右L2点的实际左Grip获取/拖动/释放保持/再抓反向，持续使用该受保护私有配置；若挂起再现，r282副本会保留实际branch/epoch/coverage。先以无Grip的三轴短手移动从现有hand_world诊断测得raw→world线性映射，直接定位实际L2点，验证新帧/当前消费，避免假定reference yaw为零或回用旧preview轴。需要记录interaction phase/edge/ownership/virtual offset区分输入意图与真正C0应用；每点GrabPose/手臂过渡、高掩体/双模式、出掩体恢复和实体头显交付仍未完成。公版/Quest/固定手动候选未改。
'''
raw=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(raw);bcopy.write_bytes(raw);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=[str(x.relative_to(r)) for x in o.iterdir() if x.is_file() and x.suffix in ('.json','.py','.log','.txt')]+[str(a.relative_to(r)),str(gp.relative_to(r))]
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=r).strip();subprocess.run(['git','add','-f','--',*files],cwd=r,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE);subprocess.run(['git','-c','core.whitespace=-blank-at-eof','diff','--cached','--check'],cwd=r,check=True)
v=subprocess.run(['git','commit','-m','Record real continuous cover view and normal cleanup with unresolved prior stalls'],cwd=r,check=True,stdout=subprocess.PIPE);print(v.stdout.decode().splitlines()[0]);print(json.dumps(summary))
