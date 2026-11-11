"""Save the actual pre-dispatch owner-links refusal and incomplete recording."""
from pathlib import Path
import datetime, hashlib, importlib.util, json, shutil, sys
H = Path(__file__).resolve().parent
R = H.parents[2]
root = R/'analysis/instances/cover-1111-r170-cloak-age'
out = H/'runtime-r170'
load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
v = load(root/'cover-output/run-result.json')
s = v['terminal_session']
q = load(out/'path-probe.json')
t = load(out/'timing-probe.json')
age = load(out/'age-probe.json')
assert v['normal_cleanup_verified'] and v['operator_exit'] == 0 and not v.get('error')
assert not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames'] == 230 and v['projection_errors'] and v['projection_thread_joined']
assert v['anchor_sources_unchanged'] and len(v['anchor_source_sha256']) == 13
assert sha(root/'native/GearsCapture.dll') == q['native_sha256']
assert q['query_code_name'] == 'owner_links' and q['owner_qualification_when'] == 'before_dispatch'
assert q['exact_owner_reader_unknown'] and q['original_started'] == q['original_returned'] == 0
assert q['owner_guard']['calls'] == 39 and q['owner_guard']['failed_call'] == 0
assert q['session_guard']['calls'] == 8 and q['session_guard']['failed_call'] == 0
assert not q['source_age_cause']['observed'] and q['source_age_cause']['body_source_age'] is None
assert t['elapsed_ns'] == 22766500 and all(x['calls'] == x['closed_spans'] for x in t['rows'])
assert age['calls'] == [39,39] and len(age['rows']) == 2
assert all(x['verdict'] == 0 and not x['expired_fields'] for x in age['rows'])
assert age['rows'][0]['tick'] == 24496 and int(q['raw']['compose']) == 24497
spec = importlib.util.spec_from_file_location('r170_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
pose = reader.inspect(s,q['native_sha256'])
(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete = next(x for x in pose['rows'] if x['name'] == 'complete')
for key in ('scene','compose','sequence','world','pawn'):
    assert complete[key] == int(q['raw'][key])
boundary = next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'))
a = load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType'] == 2
assert len(a['eye_observation']) == 5
review = load(out/'media-review.json')
assert len(review['pages']) == 12 and len(review['contacts']) == 4 and review['encode_exit'] == 0
assert review['recording_errors'] and len(review['missing_contacts']) == 1
assert review['missing_contacts'][0]['label'] == 'cover_eye_rise_restored'
for item in review['pages'] + review['contacts']:
    assert sha(Path(item['file'])) == item['sha256']
review.update(captured_span_personally_reviewed=True, full_span_personally_reviewed=False,
    reviewed_pages=12, reviewed_original_contacts=4, captured_span_complete=False,
    verdict='Available phases have severe self-model obstruction; last phase missing; FAIL/incomplete',
    cover_view_accepted=False, headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c = WindowsControl()
ids = {k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity'] = load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():
    assert c.identity(identity['pid']) != identity
index = load(root/'launcher-session.json')
for source, name in ((Path(index['session_report']),'session-original.json'),
    (boundary,'cover-boundary-original.json'),(boundary.parent/'equipment-chain.json','equipment-chain-original.json'),
    (boundary.parent/'recording.json','action-recording-original.json')):
    shutil.copyfile(source,out/name)
summary = load(out/'closeout.json')
summary.update(identities=ids, exact_owned_processes_absent=True, native_sha256=q['native_sha256'],
    runner_exit=1, runner_failure='No complete fresh projection frame', original_low_cover_entered=True,
    same_C0_complete_query_key=True, query_elapsed_ns=t['elapsed_ns'], owner_qualification=q['first_rejection_layer'],
    exact_owner_reader_unknown=True, source_age_observation=age, original_query_started=0, original_query_returned=0,
    source_age_repaired=False, cover_view_accepted=False, world_coverage_verified=False,
    movement_authorized=False, headset_accepted=False, public_changed=False,
    nested_times_must_not_be_added=True,
    next='Locate exact owner-links reader and failed already-read relation; retain all checks. No unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp = H.parent/'requirements-owner-sync.json'
g = load(gp)
doc = R/'docs/cover-system-development-20261108.md'
mirror = Path(g['development_copy']['path'])
old = doc.read_bytes()
assert old == mirror.read_bytes() and sha(doc) == g['development_copy']['sha256']
assert b'## r170' not in old
entry = '''
## r170：来源年龄首查有效，原查询前owner_links拒绝；录像尾段缺失（2026-11-11）

d921621d保存后，cover-1111-r170-cloak-age使用Native 0de22cab…60e17完成一次现有后台path观察。准备、operator导入及实际五段动作0，原A进入CT2；13项启动来源前后相同。CLOAK710/失败0，前台430/owned_activation false。Native/entries/input恢复、XR和launcher正常结束，normal_cleanup true、无强杀，game/owner/XR/worker精确身份独立确认不在。runner退出1另因录像No complete fresh projection frame，不把正常生命周期与录像错误合并成整轮通过。

同键Scene398/Compose24497/seq3648/world2312601856/Pawn95251267840/thread57812，complete首键逐项一致。查询163050996290000→163051019056500共22.7665ms，code12=owner_links，原碰撞started/returned 0/0；Owner39次、Session8次均没有failed-call，来源年龄cause未观察到，body/equipment年龄原因是unknown而非false。原解释器误把残留engine source_guard/current阶段当首次拒绝，现按code10/11/12归为执行前owner资格失败；13项解释器检查通过，仍不知道asset/collider/components哪一reader及哪条关联失败。

新增年龄见证实际classify/current各39次，只有首次两行，没有first-age行。身体Tick24496与query Compose24497分别保留；身体stereo/controller来源3649与query seq3648也不混同。首次classify在query开始后0.1933ms，tracking age0.7929ms、stereo age16.7353ms、metadata age1.3459ms；stereo距原40ms限制尚23.2647ms。current独立时钟的capture age0.6200ms；本轮没有记录到年龄拒绝，不推广为历史年龄问题已修复，也不刷新时间或放宽期限。

initial physics0.7485ms、collider1.9149ms、components8.2071ms；重复physics8次共6.4971ms、collider8次共4.8254ms、components0次，engine11.5239ms已包含其内部复核，不重复相加。所有span闭合。新下一步是定位owner_links确切reader和已读关系，不再原样跑年龄候选。

230实际投影帧，记录只到25.0188883秒，录像捕获线程已收尾。12页已有跨度概览及4原始阶段图均亲看，中立/转头/恢复/升高仍严重自身模型遮挡；rise_restored区间没有完整捕获帧，保持缺失。仅从实际现存帧生成captured-span，编码0不改变原录像错误、缺尾和画面FAIL，不称全跨度/五阶段视频通过。原始动作、查询、年龄、分段、退出及审阅记录在source-age-detail-r169/runtime-r170。完整CV01–24/AC01–22未完成，工程25%、publicv71/固定入口及头显状态不变，目标active。
'''
new = old + entry.replace('\n','\r\n').encode('utf-8')
doc.write_bytes(new)
mirror.write_bytes(new)
g['development_copy']['sha256'] = sha(doc)
g['as_of'] = datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True, normal_cleanup_verified=True, recording_complete=False,
    owner_links_before_dispatch=True, exact_owner_reader_unknown=True, source_age_repaired=False)))
