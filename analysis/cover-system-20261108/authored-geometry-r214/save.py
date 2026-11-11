from pathlib import Path
import json,hashlib,datetime
root=Path.cwd();folder=root/'analysis/cover-system-20261108/authored-geometry-r214'
result=json.loads((folder/'final-result.json').read_text());regression=json.loads((folder/'array-regression-result.json').read_text())
assert result['build_exit']==result['test_exit']==regression['build_exit']==regression['test_exit']==0
fixture_dir=folder.parent/'61492-cover-frame';expected=json.loads((folder.parent/'c0-target-r99/verification.json').read_text())['fixtures']
actual={p:hashlib.sha256((fixture_dir/p).read_bytes()).hexdigest() for p in expected};assert actual==expected
sources=['tools/native-capture/cover_authored_geometry_live.inl','tools/native-capture/cover_authored_geometry_live_test.cpp',
 'tools/native-capture/cover_authored_inventory_live.inl','tools/native-capture/cover_authored_inventory_live_test.cpp','tools/native-capture/cover_authored_array.h']
verification=dict(result=result['output'],array_regression=regression['output'],
 sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources},fixture_sha256=actual,
 association='Owned inventory joins current typed mesh/asset/instance/root and exact FName index+number to retained complete Marcus115 reference/C0 frame. Rebinds physics bones, shared decode of every raw80 row verifies cached TM/dimensions/kind/ordinal/scale/mask-derived flag/word, then full body_geometry conversion. No reads/calls/permission.',
 codec='Extracted original reader decoder into authored_array::decode and shared valid_source predicate; same codec now validates owned geometry input. Inventory stores certified per-body/kind masks and compares them across complete passes.',
 flags='All rows retained, bNoRBCollision/word copied separately into geometry result, no implicit eligibility decision or row removal. Consistent flag change leaves all7 world shapes identical.',
 tests='377 checks0 includes328. Actual saved reference/local/component frame plus SYNTHETIC2-body/7-row native-memory inventory. Independent quaternion/vector FK validates actual world poses, radius12/box6,9,12/capsule half9, localZ endpoints and root mapping11/8. Raw/cache/ordinal/mask/flags/count/scale/source tampering rejected with old output intact; missing/duplicate FName mapping, nonunit atom, alias, unsupported convex/empty result rejected. Existing query catalog consumes all7 with explicit0.002 game-unit rounding budget; not a Native query or internal-OBB proof.',
 first_failure='Reused oracle Fixture default component belonged to an older frame; initial thirteen pose/axis/end-point checks failed. Derive only independent oracle component pose from THIS retained planar matrix, leaving input bytes untouched. Another negative test wrote zero into an already-zero raw byte; changed to an actual XOR mutation. Original initial failure retained, not a game/converter repair claim.',
 remaining='Caller must close this same current C0/roles/inventory/code bracket; active PhysX shape mapping, query eligibility semantics, original queries/full world/path/occupancy, production capture integration, view/body/hit/muzzle transaction and headset acceptance still open.',
 game_started=False,full_native_built=False,engineering_percent=25)
(folder/'verification.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
guard_path=root/'analysis/cover-system-20261108/requirements-owner-sync.json';guard=json.loads(guard_path.read_text(encoding='utf-8'))
doc=root/'docs/cover-system-development-20261108.md';mirror=Path(guard['development_copy']['path']);old=doc.read_bytes()
assert old==mirror.read_bytes() and hashlib.sha256(old).hexdigest()==guard['development_copy']['sha256']
addition='''
## r214：完整authored库存→真实C0骨映射/世界几何/查询参数连通（2026-11-12）

新增cover_authored_geometry_live.inl纯owned-buffer join，对typed mesh/asset/instance/root与库存对应后，以完整Marcus115 reference和当前C0 Frame重新绑定每个FName index+number。抽出既有authored_array::decode/valid_source供reader和join共同使用，逐条raw80重解码与缓存TM/dimension/kind/ordinal/scale/Bool word/flag精确核对，不信任已改写的缓存字段。库存新增每body/kind已认证Bool mask并纳入整轮比较。通过后复用已有完整body_geometry转换；bNoRBCollision和原word单独附着每行，不隐式决定查询参与或删除形状。任何失败公共输出保持。

377检查0失败（含原328），独立数组23回归0。以原9份SHA完全相同的61492-cover-frame真实reference/C0-local/component为底座，配synthetic2-body/7条原布局内存形状，完整capture后直接进入新join；独立四元数/向量FK核对世界位姿、球半径12、盒半轴6/9/12、胶囊半段9及局部Z端点，验证骨映射11/8与root8。错raw/缓存/flag/count/mask/scale/source拒绝并保留输出；缺失和重复完整FName、非unit atom、alias/convex/empty拒绝。全部7行也经既有catalog.prepare成功转换，明确使用0.002原游戏单位的舍入预算，未执行Native查询或认证其内部OBB解释。

初次复用oracle Fixture默认component来自更旧帧，导致13个姿态/轴/端点检查失败；改由本次真实保存的平面component矩阵构造独立oracle pose，仅修改期望值，不改输入Frame任何字节。另一个负例原写0到已为0的raw字节并未制造变更，改为XOR后实际拒绝。初失败保留，不能称游戏/几何核修复。最终同时核对C0输入字节不变；一致改变bNoRBCollision后全部7行几何不变、flag随之变化，不会静默漏体积。

仍需caller同current C0/roles/库存/code bracket闭合、active PhysX对应、查询参与语义、真实完整world/path/occupancy及body/view/hit/muzzle同版事务；完整capture生产接入、视图遮挡及头显尚未完成。源码/依赖/原9夹具SHA/回执authored-geometry-r214，无游戏/原函数/完整Native构建，CV24/AC22全部保留，工程25%。
'''
data=old+addition.replace('\n','\r\n').encode('utf-8');doc.write_bytes(data);mirror.write_bytes(data)
guard['development_copy']['sha256']=hashlib.sha256(data).hexdigest();guard['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard_path.write_text(json.dumps(guard,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'checks':377,'array_regression_checks':23,'failures':0,'fixture_files_unchanged':len(actual),'documents_equal':doc.read_bytes()==mirror.read_bytes()}))
