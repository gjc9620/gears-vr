"""Preserve matched real timings and failed view, with independent cleanup."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r168-cloak-recheck';out=H/'runtime-r168'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session'];t=load(out/'timing-probe.json');q=t['query'];raw=q['raw']
assert v['normal_cleanup_verified'] and v['operator_exit']==0 and not v.get('error') and not v['remaining_owned'] and not v['forced_termination']
assert v['projection_frames']==275 and not v['projection_errors'] and v['projection_thread_joined'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['source_age_cause']['causes']==1 and q['original_started']==q['original_returned']==0
assert q['first_rejection_layer']=='engine/source_guard/current' and q['owner_guard']['failed_call']==39
assert q['session_guard']['failed_call']==8 and q['session_guard']['name']=='collider_read'
assert t['elapsed_ns']==26811100 and all(r['calls']==r['closed_spans'] for r in t['rows'])
spec=importlib.util.spec_from_file_location('r168_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(r for r in pose['rows'] if r['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert int(raw[k])==complete[k]
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
review=load(out/'media-review.json');assert len(review['pages'])==14 and len(review['contacts'])==5 and review['encode_exit']==0
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(full_span_personally_reviewed=True,reviewed_pages=14,reviewed_original_contacts=5,
    verdict='All five actual phases have severe self-model obstruction; FAIL',cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json')):shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,native_sha256=q['native_sha256'],
    original_low_cover_entered=True,same_C0_complete_query_key=True,query_elapsed_ns=t['elapsed_ns'],timing=t['rows'],
    source_age_cause=q['source_age_cause'],owner_guard=q['owner_guard'],session_guard=q['session_guard'],
    original_query_started=0,original_query_returned=0,source_age_repaired=False,cover_view_accepted=False,
    world_coverage_verified=False,movement_authorized=False,headset_accepted=False,public_changed=False,
    physics_repeat_cost_exceeds_collider_repeat=True,nested_times_must_not_be_added=True,
    next='Inspect exact query-entry source ages plus repeated physics cost; preserve freshness and all read/closing checks. No unchanged rerun.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')

gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r168' not in old
entry="""
## r168：同query collider重证候选实跑八轮，仍身体来源年龄拒绝（2026-11-11）

b8389dd3保存后，以新cover-1111-r168-cloak-recheck、Native a41fadcf…b3ce8做一次现有后台path观察。准备/导入0，13项启动来源前后相同，共享两文件SHA仍7e630461/385f5e5a；启动前磁盘47905591296字节。原A进入CT2、五段eye观察及operator0。275投影帧/errors0、编码0；14页全跨度和5实际阶段contact均亲看，候选8/7/7/7/7，五阶段严重自身头肩/躯干遮挡仍FAIL，区间图不冒充同C0/GPU或头显证明。Native/entries/input恢复、XR和launcher正常退出，normal_cleanup true、无强杀，game/owner/XR/worker精确身份独立确认不在；CLOAK719/失败0，前台427/owned_activation false。

path键Scene326/Compose19992/seq4014/world2422112512/pawn95206631040与complete首键逐项相同。engine/source_guard/current，Owner第39次context/source_age_expired/detail6，Session第8次collider_read/guard；原因见证entry14674320/Compose19992/thread38732/stage0/verdict3/causes1，body年龄拒绝、equipment当次未过期。原碰撞started/returned仍0/0；新recheck确实进入实际同步查询并完成前序复核，但未通过整链。离线晚到OwnerComponent变更的拒绝未在游戏主动注入，不称该race实跑通过。

query160968869125600→160968895936700共26.8111ms；计时invalid0/overflow0、所有span闭合。initial physics0.9051ms、collider2.5732ms、components10.5233ms；physics repeat8次共7.2814ms、collider repeat8次共4.8876ms，components repeat0；engine12.3712ms、closing0。engine已包含内部复核，不能再相加。与r166五次collider13.6629ms相比，本次八次4.8876ms是实测观察；入口年龄/调度/Actor上下文未控制，不能认定全部差异来自代码、不能据此称全query修复。新动态字段关闭检查已在正常静态游戏路径消费，完整身体/世界覆盖/移动许可仍false。

头部遮蔽累计head_section_applied623、palettes623、uncomposed426、shoulder_armor623；最终像素仍遮挡，因此不能用累计应用计数替代实际消费或画面验收。原始动作、同query分段/原因、退出及审阅记录在collider-recheck-r167/runtime-r168。下一步核对同query入口各来源已消耗的年龄及重复physics成本，保留原时间与全部动态/身份/关闭检查，不原样重跑、不放宽年龄或刷新旧样本。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口及头显状态不变，目标active。
"""
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,collider_repeat_ns=4887600,source_age_repaired=False)))
