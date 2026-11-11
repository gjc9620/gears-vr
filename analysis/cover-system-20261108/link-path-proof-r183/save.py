from pathlib import Path
import hashlib,json,re,subprocess,datetime
H=Path(__file__).resolve().parent;R=H.parents[2];load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=load(H/'native-build-result.json');manifest=load(H/'native-source-manifest.json');checks=load(H/'checks-v2/results.json');comparison=load(H/'checks-v2/comparison.json');off=load(H/'off-result.json')
assert len(checks)==2 and all(x['build_exit']==x['test_exit']==0 and not x['outside_worktree_dependencies'] for x in checks)
assert off['build_exit']==off['test_exit']==0 and 'baseline_unclosed_ancestor=1' in checks[0]['output']
assert build['flags_unchanged'] and not build['external_worktree_dependencies'] and len(build['project_dependencies'])==413
assert build['changed']==manifest['changed']==['tools/native-capture/cover_context_live.inl']
for p,d in manifest['files'].items():assert sha(H/'native-source'/p)==d
assert sha(R/manifest['changed'][0])==manifest['files'][manifest['changed'][0]] and sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
for v in ('before','after'):assert sha(H/'checks-v2'/v/'tools/native-capture/cover_link_path_proof_test.cpp')==sha(R/'tools/native-capture/cover_link_path_proof_test.cpp')
result=dict(native_sha256=build['dll_sha256'],changed=build['changed'],comparison=comparison,baseline_final_ancestor_accepted=True,after_final_ancestor_rejected=True,owner_guard_count_changed=False,source_times_changed=False,source_age_repaired=False,game_started=False,public_changed=False,checks=checks,off=off)
(H/'verification.json').write_text(json.dumps(result,indent=2)+'\n');(H/'delta.patch').write_bytes(subprocess.run(['git','diff','--','tools/native-capture/cover_context_live.inl'],cwd=R,capture_output=True,check=True).stdout)
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r183' not in old
entry='''
## r183：掩体链接完整继承链并入现有Proof，末级父类收尾缺口修复；已编译（2026-11-11）

沿r181的40ms拒绝和r182重复读取证据，未减少Owner/来源/释放的调用次数。找到cover_context_live::link_layout每次values均重新derives完整CoverLink类链，然后又begin_prepare读取属性Proof；同一sample_current三次values、每个Owner又调用sample_current。现在首次完整derives后，把每级类身份、Class类型和父类边全部认证进原link_proof，包含末级祖先的零父类边。后续使用原begin_prepare/current统一fresh复核；metadata generation或class key变化仍使原Proof失效。pawn/link已ready且当前class地址精确匹配时，不再在完整Proof前额外读一次同一class；该class身份仍由完整Proof实际重读。未改共享binding、追踪、骨骼、时间、40ms或Writer，不把缓存当授权，也没有传true跳Owner。

原206项context检查保持通过。真实生产reader前/后各288项0：不同继承层数、字段/父类/类型/身份/槽位/世界代次/录制及线程门禁，以及最后一次slot读取时把末级Actor父类改成环。旧版该晚变更确实被接受，baseline_unclosed_ancestor=1；新版最终link_proof current拒绝且输出保持。类链的初始推导一直存在，新增的是完整落入原有末尾Proof。

五轮完整sample_current（含原Owner绑定与三轮cover values）在额外继承0/4/12级的读取24220→22645（-6.50%）、28560→24345（-14.76%）、37240→27745（-25.50%）。v1只合入完整类链，前后288项0且有收益；v2再去掉Proof之前重复的相同class读。所有原始结果保留。收益依赖实际类深度，不能外推实际游戏耗时或40ms已修；OFF编译/运行0，未动关闭分支。

完整Native0d4437fd5427902f84e5571cb1e3f577c201b114bfaf2843b3e4483f998cdf6f编译链接0，exact r177仅cover_context_live.inl一文件delta，413项目依赖、worktree外漏0、GEARS flags不变，其余冻结源前后相同。测试新增main命名宏只为复用原fixture，不属于产品delta。prepare新增--native-r183认证精确候选。代码/反例/读取计数/原失败范围/manifest/build/verify在link-path-proof-r183。

下一步Git保存后用新实例短查实际CoverKey复核和原query入口。该候选尚未启动，实际40ms、完整owner/world/body几何、自身模型遮挡、CV01–24/AC01–22和实体头显未完成；工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(saved=True,native=build['dll_sha256'],comparison=comparison)))
