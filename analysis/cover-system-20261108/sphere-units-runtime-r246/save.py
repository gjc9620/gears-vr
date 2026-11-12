from pathlib import Path
import json,hashlib,datetime
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r246';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());assert v['raw_query_success'] and v['original_started']==v['original_returned']==1 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0
assert v['source_sha256']==sha(o/'native-session-status-original.txt')
media=json.loads((o/'media-review.json').read_text());assert media['encode_exit']==0
media['personally_reviewed_contacts']=[];media['full_span_personally_reviewed']=False;media['visual_acceptance']=False;(o/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
(h/'verification.json').write_text(json.dumps(dict(actual_original_call_started=1,actual_original_call_returned=1,complete_query=True,raw_hit=True,elapsed_ms=v['elapsed_ms'],normal_cleanup_verified=True,remaining_owned={},native_sha256='66a96c12d55de850daf87dc7a203a29d86d29367ff6619214c5dcf114e5097f4',world_or_self_distinguished=False,movement_permission=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r246' not in old
actual=json.loads((o/'run-result-original.json').read_text())['terminal_session']['game_identity'];s=v['sphere']
add='''
## r246：单位修正后的原版sphere查询实际调用返回，5.3198ms（2026-11-12）

新cover-1112-r246-cloak-sphere-units，准备8b0d2c4b、425冻结依赖/导入/14身份检查通过，r244 Native66a96c12…97f4；actual game{pid}/creation{creation}。这次切换到sphere动作，验证r227单位修正后未实际覆盖的原查询链；不是重跑相同asset读取。C0 query code0、Session stage7 complete、engine stage10 complete、source0/exception0；原始调用started0→1/returned0→1，5.3198ms完整关闭，raw_hit1。sphere native中心[-78.6054764,-3.88316274,12.4197283]、radius.0199999996（游戏1厘米按每米单位与.02原物理比例换算）。半径只诊断调用/ABI，不是头部实际碰撞体。

raw_hit1未区分自身与世界，不能称墙体命中/安全移动/occupancy或世界fence通过。普通query仍执行完整typed/source/owner前后关闭，未使用r241仅限asset无分派读取的借用guard。下一步新实例验证带命中对象列表的capsule path，核对自身归属、路径与终点；不能以sphere布尔结果授权body写入。

264实际投影/errors0、14总览/5contact生成、编码0，但本轮未逐张亲看，personally_reviewed_contacts空、full_span/visual/headset接受均false；不把生成图片当作视觉验证。operator0、正常清理true、自有remaining为空、anchor/reader未变。原始run/native文本、extract.py及closeout保留。CV24/AC22、完整身体碰撞/模板/full self/world fence/body-view-hit-muzzle和已知遮挡仍未完成，工程25%；publicv71/固定入口/Quest保持。证据sphere-units-runtime-r246。
'''.format(pid=actual['pid'],creation=actual['creation_time'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(query=True,elapsed_ms=v['elapsed_ms'],documents_equal=doc.read_bytes()==mirror.read_bytes())))
