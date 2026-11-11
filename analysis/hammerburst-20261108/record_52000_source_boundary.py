from pathlib import Path
import hashlib,json
A=Path(__file__).resolve().parent;R=A.parents[1]
S=A/'history-rejection-r1/source/tools/vr-common/weapon_hand_client.h'
v=dict(runtime=52000,source=str(S),source_sha256=hashlib.sha256(S.read_bytes()).hexdigest(),
 classification='stale_before5 is the Playground WeaponHandClient consumer freshness check, not a Native broker prepare/commit failure. Native inputloss total0. The withdrawal record may predate the logged read; no-current publication failure1 rows report that already-recorded withdrawal.',
 source_sites=dict(fresh=217,copy_fresh_check=548,withdraw_expired_pose=188,source_loss_observer='tools/playground/weapon_hammerburst_source_loss_observation.inl'),
 measured=dict(withdrawal_frame=347,withdrawal_sequence=3550,withdrawal_input_age_ns=100134800,last_successful_input_sync_ns=167590908592400,last_successful_input_admission_ns=167590974651900,last_successful_input_age_ns=66059500),
 limits='last successful sync is for the prior admitted source, not proven to be withdrawal347 sync. Idle source-loss logger sees prior withdrawal but loses original failed copy commit-check/since-commit details after publication cleared. Existing data cannot split exact347 producer-to-Native and Native-to-consumer delay or name first reader withdrawing it.',
 next='Locate all GT copy consumers and the original first failed copy call. Preserve its existing FrameReadObservation (input age, commit age, since commit, caller) in bounded passive output on the same thread only; no new read/clock/retry, no freshness relaxation. No unchanged game run.')
(A/'runtime-52000-source-boundary.json').write_text(json.dumps(v,indent=2),encoding='utf-8')
p=A/'runtime-52000-first-failure-analysis.json';a=json.loads(p.read_text());a['source_boundary']=v;a['interpretation']=a['interpretation'].replace('Native publication withdrawal','Playground client withdrawal');a['next_bounded_source_question']=v['next'];p.write_text(json.dumps(a,indent=2),encoding='utf-8')
p=A/'finalize_runtime_52000.py';s=p.read_text();s=s.replace('Native publication withdrawal','Playground client withdrawal');p.write_text(s,encoding='utf-8')
p=A/'active-runtime-wait.json';a=json.loads(p.read_text(encoding='utf-8-sig'));a['next']=v['next'];p.write_text(json.dumps(a,indent=2),encoding='utf-8')
with (R/'docs/hammerburst-development-20261108.md').open('a',encoding='utf-8') as f:f.write('\n52000源边界核对：stale_before5由Playground WeaponHandClient消费时fresh检查产生，并非Native broker prepare/commit失败；Native inputloss0。后续publication1只带已记录的347撤回，无法还原首个失败copy的commit-age/since-commit或调用方。上次成功输入sync908592400→admission974651900为66.0595ms，不能当作347同一源。下一步定位首个消费copy原观察字段的有界保留，现有数据不足以归责调度或并发；不得把100ms改大或沿用恢复时已按住的输入。\n')
print(json.dumps(v))
