from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/asset-shapes-r217'
result=json.loads((folder/'final-result.json').read_text());assert result['build_exit']==result['test_exit']==0
sources=['tools/native-capture/cover_physx_asset_shapes.h','tools/native-capture/cover_physx_asset_shapes_test.cpp','tools/native-capture/cover_physx_shape_world_test.cpp']
artifact=Path('J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll')
digest=hashlib.sha256(artifact.read_bytes()).hexdigest();assert digest=='22051aa75312344a824a5f7e29c6375fec3a90df50cbeff6152f0e6c3afddd48'
verification=dict(result=result['output'],sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},artifact_sha256=digest,
 association='Complete supplied PhysicsAssetInstance.Bodies -> deduplicated BodyData actors -> actual singleton/compound actor members -> original getter-derived world parameters. Body-to-actor map retains null nonroot BodyData and shared actors, while every unique shape is sampled once. No authored shape/bone assignment inferred from actor/member order.',
 closure='Two complete asset/member/world passes. Each actor list is re-read after its world rows; entire asset list re-read after all groups; complete pass equality, original getter code/slots, typed C0/image guard and final input scope closure before publishing. Reentry/alias and partial output forbidden.256 global capacity, no truncation.',
 tests='1714 checks0 includes1688. Original exact PE as data; synthetic current object graphs and C0/layout guard seam. Static singleton+dynamic3-kind compound, canonical actor/member order, full source byte preservation, cross-actor early-row mutation during late actor, member change, shared/null BodyData, unsupported convex, initial/final guard and final source mutation, reentry/alias, empty groups/all-empty groups.5 actors with320 members reject; exact256 plus empty fifth actor succeed;257 rejects and preserves previous256.',
 failures_retained='Initial signed/unsigned fixture literal comparison triggered /WX; unsigned literal fixed. Final-boundary mutation fixture initially stored a counter by value across copied guards, so target boundary was not reached; shared counter fixes the actual injection, production guard/capture logic unchanged.',
 remaining='Only the supplied main asset BodyData set; not entire player collider/components/equipment/self set. Still requires same current C0/time/code/lifetime and query-visible world closure. Authored-to-active row correspondence, units/axes/dimension semantics and eligibility, Native integration/build/runtime, path/occupancy/body/view/hit/muzzle/headset remain open.',
 game_started=False,original_DLL_loaded=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r217：整套已提供PhysicsAsset BodyData→实际Actor成员/world形状采集已实现（2026-11-12）

新增cover_physx_asset_shapes.h，将已有asset_actors、actor_shapes、shape_world组合为完整两轮batch。每个Body到unique Actor的map明确保留；非root BodyData为null记~0u，共享Actor只采集一次形状但保留所有Body引用，不把welded Actor的成员顺序当authored骨骼归属。每Actor单形状/compound实际列表都读全，保留actor_index/member_ordinal/world Snapshot；每组世界参数后再读一次成员、所有组后再读asset，两个完整pass及原code/槽/typed guard、最终scope相同才发布。任何失败保持旧整批输出，exclusive work拒绝重入和别名，总容量256，绝不截断。

1714检查0失败（含原1688），原PE SHA22051aa7作数据、objects synthetic，C0/反射guard为fixture seam。静态singleton+动态三类型compound共4行、canonical顺序/内存不变；读后Actor时改前Actor位姿由完整pass比较拒绝，成员变化/unsupported convex/首尾guard/最终输入scope变化/重入/alias保护输出。共享Actor不重复形状、null Body保留，无成员的Actor与整批全空均显式观察。五Actor共320条拒绝；前四各64且第五空时完整256成功；再加1条为257拒绝并保留此前256结果。

首次比较用int常量与unsigned序号在/WX下失败，修fixture常量；最终boundary负例初版把计数放可复制guard值内，未到目标边界，改为共享计数后实际scope变更被拒绝，未改产品准入/closing逻辑。原失败/最终编译与回执在asset-shapes-r217。

覆盖范围仅本次已提供主PhysicsAsset BodyData，不是整个player collider/components/equipment/self清单，不签发自身排除/世界fence/占用/移动许可。authored逐项对应、尺寸/单位/轴基/参与语义、同current C0/时相/生命周期、生产Native接入/完整构建/游戏/原查询及body/view/hit/muzzle/头显仍待完成，CV24/AC22保留，工程25%。无游戏/XR或原DLL加载执行。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':1714,'failures':0,'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
