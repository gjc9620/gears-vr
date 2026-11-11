"""Save bounded root-proof reuse without claiming game or movement acceptance."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r165' not in old
entry='''
## r165：组件目录第二遍Actor根结构认证去重，已检查/编译、尚未运行（2026-11-11）

依据r164同query initial components15.5895ms最大单段成本，本轮只修改cover_component_roles_live.inl中第二遍root选择：先新读Actor身份，要求与第一遍同index根节点完全相同、当前metadata字节/代次仍同，再用binding::current完整重读该root第一遍全部Proof identity/source，之后才消费缓存的五项属性偏移与数组结构。初次schema/class/五属性/inner认证保持；第二遍所有world/base/list data/count/capacity/原数组成员及递归节点仍真实读取，base的动态类型关系仍认证。新节点或顺序改变不能使用别的root字段；每次sample原clear不变，不跨sample/帧/query复用。最终members与全部root/component/asset/body Proof关闭、首尾guard及原输出不变条件保持，不改任何年龄/输入/碰撞/移动门。

actual冻结r163与新单文件覆盖通过同一focused346检查，含原component207，不称346全新项；覆盖两阶段offset、kind、reference、父类、成员Next、inner宽度、metadata和world变更，以及附着图增长与拒绝时输出保持。相同真实sample内safe_copy读取：2根24772→21520（13.13%）、8根40488→32070（20.79%）、16根61440→46134（24.91%）。仅合成fixture的整次component sample计数，不是原query毫秒、全链收益或来源年龄修复。

初checks-v1/v2/v3新增多根fixture在before/after均失败：名字池index704落到arena+0x4c000覆盖既有Body；另source_field每次实际双读，nth2仍在第一遍，不能作为第二遍变更。保存这些初失败和定位输出；v4仅将新增名字索引改到独立1000+范围、第二阶段触发改occurrence3，并移除定位输出，运行delta不变。v4 before/after346全0，原v1其余component207/guard23/query105/path391/overlap53/OFF51全0；所有运行头均来自各自冻结树且实际依赖外部worktree0。CPU fixture/Host dispatch不执行原引擎或游戏。

Native3f1d5454286e3991917f462ebab5749f614db22003d31a3810fbd985560be64d编译链接0；exact r163冻结树仅一运行文件delta，410项目依赖、GEARS flags保持。verify逐字重建去除新分支后的原控制流，确认动态root读取、完整关闭和clear原文不变；prepare新增--native-r165并认证精确单文件delta/依赖/flags/构建状态。源码/初失败/最终检查/读取对照/依赖与构建记录在component-root-repeat-r165。

下一步保存后使用新私有实例验证原CT2及同query分段，检查initial components实际成本与拒绝点是否改变，并核对画面；不原样重跑r164，不放宽时限或刷新旧样本。本条仅源码/离线/编译，尚未准备/启动游戏，不替换publicv71或固定入口。完整CV01–24/AC01–22仍开放，工程25%、身体来源年龄/自身遮挡/实体头显验收未解决，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],build_only=True,game_started=False)))
