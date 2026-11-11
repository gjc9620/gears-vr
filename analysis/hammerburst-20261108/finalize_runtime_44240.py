from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-162406';D=I/'actions/menu-firstperson-44240-162556';T=A/'runtime-44240-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in s.splitlines() if l.startswith(prefix+' ')]
def decoded(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
p=A/'runtime-44240-first-failure-analysis.json';v=read(p);assert v['actual_trajectory_count']==0 and not v['cleanup']['forced_termination']
for source in (I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
pg=decoded(T/'playground-44240.log')
for prefix in ('native_equipment','native_equipment_rig','native_equipment_main_mount','native_equipment_input_gap','hammerburst_pickup_up','hammerburst_pre_original_refire','hammerburst_service_loss'):
 v[prefix+'_rows']=rows(prefix,pg)
equipment=v['native_equipment_rows'];selected=[next(r for r in equipment if r.get('frame')==str(f)) for f in (158,159,161,163)]
assert [r['status'] for r in selected]==['unholster_pending','unavailable','held','baseline']
assert selected[2]['event']=='1' and selected[2]['press']=='1' and selected[2]['input_status']=='baseline' and selected[3]['target']=='0'
assert not v['hammerburst_pre_original_refire_rows'] and not v['hammerburst_service_loss_rows']
native=decoded(T/'capture-status.txt');v['native_input_loss']=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',native));assert v['native_input_loss']['weapon_hand_input_loss_total']=='0';v['native_age_summary']=rows('stereo_hand_loss_summary',native)
v['pickup_transition']=dict(rows=selected,scope='One Grip only; initialUP baseline156/3406 p0r0. Same event1/press1 unholster158,unavailable159,held161 after input observer baseline reset; then163 no held equipment model. Harness required held+input_status event, so it did not accept161. This does not prove no actual held receipt or stable holding; do not simply relax reader while163 drop remains unexplained.')
v['interpretation']='New PG325200c3 compiled/427 fixture checks. Actual44240 failed shoulder held receipt;0 shots/no early continuation branch reached, so no live efficacy or regression causality claim. Actual held161 logged with same event1/press1 but input baseline, then equipment reset163. Source/rig unavailable159; Nativeinputloss0/Gate28age-only. Actual source/caller binding and test receipt association need inspection before another run.'
v['next_bounded_source_question']='Trace original unholster-pending completion161 and subsequent equipment drop163 after rig unavailable159. Determine retained original intent identity versus observer baseline reset, and actual held binding/publication. Harness input_status event restriction can miss delayed completion, but do not adopt unrelated/replayed held or ignore163 drop. No new Grip/retry, source freshness/lifecycle unchanged. Early continuation still needs actual game exercise; history4629 origin still open.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 pages personally reviewed. Late near-right gun and ghost/separated hands visible, without stable tracked hold/contact acceptance. No firing/reload/new branch coverage; physical headset unaccepted.');write(p,v)
p=A/'runtime-44240-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-44240-first-failure-analysis.json',latest_runtime_result='runtime-44240-first-failure-analysis.json',requirements_complete=False,current_candidate='Private PG325200c3/Nativeb9ab7a82 early released refire compiled/427checks. Actual44240 shoulder helper rejected held161 baseline; same originalevent1/press1 observed, subsequentmodeldrop163 unresolved.0shots and earlybranch0, no live efficacy claim. All13 remain.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=44240,runner_last_poll='22606 terminal1; normal stop/launcher/Native/PG/XR0; owned game+children gone; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际44240：新续发版本未到达射击，原取枪完成与读取关联\n\nPG325200c3/Nativeb9ab7a82正常资源等待80.547s后单次运行。UP156/3406 p0r0、唯一Grip后158 event1/press1 unholster_pending，159 unavailable/rig source_unavailable，161同event1/press1 held但input_status baseline，随后163装备模型baseline target0。测试限定held+input_status event，未接受延迟完成161；因此不能称完全没held记录，也不能只放宽读取忽略163掉持有。0射线/newearlyrefire0，新功能效果未验证，无回归因果证明。下一步核原pending完成跨baseline及163逻辑model/binding，而非重按或无变化重跑。Nativeinputloss0/Gate28age-only。正常清理全0无forced、64帧错误空/编码0、3页48对亲阅，末段近右眼枪与分离重影手仍无稳定接触验收。完整13项及4629 origin问题保留。\n')
print(json.dumps(dict(shots=0,early_refire_rows=0,held161_observed=True,full_pass=False,terminal=True)))
