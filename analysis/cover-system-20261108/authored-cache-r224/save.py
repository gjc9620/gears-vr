from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/authored-cache-r224';sha=lambda b:hashlib.sha256(b).hexdigest()
results={n:json.loads((folder/(n+'-result.json')).read_text()) for n in ['match','reader']}
for name,r in results.items():
 assert r['build_exit']==r['test_exit']==0,(name,r)
 for p,d in r['source_sha256'].items():assert sha((root/p).read_bytes())==d,(name,p)
sources=['tools/native-capture/cover_authored_cache_match_live.inl','tools/native-capture/cover_authored_cache_match_live_test.cpp',
         'tools/native-capture/cover_physx_cached_shapes.h','tools/native-capture/cover_physx_cached_shapes_test.cpp']
evidence={}
for p in ['capsule-creation-r220/original-creation-certificate.json','cached-shapes-r223/original-descriptor-certificate.json','shape-cache-r222/original-cache-certificate.json']:
 path=folder.parent/p;evidence[str(path.relative_to(root))]=sha(path.read_bytes())
verification=dict(results={n:r['output'] for n,r in results.items()},sources={p:sha((root/p).read_bytes()) for p in sources},original_semantic_evidence_sha256=evidence,
 change='Owned complete per-BodySetup authored-to-selected-cache descriptor correspondence. Validate complete supplied inventory counts/order/raw-decode metadata, source setup/cache headers/module/requested scale; use selected cached scale with original strict tolerance, NoRBCollision bit0 skip, .025 margin/half dimensions/capsule[-X,Z,Y] basis; compare raw descriptor bytes with decoded metadata and generated expected pose/dimensions. Output maps original global authored index to template index and keeps disabled rows. Cached reader now retains original game source, cache source and requested scale through both passes.',
 tests='Match364 checks0 includes authored inventory328. Two bodies with7 raw shapes match4+2 enabled template shapes and retain disabled seventh. Independent hardcoded float32 descriptor bits for cached6.00005/current6, raw dims2/3/4, capsule length3 and negative zero axis signs. Reject wrong source/request/key beyond tolerance, raw/decoded mismatch, missing/order/pose/dimension/convex/mask/bone/scale tampering. Replacing cached scale with current6 rejects. Consistent re-enable of omitted shape rejects; all-disabled four-shape source matches empty template while retaining all source rows. Reader709 checks0 includes prior705 and4 explicit source metadata checks.',
 oracle=dict(cache_scale_bits='40c00069',scaled_axis_bits='3df5c315',radius_bits='3e87ae57',box_half_x_bits='3e147b24',box_half_y_bits='3e51ebea',capsule_full_length_bits='3eb85250',capsule_negated_zero_bits='80000000'),
 remaining='Geometric source-to-template association only. BodySetup identity/lifetime current scope remains caller responsibility. Does not establish active actor/cache creation identity, active shape index, flags eligibility, current world fence, production integration/game/headset/full CV24/AC22.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(old)==guard['development_copy']['sha256']
addition='''
## r224：authored→选中cache template逐项几何对应接通，保留cache来源及requested scale（2026-11-12）

新增cover_authored_cache_match_live.inl纯owned逐BodySetup匹配：先校验完整supplied inventory总数/各body/kind计数、选中body header/mask、原始80字节再次decode及其缓存字段一致；原cache来源必须同setup+cc/dc、同requested scale和有效game source/vector/selection范围。按原sphere/box/sphyl顺序使用selected cached scale重放r220 descriptor规则，NoRBCollision bit0按原创建路径省略但保留disabled行，核对固定.025 margin、box half和胶囊[-X,Z,Y] basis。逐条将cache raw descriptor字节与其decoded字段、expected pose/dimensions三方核对，不只信任可被改过的decoded值；返回global authored index→cache index及同setup/bone标签，不授予当前身份或移动权限。

同时补齐r223 Snapshot后续匹配所需的来源：原game Source、cache Source以及requested scale现在在两pass中保留/比较/发布，避免只有descriptor pointer/index无法追溯到哪个BodySetup和哪个lookup请求。当前requested scale与inventory原point producer数学结果必须一致；selected cache scale逐轴严格在原epsilon内，但实际创建尺寸用selected值，不能用今天requested值替代。外层仍须独立闭合当前setup身份/代码/生命周期，几何相等不是active actor历史创建证据。

Match364累计检查0（含原authored inventory328）：2body/7原形状对应4+2 enabled template，保留disabled第七项；独立硬编码float32 oracle针对current6、cached6.00005（40c00069），scaled3df5c315/radius3e87ae57/box half3e147b24,3e51ebea,3e87ae57/capsule full length3eb85250及负零80000000轴符号，不调用creation converter生成expected fixture。错误source/request/越容差key、raw/decoded/pose/dims/缺项/乱序/convex/mask/bone/scale均拒绝保持输出；把cached scale改成current6仍保留原descriptor会拒绝，证明不是偷偷用当前值。将省略的disabled形状一致地重新启用时缺项拒绝；4个源形状全部disabled时空template匹配仍保留4行disabled。reader709检查0（原705+4来源字段回读断言），依赖SHA均当前。

首次match编译遇既有pose_math参数actor与旧fixture全局actor的/WX shadow，调整测试include顺序使轻量pose数学先于旧fixture解析；未改既有数学/放宽warning。新body/cache匹配仍只使用合成当前元数据与保存原规则的CPU fixture，不称原游戏/实际cache执行。active cache→actor/shape身份、flags/full self/world fence和生产接入、完整CV24/AC22、body/view/hit/muzzle事务与视图遮挡/头显仍未完成，无游戏/XR/原函数/生产Native构建/安装，工程25%。证据authored-cache-r224。
'''
new=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
guard['development_copy']['sha256']=sha(new);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(match_checks=364,reader_checks=709,failures=0,documents_equal=doc.read_bytes()==mirror.read_bytes())))
