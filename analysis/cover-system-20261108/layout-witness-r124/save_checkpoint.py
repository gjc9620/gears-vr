from pathlib import Path
import datetime,hashlib,json,re
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
receipt=load(H/'native-build-result.json')
assert receipt['flags_unchanged'] and not receipt['external_worktree_dependencies']
assert sha(H/'native-build/GearsCapture.dll')==receipt['dll_sha256']
for rel in receipt['changed']:assert sha(R/rel)==sha(H/'native-source'/rel)
tests=load(H/'checks-v2/results.json')
tests=[x for x in tests if x['name']!='cover_physics_query_live']+load(H/'checks-v3/results.json')
assert len(tests)==5 and all(x['build_exit']==0 and x['test_exit']==0 for x in tests)
for row in tests:assert sha(R/'tools/native-capture'/(row['name']+'_test.cpp'))==row['source_sha256']
source=(R/'tools/native-capture/cover_physics_roles_live.inl').read_text()
names=re.search(r'enum class LayoutStep:unsigned \{(.*?)\};',source,re.S).group(1)
steps={str(n):name.strip() for n,name in enumerate(names.split(',')) if name.strip()}
(H/'layout-steps.json').write_text(json.dumps(steps,indent=2)+'\n')
record=dict(native_sha256=receipt['dll_sha256'],project_dependencies=len(receipt['project_dependencies']),
    overlays=receipt['changed'],tests=tests,checks={'roles':282,'session':105,'overlap_session':53,'path':191,'off':38},
    diagnostic_only=True,admission_predicates_preserved=True,additional_game_memory_reads=False,
    original_query_executed=False,game_started=False,movement_authorized=False,gameplay_accepted=False,
    initial_failures=['v1 generic BodyLayouts caller had no Workspace witness; optional argument preserves shared helper',
                      'v1 check driver used nonexistent cover_physics_c0_disabled name; corrected to existing cover_physics_c0_off'],
    preserved_failure_build='native-build-failed-v1/build.log',
    remaining=['actual layout substage','original query execution','endpoint/world coverage/body commit','head shoulder obstruction','CV01-24 AC01-22 headset'])
(H/'verification.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
guard_path=H.parent/'requirements-owner-sync.json';guard=load(guard_path)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path'])
assert doc.read_bytes()==mirror.read_bytes() and sha(doc)==guard['development_copy']['sha256']
entry='''
## r124：typed布局拒绝的有界字段见证，源码与私有编译（2026-11-10）

r123实际尚未进入原版查询，当前继续定位，而非改宽准入。现有PhysicsRoles读取新增LayoutStep，按原顺序标记mesh/asset/instance类、每项属性及固定offset、数组inner、Owner引用、数组边界及Body成员布局；同时记录首次/复核pass、body序号、既有proof计数和已读取的属性offset。pass0不输出上次proof值。所有信息来自现有读取和工作区，不加额外游戏内存读取、原版调用、hook、writer或逐帧诊断；最后尝试的步骤不是单独的通过/安全许可，必须结合原Code和Session阶段解释。

Session同一串行owner导出见证，现有一次Sphere观察通过原summary保存。新查询开始清诊断状态；失败仍不发布调用方结果。body_layout同时供组件读取复用，改为可选见证参数，其他调用者保持原接口与判断。OFF路径无读取/guard/原版调用。

最终MSVC W4/WX/O2/UNDEBUG：PhysicsRoles282、Session105、OverlapSession53、CapsulePath191、C0 OFF38检查均exit0。新检查包含属性offset、inner类型宽度、Owner引用、body布局/重复、完整兄弟成员、失败输出不变、pass轮次、早期guard及Session拒绝前不dispatch；Host调用缝不当真实原版查询。首轮泛型Work编译C2664和driver误写不存在的c0_disabled测试名均保留，修为可选诊断参数及现有c0_off后通过；初次Native失败树另存failed-v1。

新Native严格以r122冻结树叠加3文件，405个项目依赖、外部工作树依赖0、开关不变，编译链接exit0。SHA256 f5d3cf695c33bb78bde9a3f1d99d1d9546043f549534b99e8eb20bb2f0e39850；未替换公共v71或固定手动入口。本记录时尚未启动新game/XR，实际具体字段拒绝、原版调用、完整占用/世界覆盖、唯一身体C0提交、头肩遮挡和物理头显仍缺证，工程25%与CV01–24/AC01–22未验收保持。下一步仅使用新候选执行一次CLOAK/静音/私有投影的定向验证。

证据 analysis/cover-system-20261108/layout-witness-r124/{verification,layout-steps,native-build-result}.json 及checks-v2/v3；源码先保存后新运行。
'''
assert '## r124：' not in doc.read_text()
payload=doc.read_bytes()+entry.replace('\n','\r\n').encode('utf8');doc.write_bytes(payload);mirror.write_bytes(payload)
guard['development_copy']['sha256']=sha(doc);guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(dict(saved=True,native=receipt['dll_sha256'],checks=669,game_started=False)))
