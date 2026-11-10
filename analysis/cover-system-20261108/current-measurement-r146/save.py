"""Save exact source reuse, test dependencies and compiled-only status."""
from pathlib import Path
import datetime,hashlib,json,subprocess
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=load(H/'native-build-result.json');test=load(H/'checks-v3/result.json')
assert test['build_exit']==test['test_exit']==0 and '31 checks, 0 failures' in test['output']
assert len(build['project_dependencies'])==408 and not build['external_worktree_dependencies'] and build['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
manifest=load(H/'native-source-manifest.json')['files'];dependencies={};outside=[]
for raw in load(H/'checks-v3/dependencies.json')['Data']['Includes']:
    p=Path(raw).resolve()
    if p.is_relative_to(H/'native-source'):
        rel=p.relative_to(H/'native-source').as_posix();assert sha(p)==manifest[rel];dependencies[rel]=sha(p)
    elif p.is_relative_to(R):
        assert p==R/'tools/native-capture/cover_measurement_core_fixture.h';outside.append(dict(path=str(p),sha256=sha(p),role='explicit test-only immutable fixture'))
existing=subprocess.check_output(['git','show','2f042279:tools/native-capture/body_goal_view_adapter.h'],cwd=R,text=True,encoding='utf-8')
start=existing.index('// Math over an independently admitted source.');end=existing.index('inline Status place_cover(',start)
block=existing[start:end]
actual=(H/'native-source/tools/native-capture/body_goal_view_adapter.h').read_text(encoding='utf-8-sig')
assert actual.count(block)==1
verification=dict(dll_sha256=build['dll_sha256'],build_only=True,game_started=False,query_called=False,
    source_reused_from_commit='2f042279',measurement_block_sha256=hashlib.sha256(block.encode()).hexdigest(),
    project_dependencies=408,external_native_worktree_dependencies=0,flags_unchanged=True,
    tests=test,test_project_headers=dependencies,test_only_external_fixture=outside,
    previous_fixture_failures_preserved='checks-v1/v2: head-only synthetic Core had flags4 and no valid independent controller cohort; original classify rejected hard 0xcd08. v3 uses existing complete Core/controller fixture, without changing product checks.',
    current_tracking_limit_ns=40000000,current_metadata_limit_ns=50000000,legacy_core_limit_ns=50000000,
    original_timestamps_preserved=True,current_cover_key_checks_preserved=True,body_writer_or_movement_permission_added=False,
    public_changed=False,headset_accepted=False)
(H/'verification.json').write_text(json.dumps(verification,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
before=doc.read_bytes();assert before==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r146：冻结候选补入既有当前Tick测量接线（2026-11-11，已编译未实跑）

依据r145首个原头部58.7917ms/50ms拒绝，复用2f042279中主线已有的measure_cover_pose_math及Core/BodyTickSource两个测量重载，按原文区块补入r143冻结body_goal_view_adapter；其余place/prepare/目标转换与共享Native保持冻结。已有BodyTickSource开启时使用context.body_source，否则沿用原Core。诊断同样按实际分支记录mode，不再把V3的身份parent年龄当成其当前追踪年龄。

原V3 classify与当前CoverType/Link/slot准入及结束复核保持；独立追踪40ms、元数据50ms、旧Core50ms均不变。测量观察时间取本次metadata_end_ns，Scene/publication/ticket和原追踪时间逐字保留，不修改Core、不重取输入、不授予查询/身体写入/移动许可。当前主线本来已有此接线，本次是实际私有冻结候选的窄整合，不用整个共享脏树重建。

31项实际冻结API离线检查通过：老Core仍拒绝、合法当前Tick测量可用、同原C0几何不重复叠加HMD、原Input/Frame字节保持、绑定原metadata_end/Scene/publication、8类过期/引用/publisher/cohort/未来时间负例及覆盖Input的别名拒绝。前两版借用head-only Core夹具缺少合法独立controller cohort，原classify以hard0xcd08拒绝，失败日志保留；第三版使用既有完整Core/控制器夹具后通过，没有改产品判据。骨骼数据来自保存的实际reference/C0，只验证数学/读取契约，不称实际身体移动。

完整Native编译链接0，SHA49f76f0135f8393ff7035afc51450401e484f4a10cf95c6c7511090c1201a472；r143上精确2文件delta、408项目依赖、外部Native worktree依赖0、GEARS开关相同。测试头依赖另逐项核SHA，只有明确的自有测试夹具来自主目录。prepare新增--native-r146并验证该冻结delta；本轮没有再开游戏。下一次用新票限定复查测量完成及原path查询，不重复未改玩法矩阵。

r145只是首个实际旧头部超时的原因证据；r146未实跑，不能称查询、碰撞覆盖或画面修复通过。模型遮挡、完整身体与世界几何、抓边/松手保持/重抓、暴露/命中/枪口、CV01–24/AC01–22及头显保持开放；工程25%、publicv71与固定入口不变，目标active。
'''
updated=before+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,dll=build['dll_sha256'],checks=31,game_started=False)))
