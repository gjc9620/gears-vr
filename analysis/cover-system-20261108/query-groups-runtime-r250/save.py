from pathlib import Path
import json,hashlib,datetime,shutil
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r250';root=r/'analysis/instances/cover-1112-r250-cloak-query-read-groups';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());s=v['probe'];assert v['operator_exit']==0 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['original_started']==v['original_returned']==0 and s['cover_path_probe_attempted']=='1' and s['cover_path_probe_code']=='4'
assert s['cover_path_probe_full_rechecks']=='7' and s['cover_path_probe_borrowed_read_guards']=='24' and s['cover_path_probe_owner_guard']=='21,21,8,3,6' and v['source_sha256']==sha(o/'native-session-status-original.txt')
first=list(map(int,s['cover_path_probe_age_0'].split(',')));last=list(map(int,s['cover_path_probe_age_2'].split(',')));assert first[8]==0 and last[8]==1 and first[15:18]==last[15:18]
ages={'tracking_budget_ns':40_000_000,'initial_oldest_tracking_age_ns':first[10]-min(first[15:18]),'closing_oldest_tracking_age_ns':last[10]-min(last[15:18]),'initial_remaining_ns':40_000_000-(first[10]-min(first[15:18]))}
assert ages['initial_remaining_ns']>0 and ages['closing_oldest_tracking_age_ns']>40_000_000
shutil.copyfile(root/'cover-output/projection/recording.json',o/'recording-original.json')
files=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(files)==1;shutil.copyfile(files[0],o/'cover-boundary-original.json')
verification=dict(native_sha256='78460f418dd097eaf5cb427d73191b0445781ddcdd6558144c47bdb7deae9cf3',original_calls=0,path_result_published=False,elapsed_ms=v['elapsed_ms'],physics_rechecks=7,borrowed_read_guards=24,full_owner_calls=21,source_ages=ages,comparable_full_path_benchmark=False,normal_cleanup_verified=True,remaining_owned={},personally_reviewed_frames=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25,
 next='Budget admission must use original source birth times and defer before the first attempt if insufficient work headroom, rather than increasing ages, refreshing old samples, or retrying an original call. Final full checks still govern every result.')
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);previous=doc.read_bytes();assert previous==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r250' not in previous
actual=json.loads((o/'run-result-original.json').read_text())['terminal_session']['game_identity']
add='''
## r250：组内guard实际生效，但入口tracking已老25.9772ms，原调用前关闭仍拒绝（2026-11-12）

新cover-1112-r250-cloak-query-read-groups，准备559c6735，r249 Native78460f41…9cf3；425依赖/导入/14身份检查通过；actual game{pid}/creation{creation}。C0 attempted/finished1、q4 source、Session stage5 engine/detail4、Engine stage4 source_guard。完整physics recheck7、borrowed_read_guards24，owner21/failed21 context/source_age_expired extra6，Session guard33/failed33 physics_read detail3；source_age_cause observed1/verdict3/causes1。15.6646ms后拒绝，原started/returned0/0、正式path/命中/endpoint0均未发布。此轮是在原调用前结束，与r248原调用返回后36.7614ms不是等工作量性能比较，不能声称整条path已提速某倍或通过。

独立按原age witness整数计算：第一次完整guard的最老stereo/controller/sync已经25.9772ms，40ms原tracking预算只余14.0228ms；最后为41.4312ms。原始stereo/controller/sync出生时间在两个witness完全相同，metadata/tracking采集本身较新不代表底层样本新。原拒绝正确，未抬40ms/50ms上限、未续钟/替换source或分派未知调用。组内proof借用在实际读取中出现24次，不作全path完成证明。

253实际投影/errors0、五段ordinary操作operator0，recording及boundary原件保留；本轮未逐张亲看/未编码，不作视觉接受。正常Native/entry/XR/game清理true、自有remaining空，anchor/reader未变。下一步需要在首次真实query尝试前，按原样本出生时间判定是否仍有足够查询工作余量；不够就丢弃本C0候选、等待新的真实来源而非将旧输入续期。必须明确只在attempt/dispatch前等待，单次已尝试/未知原调用绝不自动重试；所有原关闭条件保持。此预算准入尚未实现，不能记作完成。

完整CV24/AC22、world fence/完整身体与active-authored对应/body-view-hit-muzzle/遮挡及头显继续未完成，工程25%，publicv71/固定入口/Quest保持。证据query-groups-runtime-r250。
'''.format(pid=actual['pid'],creation=actual['creation_time'])
assert ages['initial_oldest_tracking_age_ns']==25_977_200 and ages['closing_oldest_tracking_age_ns']==41_431_200
new=previous+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(verification))
