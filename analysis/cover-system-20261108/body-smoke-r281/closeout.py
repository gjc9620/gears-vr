from pathlib import Path
import json,hashlib,datetime,subprocess
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=r/'analysis/instances/cover-1112-r281-first-failure/cover-output';run=json.loads((b/'run-result.json').read_text())
p=r/'analysis/runtime/continuous-stereo-54060-20261112-152204-87de17e1.json';runtime=json.loads(p.read_text());f=runtime['native_status']
assert f['body_native_first_failure_enabled']=='1' and run['remaining_owned']=={} and not run['forced_termination']
assert f['body_native_commits']=='354' and f['body_native_apply_calls']=='354' and f['body_native_pending']=='13'
summary=dict(result='failed',runner_exit=1,operator_exit=run['operator_exit'],normal_cleanup_verified=False,forced_termination=False,remaining_owned={},native_cleanup_verified=runtime['native_cleanup_verified'],entries_restored=f['entries_restored'],xr_exit=runtime['xr_exit_code'],cover_consumed=f['body_goal_cover_consumed'],cover_view_used=f['body_goal_cover_view_used'],cover_heading_used=f['body_goal_cover_heading_used'],cover_heading_seeded=f['body_goal_cover_heading_seeded'],native_pending=f['body_native_pending'],commit_guard_rejections=f.get('body_native_commit_code_12','0'),first_failure_recording_enabled=True,first_failure_archive_present=False,cause='unproven; completed cover output remains awaiting authenticated original overwrite. This run had no rejected commit.',runtime_report=str(p),runtime_report_sha256=sha(p),run_result_sha256=sha(b/'run-result.json'),actual_first_consumption_parts=5,headset_accepted=False)
(o/'closeout.json').write_text(json.dumps(summary,indent=2)+'\n')
(o/'terminal-status-selected.json').write_text(json.dumps({k:v for k,v in f.items() if k.startswith(('body_native_','body_goal_cover_','cover_consume_','body_cover_pose_','atom_overwrite_','fresh_pose_'))},indent=2)+'\n')
(o/'visual-review.json').write_text(json.dumps(dict(all_frames=283,overview_pages_personally_reviewed=list(range(1,7)),observed=['World/approach visible through frame168.','169 onward original arms and then large own-body foreground.','192-282 both projection eyes black.'],accepted=False,grip_tested=False),indent=2)+'\n')
gpath=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gpath.read_text());a=r/'docs/cover-system-development-20261108.md';bcopy=Path(g['development_copy']['path'])
assert a.read_bytes()==bcopy.read_bytes() and sha(a)==g['development_copy']['sha256']
text='''

### r281 既有首错开关的针对性观察：已消费字段等待原始覆写，尚不能证明r280同因

同一r279-v4生产DLL，仅私有入口diagnostic_first_failure从0改1，保留全部原隔离/身份/时效/资源/恢复规则，未重编译、未重装或改手动包。runner38329 exit1、operator1；cover applied/consumed/view仍只有1，heading读取366/seeded0，pending13。关键区别：本局354次apply全部commit成功，commit_code12=0，没有首个commit失败可导出；不能沿用r280的commit拒绝解释本局。首个cover消费finish4/read0/failure0/protocol0，原函数返回、消费注册/安装均真实；13项已消费字段等待后续经认证原始覆写，之后不能建立新capture，原因仍未证实。

13项为root(0)q/t、3/4/5q、54/55q、91/92/93/95/96/97q；只作实际mask解码，不是放宽退休证据的依据。5份独立首消费二进制已从native目录复制并回读hash/大小，保存在body-smoke-r281/actual-inputs和manifest，不能冒称失败瞬间或后续覆写快照。完整283份真实projection全部6页概览亲自查看：前段世界/approach可见，169后自身臂/大面积身体，192..282双眼黑，画面不通过。

normal_cleanup_verified=false、Native entries_restored0，XR exit0、remaining_owned为空、forced_termination=false，保留内部恢复失败。既有export_pending要求stopping且callbacks/obligations均0，恰在当前13项挂起期间无法导出；metadata_reject_trace默认也未开启，因此不能声称已有后续branch轨迹。下一项是仅为该具体未完成覆写记录做GT有界采样→原子发布不可变副本→外部线程读出，类似r275首消费观测，不让导出依赖债务消失，不允许GT文件I/O/第二写入者/虚假退休。需离线验证不可变/线程边界后再做一次针对性观察，不继续无新增记录的相同游戏运行。掩体完整交付/Grip链/实体头显仍未完成。
'''
data=a.read_bytes()+text.replace('\n','\r\n').encode();a.write_bytes(data);bcopy.write_bytes(data);g['development_copy']['sha256']=sha(a);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gpath.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
files=[str(x.relative_to(r)) for x in o.iterdir() if x.is_file() and x.suffix in ('.json','.py','.log','.txt')]+[str(a.relative_to(r)),str(gpath.relative_to(r))]
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=r).strip()
subprocess.run(['git','add','-f','--',*files],cwd=r,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE);subprocess.run(['git','-c','core.whitespace=-blank-at-eof','diff','--cached','--check'],cwd=r,check=True)
result=subprocess.run(['git','commit','-m','Record consumed-cover overwrite stall and immutable diagnostic requirement'],cwd=r,check=True,stdout=subprocess.PIPE);print(result.stdout.decode().splitlines()[0]);print(json.dumps(summary))
