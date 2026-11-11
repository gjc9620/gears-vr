from pathlib import Path
import ast,datetime,hashlib,json,subprocess
H=Path(__file__).resolve().parent;R=H.parents[2];load=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checks=load(H/'results.json');comparison=load(H/'comparison.json');assert len(checks)==2 and all(x['test_exit']==x['probe_exit']==0 for x in checks)
assert sha(R/'tools/game-playtest/cover_state.py')==checks[1]['source_sha256']
for name in ('prepare_operator.py','run.py'):ast.parse((H.parent/'isolated-r105'/name).read_text(encoding='utf-8-sig'))
summary=dict(checks=checks,comparison=comparison,reader_sources={name:sha(R/name) for name in ('tools/game-playtest/cover_state.py','tools/game-playtest/world_state.py')},strict_registry_barriers=2,append_only_allowed=False,cache_lifetime='one closing call only',witnesses_dropped=False,native_rebuilt=False,native_sha256='0d4437fd5427902f84e5571cb1e3f577c201b114bfaf2843b3e4483f998cdf6f',game_started=False,registry_race_game_repaired=False,source_age_repaired=False,public_changed=False)
(H/'verification.json').write_text(json.dumps(summary,indent=2)+'\n');(H/'delta.patch').write_bytes(subprocess.run(['git','diff','--','tools/game-playtest/cover_state.py','analysis/cover-system-20261108/isolated-r105/prepare_operator.py','analysis/cover-system-20261108/isolated-r105/run.py'],cwd=R,capture_output=True,check=True).stdout)
gp=H.parent/'requirements-owner-sync.json';g=load(gp);doc=R/'docs/cover-system-development-20261108.md';mirror=Path(g['development_copy']['path']);old=doc.read_bytes();assert old==mirror.read_bytes() and sha(doc)==g['development_copy']['sha256'] and b'## r185' not in old
entry='''
## r185：候选CoverQuery收尾复用本次新读完整窗口，严格目录双收尾保持（2026-11-11）

针对r184已知registry count增一的实际拒绝，不放宽append-only、不跳过候选、不增加重试或等待。发现CoverQuery.values同时保留完整Slots raw块和内部每个字段的首次witness；finish原来先重读整个raw，又逐槽逐字段重复读取已包含的字节。现在在同一次finish内按地址/长度排序，新读原先已记录的完整窗口，所有被包含的首次witness逐项对照该新读字节；不包含或只部分重叠的窗口仍分别完整重读。没有扩展读取范围、丢字段或跨finish复用。两个原super.finish仍在全部values前后执行，identity/metadata/registry逐项严格相同，3秒/读取/数组容量限制不变，目录增长仍拒绝。

实际现有cover_state.py reader原14项检查0、新18项检查0；新增覆盖每字节变动、内部首次witness不一致、读取失败、部分重叠、第二次finish不复用缓存。既有目录增一、mask在逐槽期间变化、identity/结构指针和第二收尾中变化的反例仍拒绝。真实slot_snapshot前后对照3/16/32槽，capture读取49/101/165保持、全部46/137/249首次witness数保持、每项解码rows SHA相同。收尾读取88→67（-23.86%）、179→67（-62.57%）、291→67（-76.98%）。这是合成内存的实际reader读取次数，不是游戏时间，也不是registry race已修复。

prepare_operator新增cover_state/world_state两份实际reader来源SHA；私有runner在claim前与正常清理后核对并记录，来源改变不能记通过。原13项启动来源、共享隔离/资源门槛和输入保护不变，两个来源另列，不伪装历史案例已有该回执。生成器/runner AST核对通过。仅Python观察器变化，Native/PG/XR不重编，继续精确r183 Native0d4437fd…cdf6f。原始前后来源/测试/计数/哈希/delta与verify在closing-windows-r185。

下一步Git保存后，新实例绑定这两份reader哈希，短查候选是否通过原严格收尾、r183 query来源/耗时/原调用开始返回。若目录变化仍在原窗口内发生，继续保留实际拒绝，不选择性重跑。r183实际40ms、world/body/movement、严重自身模型遮挡、CV01–24/AC01–22及实体头显仍未完成；工程25%、publicv71/固定入口不变，目标active。
'''
new=old+entry.replace('\n','\r\n').encode('utf-8');doc.write_bytes(new);mirror.write_bytes(new);g['development_copy']['sha256']=sha(doc);g['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat();gp.write_text(json.dumps(g,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print(json.dumps(dict(saved=True,comparison=comparison,reader_sources=summary['reader_sources'])))
