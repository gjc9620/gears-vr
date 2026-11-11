from pathlib import Path
import json,hashlib,datetime
r=Path('J:/codexWorktree/gearsVr');h=r/'analysis/cover-system-20261108/asset-shape-runtime-r228';o=h/'runtime-r228';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=json.loads((o/'closeout.json').read_text());assert v['normal_cleanup_verified'] and v['operator_exit']==0 and v['remaining_owned']=={} and v['projection_errors']==[]
m=json.loads((o/'media-review.json').read_text());assert m['frames']==268 and len(m['contacts'])==5 and m['encode_exit']==0
m['personally_reviewed_contacts']=[dict(label=x['label'],file=x['file'],sha256=x['sha256'],result='FAIL: own head/shoulder/torso severely obstruct both eyes') for x in m['contacts']];m['full_span_personally_reviewed']=False;m['visual_acceptance']=False
(o/'media-review.json').write_text(json.dumps(m,indent=2)+'\n')
retention=dict(state='deferred',reason='Other GOWDE-Steam/OpenXRPoseProbe processes appeared after owned run terminal; no cleanup or media sweep against another session.',observed_pids=[37740,49828],owned_game_pid=42816,owned_xr_pid=33080,owned_run_terminal=True,other_sessions_touched=False)
(o/'retention-deferred.json').write_text(json.dumps(retention,indent=2)+'\n')
gp=r/'analysis/cover-system-20261108/requirements-owner-sync.json';g=json.loads(gp.read_text(encoding='utf-8'));doc=r/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r228' not in old
add='''
## r228：新全形状读取真实进入C0／Session后在shape inventory拒绝；正常退出（2026-11-12）

私有新实例cover-1112-r228-cloak-shapes使用r227 Native3972af3f…f3b7，prepare新增显式固定r227选择，逐422个冻结依赖SHA认证；runner新增shape模式且三个环境开关全部明确互斥，保持原输入/owner/启动/资源/静音/0.4/CLOAK/清理保护。生成operator导入通过，实际live身份guard14离线检查通过。准备与runner代码已保存9ce46d24；未修改publicv71/固定手动入口/Quest。

实际游戏42816/creation134389104154093969，XR33080；原A进入掩体、五段eye观察operator0。新读取确实触发：attempted1/finished1，admission6/session_entered1、Session engine stage5，q::Code10 owner_read，asset_shapes::Code7 shapes，14.6558ms；owner guard35/session guard16均无failed_call，source_cause未观察，original started/returned均0。整批失败，bodies/actors/shapes发布数0；不能把0解读成真实空清单，也不能声称完整读取或source-end关闭已成功。当前失败位于asset observer内shapes::observe，尚缺nested shape_code与failed_actor/member；下一步补出已有workspace诊断字段，依具体布局失败修复，不重跑旧timer或放宽来源时效。

268实际投影帧、错误0、14全跨度overview已生成、5阶段contact已逐张亲看；五张双眼自身头肩/躯干遮挡严重，视图验收FAIL。overview未全数亲看，保留full_span_personally_reviewed=false，不称全跨度人工验收。实际captured-span编码0。正常清理true，Native/entry/input/weapon drain恢复、XR/launcher正常退出，自有remaining为空、无强杀；CLOAK639次失败0，前台421次无自有激活，13项anchor来源和cover reader前后相同。

本实例终止后观察到另一个游戏37740/XR49828，未触碰；媒体保留清理因其他活动会话暂缓，仅保存deferred记录。完整CV24/AC22、active/template身份、static/weld/flags/full self/world fence、body/view/hit/muzzle同版事务和视图遮挡/头显继续未完成，工程25%。本轮的实际拒绝已缩小下一步范围，证据asset-shape-runtime-r228/runtime-r228。
'''
new=old+add.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('r228 runtime and failure records saved; documents synchronized')
