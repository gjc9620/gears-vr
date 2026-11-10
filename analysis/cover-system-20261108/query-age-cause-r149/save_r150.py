"""Preserve the real ordinary-entry failure; never infer a query cause."""
from pathlib import Path
import datetime, hashlib, json, math, shutil, sys
H=Path(__file__).resolve().parent; R=H.parents[2]
root=R/'analysis/instances/cover-1111-r150-cloak-cause'; out=H/'runtime-r150'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
result=load(root/'cover-output/run-result.json'); session=result['terminal_session']
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'))
action=load(boundary); review=load(out/'media-review.json')
assert result['operator_exit']==1 and result['normal_cleanup_verified']
assert not result['remaining_owned'] and not result['forced_termination'] and result['anchor_sources_unchanged']
assert result['projection_frames']==202 and not result['projection_errors'] and result['projection_thread_joined']
assert action['error']=="AssertionError('Original A did not enter low cover')"
assert action['after_original_A']['cover_state_raw']['CoverType']==0
assert not action.get('pose_request') and not action.get('eye_observation')
assert len(review['pages'])==11 and len(review['contacts'])==0 and review['encode_exit']==0
for page in review['pages']: assert sha(Path(page['file']))==page['sha256']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl(); identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values(): assert c.identity(identity['pid'])!=identity
for source,name in ((root/'cover-output/run-result.json','run-result-original.json'),
                    (boundary,'cover-boundary-original.json'),
                    (boundary.parent/'recording.json','action-recording-original.json'),
                    (root/'isolated-done.json','isolated-done-original.json'),
                    (root/'isolated-start-request.json','first-start-request-original.json'),
                    (root/'cover-output/operator.log','operator-original.log')):
    shutil.copyfile(source,out/name)
index=load(root/'launcher-session.json');shutil.copyfile(index['session_report'],out/'session-original.json')
review.update(full_span_personally_reviewed=True,reviewed_pages=11,
    verdict='Original A failed to enter low cover; final frames show roll/body intersection. No five-phase eye observation executed.',
    cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n',encoding='utf-8')
before=action['steps'][-2]['pose']; after=action['after_original_A']['pose']; chosen=action['chosen_candidate_only']['computed']
p=chosen['position']; f=chosen['forward']; delta=[before['position'][i]-p[i] for i in range(3)]
summary=dict(scope='Actual ordinary entry failure before pose/query request',identities=identities,
    exact_owned_processes_absent=True,normal_cleanup_verified=True,operator_exit=1,
    error=action['error'],before_A=before,after_A=after,chosen_candidate_only=chosen,
    before_A_planar_slot_distance=math.hypot(*delta[:2]),
    before_A_normal_distance=sum(delta[i]*f[i] for i in (0,1)),
    before_A_tangent_distance=sum(delta[i]*chosen['right'][i] for i in (0,1)),
    pose_request_issued=False,query_cause='unknown; ordinary entry failed before request',
    frames=202,reviewed_pages=11,contacts=0,encode_exit=0,
    cloak=result['cloak_observations'],foreground=result['foreground_observations'],
    world_coverage_verified=False,movement_authorized=False,gameplay_accepted=False,
    public_changed=False,headset_accepted=False,
    next='Compare actual entry geometry/input trajectory before any new run; do not infer source-age cause or repeat unchanged operator.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md'; mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r150：原版A未进入矮掩体，年龄分项诊断未触发（2026-11-11）

基于已保存24e0d72c，用新实例cover-1111-r150-cloak-cause准备r149 Native ac5cf37f…ebda，冻结PG/XR及共享隔离工具哈希不变。等待进程实际存活并观察原名额拒绝，资源释放后只调用既有runner一次；没有重用旧claim或启动请求，没有修改原时限、身份及输入保护。

实际game34696/creation134388127400340663、worker4360/creation134388127260774890。原始A之后CoverType仍0，operator1并保存Original A did not enter low cover。未发送pose_request，五段eye_observation未执行，因此本轮没有可用的同查询年龄原因；不能将未观察到cause解释为新鲜，也不能声称r149已定位body/equipment分项。进入前人物(-3982.51294,-145.28207,579)，控制yaw -4218；A之后(-3484.23877,-271.63196,583.015625)，仍CT0。与r148相同候选槽位但实际接近轨迹不同，原因尚未证明，不能归咎Native观察器或直接重跑。

202实际投影帧/errors0；11页全跨度已逐页亲看，编码0，原始五阶段接触图0张，不补造。前段可见持续初始场景，后段导航、滚动及身体穿插；完整掩体画面未验收。Native/entry/input/武器恢复、XR及launcher0、正常清理true，无强杀，独立核对本实例精确身份均不存在。原失败、session、操作记录和媒体哈希保存在query-age-cause-r149/runtime-r150。

下一步先核对本次普通进掩体前后的导航/方向/距离及输入记录，针对实际失败改进后再用新实例继续r149同查询诊断；不放宽任何来源年龄或碰撞证明。完整CV01–24/AC01–22仍开放，工程25%、publicv71和固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,query_cause_observed=False,reviewed_pages=11)))
