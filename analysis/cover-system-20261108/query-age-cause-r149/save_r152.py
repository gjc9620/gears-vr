"""Save actual blocked walking and the separately checked pulse repair."""
from pathlib import Path
import datetime,hashlib,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r152'
root=R/'analysis/instances/cover-1111-r152-cloak-entry'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];review=load(out/'media-review.json')
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'))
a=load(boundary);motion=load(boundary.parent/'equipment-chain.json')['motion']
assert v['operator_exit']==1 and v['normal_cleanup_verified'] and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==175 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert a['error']=="RuntimeError('Controlled walking blocked before cover; no teleport fallback')"
assert not a.get('before_original_A') and not a.get('after_original_A') and not a.get('pose_request') and not a.get('eye_observation')
assert len(review['pages'])==9 and not review['contacts'] and review['encode_exit']==0
for p in review['pages']:assert sha(Path(p['file']))==p['sha256']
assert len(motion)==10 and all(row['position']==motion[-1]['position'] for row in motion[-4:])
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in identities.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((root/'cover-output/run-result.json','run-result-original.json'),(boundary,'cover-boundary-original.json'),
                    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),
                    (boundary.parent/'recording.json','action-recording-original.json'),
                    (root/'cover-output/operator.log','operator-original.log'),
                    (root/'isolated-done.json','isolated-done-original.json'),
                    (root/'isolated-start-request.json','first-start-request-original.json'),
                    (Path(index['session_report']),'session-original.json')):shutil.copyfile(source,out/name)
review.update(full_span_personally_reviewed=True,reviewed_pages=9,
    verdict='Blocked walking before A; overshooting navigation, self-body intersections and final self occlusion. No five eye phases executed.',
    cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
summary=dict(scope='Actual ordinary walking failure before A',identities=identities,exact_owned_processes_absent=True,
    normal_cleanup_verified=True,original_A_issued=False,pose_request_issued=False,query_cause='unknown; no request',
    motion=motion,frames=175,reviewed_pages=9,contacts=0,encode_exit=0,cloak=v['cloak_observations'],foreground=v['foreground_observations'],
    source_repair='Cover timed_pulse uses elapsed ramps and existing owner publisher; finally neutral, unchanged amplitude/arrival/stall/42-step limits.',
    offline_tests=11,offline_failures=0,initial_test_failure='Float comparison: 0.6799999999999999 >= 0.68; changed assertion to assertAlmostEqual, no product change.',
    r152_preparation_failure='Operator preparation initially ran before prepared.json existed; no writes/action/claim then; rerun only preparation after predecessor completed.',
    r153_prepared_only=True,r153_game_started=False,pulse_runtime_verified=False,
    world_coverage_verified=False,movement_authorized=False,gameplay_accepted=False,public_changed=False,headset_accepted=False)
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r151准入未出生、r152普通步行受阻与定时脉冲修正（2026-11-11）

r151等待活进程观察到名额释放后共享锁内准入再次被抢占；原60秒等待耗尽，typed ResourceUnavailable、exit1、stdout空，无worker.json/log、launcher-session或game。保留claim/start-request和原stderr，normal_cleanup_required false、verified false，不补造正常清理。8490509b已保存，未重用请求。

新r152 Native ac5cf37f…ebda、PG/XR和共享工具哈希不变。准备时本方未先等前一步句柄终态，首次operator准备因prepared.json尚未存在退出1；随后确认前一步0且未产生operator/claim，重新完成准备和导入检查，资源等待仍在继续，没有因此启动两次游戏。本失误与实际游戏失败分别保存。实际game38096/creation134388136853985148、worker51936/creation134388136771900244。原版A之前的步行已失败Controlled walking blocked before cover；未发A、未取before_original_A、未发pose_request、五段eye_observation未执行，同查询年龄分项仍未知。

十个实际motion观测：距接近点161.303→113.885→207.802→341.589→233.565→269.479→180.252，最后四位置完全相同(-3913.15576,-307.79456,579)。显示越过目标后反复方向修正，最后触发原停滞限制；不能只据此认定碰撞或死区原因。175投影帧/errors0，9页全跨度亲看，编码0，接触阶段0；后段身体穿插/遮挡仍FAIL。Native/entries/input/武器/XR/launcher正常结束，独立精确身份均不在、无强杀；closeout及原始结果在query-age-cause-r149/runtime-r152。

检查驱动发现ctx.stick采用固定14次渐变发布，ctx.live/put耗时叠加到每一档保持；本次相邻motion间隔0.74–1.56秒，不能当作已测每次发布延迟，但固定次数时序缺陷由代码可见。修正cover_ordinary_approach改为实际经过时间推进0.14秒升/降渐变，沿用原0.04/0.22秒中段、0.62幅度、35到达界、3次停滞及42步上限；同一原g.put发布端、每次身份检查及finally零输入释放，没有新线程/写入端或位置/旋转setter。慢调用仍可能超时，记录实际时长和超时量，不声称硬实时保证，也不能声称入口已修好。

11项离线检查通过，包括实际脉冲慢检查/慢发布/发布异常/finally释放/停钟上限及原导航限制；初次仅浮点比较0.6799999999999999与0.68失败，改为近似断言，产品未为测试改门。prepare_operator将实际approach源码封入本实例operator并记录SHA，避免之后共享文件变化改变已准备动作。r153新私有实例已准备、实际冻结脚本导入检查0，仅准备未运行。下一步用此已保存候选做一次定向入口/查询检查，保留r150/r152实际失败，不重跑旧claim。完整CV01–24/AC01–22仍开放，工程25%、publicv71及固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,reviewed_pages=9,pulse_runtime_verified=False)))
