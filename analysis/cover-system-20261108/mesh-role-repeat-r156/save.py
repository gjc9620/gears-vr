"""Save source/build/CPU scope before any new game run."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gpath=H.parent/'requirements-owner-sync.json';g=load(gpath)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r156：逐布局/逐遍mesh关系认证去重，源码/离线/编译完成、未运行（2026-11-11）

依据r155同query body年龄拒绝/equipment未过期，检查实际physics复核链发现values在每个BodyInstance上重复认证同一mesh class及OwnerComponent声明关系。只在现有BodyLayout保存成功键(mesh_class、reference、pass)，同布局同一遍后续部件复用；pass2重新认证，每次sample原有layout清零仍执行，不跨sample/查询/帧缓存。所有部件实际identity/index/owner/main/auxiliary仍两遍读取，完整layouts_current的metadata/proof/所有成员关闭检查、owner guard及全部年龄门原文未改。不刷新来源或减少身体部件，也没有第二写入端。

actual冻结r149基线与新单文件delta两边编译真实typed reader。2部件safe_copy读取7224→7076（2.05%），16部件9800→7580（22.65%），64部件18632→9308（50.04%）。这是该合成reader整次sample的调用量，不是实际游戏毫秒、全query成本、原始碰撞调用或年龄门修复证明；实际BodyInstance数量与全query收益仍待测。

checks-v2相关consumer/guard/path/overlap/OFF各程序均0，原physics282项保持。补充checks-v3聚焦两遍中途变更：在pass1和pass2首个部件认证后分别改变声明reference、mesh父类、mesh实例类、非root owner、成员Next和metadata代次，均拒绝并保留原输出；同时核对pass/class/reference键改变、pass0不缓存及关闭证明拒绝。新focused执行388检查含原282，不能相加当作全新覆盖。checks-v2的8个其他相关程序分别282/219/207/23/105/391/53/51通过，不重复跑未改consumer；CPU fixture不证明真实引擎调用。初次checks-v1因冻结运行源码不含CPU测试翻译单元而编译失败，失败保留；仅补入测试cpp到独立检查树，运行头仍冻结，无产品门更改。

Native ababb095c3fd5a24ff37a368d99a895cecf4d4dc06e07ddfd769dd670a39f2d8 编译链接0。exact r149冻结树仅cover_physics_roles_live.inl一处运行文件delta，409项目依赖均来自本冻结树、外部worktree0，GEARS flags不变。verify确认原sample事务/全部dynamic读取（除该重复class调用）/layouts_current原文保持，既有r155证据在新SHA下拒绝，inspect_cause新增显式expected_native支持，默认仍严格r149。prepare支持--native-r156并认证单文件delta、未staged/flags/依赖；尚未准备/运行新实例。

源、比较、初失败、依赖、构建manifest和核对在mesh-role-repeat-r156。下一步Git保存后，以新私有实例一次定向检查原CT2入口、实际query层次/耗时及画面；不原样重跑r155，不放宽source_age或其他守卫。完整CV01–24/AC01–22仍开放，工程25%、publicv71/固定入口和实体头显状态不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gpath.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,build_only=True,native=v['dll_sha256'],game_started=False)))
