from pathlib import Path
import json,subprocess,re
import prepare_native_passive_rejection as prep
P=prep.P;BASE=prep.BASE;S=P/'source/tools/native-capture'
build=json.loads((P/'prebuild.json').read_text())
header=(P/'source'/prep.BROKER).read_text()
assert header.replace(build['broker_observed'],build['broker_original']).replace(prep.OBS,'')==(BASE/'source'/prep.BROKER).read_text()
s=(P/'source'/prep.GRASP).read_text()
s=s.replace(prep.DECL,'').replace(prep.LOG,'').replace(prep.RECORD,'').replace('    hand::SocketVisibilityObservation socket_observation{};\n','')
s=s.replace('''        if(current.hands[0].squeeze_active!=1||hand::control_held(current.hands[0],gears_vr::control_squeeze)){rejected(7);return;}
        if(!broker->admit_socket_visibility(f,&socket_observation)){rejected(8);return;}''','''        if(current.hands[0].squeeze_active!=1||hand::control_held(current.hands[0],gears_vr::control_squeeze)||
           !broker->admit_socket_visibility(f))return;''')
assert s==(BASE/'source'/prep.GRASP).read_text()
# The test-only reference is the exact parent method in the actual Broker.
reference=build['broker_original'].replace('admit_socket_visibility(', 'reference_socket_visibility(',1)
target_header=S/'weapon_hand_socket_reference_test.h'
target_header.write_text(header.replace(build['broker_observed'],reference+build['broker_observed']))
test=(S/'weapon_hand_broker_test.cpp').read_text().replace('#include "weapon_hand_broker.h"','#include "weapon_hand_socket_reference_test.h"')
extra=r'''
void passive_socket_original_result_tests(){
    Fixture f;f.attach();auto frame=f.frame();w::Proposal p{};
    frame.left_grip_down=0;frame.press_token=0;
    frame.controllers.hands[0].squeeze_active=1;frame.controllers.hands[0].squeeze=0;
    C(f.broker.prepare(frame,p)==w::Reply::accepted);
    C(f.broker.commit(frame,p,f.decision(frame,p,false))==w::Reply::accepted);
    w::SocketVisibilityObservation observation{};
    C(f.broker.admit_socket_visibility(frame,&observation)&&observation.gate==0);
    C(observation.source_frame==1&&observation.frozen_frame==1&&observation.source_sequence==frame.controllers.sequence);
    for(unsigned mutation=0;mutation<9;++mutation){
        auto changed=frame;
        if(mutation==1)changed.key.frame++;
        if(mutation==2)changed.controllers.sequence++;
        if(mutation==3)changed.reference_generation++;
        if(mutation==4)changed.left_tracked=0;
        if(mutation==5)changed.controllers.hands[0].squeeze_active=0;
        if(mutation==6)changed.controllers.hands[0].squeeze=.8f;
        if(mutation==7)test_thread=worker;
        if(mutation==8)f.fake.stamp+=100000001;
        const bool original=f.broker.reference_socket_visibility(changed);
        C(f.broker.admit_socket_visibility(changed,&observation)==original);
        C(f.broker.admit_socket_visibility(changed)==original);
        C(original?observation.gate==0:observation.gate!=0);
        test_thread=gt;
        if(mutation==8)f.fake.stamp-=100000001;
    }
    // Original fresh request N is refused after a new authentic N+1 commit.
    // This proves the protocol boundary only, not the live refusal cause.
    auto next=f.frame(2);next.left_grip_down=0;next.press_token=0;
    next.controllers.hands[0].squeeze_active=1;next.controllers.hands[0].squeeze=0;
    C(f.broker.prepare(next,p)==w::Reply::accepted);
    C(f.broker.commit(next,p,f.decision(next,p,false))==w::Reply::accepted);
    C(!f.broker.admit_socket_visibility(frame,&observation)&&observation.gate==12);
    C(observation.source_frame==1&&observation.frozen_frame==2);
    C(!f.broker.reference_socket_visibility(frame));
    C(f.broker.admit_socket_visibility(next,&observation)&&observation.gate==0);
    f.finish();
}
'''
assert test.count('int main(){')==1
test=test.replace('int main(){',extra+'\nint main(){\n    passive_socket_original_result_tests();')
source=S/'weapon_hand_socket_observation_test.cpp';source.write_text(test)
cmd=P/'socket-observation-test.cmd';exe=P/'socket-observation-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{source}" /Fo"{P / "socket-observation-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'socket-observation-test.log').open('wb') as log:
    result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
log=(P/'socket-observation-test.log').read_text(errors='replace');print(log[-3500:]);assert result.returncode==0
m=re.search(r'weapon_hand_broker: (\d+) checks, (\d+) failures',log);assert m and m[2]=='0'
proof=dict(passed=True,checks=int(m[1]),failures=0,exact_policy_reversal=True,actual_broker=True,exact_parent_reference_differential=True,mutations=9,authentic_next_frame_case=True,new_clocks_or_queries=False,game_started=False,limits='Production Broker with original callback/clock/module fakes. Cross-frame denial is an offline protocol result, not live cause proof.')
(P/'socket-observation-test-result.json').write_text(json.dumps(proof,indent=2));print(json.dumps(proof))
