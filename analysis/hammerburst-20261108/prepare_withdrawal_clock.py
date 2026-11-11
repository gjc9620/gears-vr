from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'history-rejection-r1';P=A/'withdrawal-clock-r1'
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patch=''
 def edit(rel,fn):
  nonlocal patch
  p=P/'source'/rel;before=p.read_text();after=fn(before);assert before!=after;p.write_text(after);patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 def client(s):
  s=s.replace('    std::int64_t commit_check_input_age_ns{-1},since_commit_check_ns{-1};','    std::int64_t commit_check_input_age_ns{-1},since_commit_check_ns{-1};\n    std::int64_t expired_read_ns{-1},expired_input_age_ns{-1};\n    unsigned expired_reader{}; // Observation only: copy1, source2, snapshot3.')
  needle='    Reply fail_gt(Reply reason) noexcept'
  helper='''    // Keep the FIRST failing read's existing clock before the original
    // withdrawal clears committed_. No added clock, source read or decision.
    void withdraw_expired_pose_gt(unsigned reader,std::int64_t read_ns) noexcept {
        const auto frame=committed_.key.frame;
        const auto sync=committed_.controllers.sync_ns;
        const auto check=committed_check_ns_;
        withdraw_expired_pose_gt();
        if(frame&&last_withdrawal_.frame==frame){
            last_withdrawal_.expired_reader=reader;
            last_withdrawal_.expired_read_ns=read_ns;
            last_withdrawal_.expired_input_age_ns=sync>0&&read_ns>=sync?read_ns-sync:-1;
            last_withdrawal_.commit_check_input_age_ns=sync>0&&check>=sync?check-sync:-1;
            last_withdrawal_.since_commit_check_ns=check>0&&read_ns>=check?read_ns-check:-1;
        }
    }
'''
  assert s.count(needle)==1;s=s.replace(needle,helper+needle)
  needle='if(!fresh(committed_,now)){withdraw_expired_pose_gt();'
  assert s.count(needle)==3
  for i in (1,2,3):s=s.replace(needle,f'if(!fresh(committed_,now)){{withdraw_expired_pose_gt({i},now);',1)
  return s
 edit('tools/vr-common/weapon_hand_client.h',client)
 def logger(s):
  needle='game_r2=%u admitted_ns=%lld\\n';assert s.count(needle)==1
  s=s.replace(needle,'game_r2=%u admitted_ns=%lld expired_reader=%u expired_read_ns=%lld expired_input_age_ns=%lld expired_commit_age_ns=%lld expired_since_commit_ns=%lld\\n')
  needle='unsigned(game.r2_held),input_admission_ns);';assert s.count(needle)==1
  return s.replace(needle,'unsigned(game.r2_held),input_admission_ns,source_withdrawal.expired_reader,source_withdrawal.expired_read_ns,source_withdrawal.expired_input_age_ns,source_withdrawal.commit_check_input_age_ns,source_withdrawal.since_commit_check_ns);')
 edit('tools/playground/weapon_hammerburst_source_loss_observation.inl',logger)
 (R/'docs/hammerburst-withdrawal-clock.patch').write_text(patch)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Retain original first stale consumer clock, commit age and reader class after unchanged withdrawal; no new clock/read or changed policy.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
