"""Checkpoint the built timing candidate without claiming runtime evidence."""
from pathlib import Path
import datetime,hashlib,json
H=Path(__file__).resolve().parent;R=H.parents[2]
load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
v=load(H/'verification.json');assert v['build_only'] and not v['game_started'] and not v['source_age_repaired']
gp=H.parent/'requirements-owner-sync.json';g=load(gp)
doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path'])
old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256']
entry='''
## r163：原查询前缀分段计时已编译，尚未游戏运行（2026-11-11）

r162仍在engine/source_begin的physics_read首复核遇到body年龄拒绝。当前读回无法区分initial physics/collider/component与后续重复复核各自成本，故本轮增加有界、仅既有私有path one-shot开启的分段观察，不删守卫、读取、重复采样或来源关闭证明，不放宽时限，也不改身体/相机/输入/移动。生产Session在8个阶段记录calls、closed spans、total/max ns；clock invalid与累加overflow明确记录，未启用Scope时不读时钟。engine包含内部复核，时间嵌套重叠，不可相加；关闭span不代表操作成功，计时本身开销包含在结果内。仅在原one-shot登记成功后开Scope，跟随当前path键记录，不跨查询使用旧结果。

新增计时focused135项包含原Session105，不称135新项。覆盖未启用不读钟/原操作仍执行、返回引用、嵌套Scope恢复、异常原样传播并闭合记录、倒退时钟、累加溢出、实际Session成功与guard拒绝时输出不变。原guard23/query105/path391/overlap53/OFF51均通过；Host替换模块/dispatch，未执行原引擎或游戏。首次编译因测试局部名inner/denied遮蔽已有fixture全局且/WX而失败，原失败保留；仅重命名两处测试变量后focused通过。解码器11项检查拒绝不完整/负数/缺项/溢出/超出query区间，不将缺项当0或通过。

Native50ced042f8d7f9a55fb8c4f3e859fbb6ea75872889d7e1dbb27c754cdad65f79编译链接0。exact r158冻结树仅cover_physics_query_live.inl、cover_physics_c0_live.inl和新cover_query_timing.h三文件delta，410项目依赖、外部worktree0、GEARS flags保持。verify删除计时包装后重建原Session及C0文本逐字相同，实际guard顺序/条件/读值/返回表达式保持。prepare新增--native-r163并认证精确delta及原依赖/flags/构建状态；未准备、启动或运行新实例，不改publicv71。

下一步使用新实例做一次原CT2/同query分段观察，并保留来源年龄及画面实际失败。取得各段时间后再选取保持全部验证的修复；此候选只提供诊断能力，不声称性能提升、年龄修复、碰撞可用或玩法完成。全部CV01–24/AC01–22仍开放，工程25%、固定入口和实体头显状态不变，目标active。源码/测试/构建与文本核对在query-timing-r163。
'''
assert b'## r163' not in old
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new)
g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(saved=True,native=v['native_sha256'],build_only=True,game_started=False)))
