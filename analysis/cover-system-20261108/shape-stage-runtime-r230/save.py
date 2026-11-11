from pathlib import Path
import json,hashlib,datetime
r=Path('J:/codexWorktree/gearsVr');h=r/'analysis/cover-system-20261108/shape-stage-runtime-r230';o=h/'runtime-r230';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();v=json.loads((o/'closeout.json').read_text());assert v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
m=json.loads((o/'media-review.json').read_text());m['personally_reviewed_contacts']=[dict(label=x['label'],sha256=x['sha256'],result='FAIL own head/shoulder/torso obstruction in both eyes') for x in m['contacts']];m['full_span_personally_reviewed']=False;m['visual_acceptance']=False;(o/'media-review.json').write_text(json.dumps(m,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r230' not in old
add='''
## r230：实际定位actor0/member0的Body归属比较失败；原构造证明d0/148字段不同（2026-11-12）

新实例cover-1112-r230-cloak-shape-detail使用r229 Native2d252d05…df41，准备逐冻结依赖核对、操作导入与身份guard14通过，prepare选择已保存27631665。实际game40660/creation134389109050030515，原靠近/进入掩体及五阶段eye操作operator0，shape模式仅一次；11.7553ms后q10 owner_read/asset7 shapes，nested asset0/shape5 links/world0，failed_actor0、member进度0、first Stage15 body_link、last未进入。第一个单形状actor type3/count1。原owner guard35/session guard16无失败，original calls0；未发布整批结果，不把partial scratch当有效形状清单。

由此针对body比较查保存的原构造：124660以local_1a8作descriptor，descriptor+8来自Actor+18（NpActor+40 optional Body），descriptor+20来自Actor+8（NpActor+30 public-owner backlink）；1336b0将NpActor+28传入Actor创建。b0790拷descriptor+20至Shape+d0、descriptor+8至Shape+148。现有reader错误把Shape+d0和NpActor+40 Body比较。证据沿用r225完整原PE字节认证的三个function，完整pseudocode/assembly及原bundle SHA保存在original-owner-construction.json；这是静态来源推导，当前游戏仅导出失败阶段，没有导出实际d0值，二者明确分开。后续以原构造字段单独replay反例并修正两条归属，不删除比较。

269投影/errors0、14总览、5实际阶段contact逐张亲看仍双眼自身头肩/躯干严重遮挡FAIL；未亲看全部overview，full_span人工验收false。编码0，正常清理true、owned remaining为空、无强杀，CLOAK589失败0，前台386无owned激活；anchor/cover reader前后相同。完整CV24/AC22与同版body/view/hit/muzzle、full self/world fence/头显仍未完成，工程25%，publicv71/Quest/固定入口不变。证据shape-stage-runtime-r230。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('r230 actual failure saved and source construction facts bound')
