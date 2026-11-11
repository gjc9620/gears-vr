from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1]
I=R/'analysis/instances/hb-cloak-1111-152629';D=I/'actions/menu-firstperson-29012-152706';T=A/'runtime-29012-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
p=A/'runtime-29012-first-failure-analysis.json';v=read(p)
assert v['actual_trajectory_count']==48 and v['trajectory_consistent'] and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-empty-reload.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json')
raw=(T/'playground-29012.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
for prefix in ('hammerburst_input_consumed','hammerburst_service_loss','hammerburst_history_rejection','hammerburst_tick_cost_summary','hammerburst_first_tick_summary'):
 v[prefix+'_rows']=rows(prefix,pg)
inputs=v['hammerburst_input_consumed_rows'];selected=[next(r for r in inputs if r['frame']==str(f)) for f in (339,341,345)]
assert [(r['trigger_pressed'],r['trigger_released'],r['status'],r['owned']) for r in selected]==[('5','5','1','0'),('6','5','10','0'),('6','5','2','1')]
assert not v['hammerburst_history_rejection_rows'] and v['hammerburst_first_tick_summary_rows'][0]['seen']=='0'
v['receipt_reader_failure']=dict(baseline=selected[0],deferred=selected[1],same_press_owned=selected[2],cause='Reader required transient PRESSED3, absent after deferred10 due existing counter-change/250ms log throttle. Periodic ACCEPTED2 owned1 matched exact new press6/release5 and was ignored. Longer held input produced extra actual rays;48 is not a finite-six pass.',repair='Accept fresh status2 or3 owned1 only with sequence newer than UP baseline and exact baseline press+1/release unchanged; status8 remains failure. No retry or production changes.',focused_tests=16)
v['interpretation']='Harness receipt overrestriction; actual same new press became owned and fired, but reader ignored ACCEPTED2 and timed out.48 consistent rays include prolonged-held extras; full finite/empty reload failed before reload/history observation. No history-cause or first-debt-cost claim.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 overview pages personally inspected; oversized near-eye gun, separated/ghost hands and white geometry remain. Full contact and headset unaccepted.')
write(p,v)
p=A/'runtime-29012-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-29012-first-failure-analysis.json',latest_runtime_result='runtime-29012-first-failure-analysis.json',requirements_complete=False,current_candidate='PGaf4dac2b/Native71f280b0;29012 failed reader after same new press owned2 logged;48 rays include prolonged held extras, no finite-six acceptance. Exact counter-cohort reader repair passed16 tests; production unchanged. History and first-debt evidence not reached. All13 retained.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=29012,runner_last_poll='32046 terminal1; stop/launcher/Native/PG/XR0; owned game+children gone; noforced;encode0',next='One targeted full-flow check after exact counter-cohort receipt reader repair, unchanged production candidate.');write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际29012：测试读取漏认同次合法持有\n\nPGaf4dac2b/Native71f280b0。339帧UP基线p5/r5；341帧新DOWN p6/r5 deferred10未持有；345帧同次p6/r5 accepted2持有且实际开火。原日志仅计数变化或250ms记录，中间pressed3可以不输出。测试只认3漏认已持有2，等待超时导致按住更久；48条一致射线包含额外开火，不能算有限六发通过。修正仅测试读取：新于UP序号、精确press+1/release不变，2或3且owned1；8仍失败，不重按。16项针对检查通过，生产保护未变。新历史观察未到达，first成本seen0/tail225保留128。游戏与Native/PG/XR正常清理0，无forced；206帧、错误空、编码0、3页48对亲阅，近眼过大枪/手分离重影/白几何仍失败。13项完整目标保留。\n')
print(json.dumps(dict(actual_rays=48,finite_pass=False,focused_tests=16,history_records=0,terminal=True)))
