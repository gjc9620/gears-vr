from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-world-r216'
result=json.loads((folder/'verified-result.json').read_text());regression=json.loads((folder/'transform-result.json').read_text())
assert result['build_exit']==result['test_exit']==regression['build_exit']==0 and len(regression['tests'])==2 and all(t['exit']==0 for t in regression['tests'])
bundle=json.loads((folder/'rea-evidence.json').read_text());records=bundle['records'];assert len(records)==14
sha='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
artifact=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
assert hashlib.sha256(artifact.read_bytes()).hexdigest()==sha and all(r['subject']['digest']['sha256']==sha for r in records)
facts={}
for record in records:
 if record['operation']=='analyze_function':
  v=record['normalized_result'];facts[record['parameters']['procedure']]=dict(evidence_id=record['evidence_id'],entry=v['procedure']['address'],body=v['procedure']['body'],pseudocode=v['pseudocode'],assembly=v['assembly'])
(folder/'focused-facts.json').write_text(json.dumps(facts,indent=2)+'\n',encoding='utf-8')
sources=['tools/native-capture/cover_physx_matrix.h','tools/native-capture/cover_physx_transform.h',
 'tools/native-capture/cover_physx_pose_math.h','tools/native-capture/cover_physx_shape_world_code.h','tools/native-capture/cover_physx_shape_world.h',
 'tools/native-capture/cover_physx_shape_world_test.cpp','tools/native-capture/cover_physx_shape_parameters_test.cpp']
verification=dict(result=result['output'],transform_regression=regression,
 sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},artifact_sha256=sha,rea_records=14,
 rea_bundle_sha256=hashlib.sha256((folder/'rea-evidence.json').read_bytes()).hexdigest(),
 original_route='PublicActor133fe0 -> helper1209f0, dynamic Body vtable primary226868 (constructor2b40 proves exact primary assignment), slots8/10 ->4e50/4e70 read position20c/210/214 and quaternion1fc/200/204/208. Shape public80 ->13e7e0 ->178ff0: static uses stored shape pose; dynamic composes Body world with stored shape pose. No cache-changing getter invoked.',
 code='Six complete getter/composition bodies1682 bytes plus RIP float1.0 bytes, previous210 parameter/stub bytes and actor/shape getter checks; current exact dispatch slots and image bounds preserved. Earlier178ff0 dossier retained in parent r215 bundle, linked by digest.',
 math='Original float32 quaternion matrix and composition grouping, no normalization or scale/unit conversion. Finite/normal inputs and proper rigid transforms only; output remains PhysX units. CPU pose is not query-world fence or physical-headset proof.',
 tests='1688checks0 includes687. All3 kinds static/dynamic with production ownership reader guard and PE bytes as data; independent asymmetric quaternion point oracle,128 varied pairs, source byte preservation, missing static Actor cache/dynamic Body not read, body table/slots/code/constants/NaN/nonunit faults, inter-pass Body position mutation. All137 read points fail-injected; guard boundaries/alias/reentry/output preservation covered. Two retained actual component matrices unchanged, original transform63checks each0.',
 first_compile_failure='Including heavyweight transform/model after the Windows-based fixture exposed existing small macro and global base/size collisions. Split unchanged48-byte Matrix34 POD into lightweight cover_physx_matrix.h, leaving original namespace/layout/convert implementation intact; new pose reader avoids unrelated model dependencies. Original compile log retained and old transform regression passes.',
 remaining='Authored-to-active shape correspondence, dimensions/unit/axis conventions and actual query eligibility, same current C0/geometry/query-visible world, Native capture integration/build, path/occupancy/body/view/hit/muzzle and headset acceptance remain open.',
 native_session_closed=True,game_started=False,original_DLL_loaded=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r216：原静态/动态形状global-pose getter只读路径与CPU组合已实现（2026-11-12）

原PhysX SHA22051aa7保持。REA按精确Body::vftable名解析226868，原2b40 constructor的LEA226868→MOV[this]证明是主表，另两个secondary表226a00/226a50不混用。主表+8/+10原4e50/4e70读取Body position20c/210/214及quaternion1fc/200/204/208；133fe0的Actor+28进入1209f0、再以+18判断Body分支。既有r215的public Shape+80→13e7e0→178ff0原完整证据证明静态shape直接用stored pose，动态需Actor/Body world组合。静态不读取未用的Actor cache或假造Body；不调用任何原getter/缓存更新。

新增cover_physx_shape_world.h、cover_physx_pose_math.h和生成world_code，保留原float32四元数矩阵及组合分组、原单位，不归一化/缩放修复来源；非有限/非normal非零输入、非单位四元数/非刚性/反射明确拒绝。typed current shape/member/C0 guard、两完整parameter+Body pose观察、最终owner/代码/guard闭合后才发布；失败保护输出并释放工作区。6完整原函数1682bytes及RIP float1.0字节、r215参数210bytes和原actor/shape getter代码/槽逐项闭合。静态Actor world cache不属于本shape分支的消费，不用它加新准入条件。

1688检查0失败（含原687）：三类型各静态/动态，真实PE只作数据、objects synthetic、production owner reader作typed guard；独立非对称四元数点变换及128组不同旋转配对，输入内存不变。静态缺未用cache/Body仍能观察；错误Body主表/槽/原代码/1.0常量/NaN/非单位pose、两轮Body移动、三guard边界、alias/重入及全部137读取点逐项故障注入保护输出。两份原保存component矩阵SHA保持，旧游戏→物理转换各63回归0。

首次编译因在Windows夹具后引入heavy transform/model触发既有small宏及base/size全局遮蔽，抽出相同namespace/48字节/4字节对齐的Matrix34轻量POD，不改原convert实现，新路径不依赖无关model头。原编译失败保留。新静态会话14完整Evidence导出后关闭，完整getter/composition及constructor事实、跨r215 bundle digest、源码/回执在shape-world-r216。

这仍是原getter语义的CPU形状世界pose观察，不是active scene/query-visible世界闭合。authored逐项对应、尺寸/单位/轴基及参与规则、生产Native接入/完整构建、完整world/path/occupancy和body/view/hit/muzzle同版事务、视图遮挡/游戏/头显仍待完成；未加载/执行原DLL，无游戏/XR/原查询。CV24/AC22保持，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':1688,'transform_regressions':[63,63],'failures':0,'rea_records':14,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
