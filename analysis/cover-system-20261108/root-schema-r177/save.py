from pathlib import Path
import json,hashlib,re,datetime,subprocess
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'physics-recheck-r175'
m=load(H/'native-source-manifest.json');build=load(H/'native-build-result.json')
assert m['changed']==['tools/native-capture/cover_component_roles_live.inl'] and build['changed']==m['changed']
assert build['flags_unchanged'] and not build['external_worktree_dependencies'] and len(build['project_dependencies'])==413
for p,d in m['files'].items():assert sha(H/'native-source'/p)==d
for p in m['changed']:assert sha(R/p)==m['files'][p]
assert sha(H/'native-build/GearsCapture.dll')==build['dll_sha256']
checks=load(H/'checks-v5/results.json');related=load(H/'related/results.json')
assert len(checks)==2 and len(related)==2 and all(x['build_exit']==x['test_exit']==0 for x in checks+related)
for variant in ('before','after'):assert sha(H/'checks-v5'/variant/'tools/native-capture/cover_root_schema_test.cpp')==sha(R/'tools/native-capture/cover_root_schema_test.cpp')
counts={r['variant']:{k:int(v) for k,v in re.findall(r'root_schema (reads|long_reads)=(\d+)',r['output'])} for r in checks}
comparison={k:dict(before=counts['before'][k],after=v,reduction=1-v/counts['before'][k]) for k,v in counts['after'].items()}
assert all(v['after']<v['before'] for v in comparison.values())
summary=dict(native_sha256=build['dll_sha256'],product_changed=m['changed'],project_dependencies=413,flags_unchanged=True,
    comparison=comparison,checks=checks,related=related,game_started=False,source_age_repaired=False,cover_view_accepted=False,
    world_coverage_verified=False,movement_authorized=False,headset_accepted=False,public_changed=False,
    next='Prepare exact r177 private candidate, then one changed-candidate short controlled path check after resource admission. Verify original call and first actual refusal; full cover scope remains open.')
(H/'verification.json').write_text(json.dumps(summary,indent=2)+'\n')
patch=subprocess.run(['git','diff','--','tools/native-capture/cover_component_roles_live.inl'],cwd=R,capture_output=True,check=True).stdout
(H/'delta.patch').write_bytes(patch)
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r177' not in old
entry='''
## r177：联合扫描根目录五项属性，完整成员链收尾保持；已编译（2026-11-11）

从r176首次components9.7828ms和真实40ms拒绝继续，检查发现每个Root独立查找WorldInfo/Components/AllComponents/Base/Attached时反复遍历相同继承与成员链。现在只在cover_component_roles_live.inl新增root_schema，一次走每级Class目录并选出五项：保留32级父链/4096成员边界、类/extent/head/parent来源、最近声明者语义、同级重名拒绝，以及每个选中描述符的fresh身份、类型、dimension/width/offset/reference检查。所有访问过的完整成员身份与next进入同sample MemberStore，包括未选和末尾成员；另一Root仅在当前owner/head相同后共享完整目录，仍fresh认证选中字段。sample仍clear所有目录，repeat与最终current继续完整重读；不跨execute缓存，不改原始时间、40ms、guard、writer或移动许可。

真实生产组件reader原207项保持通过；前/后实际读取对照各308项通过，覆盖完整schema/共享目录、字段/链接/头/extent/父类/非选中名字变更、坏dimension/offset/kind/owner、环与末尾重复字段。基础schema加完整closing读取2078→949，减少54.33%；追加128项实际成员的长链15646→4917，减少68.57%；长链末尾改名形成late重复仍被最终成员复核拒绝。这是离线读次数，不是游戏耗时或完整碰撞通过。相关query105/path391项全部通过，依赖只来自各自冻结scratch或系统。

早期失败保留：v1缺少冻结测试include，编译未开始测试；v2测试中聚合Workspace清零形成大栈临时量，两版均stack overflow，改为测试专用heap reset后v3两版290项通过。v4新增长链fixture的名字表地址与原Class区间重叠，前/后同样失败；修正合成名字索引到不重叠区间后v5两版308项通过。未改产品去适应fixture，也未把失败算通过。

完整Native de07b0c1daf61f1aa16f5824c986bebe0f136f895bdea51320eae30aae01b8f1编译链接0；exact r175冻结树仅组件角色一文件delta，413项目依赖、worktree外漏0、GEARS flags不变，其他冻结源前后哈希一致。r175 physics复核/r173关联修复保持。原始脚本、失败、前后结果、manifest、build和verification保留在root-schema-r177；没有替换公开包或固定入口，尚未准备/启动此候选。

下一步保存后按资源准入准备精确候选，做一次有修改依据的短path观察，核对初始组件耗时、完整来源、原query started/returned及第一个实际失败；真实年龄修复、完整owner/world/body几何、严重自身模型遮挡、CV01–24/AC01–22和实体头显均未通过。工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=build['dll_sha256'],comparison=comparison)))
