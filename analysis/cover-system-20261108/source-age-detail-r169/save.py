"""Checkpoint copied source-age diagnostics without claiming a repair/run."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r169' not in old
entry='''
## r169：同query来源年龄细分观察已编译，未运行（2026-11-11）

r168仍在第八轮collider复核body年龄拒绝，但原因位没有细分tracking40ms、metadata50ms、capture基准或既有age_burned。因此本轮增加通用纯值观察头body_source_age_observation.h，在既有body_tick_source_live的classify调用后复制它已有的observed_now，在current成功/年龄返回点复制它已有的now/capture_begin。复制原始metadata begin/end、tracking begin/end、stereo/controller received、sync、输入序列、Tick/世界/Pawn/Scene/线程和实际verdict/latched。没有新取时、游戏读取、追踪获取、source续时、状态判定、writer或动作许可。原hard提前返回仍原样，缺少观察保持未知。

仅既有私有path one-shot登记成功后，在同一query作用域开启TLS观察；world/Pawn/Scene/线程须与scope匹配，compose关联该实际query。固定四条：首次classify/current及各自首次source_age_only，另有计数/溢出标志。首次与首次失败不被后续覆盖，退出恢复原TLS；不同线程无观察，新scope清空旧记录。四条值复制仍有观察开销，不声称零成本。

记录区分两次已有取时：classify的tracking/metadata阈值只按它自己的observed_now解释，不能用稍后的current.now倒推早先分类。current单独记录capture年龄和latched。首次记录附相对query begin偏移，不冒充零时刻入口；原始时间不改。解码器要求精确Native及同query key、同Tick/输入cohort、时间落在query区间和有效采集次序，缺项/未观察/溢出不当作新鲜或通过；硬拒绝不推断年龄子项。40/50ms阈值源文件在冻结delta中未改。

通用观察器26项、解释器15项通过，包含OFF/TLS外线程/键不匹配/首条与首失败保持/嵌套恢复/计数溢出、新scope清理、40ms等值和超1ns边界、缺项/错key/cohort/区间，以及current稍晚导致的数值超龄不得倒推原classify、latched单独保留。相关cause25/timing135/guard23/path391/OFF51通过；CPU fixture/Host不调用游戏原引擎。verify移除纯观察代码后逐字重建原body源码及C0控制流，原clock调用次数保持。

Native0de22cab918329d51091f4e640043ffb43ec0f17586a93affab763d333660e17编译链接0。exact r167冻结树仅新观察头、body_tick_source_live.inl和cover_physics_c0_live.inl三文件delta，411项目依赖、外部worktree0、GEARS flags保持。prepare新增--native-r169认证精确delta及原依赖/构建状态。源、检查、解释器、依赖、构建和核对在source-age-detail-r169。本轮收到HB单实例对照临时让行及提前解除通知；无本方游戏/编码、无待登记启动等待，未新增游戏，构建自然完成，不改全局锁或并发保护；不因解除通知额外启动测试。

下一步Git保存后用新实例取得实际年龄细分及同query分段，再选择来源到查询调度或读取成本修复；不原样重跑r168、不放宽年龄或刷新旧样本。本条仅源码/离线/编译，未准备或运行新实例，身体年龄/自身遮挡及完整CV01–24/AC01–22、实体头显仍开放，工程25%、publicv71/固定入口保持，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],build_only=True,game_started=False)))
