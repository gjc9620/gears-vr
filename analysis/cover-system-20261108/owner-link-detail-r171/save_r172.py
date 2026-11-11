"""Save exact failing asset back-link and normal lifecycle, without acceptance."""
from pathlib import Path
import datetime,hashlib,importlib.util,json,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
root=R/'analysis/instances/cover-1111-r172-cloak-links';out=H/'runtime-r172'
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(root/'cover-output/run-result.json');s=v['terminal_session']
q=load(out/'path-probe.json');t=load(out/'timing-probe.json');links=load(out/'owner-links.json');age=load(out/'age-probe.json')
assert v['normal_cleanup_verified'] and v['operator_exit']==0 and v['start_exit']==0 and not v.get('error')
assert not v.get('projection_error') and not v['projection_errors'] and v['projection_thread_joined']
assert not v['remaining_owned'] and not v['forced_termination'] and v['anchor_sources_unchanged']
assert len(v['anchor_source_sha256'])==13 and sha(root/'native/GearsCapture.dll')==q['native_sha256']
assert q['query_code_name']=='owner_links' and q['original_started']==q['original_returned']==0
assert q['owner_guard']['calls']==39 and not q['owner_guard']['failed_call']
assert q['session_guard']['calls']==8 and not q['session_guard']['failed_call']
assert links['reader']=='asset' and links['phase']=='before_dispatch' and links['reverse_mismatch']
assert links['row']['reverse']==28 and links['row']['actor']==2309310912
assert not links['scene_mismatch'] and not links['internal_invalid']
assert t['elapsed_ns']==22783100 and all(x['calls']==x['closed_spans'] for x in t['rows'])
assert age['calls']==[39,39] and len(age['rows'])==2 and all(not x['expired_fields'] for x in age['rows'])
spec=importlib.util.spec_from_file_location('r172_pose',H.parent/'pose-probe-r126/inspect_pose_probe.py')
reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
pose=reader.inspect(s,q['native_sha256']);(out/'pose-probe.json').write_text(json.dumps(pose,indent=2)+'\n')
complete=next(x for x in pose['rows'] if x['name']=='complete')
for k in ('scene','compose','sequence','world','pawn'):assert complete[k]==int(q['raw'][k])
boundary=next((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));a=load(boundary)
assert a['passed'] and a['after_original_A']['cover_state_raw']['CoverType']==2 and len(a['eye_observation'])==5
review=load(out/'media-review.json')
assert review['frames']==256 and len(review['pages'])==13 and len(review['contacts'])==5 and review['encode_exit']==0
assert not review['recording_errors'] and not review['missing_contacts'] and review['captured_span_complete']
for item in review['pages']+review['contacts']:assert sha(Path(item['file']))==item['sha256']
review.update(captured_span_personally_reviewed=True,full_span_personally_reviewed=True,reviewed_pages=13,
    reviewed_original_contacts=5,verdict='All five cover phases have severe self-model obstruction; FAIL',cover_view_accepted=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(review,indent=2)+'\n')
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();ids={k:s[k] for k in ('game_identity','owner_identity','xr_identity')}
ids['worker_identity']=load(root/'isolated-worker.json')['worker_identity']
for identity in ids.values():assert c.identity(identity['pid'])!=identity
index=load(root/'launcher-session.json')
for source,name in ((Path(index['session_report']),'session-original.json'),(boundary,'cover-boundary-original.json'),
    (boundary.parent/'equipment-chain.json','equipment-chain-original.json'),(boundary.parent/'recording.json','action-recording-original.json')):
    shutil.copyfile(source,out/name)
summary=load(out/'closeout.json');summary.update(identities=ids,exact_owned_processes_absent=True,
    native_sha256=q['native_sha256'],runner_exit=0,original_low_cover_entered=True,same_C0_complete_query_key=True,
    query_elapsed_ns=t['elapsed_ns'],owner_links=links,source_age_repaired=False,owner_links_repaired=False,
    original_query_started=0,original_query_returned=0,cover_view_accepted=False,world_coverage_verified=False,
    movement_authorized=False,headset_accepted=False,public_changed=False,
    next='Authenticate original internal-actor type and reverse-link layout from saved binary evidence. Do not skip actor or guess an offset.')
(out/'closeout.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r172' not in old
entry='''
## r172：实际asset内部反向关联值28，原碰撞未执行；正常结束（2026-11-11）

4bb31ba6保存后，新cover-1111-r172-cloak-links以Native a2b53166…c8b95完成一次现有后台path观察。prepare/operator导入0，原A进入CT2、五段eye动作及operator0；启动前C盘14653812736字节、原资源准入通过。13项启动来源前后相同，共享两文件仍7e630461/385f5e5a。CLOAK689/失败0，前台419/owned_activation false。Native/entries/input恢复、XR/launcher/runner正常0、无强杀、normal_cleanup true，game/owner/XR/worker精确身份独立核验不在。

query键world2310045952/Pawn95274860800/Scene396/Compose24376/seq4161/thread21936，与complete首键逐项一致；165621403132800→165621425915900共22.7831ms。code12 owner_links，原碰撞started/returned仍0/0。新首失败见证明确reader=asset、before_dispatch、reverse分支：Actor2309310912/Table140720217927848，预期与实际Scene均15959962496、Internal95275827456格式有效；既有Internal+0x10读取成功但Reverse=28，未指回Actor。不是本次场景错配或内部指针格式失败。仍不证明Actor损坏、具体内部类型、正确反向字段或唯一生命周期原因；下一步按原二进制核对类型与布局，不跳过该Actor、不猜偏移、不删除关联检查。

Owner39/Session8均无failed-call，年龄cause未观察，body/equipment原因unknown。classify/current各39次，只有首次两行：身体Tick24375、来源序列4163，与query Compose24376/seq4161分别保留；首查偏移0.2170ms，tracking0.8123ms、stereo1.4843ms、metadata1.4514ms，无记录到年龄拒绝，不将历史年龄问题视为已修复。initial physics0.7406ms/collider2.5525ms/components7.6377ms；physics repeat8共6.9899ms/collider repeat8共4.2953ms/components repeat0，engine11.4666ms含内部复核，不能重复加；全部span闭合。

256投影帧/errors0，已有捕获跨度到29.515秒；13页全跨度概览及五阶段原图00173/00192/00211/00231/00249全部亲看，中立/转头/恢复/升高/下降仍严重自身头肩躯干遮挡，画面FAIL。五段联系只按实际时间区间，不冒充同C0 GPU原子证明；编码0不表示视图通过。原始动作、退出、精确失败值、年龄、分段和审阅在owner-link-detail-r171/runtime-r172。完整CV01–24/AC01–22及头显未完成，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,normal_cleanup_verified=True,reader='asset',reverse=28,owner_links_repaired=False)))
