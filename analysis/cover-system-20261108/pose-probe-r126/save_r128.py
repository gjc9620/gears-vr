from pathlib import Path
import ast,datetime,hashlib,json,re,shutil,sys
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r128'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
root=R/'analysis/instances/cover-1110-r128-cloak-registry';index=load(root/'launcher-session.json')
report=Path(index['session_report']);session=load(report);done=load(root/'isolated-done.json')
assert session['phase']=='running' and not session.get('native_cleanup_verified')
assert done['launcher_exit']==1 and done['owned_game_absent'] and not done['forced_termination']
sys.path.insert(0,str(R/'tools/native-capture'))
from run_stereo_continuous import WindowsControl
c=WindowsControl();identities={k:session[k] for k in ('game_identity','owner_identity','xr_identity')}
worker=load(root/'isolated-worker.json')['worker_identity'];identities['worker_identity']=worker
for identity in identities.values():assert c.identity(identity['pid'])!=identity
lost={}
for name in ('cover-output/run-result.json','cover-output/projection/recording.json'):
    p=root/name;assert p.stat().st_size==0;lost[name]=dict(bytes=0,sha256=sha(p))
for source,name in ((report,'session-last-running-original.json'),(root/'launcher-session.json','index-original.json'),
                    (root/'isolated-done.json','isolated-done-original.json'),(root/'cover-output/operator.log','operator-original.log'),
                    (root/'isolated-launcher.log','launcher-original.log'),(root/'native/session-status.txt','native-last-status-original.txt')):
    assert not (out/name).exists();shutil.copyfile(source,out/name)
assert 'No space left on device' in (out/'operator-original.log').read_text()
raw=dict(re.findall(r'([A-Za-z_][A-Za-z_0-9]*)=([^\s]+)',(out/'native-last-status-original.txt').read_text()))
assert raw['pid']==str(session['game_identity']['pid']) and raw['phase']=='running'
media=load(out/'media-recovery.json');assert media['files']==media['decoded']==51 and len(media['pages'])==3
media.update(full_span_personally_reviewed=True,visual_result='51 surviving ordinal projection files show start scene and arms; no cover entry evidence; no timestamps or VFR manufactured')
(out/'media-recovery.json').write_text(json.dumps(media,indent=2)+'\n')
archive=Path('J:/gearsVr/analysis/cover-disk-recovery-20261110-r128')
names=['cover-1110-r118-cloak-source-stage','cover-1110-r119-cloak-source-stage','cover-1110-r120-cloak-source-stage',
       'cover-1110-r123-cloak-source-stage','cover-1110-r125-cloak-layout','cover-1110-r127-cloak-pose-gate']
archives=[]
for name in names:
    original=R/'analysis/instances'/name;target=archive/'instances'/name
    assert original.is_junction() and original.resolve()==target.resolve()
    receipt=archive/'instances'/(name+'-hashes.json')
    archives.append(dict(original=str(original),target=str(target),
        sha_manifest=str(receipt) if receipt.exists() else None,sha_manifest_sha256=sha(receipt) if receipt.exists() else None))
intermediates=load(archive/'moved-intermediates.json');assert len(intermediates)==57
for row in intermediates:assert not Path(row['source']).exists() and sha(Path(row['archived'])).upper()==row['sha256']
for rel in ('layout-witness-r124/runtime-r125','overlap-query-r122/runtime-r123'):
    old=load(H.parent/rel/'media-review.json')
    for contact in old['contacts']:assert sha(Path(contact['file']))==contact['sha256']
