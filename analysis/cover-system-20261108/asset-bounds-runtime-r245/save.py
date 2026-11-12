from pathlib import Path
import json,hashlib,datetime
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r245';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());assert v['successful_read_and_bounds'] and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['operator_exit']==0 and v['independent_certificates']==36
assert v['source_sha256']==sha(o/'native-session-status-original.txt')
media=json.loads((o/'media-review.json').read_text());assert len(media['contacts'])==5 and media['encode_exit']==0
for c in media['contacts']:assert sha(Path(c['file']))==c['sha256']
media['personally_reviewed_contacts']=[dict(label=c['label'],sha256=c['sha256'],result='FAIL own shoulder/torso armour obstructs both eyes; severe on right side') for c in media['contacts']]
media['full_span_personally_reviewed']=False;media['visual_acceptance']=False;(o/'media-review.json').write_text(json.dumps(media,indent=2)+'\n')
(h/'verification.json').write_text(json.dumps(dict(native_sha256='66a96c12d55de850daf87dc7a203a29d86d29367ff6619214c5dcf114e5097f4',successful_read_and_bounds=True,counts=v['counts'],elapsed_ms=v['elapsed_ms'],derivation_ms=v['derivation_ms'],independent_certificates=36,normal_cleanup_verified=True,remaining_owned={},actual_packet_sha256=sha(o/'actual-bounds-packet.json'),raw_status_sha256=v['source_sha256'],original_calls=0,mathematical_only=True,query_permission=False,movement_permission=False,visual_accepted=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25),indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r245' not in old
actual=json.loads((o/'run-result-original.json').read_text())['terminal_session']['game_identity']
add='''
## r245：实际36形状范围全组成功，计算0.026ms，视图仍遮挡（2026-11-12）

新cover-1112-r245-cloak-asset-bounds，准备7547f879、425冻结依赖/导入/14身份检查通过，使用r244 Native66a96c12…97f4；actual game{pid}/creation{creation}。完整实际读取29 Body/29唯一Actor/36 shape（21capsule/13box/2sphere）后，owned Snapshot范围集合code0、36行全数输出。C0观察总5.5001ms，其中完整关闭后的数学范围派生0.026ms；owner7/Session1173无失败，完整typed recheck1、组内guard1172，原query started/returned均0。每行actor/member/shape与raw geometry逐一对应；Decimal160再次独立枚举36形状的数学极值，全部被实际模块输出bounds包住。原时间/proof不续期，输出明确derived_after_close=1/mathematical_only=1/query_permission0/movement_permission0，未称世界query或移动通过。

264实际投影/errors0、14总览生成、5contact逐张亲看：两眼右侧自身肩甲/躯干遮挡，右眼尤其严重，转头/抬头与恢复均仍FAIL；不能把较多环境可见解释为修复。全overview未亲看、图片与此C0未证明同一时刻，头显与拟合均不接受。编码0/operator0，正常清理true、自有remaining为空，CLOAK560失败0/前台379无owned激活，anchor/reader未变。原始run/native文本、可复跑extract.py、actual-bounds-packet.json与每轴Decimal极值保留。

这轮将完整实际形状的数学范围接入原只读路径并验证，不扩大到full self/world fence/原query/同版body-view-hit-muzzle提交。接下来应处理实际场景查询与掩体body goal仍拒绝cover的缺口，不能重复相同读取代替玩法推进。完整CV24/AC22、工程25%、publicv71/固定入口/Quest保持。证据asset-bounds-runtime-r245。
'''.format(pid=actual['pid'],creation=actual['creation_time'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(counts=v['counts'],elapsed_ms=v['elapsed_ms'],derivation_ms=v['derivation_ms'],documents_equal=doc.read_bytes()==mirror.read_bytes())))
