from pathlib import Path
import datetime,hashlib,json,re,subprocess
from instrument import instrument
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
baseline=H.parent/'layout-witness-r124/native-source/tools/native-capture/body_c0_goal_policy_live.inl'
frozen=H/'native-source/tools/native-capture/body_c0_goal_policy_live.inl'
assert frozen.read_text()==instrument(baseline.read_text())
before=subprocess.check_output(['git','show','61e93fd1:tools/native-capture/body_c0_goal_policy_live.inl'],cwd=R).decode('utf8')
assert (R/'tools/native-capture/body_c0_goal_policy_live.inl').read_text()==instrument(before)
receipt=load(H/'native-build-result.json');assert receipt['changed']==['tools/native-capture/body_c0_goal_policy_live.inl']
assert not receipt['external_worktree_dependencies'] and receipt['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==receipt['dll_sha256']
test=load(H/'checks-v8/results.json')[0];assert not test['build_exit'] and not test['test_exit'] and len(test['cases'])==8
check_source=H/'check-source/tools/native-capture/body_c0_goal_policy_live.inl'
assert check_source.read_bytes()==(R/'tools/native-capture/body_c0_goal_policy_live.inl').read_bytes()
deps=load(H/'checks-v8/body_c0_goal_policy_live-dependencies.json')['Data']['Includes']
project={}
for raw in deps:
    p=Path(raw)
    if p.is_relative_to(R):project[str(p.relative_to(R))]=sha(p)
checks=sum(int(re.search(r': (\d+) checks', (H/'checks-v8'/f'case-{n}.log').read_text()).group(1)) for n in range(16,24))
assert checks==158
enum=re.search(r'enum class CoverProbeStep:unsigned \{(.*?)\};',frozen.read_text(),re.S).group(1)
stages={str(n):v for n,v in enumerate(enum.split(',')) if v!='count'}
(H/'probe-stages.json').write_text(json.dumps(stages,indent=2)+'\n')
record=dict(classification='progress',native_sha256=receipt['dll_sha256'],project_dependencies=len(receipt['project_dependencies']),
    diagnostic_only=True,game_started=False,query_executed=False,movement_authorized=False,headset_accepted=False,
    canonical_original_commit='61e93fd1',canonical_sha256=sha(R/'tools/native-capture/body_c0_goal_policy_live.inl'),
    frozen_original_sha256=sha(baseline),frozen_instrumented_sha256=sha(frozen),
    transformations_exact=True,source_choice_and_predicate_order_preserved=True,extra_game_reads=False,
    canonical_and_candidate_difference='canonical has current Tick measurement overload and completed consumer; r124 frozen candidate retains earlier Scene measurement and original completion policy; only add witnesses to both',
    tests=test,checks=checks,test_dependencies=project,
    fixture='existing recorded body geometry plus explicit synthetic Core; actor/property/query are Host seams',
    private_fixture_scope='archive61e93fd1 + owned method/test; exclude unused cosmetic include and unexecuted mode>=26 block, retain actual pose_begin; no production weakening',
    failed_attempts=['v1/v2: shared unfinished authored hand dependencies; missing Vector definition in old Goal test seam',
        'preparation git archive included ignored third-party path and refused before archive creation',
        'v3/v4: isolated fixture missing ignored unused hand_part_grasp include',
        'v5: unused cosmetic Preset dependency and newer mode>=26 consumer APIs not in committed source',
        'v6/v7: private branch extraction erroneously removed actual goal pose_begin; witness assertions caught failures; exact brace-bounded extraction fixes it'],
    remaining=['actual pre-probe reject reason','actual PhysicsRoles layout field','original query execution','full body path endpoint world coverage and C0 commit','head shoulder obstruction','CV01-24 AC01-22 headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=H.parent/'requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry=f'''
## r126：掩体姿态测量前置拒绝见证，编译与定向检查（2026-11-10）

上轮r125已正常CT2但probe attempted0，本轮继续定位之前的掩体姿态测量。核对发现当前canonical早已包含035f9d7f的当前Tick测量重载与e60a3af7完成消费者核验，而冻结r124继承的r102候选仍使用更早Scene测量/原完成逻辑。版本差异成立，是否导致r125未测量仍未证明；不将差异直接说成根因，也不盲目切算法或混入共享在研源码。

在同一sample_current_cover_pose增加17类固定见证：stopping/idle、current、key/type/slot/link、marker及measurement原Status、closing key/current/stop/revision、缺entry、complete与exception。每项只保存首份不可变Scene/Compose/sequence/world/Pawn/原Status，固定计数；摘要通过acquire读已完成槽，不在原查询/读取期间持锁。不增加游戏内存读取、原调用或身体写者；保持每个谓词原顺序和原返回语义，complete只表示到达probe入口，不是查询/移动许可。

同一instrument变换分别用于canonical与r124冻结方法，并逐文本复核；冻结候选仅body_c0_goal_policy_live.inl一文件变化，405项项目依赖、外部工作树依赖0、开关不变，Native编译链接exit0，SHA256 {receipt['dll_sha256']}。本次未启动游戏/XR，未替换公共v71或固定手动入口。canonical的Tick/完成消费者改动保持，但本私有DLL未暗中吸收它们。

实际Goal/adapter/marker/Backend/Kernel路径定向8场景（16–23）共158检查0失败，W4/WX/O2/UNDEBUG编译通过；身份/属性/Core/原查询为明确Host缝，未当游戏证据。共享手型在研导致首轮编译失败；隔离61e93fd1源码后只叠本方方法/测试，排除未使用的cosmetic include及不执行的mode>=26新API分支。首轮私有截取误删实际pose_begin，断言实际拒绝，改为精确括号范围恢复真实入口后v8全通过。失败编译、私有断言与准备错误完整保留，不改产品保护来通过。

证据pose-probe-r126/{{verification,probe-stages,native-build-result}}.json、checks-v8及prepare_checks.py/instrument.py。下一步使用新候选一次定向后台验证，先取得probe前拒绝项，再决定如何接回已存在的当前Tick测量。完整CV01–24/AC01–22、头肩遮挡、身体碰撞/占用/唯一C0提交与联合战斗、物理头显仍未通过；工程25%不变，目标active。
'''
assert '## r126：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native=receipt['dll_sha256'],checks=checks,game_started=False)))
