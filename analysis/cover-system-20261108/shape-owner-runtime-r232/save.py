from pathlib import Path
import json,hashlib,datetime
r=Path('J:/codexWorktree/gearsVr');h=r/'analysis/cover-system-20261108/shape-owner-runtime-r232';o=h/'runtime-r232';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();v=json.loads((o/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
m=json.loads((o/'media-review.json').read_text());m['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in m['contacts']];m['full_span_personally_reviewed']=False;m['visual_acceptance']=False;(o/'media-review.json').write_text(json.dumps(m,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r232' not in old
add='''
## r232：修正后的Actor/Body归属实际通过；下一拒绝转至world pose（2026-11-12）

新cover-1112-r232-cloak-owner-fields使用r231 Native3ca85b2f…7471；准备422冻结依赖、生成操作导入/身份guard14通过，选择代码ade0214b。实际game39576/creation134389114448133788，原进入掩体和五段eye操作operator0。一次全形状读取19.8285ms，原owner guard47/session guard22无失败，original calls0。actor0/member0的inventory first/last Stage19 complete均成功，原Stage15 body_link拒绝不再出现；nested asset0/shape0/world8 pose，q10 owner_read、asset detail8 world。失败输出整批仍未发布，不能声称整批成功或world/body闭合。

这给r231字段修复取得实际越过旧失败的证据；后续world pose在参数四元数、Body四元数或compose何处拒绝尚未知，补已有raw输入与阶段记录，不提前放宽rigid/normal阈值或归一化原数据。

264实际投影帧/errors0、14总览、5阶段contact逐张亲看仍自身头肩/躯干严重遮挡FAIL；未全数亲看overview，full_span人工验收false，编码0。正常清理true、自有remaining为空、无强杀，CLOAK620失败0、前台412无owned激活，anchor和cover reader前后相同。完整CV24/AC22、full self/world fence、active/template、body/view/hit/muzzle及视图遮挡/头显继续未完成，工程25%，publicv71/固定入口/Quest不变。证据shape-owner-runtime-r232。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('Saved actual repaired owner qualification and next pose rejection')
