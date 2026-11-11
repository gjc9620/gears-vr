from pathlib import Path
import json,re
A=Path(__file__).resolve().parent
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
p=A/'runtime-6896-first-failure-analysis.json';v=read(p);raw=(A/'runtime-6896-text/playground-6896.log').read_bytes();s=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
records=[dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith('native_lancer_r2 ')]
gap=next(r for r in records if r.get('copied_frame')=='377' and r.get('source_status')=='0');assert gap['copied_input_age_ns']=='101866700' and gap['continuity_failure']=='10'
v['post_up_actual_source_gap']=gap
v['input_transition_findings']='OriginalR2Status1 is BASELINE (2 accepted,3 pressed,8 blocked). After freshUP375 and anotherUP376, actual copiedFrame377 expired101.8667ms, source_status0/continuity_failure10, sealing Input. Next fresh frame383 alreadyDOWN is correctly consumed as BASELINE1, then384blocked8 until a real release. Strict anti-replay works as designed; do not replay this press or claim Input state bug. Next investigate source freshness/cost across original Scene publication and PlayerTick consumer.'
write(p,v)
p=A/'full-requirements-audit-20261110.json';a=read(p);a['latest_input_transition']=dict(evidence='runtime-6896-first-failure-analysis.json',stale_frame=377,age_ns=101866700,baseline_frame=383,blocked_frame=384,anti_replay_preserved=True);write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a['next']='Read original Scene publication/PlayerTick source freshness and cost for actualFrame377101.8667ms gap afterUP. Input correctly baselines nextDOWN383 and blocks384; no replay or guard relaxation, no unchanged run. All13 retained.';write(p,a)
p=A.parents[1]/'docs/hammerburst-development-20261108.md'
with p.open('a',encoding='utf-8') as f:f.write('\n\n6896后续精确转移已查明：OriginalR2Status1是baseline，非quiet；UP375/376后，实际copiedFrame377在PG读取时101.8667ms过期（source_status0/continuity_failure10），Input据此seal。下次新鲜Frame383已经DOWN，按原防重放只作baseline1，384blocked8直到真UP。这是预期安全转移，不再推测Input状态机错误；下一步只查原Scene发布至PlayerTick消费的时效/耗时，不补按、不放宽拒绝、不原样重跑。\n')
print(v['input_transition_findings'])
