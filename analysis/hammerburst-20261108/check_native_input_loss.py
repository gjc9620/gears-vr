"""Run actual frozen Broker fixture, including every new loss branch."""
from pathlib import Path
import json,subprocess,hashlib,re
A=Path(__file__).resolve().parent;P=A/'native-input-loss-r1';S=P/'source/tools/native-capture'
includes=('weapon_hand_call_broker_test.inl','main_mount_broker_test.inl','body_main_mount_broker_test.inl','committed_hand_consumer_test.inl')
for name in includes:
 if not (S/name).exists():
  (S/name).write_bytes(subprocess.check_output(['git','show','7c30b73a:tools/native-capture/'+name],cwd=A.parents[1]))
s=(S/'weapon_hand_broker_test.cpp').read_text()
cases=r'''
void input_loss_observation_tests(){
    using Rows=std::array<w::Broker::InputLossObservation,16>;
    for(unsigned stage=1;stage<=3;++stage){
        Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
        if(stage==1){f.fake.stamp+=100000001;C(f.broker.prepare(frame,p)==w::Reply::unavailable);C(f.fake.prepares==0);}
        else {
            C(f.broker.prepare(frame,p)==w::Reply::accepted);auto decision=f.decision(frame,p);
            f.fake.stamp=stage==2?f.fake.stamp.load()+100000001:0;
            C(f.broker.commit(frame,p,decision)==w::Reply::unavailable);C(f.fake.commits==0);
        }
        Rows rows{};unsigned count{};const auto total=f.broker.input_loss_observations(rows,count);
        C(total==1&&count==1&&rows[0].stage==stage);
        C(rows[0].frame==frame.key.frame&&rows[0].sequence==frame.controllers.sequence);
        C(rows[0].now_ns==f.fake.stamp.load()&&rows[0].sync_ns==frame.controllers.sync_ns);
        C(rows[0].controller_ns==frame.controllers.received_ns&&rows[0].stereo_ns==frame.stereo.received_ns);
        C(f.fake.observed_invalidation.reason==w::InvalidateReason::input_lost&&!f.broker.authorized());
        // Match Native's existing Pending RAII: early commit rejection still owes abort.
        if(f.broker.pending_transactions())C(f.broker.abort_frame(frame.key,p.nonce)==w::Reply::accepted);
        f.fake.stamp=1000000000;f.finish();
    }
    {Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
        C(f.broker.prepare(frame,p)==w::Reply::accepted);auto decision=f.decision(frame,p);
        C(f.broker.commit(frame,p,decision)==w::Reply::accepted);
        Rows rows{};unsigned count{};C(f.broker.input_loss_observations(rows,count)==0&&count==0);
        for(unsigned i=0;i<20;++i)C(f.broker.invalidate(w::InvalidateReason::input_lost)==w::Reply::accepted);
        C(f.broker.input_loss_observations(rows,count)==20&&count==16);
        for(unsigned i=0;i<count;++i)C(rows[i].stage==4&&rows[i].now_ns==0&&rows[i].frame==frame.key.frame);
        f.finish();
    }
}
'''
needle='int main(){';assert s.count(needle)==1
s=s.replace(needle,cases+needle+'\n    input_loss_observation_tests();')
(S/'weapon_hand_input_loss_test.cpp').write_text(s)
cmd=P/'broker-test.cmd';exe=P/'broker-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{S / "weapon_hand_input_loss_test.cpp"}" /Fo"{P / "broker-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'broker-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'broker-test.log').read_text(errors='replace'));r.check_returncode()
(P/'broker-test-result.json').write_text(json.dumps(dict(exit_code=0,actual_broker=True,baseline_suite_included=True,all_four_diagnostic_stages=True,ring_wrap_test=True,game_started=False,source_sha256=hashlib.sha256(s.encode()).hexdigest(),limits='Actual Broker with bounded callback/clock/module/engine fakes; no gameplay/headset acceptance.'),indent=2))
p=P/'prepared.json';proof=json.loads(p.read_text());m=re.search(r'weapon_hand_broker: (\d+) checks, (\d+) failures',(P/'broker-test.log').read_text(errors='replace'));assert m and m[2]=='0'
proof.update(broker_checks=int(m[1]),broker_failures=int(m[2]),tests_only_restore_commit='7c30b73a',test_include_sha256={n:hashlib.sha256((S/n).read_bytes()).hexdigest() for n in includes})
p.write_text(json.dumps(proof,indent=2))
