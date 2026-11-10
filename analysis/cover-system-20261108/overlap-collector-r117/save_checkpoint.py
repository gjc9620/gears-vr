from pathlib import Path
import json,hashlib,datetime,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf8'))
build=load(H/'native-build-result.json');manifest=load(H/'native-source-manifest.json')['files']
production=['cover_physx_overlap_stream.h','cover_physx_stream.h','cover_physx_query.h']
for name in production:
    assert sha(R/'tools/native-capture'/name)==manifest['tools/native-capture/'+name]
assert not build['external_worktree_dependencies'] and build['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
checks=load(H/'checks-v2/results.json');assert all(c['build_exit']==0 and c['test_exit']==0 for c in checks)
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
control=WindowsControl();runtime=[]
for revision in ('r118','r119','r120'):
    root=R/('analysis/instances/cover-1110-'+revision+'-cloak-source-stage')
    result=load(root/'cover-output/run-result.json');session=result['terminal_session'];index=result['terminal_state']
    assert session['native_status']['cover_sphere_probe_attempted']=='0'
    assert not result['remaining_owned'] and not result['normal_cleanup_verified']
    assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
    assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
    for key in ('game_identity','owner_identity','xr_identity'):
        assert control.identity(session[key]['pid'])!=session[key]
    mute=index['test_audio_guard']['mute_verification']
    assert mute['initially_muted_sessions']==1 and mute['verified_muted_observations']>0 and not mute['verify_failed'] and not mute['target_shared_rejected']
    runtime.append(dict(revision=revision,game=session['game_identity'],xr=session['xr_identity'],
        result=str(root/'cover-output/run-result.json'),result_sha256=sha(root/'cover-output/run-result.json'),
        query_attempted=False,error=result.get('error'),operator_exit=result.get('operator_exit'),
        projection_frames=result.get('projection_frames'),projection_error=result.get('projection_error'),
        projection_thread_joined=result.get('projection_thread_joined'),cloak=result['cloak_observations'],
        mute_verification=mute,normal_cleanup_verified=False,unified=session['unified_weapons_cleanup'],
        native_restore=True,entry_restore=True,pad_restore=True,xr_exit=0,launcher_exit=0,owned_absent=True))
for revision in ('r119','r120'):
    p=H/('review-'+revision)/'media.json';data=load(p)
    assert data['encode_exit']==0
    data['full_span_personally_reviewed']=True
    data['review']='All four overview pages personally viewed: actual stereo scene and both hands throughout; no cover action. No hand contact or cover acceptance.'
    p.write_text(json.dumps(data,indent=2)+'\n')
next_root=R/'analysis/instances/cover-1110-r121-cloak-source-stage'
assert not (next_root/'launcher-session.json').exists() and not (next_root/'cover-output/claim.json').exists()
record=dict(classification='progress',production={n:sha(R/'tools/native-capture'/n) for n in production},
    native_sha256=build['dll_sha256'],project_dependencies=len(build['project_dependencies']),flags_unchanged=True,
    final_checks=checks,raw_collector_only=True,enumeration_query_integrated=False,endpoint_occupancy_verified=False,
    original_enumeration_executed=False,public_changed=False,runtime=runtime,
    recorder_checks=load(H.parent/'isolated-r105/projection-check.json'),
    cloak_checks=load(H.parent/'isolated-r105/cloak-guard-check.json'),
    reader_checks=load(H.parent/'isolated-r105'/next_root.name/'reader-check.json'),
    next_instance=next_root.name,next_started=False,remaining=['source4 original failure','typed/code-closed original overlap enumeration','self-aware endpoint/world coverage','full C0 body/eye/hit/muzzle transaction','early stop fire disable2','CV01-24 AC01-22 and headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r117–r121：终点形状收集器、录制与独立读取器修复（2026-11-10）

新增 cover_physx_overlap_stream.h，按 r114 的九参数原版形状枚举 ABI 收集8字节公共形状指针，保留顺序／重复与所有报告；固定64项caller batch、1024项输出及报告预算。允许caller batch内的合法剩余子区间（含零count one-past），错位／越界／未认证的原分配缓冲区明确拒绝而非视为空。容量／报告耗尽保持truncated，返回count不能掩盖未收齐；坏receiver、跨线程、过期回调与别名不读陌生对象／不覆盖输出。原调用异常保留失败，不声称原锁或生命周期已恢复。

Sweep与Overlap共用线程内唯一reporter入口，双向拒绝重入，不增加锁或第二写入端。query.h只包含并编译该适配层，尚无原版枚举生产调用；现有终点仍是原始bool overlap。完整typed source、代码认证、逐形状归属、世界／变换闭合和终点占用还需接入；该collector不是移动许可，未知结果不放行。

最终CPU：OverlapStream54、原Stream40、Session96、CapsulePathSession191，均exit0。54项包括实际九参数适配器配合成callee、正常返回但callback截短、原调用SEH、子区间／尾项、错误缓冲区、预算与跨类型重入。v1的44／40／Owned144亦通过，随后补充边界和异常检查；不是原DLL执行。Native从r111冻结源仅叠加 overlap_stream、stream、query 三文件，404项依赖全在冻结树、开关不变。SHA256 `f529a65ecaf4e3f23581b67d92ba680555355b082a3b0e4901633217a3764347`，私有诊断使用，公共v71未动。

r118 game58584启动后未在七秒取得两帧，record线程第一次stop join也超时，只有一张已审启动等待卡；未启动动作／probe attempted0。保留原projection_error和缺失manifest，不补造录制通过；runner最终终结且game／owner／XR均正常消失。录制原因未被该次无分段记录唯一定位。

本方复制既有ProjectionRecording做局部阶段观察，记录identity、request、等文件、打开／加载／转换／保存及append；等文件期间响应原stop_event。输入前改为等实际 first_stereo_submission（XR单调时间与非runner观察标记）再开始录制；两帧分别五秒的原等待预算合计允许十二秒，不以Native running或等待卡当场景证明。失败后在同一owner正常停止后重等原capture线程，保留初次错误。12项实际recorder／就绪函数配合成BMP检查、8项实际cloak guard检查通过，非游戏证据。

r119 game54576与r120 game50960实际录制分别78／80帧，录制errors空、线程正常结束。两次共8张全跨度overview已逐页亲看，均为真实场景与双手；没有掩体动作，不是手部贴合或完整掩体通过。两份VFR编码exit0，原时间与图像保留。两次reader仍在输入前拒绝：旧断言分别只允许index running／session_owned；启动器on_report交接后 prepare_level 又写 waiting_for_level，不能把该索引阶段当最终游戏运行状态。该真实读取失败不是source4重现。

读取器保留精确实例、原session_id、game／owner／XR完整身份、XR制品和Source1核对；只允许已知交接索引 session_owned／waiting_for_level，且实际同编号session必须running。终态／导航态不放行。16项实际生成reader的合成文件／身份／Reader构造检查及14项live()检查通过，不刷新原样本时间／100ms边界。r121已用当前入口重新prepare并通过实际依赖导入检查，资源上限发现两个其他活跃实例后启动前拒绝，尚无claim／game；r118–120不可重跑，后续只运行已修正r121。

r118／119／120隐藏观察567／414／494次，失败均0。静音回读分别252／193／222次已静音、verify_failed0、target_shared_rejected0、initially_muted_sessions均1；muted0表示原已静音而无须本次写入，shared_rejected为全扫描累计，不当成本实例失败。均Native／入口／手柄恢复true，XR0／launcher0，精确自有进程已不存在，未强杀；三次fire_disable2／hand_stop5／drainedfalse、normal_cleanup_verified=false照实保留，不称全清理通过。保留早停开火确认缺口。

证据 overlap-collector-r117/verification.json、native-build-result.json、review-r119／review-r120/media.json 及各私有实例原报告；原始失败、等待卡与录制错误完整保留。source4根因、掩体头肩遮挡、终点认证及实际C0身体提交仍未解决，工程25%不变，CV-01–24／AC-01–22与物理头显继续开放。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native=build['dll_sha256'],r121_started=False)))
