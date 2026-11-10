from pathlib import Path
import json,hashlib,datetime
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf8'))
build=load(H/'native-build-result.json');manifest=load(H/'native-source-manifest.json')['files']
production=['cover_physx_overlap_code.h','cover_physx_shape_owner.h','cover_physx_query.h','cover_physics_query_live.inl','cover_physics_c0_live.h','cover_physics_c0_live.inl']
for name in production:assert sha(R/'tools/native-capture'/name)==manifest['tools/native-capture/'+name]
assert not build['external_worktree_dependencies'] and build['flags_unchanged'] and sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
checks2=load(H/'checks-v2/results.json');checks3=load(H/'checks-v3/results.json')
assert all(c['build_exit']==0 and c['test_exit']==0 for c in checks2+checks3)
record=dict(classification='progress',production={n:sha(R/'tools/native-capture'/n) for n in production},
    native_sha256=build['dll_sha256'],project_dependencies=len(build['project_dependencies']),flags_unchanged=True,
    selected_enumeration_ranges=load(H/'certificate.json'),checks_v2=checks2,checks_v3=checks3,
    original_enumeration_executed=False,new_candidate_game_started=False,public_changed=False,
    scope='raw capsule shape enumeration and every returned shape ownership in same typed C0 transaction; no endpoint occupancy/coverage/world certificate',
    initial_failure='checks-v1 driver omitted DLL-as-data argument for new executable; test exited2 without running; corrected in local driver, checks-v2/v3 pass',
    remaining=['actual original source4 failure','connect path endpoint to new enumeration','Sphere/Box enumeration and complete body/shape coverage','world/transform fence','actual C0 body commit and view/hit/muzzle consumers','early-stop fire_disable2','CV01-24 AC01-22 and headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r122：完整胶囊重叠枚举事务与同源形状归属（2026-11-10）

新增 QueryEngine::overlap_all(Capsule)，提供 AllOverlap 和 AllOverlapOwners 原始观察。与原 sweep_all 共用同一 collect_all 事务、busy owner、原始调用计数、typed asset/collider/component 前后核对和最终发布；不复制第二套身份准入。每个返回的公共shape通过现有完整成员／内部体／Scene反链观察，再计算保守self_sources，保留外部及冲突命中、顺序和重复项。地址适配使用私有carrier复用归属观察，不把它伪称原版64字节sweep Hit，也不对外发布伪包。

新枚举独立校验 public vtable+2f0 与 internal+248，及五段完整过程范围：172160=201、A9F10=1233、1275A0=21、188440=291、1AB130=151，共1897bytes。范围来自完整Ghidra body而非第一个unwind片段；字节由原SHA22051aa7…dd48 DLL只读取得。枚举前、原返回后、逐shape归属结束后均复核。旧28段／17916bytes及原虚表认证保留，新校验只作用于新枚举，不给无关查询增加全局阻断。REA provider已关闭，原信息／汇编与完整snapshot保留。

支持合法caller-buffer子区间；截短、陌生fallback buffer、返回count不符与任何来源／代码变化均保留调用方旧输出。未知原调用异常仍pin模块、pending并禁止本owner重试；没有解锁／清理冒充。原始零返回只记观察，不签发占用或移动许可。零轴Capsule仍在dispatch前拒绝。

Session与C0入口接入同一原writer／当前捕获C0／未提交身体上下文；带归属的大结果留在Session私有成员，避免139KB游戏栈临时值。预算与胶囊输入在外部guard前后保持一致；停机／排空、部件替换及元数据变化不发布旧结果。关闭构建返回unavailable且输出不改，无每帧调用、输入注入或新的pose writer。

CPU：QueryEngine旧Query210／OwnedSweep144通过；新OverlapQuery最终87（真实事务、Host模块与原调用缝，原DLL仅作数据）、新OverlapSession53（真实typed roles／Session，派发和owner包为Host缝）、既有Session96／CapsulePath191、C0 OFF38，全部exit0。新测试涵盖五段尾部／两虚表变异、重入与pin、空与重复结果、错误buffer／count、预算截短、来源／代码／归属失败、原未知异常及停机。v1驱动漏给新程序DLL数据参数，exit2未执行检查；局部驱动修正后v2/v3通过，原失败保留，不当游戏／原调用错误。

Native从r117冻结源仅覆盖以上六个生产文件，405项项目依赖全在冻结树，无外部工作树依赖、开关不变。SHA256 `34c15590a10a1fa9f2f380ed0b2285afbd3d5889939e1c30f3f46542076dd1e9`。仅编译，未安装／运行；公共v71未改。保存后新运行需当前共享入口重新prepare独立实例，不复用旧入口哈希的r121。

重要未完成：新接口仅Capsule原始形状枚举；原分段路径终点仍为bool overlap，尚未替换为该接口，Sphere／Box及完整身体覆盖、transform／world fence、终点占用和C0实际身体／视图／命中／枪口提交都未证明。原source4仍未在修正后的输入路径触发。工程25%、头肩遮挡及全部CV-01–24／AC-01–22／物理头显未完成状态不变。证据 overlap-query-r122/verification.json、certificate.json、各检查和native-build-result.json。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native=build['dll_sha256'],project_dependencies=len(build['project_dependencies']))))
