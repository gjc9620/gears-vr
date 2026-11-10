"""Preserve the source/build checkpoint and mirror the existing development log."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=load(H/'native-build-result.json');checks=load(H/'checks-v3/results.json')
assert len(checks)==1 and checks[0]['build_exit']==checks[0]['test_exit']==0
assert '50 checks, 0 failures' in checks[0]['output']
assert len(build['project_dependencies'])==408 and not build['external_worktree_dependencies'] and build['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
assert len(build['changed'])==2
verification=dict(scope='Source, synthetic immutable data, compiler/linker only; no game/XR started',
    dll_sha256=build['dll_sha256'],project_dependencies=408,external_worktree_dependencies=0,
    frozen_delta=build['changed'],flags_unchanged=True,checks=checks,parser_cases=8,
    measurement_policy_ns=50000000,tracking_policy_unchanged=True,
    gate_transform=load(H/'transform-verification.json'),
    initial_failures_preserved=dict(checks_v1='Existing generic Core test has fixed size/offset assertions that do not match current headers; importing it also produced fixture-name shadow warnings.',
        checks_v2='Boundary fixture incorrectly put newer controller clocks after its older publication; corrected synthetic cohort ordering, without changing validators.'),
    game_cause_proven=False,query_called=False,movement_authorized=False,public_changed=False,headset_accepted=False,
    remaining='One new exact private runtime check to read the first actual core predicate witness; retain original age and identity checks.')
(H/'verification.json').write_text(json.dumps(verification,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
guard=H.parent/'requirements-owner-sync.json';g=load(guard)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
before=doc.read_bytes();assert before==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r143：同一测量首个 core 拒绝的只读分项候选（2026-11-11，未实跑）

r142实际measurement258/core、query attempted0保留。新cover_measurement_core_observation.h只检查已持有数据：记录原policy.now、50ms上限、原publication/HMD/controller时间、票据/引用代次/头部标记、严格Core与原non-age谓词、控制器当前匹配，以及独立Tick模式的实际classify/rejection_detail和metadata/Tick原时间。未来/无效时钟不转换为巨大无符号年龄；旧Core年龄诊断与Tick分类分开，不把旧Scene过期直接解释为当前Tick过期。

沿用既有measurement首个失败CAS槽，同一条Scene/Compose/seq下附加cover_pose_probe_core_8 CSV；只在detail=core的首个失败写一次，release/acquire发布，不新增game read、clock、锁、writer、查询或自动重试。原接受条件、源选择、顺序和关闭复核保持；canonical及精确冻结测量函数去除observer参数后的文本与各自之前逐字相同。

专用合成数据50项通过，覆盖50ms边界/超界、publication/头部/控制器分别过期、未来时间、引用奇数、空票据、非法flags、旧Scene与合法当前Tick区分、Tick年龄/硬拒绝及输入字节保持；CSV解释8项通过。第一版复用旧通用Core测试因固定ABI尺寸/偏移断言与当前头不匹配而编译失败，日志保留，未改旧测试。第二版边界夹具时钟排列不合法而失败，按有效cohort顺序修正后第三版50项通过；没有放宽产品校验。

完整Native在r139原冻结树只改body_c0_goal_policy_live.inl并新增一个观察头，SHA5522ac2a49119075662a24176601bee613eec20467d672cdadeefe54d04ec093；408项目依赖均来自冻结树、外部worktree依赖0、GEARS开关相同、编译链接0。当前主线的Tick来源分支与冻结旧Core分支分别原样保留，未混入共享未提交Native改动。prepare新增--native-r143，只接受这两个delta与上述冻结编译证明。

本轮未启动游戏/XR，不能认定r142的core首败是年龄或其他具体条件，也没有新的查询/身体移动/画面通过证据。下一次用新票限定读取首个实际拒绝分项，保留50ms/100ms与原身份、输入、恢复检查；不重跑未改玩法矩阵。r142模型遮挡FAIL、完整身体/世界碰撞覆盖、CV01–24/AC01–22与头显仍开放；工程25%、publicv71、固定手动入口不变，目标active。
'''
updated=before+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,dll=build['dll_sha256'],checks=50,parser_checks=8,game_started=False)))
