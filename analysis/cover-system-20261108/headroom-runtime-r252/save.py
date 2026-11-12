from pathlib import Path
import json,hashlib,datetime,shutil
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r252';root=r/'analysis/instances/cover-1112-r252-cloak-query-headroom';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());s=v['probe'];b=v['budget'];assert v['operator_exit']==0 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['original_started']==v['original_returned']==2 and s['cover_path_probe_code']=='4'
assert b['cover_path_budget_checks']=='1' and b['deferred']==b['invalid']=='0' and b['mode']=='2' and b['timestamps_renewed']=='0' and b['first']==b['last']==b['admitted']
admit=list(map(int,b['admitted'].split(',')));assert admit[0]==3 and admit[2]>=int(b['reserve_ns']) and admit[1]-admit[4]==3_186_200
first=list(map(int,s['cover_path_probe_age_0'].split(',')));last=list(map(int,s['cover_path_probe_age_2'].split(',')));assert first[15:18]==last[15:18] and first[8]==0 and last[8]==1
assert v['source_sha256']==sha(o/'native-session-status-original.txt')
shutil.copyfile(root/'cover-output/projection/recording.json',o/'recording-original.json');files=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(files)==1;shutil.copyfile(files[0],o/'cover-boundary-original.json')
verification=dict(native_sha256='35ef90d39c67953e88937df79198dd59ad464fb99e56e2419f1814778419e277',original_started=2,original_returned=2,whole_path_published=False,elapsed_ms=v['elapsed_ms'],admission_checks=1,actual_deferrals=0,actual_defer_branch_exercised=False,admitted_tracking_age_ns=admit[1]-admit[4],admitted_remaining_ns=admit[2],initial_guard_tracking_age_ns=first[10]-min(first[15:18]),closing_guard_tracking_age_ns=last[10]-min(last[15:18]),physics_rechecks=27,borrowed_read_guards=132,component_rechecks=4,component_recheck_total_ns=11_294_700,normal_cleanup_verified=True,remaining_owned={},personally_reviewed_frames=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25,
 next='Inspect exact duplicate schema/identity reads within each synchronous component proof pass. Keep separate passes, dynamic rereads, final full checks and original timestamps; no further unchanged query or arbitrary reserve escalation.')
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r252' not in old
actual=json.loads((o/'run-result-original.json').read_text())['terminal_session']['game_identity']
add='''
## r252：首次准入已有36.8138ms余量，两个原调用返回，38.8218ms完整关闭仍过期（2026-11-12）

新cover-1112-r252-cloak-query-headroom，准备14cf4a53，使用r251最终v4 Native35ef90d3…e277，426冻结依赖/导入/14身份检查通过；actual game{pid}/creation{creation}。budget checks1/deferred0/invalid0/mode2，first/last/admitted完全相同：code3 ready，原now231642720759700、limiting index3/stereo born231642717573500，样本3.1862ms、remaining36.8138ms（大于30ms reserve）。这次首次即可进入，没有实际等待/延后，不能把pure helper测试称为实际defer分支验证；没有调整30ms初值。

path attempted/finished1、原started0→2/returned0→2：第一段扫掠与终点原调用均正常返回，whole q4 source，Session stage5 engine/detail4，Engine stage8 source_end；完整owner73/failed73 context/source_age_expired extra6、Session guard164/failed164 physics_read detail3，source_age_cause observed1/verdict3/causes1。38.8218ms后整体未发布，segments/hits/endpoint0不是无碰撞。初始full guard tracking年龄3.3735ms，最后42.0053ms；两个witness原stereo/controller/sync出生时间完全相同，原40ms约束正确拒绝，既不续钟也不在原调用已发生后自动重试。

实际完整physics recheck27、borrowed_read_guards132。timing：initial physics.4093ms/collider1.6861ms/components2.5410ms；physics repeat27合2.5125ms、collider repeat27合4.1080ms、component repeat4合11.2947ms；engine33.3828ms，owner bracket64合14.0622ms（嵌套值不相加当总耗时）。这次足够新仍无法在40ms内完整关闭，说明30ms reserve不是完成保证，不能靠继续抬reserve或重跑凑成功。下一步检查单个component proof pass内重复的相同schema/identity字段，保留所有独立pass/动态值回读及最终完整证明，禁止跨原调用缓存proof。

264实际投影/errors0、五段ordinary操作operator0；recording/boundary/原run/native文本保留，本轮未亲看/编码，不作视觉接受。正常Native/entry/XR/game清理true、自有remaining空，anchor/reader未变。extract首版误认新budget为行首、随后动态脚本转义丢失正则反斜线；仅修离线解析，重新读取同一不可变文本，无新游戏/输入；最终budget与原文逐字段一致。完整CV24/AC22、full self/world fence/active-authored对应/body-view-hit-muzzle/遮挡与头显仍未完成，工程25%；publicv71/固定入口/Quest保持。证据headroom-runtime-r252。
'''.format(pid=actual['pid'],creation=actual['creation_time'])
assert verification['initial_guard_tracking_age_ns']==3_373_500 and verification['closing_guard_tracking_age_ns']==42_005_300
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(verification))
