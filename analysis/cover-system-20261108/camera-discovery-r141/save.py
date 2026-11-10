"""Append this bounded source checkpoint to the existing mirrored development log."""
import datetime
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
guard=ROOT/'analysis/cover-system-20261108/requirements-owner-sync.json'
state=json.loads(guard.read_text(encoding='utf-8'))
canonical=ROOT/'docs/cover-system-development-20261108.md'
mirror=Path(state['development_copy']['path'])
before=canonical.read_bytes()
assert before==mirror.read_bytes()
assert hashlib.sha256(before).hexdigest()==state['development_copy']['sha256']
payload='''
## r141：激活前 CameraActor 发现路径收窄（2026-11-11，源码／离线）

- r140 的 3536.1894 ms 严格采集超时仍按原始失败保留：Native 未激活，r139 DLL 未执行；本次没有子步骤实测，不能认定 CameraActor 查找是唯一原因。
- `cinematic_anchor.py` 冷查找复用现有 Resolver 的批量类候选发现，避免逐个解码普通游戏实例名称；已有 CameraActor 缓存候选仍先处理，兼容无该能力的原精确查找。只改变发现提示，选中对象仍经过 fresh 注册表／名称／类证明、完整证书复核及时间戳括号内两次目标验证。
- 额外拒绝提示称 CameraActor、但实时名称或 number 已变的候选；发现提示不能更名当前类。原 200 ms 获取期限、身份、新鲜度、输入及恢复保护全部保持，不把发现移到期限外，也不续时。
- 41 项相关离线检查全部通过，包括真实生产类扫描器与合成反射内存接线、热缓存避免全扫描、假提示、注册表后变、扫描失败和冷发现超过原期限。独立 771 对象固定内存比较取得相同候选，旧逐对象查找 771 次读取、现有类提示扫描 7 次；只证明此合成读取量，不是实际延迟或 r140 因果修复。
- 配置组有两项既有失败：`test_guard_attempts_exhaustion_not_retried_by_sample_layer` 和 `test_sample_after_guard_exhaustion_keeps_actual_partial_sample`。修改前 HEAD 的 cinematic 源码与修改后分别复跑同组，均为相同两项失败（160 对 5、32 对 1）；不报告整组全绿，也不在此改动采样重试策略。
- 本次尚未启动游戏／XR、未更换任何 DLL 或公开 v71；r139 实际查询耗时、完整身体路径与占用、抓边移动、遮蔽／受击／枪口及头显仍待验证。工程进度 25% 保持。下次先绑定实际启动脚本哈希，再使用新私有实例定向验证启动与已有 path 查询；不复用已消费 r140。
- 证据：`analysis/cover-system-20261108/camera-discovery-r141/verification.json`、对应基线／修改后日志及 `verify.py`。本轮是源码与验证证据进展，完整 CV-01–24／AC-01–22 目标继续 active。
'''
after=before+payload.replace('\n','\r\n').encode('utf-8')
canonical.write_bytes(after);mirror.write_bytes(after)
state['development_copy']['sha256']=hashlib.sha256(after).hexdigest()
state['as_of']=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=8))).isoformat()
guard.write_text(json.dumps(state,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(state['development_copy']['sha256'])
