"""Save a built diagnostic candidate; leave full cover acceptance open."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['owner_links_repaired']
p=H.parent/'isolated-r105/prepare.py';text=p.read_text(encoding='utf-8-sig').rstrip()+'\n'
compile(text,str(p),'exec');assert "parser.add_argument('--native-r171'" in text
p.write_text(text,encoding='utf-8')
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r171' not in old
entry='''
## r171：owner_links确切读取器和已读关联观察已编译，未实跑（2026-11-11）

r170排除本次年龄拒绝后，下一实际阻点是原版调用前owner_links，旧状态没有具体读取器。因此新增cover_owner_link_observation.h，在asset、collider、components已有六个links返回点复制既有Actor/Table/预期Scene/实际Scene/Internal/已读Reverse与线程；记录scene_or_internal或reverse分支。reverse尚未读取时明确reverse_read=false而非宣称关联为零。只有既有私有path one-shot同步作用域开启观察，绑定world/Pawn/Scene/Compose/thread，查询原有owner组分别限定before/after dispatch阶段。TLS只保留首条失败，阶段及嵌套退出恢复；未观察保持unknown。无新时钟、游戏读取、原版调用、筛选、grant、source续时或第二身体writer。

定向89项检查实际调用三种生产reader，覆盖场景、内部指针、反向关联三类失败，共九条实际分支，启用前后读取次数和返回拒绝一致；另含OFF、无阶段、无效键、跨线程、嵌套、首失败保持及新scope清旧。相关asset90/collider92/components108/path391/OFF51通过；解释器22项通过，精确Native/同query key沿用现有reader，拒绝矛盾分支和伪造反向读取，不把缺失当成功。首次测试遗漏bcrypt链接库、随后新fixture Body块只有0x90却写0x90字段导致异常，两次失败保留；修正为0x98后定向89项0，不涉及产品检查放宽。

verify移除纯观察代码后逐字重建r169的三个reader、QueryEngine和C0五份原文件，原判定/调用/读取/关闭次序不变。Native a2b53166f37fbed0329d9019c8e97a21962499e3e0786b69872dc52c901c8b95完整编译链接0；exact r169冻结源六文件delta，412项目依赖、外部worktree0、GEARS flags保持。检查、解释器、构建、依赖及delta保存在owner-link-detail-r171；prepare新增--native-r171，仍认证精确delta及原冻结构建约束。

本条只有源码/离线/编译，尚未准备或启动新游戏。下一步Git保存后在新实例用现有受控path查询取得第一条实际失败关联，按真实分支决定修复；不原样重跑旧年龄候选、不跳过异场景Actor、不弱化关联或释放保护。owner_links、严重自身模型遮挡、CV01–24/AC01–22和实体头显仍开放，工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],build_only=True,game_started=False)))
