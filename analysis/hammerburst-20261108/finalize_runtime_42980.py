from pathlib import Path
import json,hashlib,re,shutil
A=Path(__file__).resolve().parent;R=A.parents[1];I=R/'analysis/instances/hb-cloak-1111-181143'
D=I/'actions/menu-firstperson-42980-181210';T=A/'runtime-42980-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2),encoding='utf-8')
def decoded(p):
    b=p.read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
def rows(prefix,s):return [dict(re.findall(r'(\w+)=([^\s]+)',line)) for line in s.splitlines() if line.startswith(prefix+' ')]
p=A/'runtime-42980-fresh-full-empty.json';v=read(p)
assert v['operator_exit']==0 and v['native_cleanup_verified'] and v['xr_exit_code']==0 and v['playground_cleanup']['passed']
assert not v['cleanup']['forced_termination'] and v['actual_trajectory_count']==84 and v['trajectory_consistent']
for source in (D/'hammerburst-empty-reload.json',D/'ordinary_empty_replacement/hammerburst-replacement.json',I/'hb-source-binding.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
    target=T/source.name;shutil.copy2(source,target);digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
    v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['empty']=read(D/'hammerburst-empty-reload.json');v['replacement']=read(D/'ordinary_empty_replacement/hammerburst-replacement.json')
e=v['empty'];r=v['replacement'];assert all(e['checks'].values()) and all(r['checks'].values())
assert not e['gameplay_setters'] and not e['reserve_exhaustion_attempted']
assert len(e['drain_loaded_only']['burst_observations'])==12 and all(b['actual_spent']==6 for b in e['drain_loaded_only']['burst_observations'])
assert [(r[k]['loaded'],r[k]['reserve']) for k in ('before','after','after_return_fire')]==[(0,78),(78,0),(72,0)]
assert len(r['shots'])==6 and not e['empty_trigger_shots']
pg=decoded(T/'playground-42980.log')
for prefix in ('hammerburst_pre_original_refire','hammerburst_service_loss','hammerburst_history_origin','hammerburst_latch_history','hammerburst_latch_rejected','hammerburst_extraction_history','hammerburst_reload_transition','hammerburst_reload_complete'):
    v[prefix+'_rows']=rows(prefix,pg)
early=v['hammerburst_pre_original_refire_rows'];assert len(early)==12
assert len({x['event'] for x in early})==12 and all(int(x['after_count'])==int(x['before_count'])+1 and x['pending_after']=='0' for x in early)
assert not v['hammerburst_service_loss_rows'] and not v['hammerburst_latch_rejected_rows'] and not v['hammerburst_history_origin_rows']
assert len(v['hammerburst_reload_complete_rows'])==1 and v['hammerburst_reload_complete_rows'][0]['loaded']=='78'
fields=dict(re.findall(r'(\w+)=([^\s]+)',decoded(T/'capture-status.txt')))
v['native_counts']={k:fields[k] for k in ('weapon_hand_input_loss_total','body_context_grasp_commit_4')}
assert v['native_counts']['weapon_hand_input_loss_total']=='0'
v['native_candidate']=read(A/'fresh-passive-native-r1/prepared.json')['native_sha256']
v['mechanical_slice_passed']=True;v['whole_weapon_accepted']=False
v['interpretation']='Exact new PG5fde320a/Nativef0635832 pair: core6+12drain6=78 actual consistent rays, original empty0/78 trigger no-shot, opening/extraction/newmag/seat/close completes78/0, return6 rays and typed72/0.84spent+72remaining=156initial. Twelve bounded logged earlier-refire events are distinct/count+1/pending0; no original service/latch/history-origin failure captured, Nativeinputloss0. New BodyMount ABI6 did not prevent complete original flow. Not reserve-exhaustion:72 loaded remain, e.reserve_exhaustion_attemptedfalse. Not retained-original/perfect-cancel/stow/all interruptions/fit/headset acceptance.58352 new fresh foregrip branch proof remains separate.'
v['prelaunch_resource_note']='An earlier attempt was refused before prepare/game creation by the existing5GiB workspace margin. Exact terminal caches173534/180416 preserved by SHA verified J archive+junction, then this run admitted normally. No resource threshold bypass or forced process exit.'
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 paired overview pages personally inspected.763 frames/no capture errors/encode0. Rifle stays near eye for much of fire/reload; hands/arms separate or ghost, white geometry transitions, guide outlines visible. Complete hand/part/all-angle fitting unaccepted. Pause-card112.90/114.77s then world116.56s retained, no general interruption acceptance.')
v['next_bounded_work']='Use existing saved actual body/weapon/part asset identity and geometry to check visible wrist/palm/fingers against HB foregrip and magazine from unobstructed angles; investigate two weight0/whole-C0 transitions separately. Then add exact missing zero-reserve/perfect-cancel/retained-original/stow cases as separate actions. Do not repeat unchanged four-state/full empty flows. Preserve all13, previous failures and physical-headset boundary.'
write(p,v)
p=A/'runtime-42980-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13
a.update(latest_runtime_result='runtime-42980-fresh-full-empty.json',latest_full_empty_reload_pass='runtime-42980-fresh-full-empty.json',requirements_complete=False,current_candidate='PG5fde320a/Nativef0635832 fresh-passive ABI6:58352 actual new-branch four-state PASS/275query records,42980 complete ordinary empty reload+return-fire PASS/84consistent rays/0-78veto/78-0reload/72-0return. Two weight0 and whole-C0/fit/remaining requirements unaccepted. No public promotion.')
evidence=dict(path=str(A/'runtime-42980-fresh-full-empty.json'),sha256=hashlib.sha256((A/'runtime-42980-fresh-full-empty.json').read_bytes()).hexdigest())
for requirement in a['requirements']:
    if requirement['id']=='empty_reload':requirement.update(status='current_fresh_pair_full_ordinary_empty_reload_return_fire_passed_single_game',limit='42980 new ABI6 pair completes original0/78 veto->78/0 reload->72/0 return6. Parent20924 preserved; reserve exhaustion/perfect-cancel/retained original/fitting remain separate.')
    if requirement['id']=='finite_fire':requirement.update(status='current_fresh_pair_14_original_six_bursts_84_consistent_rays_single_game',limit='42980 core6+12drain6+return6 actual84; finite typed arithmetic. Not indefinite/all source-loss/0loaded-0reserve coverage.')
    if requirement['id'] in ('empty_reload','finite_fire','replacement'):requirement.setdefault('evidence',[]).append(evidence)
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=42980,runner_last_poll='4180 terminal0; normal stop/launcher/Native/PG/XR0; noforced/owned game+children absent; encode0',next=v['next_bounded_work']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as stream:
    stream.write('\n\n## 2026-11-11 实际42980：新pair完整普通空仓换弹与回枪六发通过\n\n精确PG5fde320a/Nativef0635832新ABI6 pair复查通过：core6+12drain6=78一致射线，空仓0/78真实禁射，原开盖/提旧mag/取新mag/座入/合盖完成78/0，回枪6射线后typed72/0，总84spent+72remaining=156initial。12条已捕获原early-refire event各不同/count+1/pending0，受原cap12；service/latch/history-origin拒绝无捕获，Nativeinputloss0，不能称未触发分支全修或所有源丢失都覆盖。新增fresh被动路径58352已实际执行，当前42980证明接口改动未阻止完整机械链路；保留父20924和全部历史失败。reserve_exhaustion_attempted=false、仍loaded72，不算0/0；完美/取消/保留原mag/收枪/全中断/贴合/头显仍未验收。一次前置磁盘余量拒绝发生在prepare/game前，原阈值保持，173534/180416已终止缓存逐文件SHA转存J盘并映射回读后再正常准入，无forced。正常stop/launcher/Native/PG/XR0，763帧错误空/编码0、3页48对亲阅，近眼枪体/手臂分离重影/白几何与guide outlines仍未通过全贴合；112.90/114.77s暂停卡、116.56s世界图像不扩称全中断。下一步查保存的真实body/weapon/part资产与皮肤接触、两次weight0/整段C0转换，以及其余独立缺项，禁止无变化重复四态或完整空仓。所有13项保留/public未改。\n')
print(json.dumps(dict(current_pair_full_reload_pass=True,actual_rays=84,loaded=72,reserve=0,Native_input_loss=0,whole_complete=False)))
