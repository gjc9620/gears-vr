from pathlib import Path
import json,hashlib,datetime,subprocess
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=r/'analysis/instances/cover-1112-r280-heading-view/cover-output';run=json.loads((b/'run-result.json').read_text())
p=r/'analysis/runtime/continuous-stereo-38916-20261112-151502-b310f0fd.json';runtime=json.loads(p.read_text());f=runtime['native_status']
boundary=next(b.glob('menu*/cover-boundary.json'));v=json.loads(boundary.read_text())
recording=b/'projection/recording.json';frames=json.loads(recording.read_text())['frames']
assert run['remaining_owned']=={} and run['forced_termination']==False
summary=dict(result='failed',runner_exit=1,operator_exit=run['operator_exit'],normal_cleanup_verified=run['normal_cleanup_verified'],forced_termination=run['forced_termination'],remaining_owned=run['remaining_owned'],native_cleanup_verified=runtime['native_cleanup_verified'],entries_restored=f['entries_restored'],xr_exit=runtime['xr_exit_code'],cover_consumed=f['body_goal_cover_consumed'],cover_view_used=f['body_goal_cover_view_used'],cover_heading_used=f['body_goal_cover_heading_used'],cover_heading_seeded=f['body_goal_cover_heading_seeded'],native_pending=f['body_native_pending'],commit_guard_rejections=f['body_native_commit_code_12'],first_failure_recording_enabled=f['body_native_first_failure_enabled'],cause='unknown; commit guard rejected after one completed cover view; do not infer source age or view heading as cause',runtime_report=str(p),run_result_sha256=sha(b/'run-result.json'),runtime_report_sha256=sha(p),boundary_sha256=sha(boundary),frames=len(frames),paired_returned=False,headset_accepted=False)
(o/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
(o/'visual-review.json').write_text(json.dumps(dict(all_frames=len(frames),overview_pages_personally_reviewed=list(range(1,8)),observed=['Original world and approach visible in both projection eyes through frame174.','175-176 own shoulder/arm intrusion; 177-195 altered body/view with large foreground own body.','196-288 are black in both eyes. Continuous-view five phases fail; no visual acceptance.'],heading_fix_visual_accepted=False,grip_tested=False),indent=2)+'\n')
gpath=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gpath.read_text());a=r/'docs/cover-system-development-20261108.md';bcopy=Path(g['development_copy']['path'])
assert a.read_bytes()==bcopy.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r280 实际检查失败：一次cover消费后commit保护拒绝/恢复未完成

r280 runner69932 exit1、operator1。heading连续读取314（阶段最后）/362（终态）、seeded0，使用进入前已认证standing朝向，非旧preview fallback。cover applied/consumed/view只有1，后续转头阶段8秒内无新增消费/view，原检查及时报错。Native captures366、commits359、apply360，commit_code12 guard_rejected1、pending13；仅凭这些不能证明source-age、坐标或朝向造成失败。既有first-failure记录当时OFF，无法补造失败中的精确字段。r274也有退休阻塞历史，但不认定两者同因。

完整289份实际projection保存，亲自审阅7页全跨度概览：approach世界可见，175..176自身肩/臂显著，177..195大面积前景自身身体，196..288双眼全黑。此次连续观察画面不通过，不能把heading读取计数当显示成功；Grip/完整手臂尚未测。本次未增加遮挡或碰撞工作。

normal_cleanup_verified=false，Native cleanup deadline expired、entries_restored0，XR exit0；runner终态remaining_owned为空、forced_termination=false。所属进程结束与Native内部恢复验收分开，保留isolated-cleanup-pending.json，不抹除失败或绕过资源保护。runtime continuous-stereo-38916-20261112-151502-b310f0fd、closeout.json、terminal-status-selected.json、visual-review.json为证据。下一步仅启用既有diagnostic_first_failure在受保护私有入口记录首个commit失败/挂起的实际source/单位/上下文，先查清拒绝点，保持原有身份/时效/原子写入/恢复规则；不重新跑同一无首错记录配置。r279源码和离线/编译证据仍成立，尚无合格交付。
'''
data=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(data);bcopy.write_bytes(data);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gpath.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=[str(x.relative_to(r)) for x in o.iterdir() if x.is_file() and x.suffix in ('.json','.py','.log','.txt')]+[str(a.relative_to(r)),str(gpath.relative_to(r))]
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=r).strip()
subprocess.run(['git','add','-f','--',*files],cwd=r,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
subprocess.run(['git','-c','core.whitespace=-blank-at-eof','diff','--cached','--check'],cwd=r,check=True)
subprocess.run(['git','commit','-m','Preserve failed cover commit and full-span black-output evidence'],cwd=r,check=True,stdout=subprocess.PIPE)
print(json.dumps(summary))
