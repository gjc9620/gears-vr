from pathlib import Path
import json,hashlib,datetime
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
root=R/'analysis/instances/cover-1110-r113-cloak-source-stage'
p=root/'cover-output/run-result.json';result=json.loads(p.read_text());session=result['terminal_session']
assert result['operator_exit']==1 and not result['remaining_owned']
assert session['native_status']['cover_sphere_probe_attempted']=='0'
assert session['native_cleanup_verified'] and session['entry_restore_verified'] and session['vr_gamepad_cleanup']['restored']
assert session['xr_exit_code']==0 and result['isolated_done']['launcher_exit']==0
assert not result['normal_cleanup_verified'] and session['unified_weapons_cleanup']['drained'] is False
next_root=R/'analysis/instances/cover-1110-r116-cloak-source-stage'
assert not (next_root/'cover-output/claim.json').exists() and not (next_root/'launcher-session.json').exists()
assert 'imports and transformations passed' in (next_root/'cover-output/import-check.log').read_text()
record=dict(classification='progress',runtime_report=str(p),runtime_report_sha256=sha(p),
    game=session['game_identity'],xr=session['xr_identity'],source4_reexercised=False,
    first_failure='r113 generated wrapper derives O from old parent depth; resolve_camera import fails before input',
    original_query_attempted=False,cloak=result['cloak_observations'],projection_frames=result['projection_frames'],
    normal_cleanup_verified=False,unified_weapons_cleanup=session['unified_weapons_cleanup'],
    native_restore=True,entry_restore=True,pad_restore=True,xr_exit=0,launcher_exit=0,remaining_owned={},
    visual_review=['r113 frames0/1 byte-identical to personally reviewed r112 startup waiting card','r113 frame2 personally reviewed: actual stereo scene and both hands; no cover action'],
    offline_failure='r115 import-only preflight found missing tools/game-playtest path; never launched',
    fix='absolute canonical OLD binding + game-playtest path + actual generated action --prepare-import-check',
    next_instance=next_root.name,next_instance_started=False,next_import_check_sha256=sha(next_root/'cover-output/import-check.log'),
    identity_checks=14,identity_checks_scope='actual generated live() with synthetic identities',
    enumeration_findings_sha256=sha(H/'findings.json'),enumeration_evidence_sha256=sha(H/'selected-evidence.json'),
    implemented_enumeration=False,original_enumeration_executed=False,provider_closed=True,public_v71_changed=False,
    remaining=['actual source4 stage','early-stop fire disable acknowledgement2','full endpoint observation and world/shape coverage','body C0 commit/view/hit/muzzle consumers','CV01-24 AC01-22 and headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=R/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf8'))
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r113–r116：实际脚本路径修复与终点形状枚举取证（2026-11-10）

r113 在资源允许后启动，game59296／creation134387853785518413，XR53608／creation134387853851311494；Native仍r111。脚本已不再遮蔽标准 operator，但旧 wrapper 根据 parents[3] 构造依赖目录，迁移后误指 analysis/analysis，resolve_camera 导入失败，未发掩体输入；probe attempted0，不是 source4 重现。382 次本方窗口 CLOAKED 观察无失败，实际双眼3帧：前两帧与已审启动等待卡逐字节一致，最后真实场景和双手已亲自看图。完整原图与失败日志保留。

本方 Native／入口／手柄恢复true、XR0／launcher0，game／owner／XR 精确身份已不存在，未强杀。fire_disable2／hand_stop5／drainedfalse及 normal_cleanup_verified=false 原样保留，不能称全部收尾通过；实际查询、动作及早停开火确认问题仍未解决。

准备器改为显式绑定 canonical OLD，补 game-playtest 依赖；新增实际生成动作 --prepare-import-check，只执行依赖导入、原有源码转换与编译，早于游戏／输入部分退出，不修改产品规则。r115 新检查在启动前抓到缺失 game-playtest，失败保留且无game。修正后 r116 完成该实际预检和14项 live() 身份检查；0.4、原制品哈希、私有读写与新鲜度保护不变。r116 申请在两个其他活跃worker资源上限处被拒绝，尚无 claim／launcher-session，后续只在资源允许时运行；r113已消耗不可重跑。

等待资源期间对原SHA22051aa7…dd48 PhysX做只读静态取证。公共 Scene vtable+2f0 → RVA172160（完整201bytes），内部+248 → A9F10。该候选通过1AB130枚举，准确分支逐形状virtual+120，virtual+40返回公共形状，输出为8字节形状指针；与既有64字节 sweep Hit 不同。公共原count经1275A0尾跳188440作备用场景合并，不能仅据反编译void误判无返回。

重要接线边界：备用场景分支会按已返回数移动 caller buffer、减少容量；耗尽时还可能传null/0触发原分配，回调数据不保证等于初始batch。后续必须认证全部参数、缓冲区及回调预算／计数，不能直接复用现有等基址64字节Hit collector。准确参数9与候选-only分支要分清。此为静态候选，不是实际原版枚举调用；尚无终点占用、自身完整集合或移动许可，零Capsule／Sphere sweep仍不启用。

REA provider已关闭，完整snapshot及8条聚焦证据已保存。证据 overlap-enumeration-r114/{findings,selected-evidence,verification}.json。当前没有新的 Native／PG 实现或构建；公共v71不变。下一步先运行r116定位实际source4，再实现并校验终点形状枚举、完整身体路径和C0提交接线。工程25%及全部CV-01–24／AC-01–22、头肩遮挡、物理头显未完成状态不变。
'''
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,next_instance_started=False,source4_unexercised=True)))
