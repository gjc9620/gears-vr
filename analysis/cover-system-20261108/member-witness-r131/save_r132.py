"""Close the consumed r132 run without extending raw-query evidence into safety."""
from pathlib import Path
import datetime, hashlib, importlib.util, json, shutil, sys

H = Path(__file__).resolve().parent
R = H.parents[2]
out = H / 'runtime-r132'
load = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
def save(name, value):
    (out / name).write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

root = R / 'analysis/instances/cover-1110-r132-cloak-member-witness'
result = load(root / 'cover-output/run-result.json')
index = load(root / 'launcher-session.json')
session = result['terminal_session']
assert result['normal_cleanup_verified'] and not result['remaining_owned']
assert result['projection_thread_joined'] and result['operator_exit'] == 0
assert not result.get('error') and not result['projection_errors']
assert result['projection_frames'] == 276 and not result['forced_termination']
assert result['cloak_observations'] == dict(samples=626, failures=[])
assert session['unified_weapons_cleanup'] == dict(fire_disable_code=0, hand_stop_code=5, drained=True)
mute = index['test_audio_guard']['mute_verification']
assert mute['verified_muted_observations'] == 273
assert not mute['verify_failed'] and not mute['target_shared_rejected']
assert result['disk_before']['admitted'] and result['disk_before']['free_bytes'] == 6725824512
assert result['disk_before']['minimum_free_bytes'] == 4294967296
sys.path.insert(0, str(R / 'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control = WindowsControl()
identities = {k: session[k] for k in ('game_identity','owner_identity','xr_identity')}
identities['worker_identity'] = load(root / 'isolated-worker.json')['worker_identity']
for identity in identities.values():
    assert control.identity(identity['pid']) != identity

source = Path(index['session_report'])
saved = out / 'session-original.json'
if saved.exists():
    assert sha(source) == sha(saved)
else:
    shutil.copyfile(source, saved)
native = load(H / 'native-build-result.json')['dll_sha256']
query = module('cover_query', H.parent / 'source-stage-r104/inspect_probe.py').inspect(load(saved), native)
pose = module('cover_pose', H.parent / 'pose-probe-r126/inspect_pose_probe.py').inspect(load(saved), native)
assert query['queried_and_returned'] and query['code'] == 0
assert query['original_started'] == query['original_returned'] == 1
assert query['session_stage'] == query['engine_stage'] == 'complete'
assert query['raw']['raw_hit'] == '0'
assert query['raw']['layout_member_identities'] == '225' and query['raw']['layout_member_capacity'] == '0'
assert query['raw']['layout_step'] == '40' and query['raw']['layout_offsets'] == '584,592,96,108,104'
assert not pose['missing']
complete = next(r for r in pose['rows'] if r['name'] == 'complete')
measurement = next(r for r in pose['rows'] if r['name'] == 'measurement')
assert complete['count'] == 279 and measurement['count'] == 54 and measurement['measurement_status'] == 'core'
assert (complete['scene'], complete['compose'], complete['sequence']) == (447,27519,3345)
assert (measurement['scene'], measurement['compose'], measurement['sequence']) == (451,27767,3352)
save('query-probe.json', query)
save('pose-probe.json', pose)

media = load(out / 'media-review.json')
assert media['frames'] == 276 and media['encode_exit'] == 0
assert len(media['pages']) == 14 and len(media['contacts']) == 5
for record in media['pages'] + media['contacts']:
    assert sha(Path(record['file'])) == record['sha256']
media.update(full_span_personally_reviewed=True, all_contacts_personally_reviewed=True,
    visual_result='FAIL: severe own head/shoulder/torso obstruction in both eyes across all five cover phases; long static opening and intermittent hand visibility preserved',
    same_C0_GPU_proven=False, headset_accepted=False)
save('media-review.json', media)
boundary = load(next((root / 'cover-output').glob('menu-firstperson-*/cover-boundary.json')))
assert len(boundary['eye_observation']) == 5
assert all(p['observation']['cover_state_raw']['CoverType'] == 2 for p in boundary['eye_observation'])
reacquisitions = [s['registry_reacquisition'] for s in boundary['steps'] if s.get('registry_reacquisition')]
archives = []
for name in ('cover-1110-r128-cloak-registry','cover-1110-r130-cloak-pose-witness'):
    original = R / 'analysis/instances' / name
    target = Path('J:/gearsVr/analysis/cover-disk-recovery-20261110-r128/instances') / name
    assert original.is_junction() and original.resolve() == target.resolve()
    receipt = target.with_name(name+'-hashes.json')
    assert receipt.exists()
    archives.append(dict(original=str(original), target=str(target), manifest=str(receipt), sha256=sha(receipt)))
save('additional-owned-archive.json', archives)
save('closeout.json', dict(classification='progress: original Sphere query executed and returned after full member closure',
    identities=identities, exact_owned_processes_absent_at_closeout=True, native_sha256=native,
    original_report=str(source), original_report_sha256=sha(saved), input_and_phase_sequence=True,
    complete_member_witnesses=225, capacity_exhausted=False, original_query_started=1, original_query_returned=1,
    raw_hit=0, collision_clear=None, movement_authorized=False, complete_body_path_verified=False,
    capsule_enumeration_game_executed=False, probe_admission=complete, separate_measurement=measurement,
    same_C0_across_rows=False, registry_reacquisition_branch_observed=bool(reacquisitions),
    normal_cleanup_verified=True, unified_weapons_cleanup=session['unified_weapons_cleanup'],
    cloak=result['cloak_observations'], mute=mute, disk_before=result['disk_before'], visual_result='FAIL',
    public_changed=False, headset_accepted=False,
    remaining=['complete body path and endpoint with full world coverage and current occupancy',
        'Core measurement refusals in distinct C0s', 'same-pose C0 body/view/hit/muzzle consumers',
        'head and shoulder view obstruction', 'CV01-24 AC01-22 and physical headset acceptance']))

gp = H.parent / 'requirements-owner-sync.json'
guard = load(gp)
doc = R / 'docs/cover-system-development-20261108.md'
mirror = Path(guard['development_copy']['path'])
assert doc.read_bytes() == mirror.read_bytes() and sha(doc) == guard['development_copy']['sha256']
entry = '''
## r132：完整成员闭合后原版Sphere首次实际执行；双眼遮挡仍失败（2026-11-10）

r131源码已保存d59ff30d。新独立实例cover-1110-r132-cloak-member-witness使用Native4a9a6f6b…744b，原冻结PG/XR、Source1、0.4；game51668/creation134387980269591055、owner18892/creation134387980196534942、XR31304/creation134387980465030763。原版接近/A进入CT2及五段观察完成，operator0；本次没有抓边或虚拟身体移动。

首次Sphere requested/attempted/finished均1，code0、raw_hit0；Session complete7、Engine complete10，original_started/returned均0→1。完整LayoutStep40/pass2，225项独立成员见证、capacity0，offsets584,592,96,108,104，结束复核通过。Scene447/Compose27519/sequence3345，世界2448851200/gen1、Pawn95227143808、Link2302462176/slot0；原球(-3914.51929,-154.767059,614.237732,r1)，读取括号109300844682600→109300863139300ns。这证明该次原始球查询调用并返回，不证明完整形状/世界覆盖、身体路径、终点占用或移动许可；新capsule枚举接口仍未在游戏调用。

姿态预检complete279，首份同Scene447/Compose27519/sequence3345；measurement/core拒绝54，首份Scene451/Compose27767/sequence3352。它们属于不同C0，不拼成一次事务，也不以数量对比宣称时限或测量根因修复。后续Core拒绝单独保留。

276张真实私有双眼投影、错误0、线程joined、编码0；14张全跨度overview和5张原始phase接触图全部亲自查看。中立、转头、恢复、抬高、恢复五段均严重自身头肩/躯干遮挡，visualFAIL；开头长静止及间歇手部可见性保留。phase严格按事件区间内完整请求/观察括号选图，不称同C0或GPU消费证明。

正常停止、Native/入口/pad恢复，fire_disable0/hand_stop5/drainedtrue，XR0/launcher0，无强杀，保存时精确game/owner/XR/worker均不在。CLOAK626次失败0，静音验证273/verify_failed0/target_shared_rejected0。实际disk_before free6725824512≥4294967296；不追补r128缺失恢复回执。r128与r130仅本方已消费实例全文件SHA前后一致归档J盘、原C路径junction保持，各921792452/1017521687字节，未动其他实例。

证据member-witness-r131/runtime-r132/{session-original,query-probe,pose-probe,media-review,closeout,additional-owned-archive}.json。下一步接完整身体路径/终点的形状枚举与当前世界覆盖，再连接唯一C0身体提交及同版视图/命中/枪口；自身头肩遮挡和Core拒绝仍需分别修复。全部CV01–24/AC01–22未完成，工程25%、公共v71、固定手动入口不变，目标active。
'''
assert '## r132：' not in doc.read_text(encoding='utf8')
payload = doc.read_bytes() + entry.replace('\n','\r\n').encode('utf8')
doc.write_bytes(payload)
mirror.write_bytes(payload)
guard['development_copy']['sha256'] = sha(doc)
guard['as_of'] = datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(guard, ensure_ascii=False, indent=2)+'\n', encoding='utf8')
print(json.dumps(dict(saved=True, queried_and_returned=True, members=225, normal_cleanup=True, visual_result='FAIL')))
