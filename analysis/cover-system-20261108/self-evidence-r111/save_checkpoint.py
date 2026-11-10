from pathlib import Path
import hashlib,json,datetime
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
sources=['cover_physx_self_evidence.h','cover_physx_query.h','cover_physics_query_live.inl','cover_physx_owned_sweep_test.cpp','cover_physics_capsule_path_live_test.cpp']
checks=json.loads((H/'checks-v2/results.json').read_text())
assert all(x['build_exit']==0 and x['test_exit']==0 for x in checks)
build=json.loads((H/'native-build-result.json').read_text())
assert not build['external_worktree_dependencies'] and build['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
record=dict(sources={n:sha(R/'tools/native-capture'/n) for n in sources},checks=checks,
    native_sha256=build['dll_sha256'],project_dependencies=len(build['project_dependencies']),
    game_started=False,original_dll_executed=False,public_changed=False,headset_accepted=False,
    evidence_scope='per-returned-shape positive provenance in closed query; raw hits retained; path propagation; no self-inventory/world fence/clearance',
    initial_failures=['check output parent absent before first compiler launch','checks-v1: owned144/OFF36 passed, path and Session C4459 /WX name collisions; renamed parameters, checks-v2 all pass'],
    remaining=['actual source4 query failure','complete body and shape coverage','world/transform fence','endpoint self-aware occupancy','actual C0 body commit and view/weapon/hit consumers','CV01-24 AC01-22 and headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json'
guard=json.loads(guard_path.read_text(encoding='utf8'))
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r111：逐命中自身来源证据与整条路径保留（2026-11-10）

在 r110 原始查询结果上增加 self_sources：只给当前查询已认证的具体形状记录人物碰撞体、Pawn 自有组件或主 Mesh 物理体的正向来源。形状所属公共 Actor、内部物理体、Scene、组件 BodyOwner 和主数组成员必须一致；附件根／清单成员身份不授予自身排除。任一外部组件或外部骨骼体与该 Actor 别名冲突、主 Mesh 归属缺失、内部体／场景不符时，标记为零，继续保留命中。主查询关闭全部 typed source 后计算，无额外原版调用、输入刷新或写入端。

所有原始命中、顺序与重复项继续保留，整条分段路径复制相应来源字节，空结果及尾部清零。正向标记仅证明这些已返回形状的来源，不证明完整自身集合、物理世界静止、空返回覆盖完整，也不授予身体移动；终点原始 overlap 仍未区分自身，尚不能放行。

CPU：OwnedSweep 144、CapsulePathSession 191、Session 96、OFF 36，均 exit0。真实生产 QueryEngine／Session 配 Host 模块与派发缝；没有执行原 DLL 或游戏。正向来源及内部体不符、外部组件／PAA 别名、场景／来源不符等为明确包变异检查，不能称实际游戏分支已覆盖。首轮输出目录父级缺失、以及 /WX C4459 测试全局名冲突均保存，修正后检查通过。

从 r110 冻结 Native 源仅覆盖 self_evidence、query、query_live 三文件，403 项项目依赖全在冻结树，开关不变。Native SHA256 `58bdbf940abb2e087ff845df7715087f452c2a71a581033caa3bb382d023f333`。仅编译，未安装／运行，公共 v71 不变。证据 `analysis/cover-system-20261108/self-evidence-r111/verification.json` 与 native-build-result.json。

收到主人最新“修复后台后继续跑”转达并核对 independent-runtime-20261110.md 最后一节；中央暂停标记已由维护方归档。后续仅以 cloak1110／绑定模拟器重新 prepare 全新本方实例，保留身份、新鲜度、静音、存档和正常清理。旧 background1110b 不复用，旧仅前台焦点检查不作为桌面不可见证明。先完成隐藏录制接线与定向 source-stage 运行，再推进终点占用、完整身体与实际提交；不向统筹主动发送占用／进度。

全目标及 CV-01–24／AC-01–22 未完成，工程 25% 不变；实际 source4、头肩遮挡及完整游戏／双眼／头显验收继续开放。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8')
doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc)
guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(native=record['native_sha256'],development=sha(doc),checks_pass=True)))
