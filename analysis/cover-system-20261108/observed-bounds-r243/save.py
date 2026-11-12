from pathlib import Path
import json,hashlib,datetime
h=Path(__file__).resolve().parent;r=h.parents[2];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
b=json.loads((h/'build-result.json').read_text());p=json.loads((h/'packet-certificates.json').read_text())
assert p['verified'] and p['rows']==36 and all(v['build_exit']==0 and v['test_exit']==0 for v in b['cases'].values())
for case in b['cases'].values():
    for path,digest in case['source_sha256'].items():assert sha(r/path)==digest,path
v={'implemented':True,'compiled':True,'native_integrated':False,'checks':34004,'failures':0,'actual_packet_rows_verified':36,'independent_decimal_precision':160,'native_or_game_run':False,'gameplay_accepted':False,'headset_accepted':False,'engineering_percent':25,'scope':p['scope']}
(h/'verification.json').write_text(json.dumps(v,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r243' not in old
add='''
## r243：原始观测形状的数学保守AABB与36实际行独立核对（2026-11-12）

新增cover_physx_observed_bounds.h，不改变严格rigid转换器，也不归一化原q。box逐行取原始矩阵列与half的绝对乘积和；capsule取原矩阵Y列乘half_segment的线段再加原未缩放半径；sphere保持原半径。double乘积对binary32输入精确，每次累加和最终端点向外nextafter，避免相消导致范围缩小。仅数学几何AABB：不是PhysX窄相位浮点执行误差证明，不提供query姿态、scene fence、freshness或移动许可。

owned Snapshot适配器重新执行stored/body原getter与composition，逐字节匹配world/actor矩阵与各自Gram记录；body存在标记与internal_actor一致，失败不写output。另有明确只接受供给几何的primitive入口用于历史数据，不能认证来源。非法尺寸/非finite/反射或过大Gram/错误rounding/alias拒绝。34004检查0失败，MSVC W4/WX编译0；独立角点/线段端点枚举覆盖不同转轴、近单位q、巨大平移和闭合篡改。MSVC long double与double同精度，不能把该枚举称为高精度证明。

r242原始packet SHA0cb97269…c445，36行全部通过新增C++ primitive后，另用Python Decimal160枚举每个box角点、capsule线段加原半径及sphere极点；全部精确数学极值处于输出上下界内，逐轴slack和原输出SHA保留packet-certificates.json。历史packet缺少每行body原q/p，故该回放只检验primitive入口，未伪称通过owned Snapshot复核。新增头尚未接入Native编译/实时集合，没有新游戏或XR运行，工程25%、CV24/AC22与视图遮挡继续未完成。证据observed-bounds-r243。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(v))
