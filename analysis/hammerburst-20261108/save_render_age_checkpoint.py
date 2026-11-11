from pathlib import Path
import hashlib,json,re
A=Path(__file__).resolve().parent;R=A.parents[1]
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
compile((A/'run_isolated_empty.py').read_text(),str(A/'run_isolated_empty.py'),'exec')
native=json.loads((A/'native-render-age-r1/prepared.json').read_text())
pg=json.loads((A/'render-age-pg-r1/prepared.json').read_text())
for proof,key in ((native,'native_sha256'),(pg,'dll_sha256')):
 assert hashlib.sha256(Path(proof['dll']).read_bytes()).hexdigest()==proof[key]
classification={}
for pid in (58164,22812,50828):
 p=A/f'runtime-{pid}-text/capture-status.txt'
 if not p.exists():
  matches=list((A/f'runtime-{pid}-text').glob('*capture*'));assert len(matches)==1,matches;p=matches[0]
 raw=p.read_bytes();s=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig',errors='replace')
 classification[str(pid)]={k:int(v) for k,v in re.findall(r'stereo_rejection reason=(\w+) count=(\d+)',s)}
 assert classification[str(pid)],p
write(A/'render-age-pair-checkpoint.json',dict(native=native['native_sha256'],playground=pg['dll_sha256'],policy_cases=17,adapter_checks=457,prior_native_rejection_counts=classification,runtime_exercised=False,whole_weapon_accepted=False,scope='Private explicit --render-age-continuity only. Reject stale render/pose; keep logical state only when original Gate continuity and clock prove age-only. Existing real-UP debt WAIT retains original250ms and fresh20ms dispatch guards. True loss still cancels. No public/manual change.'))
p=A/'full-requirements-audit-20261110.json';v=json.loads(p.read_text());assert len(v['requirements'])==13
v['current_candidate']='Private render-age pair: Native71f280b0 +PGb7dcb9ec; compiled17 original Gate cases and457 actual-adapter checks; not game tested.'
v['render_age_pair_checkpoint']='analysis/hammerburst-20261108/render-age-pair-checkpoint.json'
v['requirements_complete']=False;write(p,v)
p=A/'worker-052600-exit-approval.json';v=json.loads(p.read_text());v['next']='Already executed under recorded owner resource release; explicit subsequent reply recorded. No second operation on historical PID.';v['latest_user_message_is_goal_continuation_only']=False;write(p,v)
p=R/'docs/hammerburst-development-20261108.md'
with p.open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 私有画面时效连续性候选（实测前保存）\n\n58164原Native记录47次stale，其他拒绝类别0；原Gate对同参考系的纯年龄过期保留连续性，外层却无条件撤销手状态。私有Native71f280b0仅在原Gate前后同稳定epoch、无pending、focused且原接收时间确已超过100ms时保留逻辑等待；原画面拒绝与真实失跟踪/身份/时钟错误撤销仍保留。配合PGb7dcb9ec将已验证的真实UP后WAIT接回原回调，原250ms事件截止、100ms来源和20ms派发余量不变。17项原Gate检查与457项实际适配器检查通过，完整Native/PG编译通过；尚无此组合的游戏证据，不宣称修复成功。公共v71与手动入口不变，全部13项要求继续保留。\n')
print(json.dumps(classification))
