from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-135316';D=I/'actions/menu-firstperson-45516-135341';T=A/'runtime-45516-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
p=A/'runtime-45516-first-failure-analysis.json';v=read(p)
for source in (D/'hammerburst-empty-reload.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 assert source.exists(),source
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
raw=(T/'capture-status.txt').read_bytes();s=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
def rows(prefix,text):return [dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in text.splitlines() if line.startswith(prefix+' ')]
v['native_age_summary']=rows('stereo_hand_loss_summary',s);v['native_age_rows']=rows('stereo_hand_loss',s)
assert v['native_age_summary']==[dict(total='27',age_only_kept='27',retained='27')]
assert 'weapon_hand_input_loss_total=0 ' in s
v['native_input_loss_total']=0
v['native_sha256']=read(A/'native-render-age-r1/prepared.json')['native_sha256']
raw=(T/'playground-45516.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
for prefix in ('hammerburst_release_debt_wait','hammerburst_stale_debt_wait','native_hammerburst_release_rejected','native_hammerburst_continuation_rejected','hammerburst_pose_gap_incumbent','hammerburst_latch_seek_unavailable'):
 v[prefix+'_rows']=rows(prefix,pg)
v['empty']=read(D/'hammerburst-empty-reload.json');d=v['empty']['drain_loaded_only'];receipts=[x['first_fire_input'] for x in d['confirmed_inputs']]
v['test_deadline_first_failure']=dict(whole_window_seconds=18,last_burst_index=12,last_completed_loaded=6,last_down_sent_ns=receipts[-1]['down_sent_ns'],last_release_sent_ns=receipts[-1]['release_sent_ns'],down_to_release_ns=receipts[-1]['release_sent_ns']-receipts[-1]['down_sent_ns'],first_to_last_input_span_ns=receipts[-1]['release_sent_ns']-receipts[0]['baseline_sent_ns'],first_shot_observed=False,source_stage3_after_real_up='first debt requires trigger still DOWN; observed release rejection count6 is prior completed cohort, not six new shots')
v['interpretation']='72 source-consistent rays, core6+11 drain bursts, last completed typed6/78. Last DOWN truncated after87.6207ms by original18s test-flow deadline; total firstbaseline→release17.99699s. Native age-only branch27 actual hits all before first fire, input_loss0. No in-fire age-gap proof, released-WAIT0, no empty veto/reload reached. Test flow repaired to36s and refuses DOWN with<3.5s left; production deadlines unchanged.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='Near-right-eye gun oversized; ghost/separated hand and transient white geometry remain. No dense all-angle/contact/headset acceptance.')
write(p,v)
p=A/'runtime-45516-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-45516-first-failure-analysis.json',latest_runtime_result='runtime-45516-first-failure-analysis.json',requirements_complete=False,current_candidate='Private Native71f280b0/PGb7dcb9ec actual45516:72 rays, deadline-truncated final drain DOWN; normalcleanup. Harness36s/min3.5s beforeDOWN repaired,12 tests pass. Age27hits beforefire only; no full acceptance.')
a['render_age_runtime']=dict(evidence=str(A/'runtime-45516-first-failure-analysis.json'),age_only_hits=27,input_loss=0,limit='Hits before fire only; true-loss rejection offline17 cases. Does not prove mid-burst continuity or complete fix.');write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=45516,runner_last_poll='35497 terminal1; stop/launcher/Native/PG/XR normal0; game/children gone, noforced',next='Save bounded test-harness correction then one targeted full empty recheck; no product binary rebuild or unchanged source loop.');write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际45516：有限弹药测试窗口截断\n\nNative71f280b0/PGb7dcb9ec实测72条一致射线，core6加11组六连发，最后完整库存6/78。第12组DOWN仅87.6207ms即被原18s整段测试截止收尾UP，首baseline至末UP17.99699s，未到空仓。不是产品超时修复通过；Native年龄分支27次实际命中均在开火前，input_loss0，连发中间断与UP-WAIT仍无实测覆盖。单独修正私有测试适配器为36s整段窗口，DOWN前至少留3.5s，否则不按下；原产品100ms/20ms/250ms及单次首发观察不变，12项回归通过。正常清理全0、无forced、316帧/无捕获错误/编码0，3页48对全跨度影像亲阅，近右眼巨枪/手分离/白几何仍不合格。全部13项保留，不算整枪通过。\n')
print(json.dumps(v['test_deadline_first_failure']))
