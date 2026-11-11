"""Preserve age refusal before the repaired owner-reader was reached."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r174-cloak-backlink';out=H/'runtime-r174'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];q=load(out/'path-probe.json')
t=load(out/'timing-probe.json');a=load(out/'age-probe.json');links=load(out/'owner-links.json')
assert v['normal_cleanup_verified'] and v['operator_exit']==v['start_exit']==0 and not v.get('error')
assert not v.get('projection_error') and not v['projection_errors'] and v['projection_thread_joined']
assert not v['remaining_owned'] and not v['forced_termination'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['query_code_name']=='source' and q['original_started']==q['original_returned']==0
assert q['owner_guard']['failed_call']==35 and q['session_guard']['failed_call']==7
assert q['source_age_cause']['body_source_age'] and not q['source_age_cause']['equipment_age'] and not links['observed']
assert q['raw']['engine_stage']=='4' and t['elapsed_ns']==29103600
assert a['calls']==[35,35] and len(a['rows'])==4
assert set(a['classified_expired_fields'])=={'stereo_received','controller_received','sync'}
assert all(not x['expired_fields'] for x in a['rows'][:2])
assert a['rows'][2]['ages_ns']['stereo_received']==40312200 and a['rows'][2]['ages_ns']['metadata_begin']==31923700
spec=importlib.util.spec_from_file_location('r174_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(x for x in pose['rows'] if x['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert complete[k]==int(q['raw'][k])
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));boundary_data=load(boundary)
assert boundary_data['passed'] and boundary_data['after_original_A']['cover_state_raw']['CoverType']==2
assert len(boundary_data['eye_observation'])==5
review=load(out/'media-review.json')
assert review['frames']==286 and len(review['pages'])==15 and len(review['contacts'])==5 and review['encode_exit']==0
assert not review['recording_errors'] and not review['missing_contacts'] and review['captured_span_complete']
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=True,reviewed_pages=15,
    reviewed_original_contacts=5,verdict='All five actual cover phases have severe self-model obstruction; FAIL',cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json')):shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,runner_exit=0,
    native_sha256=q['native_sha256'],same_C0_complete_query_key=True,query_elapsed_ns=t['elapsed_ns'],
    source_age_cause=q['source_age_cause'],source_age_observation=a,owner_links=links,
    repaired_owner_reader_reached=False,owner_links_game_repaired=False,source_age_repaired=False,
    original_query_started=0,original_query_returned=0,cover_view_accepted=False,world_coverage_verified=False,
    movement_authorized=False,headset_accepted=False,public_changed=False,
    next='Reduce current-query source/metadata read cost or source-to-query delay with unchanged original times and guard semantics. No unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r174' not in old
entry='''
## r174：修正Actor候选在原owner读取前已发生40ms年龄拒绝（2026-11-11）

4d17da8f保存后，新cover-1111-r174-cloak-backlink、Native f5851962…426cd完成一次现有后台path观察。prepare/导入0，原A进入CT2、五段动作及operator0；启动前C盘7076175872字节，原资源准入通过。13项启动来源前后相同，共享两文件仍7e630461/385f5e5a。CLOAK740/失败0，前台459/owned_activation false。Native/entries/input恢复、XR/launcher/runner正常0、无强杀、normal_cleanup true，game/owner/XR/worker精确身份独立确认不在。

query键world2309259520/Pawn95208079616/Scene376/Compose23187/seq4183/thread40184与complete首键逐项一致；167700877905600→167700907009200共29.1036ms。code4 source，engine/source_guard/current；Owner第35次context/source_age_expired/detail6，Session第7次collider_read/guard，原因body=true/equipment=false。engine begin的source_guard阶段失败早于asset/collider/components owner组，新关联reader尚未到达；first link witness未观察，原版碰撞started/returned0/0。因此不能用本轮未再报owner_links宣称关联修复实跑通过，也不能将本次更早的年龄失败归因于尚未执行的新字段读取。

classify/current各35次，首次与首次age共四行。身体Tick23186/来源序列4184与query Compose23187/seq4183分别保留。首查比query begin晚1.0829ms，此时tracking3.2878ms、stereo12.2946ms、metadata3.9061ms，capture current3.1347ms；原40ms的最旧stereo还剩27.7054ms。首次age记录在167700907006100，stereo40.3122ms、controller40.2832ms、sync40.3065ms全部超原40ms；tracking31.3054ms与metadata31.9237ms未超，current capture31.1519ms也未超，latched=false。首次真实年龄子项已明确，未续时间或放宽期限。

initial physics2.2441ms/collider2.9930ms/components10.1253ms；重复physics7次共6.0740ms、collider7次共4.1032ms、components0次，engine11.8714ms含内部复核，不重复相加。全部span闭合。下一修复方向是同query来源到查询时延或初始组件/元数据与重复读取成本，保留全部身份/动态字段/当前值/收尾及原始时间；不通过不变重跑挑选较新输入或更快调度的一轮。

286投影帧/errors0、编码0，已有跨度到32.773秒；15页全跨度概览和五阶段原始00202/00221/00241/00258/00279均亲看，全部严重自身头肩躯干遮挡，视图FAIL。实际区间帧不是同C0/GPU原子证明，不作头显验收。原始动作、退出、精确原因、四行年龄、分段和审阅在actor-backlink-r173/runtime-r174。完整CV01–24/AC01–22及头显仍未完成，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,repaired_owner_reader_reached=False,source_age_repaired=False)))
