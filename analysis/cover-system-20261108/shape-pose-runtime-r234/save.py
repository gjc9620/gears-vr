from pathlib import Path
import json,hashlib,datetime
r=Path('J:/codexWorktree/gearsVr');h=r/'analysis/cover-system-20261108/shape-pose-runtime-r234';o=h/'runtime-r234';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();v=json.loads((o/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
m=json.loads((o/'media-review.json').read_text());m['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in m['contacts']];m['full_span_personally_reviewed']=False;m['visual_acceptance']=False;(o/'media-review.json').write_text(json.dumps(m,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r234' not in old
add='''
## r234：实际world Stage7 Body quaternion；原数据近单位但严格矩阵判定拒绝（2026-11-12）

新cover-1112-r234-cloak-pose-detail使用r233 Native63888223…2caa，准备/操作导入/身份guard14通过，准备702f44e7。actual game25596/creation134389118562337067；原进入掩体与五段eye操作operator0。一次全形状读取14.9296ms，actor0/member0 inventory两pass完成，world first Stage7 Body pose、parameters0成功，原query调用0、owner guard47/session guard22无失败，整批未发布。Body q=.559741497,-.394303977,.607427657,.402782083，p=-78.8432846,-3.34397388,11.5933132；stored q=0,.707106769,.707106769,0，p=.0599999949,-6.82121011e-15,-2.13162816e-16，均为9位round-trip float原值，不是归一化结果。second world pass尚未进入，不把其零值当数据。

原Body q norm平方0.9999879342658389，在已有3e-5 norm范围；按原1209f0逐float算式重建后Gram最大偏差3.289429316311043e-5、Frobenius5.71721064577549e-5、det0.9999596083569477。故现有四元数norm检查通过而严格矩阵3e-5检查拒绝。原getter不归一化。后续用实际数据replay，再以显式原矩阵误差记录区分近旋转读取和严格rigid query权限，不能只提高常量或偷偷归一化。

259实际投影/errors0，13总览已生成、5阶段contact逐张亲看仍双眼自身头肩/躯干遮挡FAIL，未全数亲看overview，编码0。正常清理true、自有remaining为空、无强杀，CLOAK568失败0、前台380无owned激活，anchor/reader前后相同。其他游戏/XR会话未触碰。完整CV24/AC22、active/template/full self/world fence/body/view/hit/muzzle/视图遮挡及头显仍未完成，工程25%，publicv71/固定入口/Quest不变。证据shape-pose-runtime-r234，算式证据另见observed-pose-r235/original-pose-arithmetic.json。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Saved actual raw quaternion failure and original-float matrix evidence')
