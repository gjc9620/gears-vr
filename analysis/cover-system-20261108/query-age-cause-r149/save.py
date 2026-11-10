"""Save source/build evidence and synchronize the existing development log."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=load(H/'native-build-result.json');test=load(H/'checks-v1/result.json');guards=load(H/'guard-verification.json')
assert test['build_exit']==test['test_exit']==0 and '25 checks, 0 failures' in test['output']
assert len(build['project_dependencies'])==409 and not build['external_worktree_dependencies'] and build['flags_unchanged']
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
header='tools/native-capture/cover_query_source_cause.h'
assert sha(R/header)==sha(H/'native-source'/header)
assert len(guards)==2 and all(r['query_guard_order_and_conditions_unchanged'] and r['original_commit_journal_filter_unchanged'] for r in guards)
verification=dict(scope='Source, data-only observer, compiler/linker; no game/XR started',dll_sha256=build['dll_sha256'],
    project_dependencies=409,external_native_worktree_dependencies=0,flags_unchanged=True,changed=build['changed'],tests=test,
    parser_cases=15,guard_text_verification=guards,header_sha256=sha(R/header),
    observed_data='Existing source_rejected causes only; bit1 BodyTickSource source_age_only, bit2 equipment freshness false',
    scope_key='Entry address + Compose + thread + prepare stage + source_age_expired verdict; private query only',
    new_game_reads=0,new_clock_reads=0,new_game_writer=False,new_retry=False,
    existing_commit_journal_filter_unchanged=True,actual_r148_cause_proven=False,public_changed=False,headset_accepted=False,
    remaining='One fresh private query check to obtain an actual matching source cause; no unchanged r148 rerun.')
(H/'verification.json').write_text(json.dumps(verification,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
before=doc.read_bytes();assert before==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r149：私有查询同次prepare年龄原因观察候选（2026-11-11，已编译未实跑）

r148组件读取阶段Owner context/source_age_expired/detail6没有body/equipment分项，不能凭35.2868ms推断总年龄。原ActualContext已将原因传入ready_wrist::source_rejected：bit1为BodyTickSource::source_age_only，bit2为equipment_fresh=false；原journal只记录commit，因此不覆盖本次prepare失败。

新增cover_query_source_cause.h纯记录器，只有private sphere/path查询在原context.current(prepare)调用范围临时安装线程局部Scope；按Entry地址、Compose、thread、prepare与source_age_expired精确匹配，只收首个已有回调原因。相同地址晚一Compose、错线程/阶段/判词、嵌套查询和Scope结束后的事件不能串入；退出/异常恢复原Scope，不保留栈指针。ready_wrist原commit journal筛选与行为不动；默认无private查询时不安装观察。没有额外game read/clock/锁/输入/lease/guard/engine调用或writer。

query原RAII发布旁附加原因副本，现有path一发结果保存并输出cover_path_probe_source_age_cause七项。解释器先核精确Native，再核同一Compose/thread及Owner context/detail6才解释两位原因；未观察时保持unknown，不将0视为新鲜或通过。只解释现有拒绝，不授予碰撞/身体移动许可。

实际纯记录器25项通过，覆盖关闭/空key/入口、Compose、thread、stage、verdict不符，其他真实CPU线程、嵌套、异常退出、首证不覆盖和地址重用；解释15项通过。canonical与精确冻结query函数去掉观察代码后，原guard顺序/判据逐字相同，原source_rejected commit筛选亦逐字相同。源码按各自原文窄变换，不把共享其他Native改动带入冻结树。

完整Native在r146冻结上仅改ready_wrist_entry_live.inl、cover_physics_c0_live.inl并新增一个头；编译链接0，SHAac5cf37f3683a20900d3927510e56ab42cb0528080f0ea31f360f3537344ebda。409项目依赖全部冻结，外部Native worktree依赖0，GEARS开关相同。prepare新增--native-r149，只接受这3文件delta。原测量、查询和当前BodyTick来源/装备时限未改。

本轮未再启动游戏，r148具体哪一位为真仍未知，不称已定位或修复。下一次新票只取得同次原因及原查询结果，不重复未改玩法矩阵，不续时旧来源。measurement neutral257的实际数学分项、完整身体/世界覆盖、抓边/松手保持/重抓、遮挡/受击/枪口及CV01–24/AC01–22仍开放；工程25%、publicv71和固定入口不变，目标active。
'''
updated=before+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(updated);mirror.write_bytes(updated)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,dll=build['dll_sha256'],checks=25,parser_checks=15,game_started=False)))
