from pathlib import Path
import hashlib,json,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-135901';D=I/'actions/menu-firstperson-6896-135950';T=A/'runtime-6896-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def rows(prefix,text):return [dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in text.splitlines() if line.startswith(prefix+' ')]
p=A/'runtime-6896-first-failure-analysis.json';v=read(p)
for source in (D/'hammerburst-empty-reload.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
 target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
 v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
raw=(T/'capture-status.txt').read_bytes();s=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
v['native_age_summary']=rows('stereo_hand_loss_summary',s);v['native_age_rows']=rows('stereo_hand_loss',s)
assert v['native_age_summary']==[dict(total='93',age_only_kept='93',retained='32')]
fields=dict(re.findall(r'(weapon_hand_input_loss\w*)=([^\s]+)',s));assert fields['weapon_hand_input_loss_total']=='1' and fields['weapon_hand_input_loss_0_stage']=='1'
v['native_input_loss']=fields;v['native_sha256']=read(A/'native-render-age-r1/prepared.json')['native_sha256']
raw=(T/'playground-6896.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
for prefix in ('hammerburst_release_debt_wait','hammerburst_stale_debt_wait','native_hammerburst_release_rejected','native_hammerburst_continuation_rejected','hammerburst_pose_gap_incumbent','hammerburst_latch_seek_unavailable'):
 v[prefix+'_rows']=rows(prefix,pg)
v['empty']=read(D/'hammerburst-empty-reload.json');receipt=v['empty']['drain_loaded_only']['confirmed_inputs'][0]['first_fire_input']
assert receipt['remaining_before_down_ns']>35_000_000_000
v['input_receipt_correction']=dict(remaining_before_down_ns=receipt['remaining_before_down_ns'],original_down=receipt['down'],first_explicit_block=next(r for r in rows('native_lancer_r2',pg) if r.get('trigger_pressed')=='2' and r.get('status')=='8'),reader_error='Accepted high trigger without requiring actual status3/owned1; status1/owned0 was not a consumed press. Corrected helper now waits only actual ownership or explicit8 rejection;14 tests pass, no retry.')
v['interpretation']='Core6 consistent rays then Native prepare frame367 rejected original source: flags13 missing input_matched, controllers.sequence/sync0, stereo age100.2764ms at original broker clock. Strict prepare invalidated input as designed. First drain DOWN later frame383 status1/owned0; frame384 explicitblocked8. No shot or empty/reload. Exact cause of press ownership rejection after fresh UP not yet proven. Age-only outer Gate93kept, no external loss; does not override strict prepare. Do not weaken guard or rerun unchanged.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='Near-right-eye gun oversized, ghost/separated hand and white geometry remain. No dense all-angle/contact/headset acceptance.');write(p,v)
p=A/'runtime-6896-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13;a.update(latest_runtime_failure='runtime-6896-first-failure-analysis.json',latest_runtime_result='runtime-6896-first-failure-analysis.json',requirements_complete=False,current_candidate='Private Native71f280b0/PGb7dcb9ec actual6896 core6 then strict prepare input_loss stage1. Gate93 age-only kept; full flow failed. Reader quietDOWN ownership correction14tests; next inspect actual Input transition after freshUP without relaxing broker/anti-replay.')
a['latest_strict_prepare_failure']=dict(evidence=str(A/'runtime-6896-first-failure-analysis.json'),frame=367,stereo_age_ns=100276400,flags=13,controllers_sequence=0,guard_unchanged=True);write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=6896,runner_last_poll='72247 terminal1; stop/launcher/Native/PG/XR normal0; game/children gone, noforced',next='Inspect actual R2 Input new DOWN transition after strict prepare invalidation and fresh UP375. No unchanged rerun, no weakening100ms/source/identity/anti-replay. All13 retained.');write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n\n## 2026-11-11 实际6896：严格prepare后输入拒绝\n\n同Native71f280b0/PGb7dcb9ec，修正后的36s测试窗口实际剩余35.4076621s，核心6条一致射线后，Native原prepare367因flags13缺input_matched、controller序号/时戳0、原Stereo年龄100.2764ms拒绝并撤销input。外层年龄分支93次保留不覆盖严格prepare。其后新鲜UP375、DOWN383实际status1/owned0，384明确blocked8，未开火；不能把quiet当已消费DOWN，已修正测试读取仅承认status3/owned1或明确拒绝8，14项检查通过，无补按。尚未证明该新DOWN拒绝的具体转移原因，下一步读实际Input状态流，保留严格时效和防重放。正常清理全0、无forced、170帧/捕获错误空/编码0、3页48对亲阅，手枪贴合仍失败。整枪和空仓仍不通过，全部13项要求保留。\n')
print(json.dumps(dict(shots=v['actual_trajectory_count'],input_loss=fields['weapon_hand_input_loss_total'],native_age=v['native_age_summary'],next=a['next'])))
