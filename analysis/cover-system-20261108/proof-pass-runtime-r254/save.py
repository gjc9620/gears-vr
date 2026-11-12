from pathlib import Path
import json,hashlib,datetime,shutil
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r254';root=r/'analysis/instances/cover-1112-r254-cloak-proof-pass';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());assert v['operator_exit']==1 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['original_started']==v['original_returned']==0 and v['probe']['cover_path_probe_attempted']=='0' and v['budget']['cover_path_budget_checks']=='0'
files=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(files)==1
shutil.copyfile(files[0],o/'cover-boundary-original.json');shutil.copyfile(root/'cover-output/operator.log',o/'operator-original.log');shutil.copyfile(root/'cover-output/projection/recording.json',o/'recording-original.json')
failure=json.loads(files[0].read_text())['candidate_closing_rejection'];d=failure['difference'];assert d['kind']=='registry' and d['before_header']['data']==d['after_header']['data'] and d['before_header']['capacity']==d['after_header']['capacity'] and d['after_header']['count']==d['before_header']['count']+1
verification=dict(native_sha256='631c2cf8391ed08bad667b506eceefdc23f1c3cf48f0fbcb3f985a17a5a61501',path_attempted=False,original_calls=0,actual_component_cache_exercised=False,operator_exit=1,normal_cleanup_verified=True,remaining_owned={},candidate_closing_rejection=failure,projection_frames=135,projection_errors=[],personally_reviewed_frames=False,gameplay_accepted=False,headset_accepted=False,scope='Strict pre-input candidate closure rejected one GObjects registry append; no performance/path result. Preserve this failure and repair bounded fresh pre-input observation rather than unchanged game reruns.')
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r254' not in old
add='''
## r254：再次在候选原A之前因GObjects新增一项拒绝，未到Native优化路径（2026-11-12）

新cover-1112-r254-cloak-proof-pass，准备f4caa1d6，r253 Native631c2cf8…1501/427依赖与14身份检查通过。第一次准备名称41字符超40限制，在instance创建前拒绝；改短名后正常准备，name-rejection记录保留，无额外运行。实际ordinary candidate q.finish严格拒绝：GObjects header5398218544，data372178944/capacity225272保持，count218209→218210。与r247为同一类前置registry追加，不称Native proof优化失败。path attempted0、budget checks0、原query0/0，实际去重性能仍未覆盖。

operator1、135实际投影/errors0；正常Native/entry/XR/game清理true，自有remaining空，anchor/reader未变。candidate拒绝原JSON、operator traceback、recording清单、run/native文本均保留；未亲看/编码，不作视觉验证。r247曾一次新实例重读后继续到path；此次再次出现后不再用无变化重跑凑通过。下一步只在候选操作/原A/Native path都未开始的阶段，针对已证实同地址/容量且count+1的GObjects拒绝，允许一次完整新鲜候选读取，旧样本仍作废，保持原q.finish所有严格barrier。身份/字段/name registry/重分配/未知差异不作此处理，第二次再拒绝即终止。

修复尚未实现，r253仍仅编译/离线通过；CV24/AC22、实际path/full self/world fence/body-view-hit-muzzle/遮挡/头显继续未完成，工程25%。publicv71/固定入口/Quest保持。证据proof-pass-runtime-r254。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(verification))
