from pathlib import Path
import json,shutil,hashlib,re
A=Path(__file__).resolve().parent;R=A.parents[1]
I=R/'analysis/instances/hb-cloak-1111-171202'
D=I/'actions/menu-firstperson-58780-171312';T=A/'runtime-58780-text'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,d):p.write_text(json.dumps(d,indent=2),encoding='utf-8')
p=A/'runtime-58780-passive-commit.json';v=read(p)
assert v['actual_trajectory_count']==0 and not v['cleanup']['forced_termination']
for source in (D/'hammerburst-twohand.json',I/'hb-source-binding.json',I/'native/capture-status.txt',I/'native/session-status.txt'):
    target=T/source.name;shutil.copy2(source,target)
    digest=hashlib.sha256(source.read_bytes()).hexdigest();assert hashlib.sha256(target.read_bytes()).hexdigest()==digest
    v['verified_archive'].append(dict(source=str(source),archive=str(target),sha256=digest))
v['foregrip']=read(D/'hammerburst-twohand.json');c=v['foregrip']['checks']
assert c['neutral_socket_no_weapon_steering_degrees']<.75 and c['neutral_wrist_stays_with_gun']['passed']
assert not c['neutral_wrist_follows_actual_weapon']['passed'] and len(v['foregrip']['states'])==3
observed=read(A/'passive-commit-58780.json');assert observed['records']==127
assert observed['summary']['all_applied'] and observed['summary']['position_fitted']==127
assert observed['summary']['max_target_solve_distance']<.002 and observed['summary']['max_target_solve_degrees']<.001
v['same_commit_observation']=dict(path=str(A/'passive-commit-58780.json'),sha256=hashlib.sha256((A/'passive-commit-58780.json').read_bytes()).hexdigest(),summary=observed['summary'],records=127,full_weight_records=sum(r['weight']==1 for r in observed['rows']))
v['native_candidate']=read(A/'native-passive-commit-r1/prepared.json')['native_sha256']
v['media'].update(encoded=True,encode_exit=0,personally_reviewed_overview=True,reviewed_pairs=48,visual='All3 paired overview pages personally inspected. Near-eye oversized rifle dominates the view; left hand remains flat/open against or beside fore-end, transient ghost/white geometry. Surface/all-angle fitting remains unaccepted.')
v['next_bounded_source_question']='Inspect existing sole C0 committed local-atoms -> render skin/palette consumer, original SpaceBases update timing, and matching bones/skin mapping for actual HB left hand. Current127 same-GT committed records prove goal and solved world wrist move; not upload/hardware/everyDraw/visible fit. Establish which existing consumer the async SpaceBases reader observes before changing the four-state verdict or pose. Preserve source freshness, fixed lengths, exact identity, single writer and1cm/2deg thresholds. No unchanged game run.'
write(p,v)
p=A/'runtime-58780-media-review/prepared.json';m=read(p);m.update(overview_reviewed=True,reviewed_pairs=48,full_fit_accepted=False,physical_headset_accepted=False,findings=v['media']['visual']);write(p,m)
p=A/'full-requirements-audit-20261110.json';a=read(p);assert len(a['requirements'])==13
a.update(latest_runtime_result='runtime-58780-passive-commit.json',latest_runtime_failure='runtime-58780-passive-commit.json',requirements_complete=False,current_candidate='PG325200c3/Nativee4e3155d. Ordinary full empty reload20924 PASS preserved.58780 no-drive PASS, async SpaceBases follow FAIL;127 same successful C0 records show moving goal/solved wrist, target residual .0002081raw/.00001356deg. Final skin/upload and current Grip/release unproved; all13 retained.')
for requirement in a['requirements']:
    if requirement['id']=='foregrip_states':
        requirement.update(status='current_no_drive_passed_async_follow_failed_C0_goal_solve_moves_render_boundary_open',limit='127 successful passive C0 records: all applied/fitted,106 full weight, target/solve residual tiny; async game-array wrist still fails1cm/2deg. Do not claim target frozen or final visible fit proved. Current Grab/release not reached.')
        requirement.setdefault('evidence',[]).append(dict(path=str(A/'runtime-58780-passive-commit.json'),sha256=hashlib.sha256((A/'runtime-58780-passive-commit.json').read_bytes()).hexdigest()))
write(p,a)
p=A/'active-runtime-wait.json';a=read(p);a.update(terminal=True,game_pid=58780,runner_last_poll='73697 terminal1; stop/launcher/Native/PG/XR normal0; owned game+children gone; noforced;encode0',next=v['next_bounded_source_question']);write(p,a)
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:
    f.write('\n\n## 2026-11-11 实际58780：同帧C0目标与求解会移动，最终显示环节待查\n\nPG325200c3/Nativee4e3155d，护木四态仅到被动跟随检查。未按Grip移动左手不驱动枪通过；异步SpaceBases腕跟枪误差1.023891raw/2.285720deg，仍失败，Grab/释放未到。新增127次原成功passive C0 commit全部applied/fitted、106次weight1；请求socket最大移动.904215raw/2.291829deg，最终world[11]也移动，求解target与最终腕最大误差.00020810raw/.00001355deg。这推翻把异步腕近乎不动直接解释为C0目标/求解冻结的假设，但不证明后续skin/palette/upload/每Draw或实际表面贴合。下一步查现有唯一C0 local-atoms到实际皮肤消费者与SpaceBases读取阶段/骨骼映射，不增writer、不放宽阈值、不重复无变化game。正常Native/PG/XR/stop/launcher0、无forced/前台违规，476帧错误空/编码0，3页48对亲阅：近眼大枪、左手张平/分离和过渡重影白几何仍不合格。20924完整空仓换弹/回枪六发通过保留，全13项继续。\n')
print(json.dumps(dict(terminal=True,same_commit_records=127,C0_target_solve_moves=True,render_fit_accepted=False,whole_complete=False)))
