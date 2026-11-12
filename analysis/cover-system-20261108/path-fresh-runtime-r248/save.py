from pathlib import Path
import json,hashlib,datetime,shutil
h=Path(__file__).resolve().parent;r=h.parents[2];o=h/'runtime-r248';root=r/'analysis/instances/cover-1112-r248-cloak-path-fresh';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());s=v['sphere'];assert v['operator_exit']==0 and v['normal_cleanup_verified'] and not v['remaining_owned'] and v['original_started']==v['original_returned']==1 and s['cover_path_probe_attempted']=='1' and s['cover_path_probe_code']=='4' and s['cover_path_probe_owner_guard']=='91,91,8,3,6'
assert v['source_sha256']==sha(o/'native-session-status-original.txt')
shutil.copyfile(root/'cover-output/projection/recording.json',o/'recording-original.json')
paths=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(paths)==1;shutil.copyfile(paths[0],o/'cover-boundary-original.json')
verification=dict(original_started=1,original_returned=1,path_result_published=False,elapsed_ms=v['elapsed_ms'],failure='Final owner/source age rejection after the first original query returned; not original-query fault or missing owner link.',physics_repeat_calls=42,collider_repeat_calls=42,component_repeat_calls=2,full_owner_calls=91,normal_cleanup_verified=True,remaining_owned={},personally_reviewed_frames=False,gameplay_accepted=False,headset_accepted=False,engineering_percent=25,next='Optimize synchronous no-dispatch source-reading groups with complete typed/full owner checks before dispatch and after close. Never borrow a proof across original calls or renew timestamps.')
(h/'verification.json').write_text(json.dumps(verification,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r248' not in old
actual=json.loads((o/'run-result-original.json').read_text())['terminal_session']['game_identity']
add='''
## r248：新鲜严格候选通过，path首次原调用返回后在36.7614ms因时效拒绝（2026-11-12）

仅一次新实例cover-1112-r248-cloak-path-fresh复核r247明确的一项registry新增，准备08bc6e36、r244 Native66a96c12…97f4，actual game{pid}/creation{creation}。未修改产品/原reader/任何关闭条件，原候选选择与五段ordinary观察operator0。此时实际到达path：attempted/finished1、q4 source、Session stage5 engine/detail4、engine stage9 guard_end/source_code0/exception0。原调用started0→1/returned0→1；不是原调用故障，也不是r247未尝试状态。36.7614ms后完整owner91/failed91 context source_age_expired extra6，Session guard45/failed45 owner_end，source_age_cause observed1/verdict3/causes1。hits/segments/endpoint均0是未发布，不是无碰撞；原调用一次返回之后不自动重试。

timing：physics initial.4819ms、collider initial1.7541ms、components initial2.4775ms，physics repeat42次合3.4007ms、collider repeat42次合5.7734ms、components repeat2次合4.7551ms；engine31.1325ms；owner brackets86次合17.0784ms（嵌套项目不相加当总耗时）。首次与最后source-age窗口保留原tick/metadata/tracking时间，明确是原年龄过期，不抬deadline或续钟。此前sphere原调用1/1已完成全关闭；现在缺口具体缩到带owner集合的读重复与关闭时效。

266实际投影/errors0，记录清单保存但未逐张亲看/未编码，视觉接受false；正常Native/entry/XR/game清理true，自有remaining空，anchor/reader未变。下一步对QueryEngine纯读sources及owner归属的同步分组减少重复完整角色证明；borrow只限无dispatch组内，分组前后完整typed证明与原owner/cover/pose/原时效必须保持，所有原调用边界继续用普通完整guard。不能将r241 asset-only借用guard直接包住原query。优先复用现有组内读取/最终关闭结构及fault/mutation测试，修复后一次新实例定向检查。

CV24/AC22、完整身体几何/full self/world fence/body-view-hit-muzzle/遮挡和头显仍未完成，工程25%；publicv71/固定入口/Quest保持。证据path-fresh-runtime-r248。
'''.format(pid=actual['pid'],creation=actual['creation_time'])
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(verification))