guard_source=R/'tools/multi-instance/isolated_sim.py';receipts_source=R/'tools/multi-instance/runtime_receipts.py'
ast.parse(guard_source.read_text());ast.parse(receipts_source.read_text())
assert guard_source.read_text().count('require_disk(ROOT)')==3
record=dict(classification='progress: disk-full failure preserved; only own stopped evidence archived to recover disk',
    identities=identities,process_absence_verified=True,normal_cleanup_verified=False,native_cleanup_verified=None,entry_restore_verified=None,xr_exit_code=None,
    launcher_exit=1,forced_termination=False,final_report_lost=True,last_report_phase='running',
    first_failure='ENOSPC during read_private_input evidence write before ordinary cover observation; final report and frame timing manifest also zero-length',
    lost_files=lost,cover_actions_verified=False,pre_probe_failure_observed=False,query_executed=None,collision_clear=None,
    last_running_native_probe_counts={k:v for k,v in raw.items() if k.startswith('cover_pose_probe_count_')},
    native_status_is_terminal=False,media_sha256=sha(out/'media-recovery.json'),
    audio_scope='one surviving mute_state_verified receipt with verified_observations1; no full-span summary',
    archives=archives,intermediate_archive=str(archive/'moved-intermediates.json'),intermediates_bytes=sum(x['bytes'] for x in intermediates),
    disk_free_when_failed=0,disk_free_after_recovery=shutil.disk_usage(R).free,
    shared_guard_review=dict(source_sha256=sha(guard_source),receipts_sha256=sha(receipts_source),minimum_bytes=4*1024**3,
        scope='read source: prepare, resource admission and worker pre-game check; shared implementation not edited here; no new game verification'),
    remaining=['fresh run under current disk guard','actual observer repair branch','pre-probe reject reason and PhysicsRoles layout','all cover requirements and headset'])
(out/'closeout.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r128：C盘耗尽导致证据写失败，保留不完整收尾并恢复本方空间（2026-11-10）

b121d9c4保存r127观察器修正后，新票cover-1110-r128-cloak-registry完成实际冻结adapter导入检查，Native仍d7cb104d…c2e5，PG/XR不变，0.4；资源满时持续只读等待，释放后实际启动一次。game20160/creation134387945036784154，owner55412/creation134387944961903055，XR42940/creation134387945102409663。实际epoch1投影已发生，但read_private_input第一次写证据时ENOSPC，operator未取得输入观测；随后run-result.json及projection/recording.json最终写入也失败，二者保留原始0字节，不重建为成功报告。

现场确认C盘free0。启动器exit1，isolated-done记录owned_game_absent true/无强杀；另以精确PID/creation/image复核game/owner/XR/worker均不在。可读的原session与Native status仍是running旧快照，Native/entry/pad正常恢复、drained与XR exit均无终态回执，normal_cleanup_verified=false（未证），不能由进程消失补签正常清理。最后可读gate计数均0只是旧观察，不当最终查询证明。没有已验证的原版掩体进入、前置姿态拒绝或原查询执行。

保留51张实际JPEG且全部可解码，3张按文件序号的overview均亲自查看，只见起点场景与手臂；计时manifest已丢失，不编造帧时刻、phase对应或VFR视频。音频日志只剩一次mute_state_verified/verified_observations1，无整段summary，不称完整静音验收。原失败日志、最后running报告、Native status和0字节摘要均单独保留。

仅迁移本方已终态且精确进程已不存在的r118/r119/r120/r123/r125/r127旧实例至J:/gearsVr/analysis/cover-disk-recovery-20261110-r128，保留原C路径junction；r118/119/120/127全文件前后SHA一致，r123/125原10张phase图SHA一致。另将本方r124/r126的57份可重建obj/pdb（约64MB）移至同一J归档并逐SHA，源码、DLL、日志及其他会话文件均保留。初始复制后递归删除方案被自动审查拒绝，改用直接Move-Item归档加原路径映射完成，无单独递归删除；不绕开用户文件边界。空间回升约6GB，随后其他正常写入导致浮动，以closeout瞬时读数为准。

已只读审阅统筹新增共享require_disk：prepare、资源准入及worker真正启动前均检查证据卷至少4GiB，不终止现有实例或删文件；本会话未修改共享工具，也不改旧票hash绕过检查。后续使用新票绑定最新来源。证据pose-probe-r126/runtime-r128/{closeout,media-recovery}.json及原始日志。完整CV01–24/AC01–22、工程25%、公共v71、头肩遮挡及物理头显未完成状态保持；下一步仍是新磁盘准入下的定向掩体观察。
'''
assert '## r128：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,processes_absent=True,normal_cleanup_verified=False,disk_free=record['disk_free_after_recovery'])))
