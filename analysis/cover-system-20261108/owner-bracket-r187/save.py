from pathlib import Path
import datetime,hashlib,json,subprocess,sys
R=Path.cwd();H=R/'analysis/cover-system-20261108/owner-bracket-r187';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
build=load(H/'native-build-result.json');checks=load(H/'checks-v5/results.json');related=load(H/'related/results.json');off=load(H/'off.json')
assert all(r['build_exit']==r.get('test_exit')==0 for r in checks+related+[off])
assert not build['external_worktree_dependencies'] and build['flags_unchanged'] and len(build['project_dependencies'])==413
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
manifest=load(H/'native-source-manifest.json');assert len(manifest['changed'])==3
for p,d in manifest['files'].items():assert sha(H/'native-source'/p)==d
for p in manifest['changed']:assert sha(R/p)==manifest['files'][p]
decoders=[]
for p,args,count in [('guard-witness-r137/inspect_guard_probe.py',['--selftest'],11),('query-timing-r163/inspect_timing.py',[],12)]:
 v=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8',str(H.parent/p),*args],capture_output=True,text=True);assert v.returncode==0;data=json.loads(v.stdout);assert data['cases']==count and data['failures']==0;decoders.append(dict(file=p,sha256=sha(H.parent/p),result=data))
verification=dict(native_sha256=build['dll_sha256'],product_changes=manifest['changed'],comparison=load(H/'checks-v5/comparison.json'),checks=checks,related=related,off=off,decoders=decoders,game_started=False,query_original_called=False,source_age_repaired=False,cover_view_accepted=False,full_requirements_accepted=False,headset_accepted=False,public_changed=False,engineering_percent=25,failures_preserved={'v1':'test global shadow warning under /WX; no tests ran','v2':'fixture Owner lacked production session.enabled check; corrected fixture, no production stop bug claim','v3':'Root=0 was valid initial capture; wrong expectation; after build missing timing header overlay','v4':'baseline passed; new generic lambda captured constexpr failed MSVC compile; corrected type expression'},next='Pin r187 private candidate and require owner_bracket timing bin before one changed-candidate controlled check; preserve original source clocks and 40ms limit.')
(H/'verification.json').write_text(json.dumps(verification,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'];assert b'## r187' not in old
entry='''
## r187：相邻纯读取使用完整前后Owner校验，失败凭据不可恢复；完整候选已编译（2026-11-11）

针对r186尚未到原查询就因原40ms来源过期的失败，本轮只将相邻纯读取归为同一次同步事务。initial physics/collider/components由6次完整Owner检查改为前后2次，每次physics/collider recheck由4次改为前后2次。完整Owner仍使用现有C0、CoverKey、输入、原时间/40ms及实际pose metadata条件；内部每个typed reader保留两遍实际值、动态字段比较和完整初末proof。内部额外核对enabled/busy、当期执行线程、binding context、身份/Scene/special-move及metadata；组内没有引擎调用、身体写入或公共结果发布，完整结束校验通过前scratch不可信，也不跨组保留Owner决定。非collider路径沿用原Owner检查。任何组失败或会话失效都消费physics/collider的recheck_ready；Owner恢复也不能在同一操作中重用失败凭据，须重新初始采样。

实际Session/typed reader夹具对照：原105检查通过；旧版400项/新版328项均0失败（完整边界次数不同，因此按每个实际边界生成的拒绝用例数量不同）。成功路径完整Owner16→8次，读取70488→59976，减少14.913%；只证明该离线夹具读取量，不代表游戏耗时、来源过期或完整查询已修复。覆盖所有完整Owner边界拒绝、读取中来源失效/释放重按/人物替换/停止/世界与Scene变化/错线程/Root变更、初始读取失效及失败后权限恢复再试；Host只替代原查询分派，原游戏调用0。相关真实生产Session路径391、overlap53、guard23、timing136项均0失败，OFF51项0失败；timing另包含原105。解释器11/12例0失败，只有解码含义证明。

Session witness追加owner_begin/end，原编号不变；timing追加owner_bracket第8槽。新组的initial/repeat分段不含组外完整Owner耗时，owner_bracket记录该成本；engine仍含内部重叠，不能相加，也不能将新旧分段直接称为速度提升。历史8槽记录仍可解读，新候选实际证据必须要求第8槽存在。

精确r183冻结源只覆盖cover_physics_query_live.inl、cover_query_guard_observation.h、cover_query_timing.h三份。完整Native编译/链接0，SHA256 e09b3e0ff8ea8da3c6998022254932835763540052a2eb0bc2201a688c64f6af；413份项目依赖均来自本次冻结树，构建开关不变，无外部工作树依赖。源码、来源清单、编译回执和对照在owner-bracket-r187。

保留v1测试全局遮蔽警告、v2夹具漏session.enabled、v3初始Root0原本合法的错误预期及漏timing头覆盖、v4模板constexpr编译失败。v5修正后通过，不将这些夹具/编译失败称为产品已复现故障。本轮未启动游戏、未安装public。下一步精确固定r187候选与新增计时槽后做一次有变化的受控检查。r186严重自身模型遮挡、世界碰撞覆盖、实际身体/枪口/受击、全CV01–24/AC01–22及实体头显仍未完成；工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
paths=[*manifest['changed'],'tools/native-capture/cover_query_owner_bracket_test.cpp','analysis/cover-system-20261108/guard-witness-r137/inspect_guard_probe.py','analysis/cover-system-20261108/query-timing-r163/inspect_timing.py',doc.relative_to(R).as_posix(),gp.relative_to(R).as_posix()]
for directory in [H,*[H/f'checks-v{i}' for i in range(1,6)],H/'related']:
 if directory.exists():
  paths.extend(p.relative_to(R).as_posix() for p in directory.iterdir() if p.is_file() and (p.suffix in ('.py','.json','.log') or p.name=='delta.patch'))
paths=sorted(set(paths));(H/'stage-paths.json').write_text(json.dumps(paths,indent=2)+'\n');print(json.dumps(dict(saved=True,native=build['dll_sha256'],stage_files=len(paths))))
