"""Exercise actual frozen HandClient API, not the event adapter's Client seam."""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent
P=A/'released-debt-r1'
source=(P/'source/tools/vr-common/weapon_hand_client_test.cpp').read_text()
source=source[:source.index('int main(){')]+r'''int main(){
    const auto inherited=existing_action_lifetime_cases();unsigned stale_cases{};
    for(bool support:{false,true}){
        Host h;HandClient c(h.adapter(true));h.connect(c);h.ready();
        if(!support){h.next(false);assert(h.step()==Grip::single);}
        ExistingActionLifetime life{};{GT gt;assert(c.capture_existing_action_lifetime_on_gt(h.frame.key,life));}
        h.now=h.frame.controllers.sync_ns+102130700;Frame out{};
        assert(!h.copy(out)&&!c.armed()&&same_frame(out,Frame{}));
        {GT gt;const auto withdrawal=c.withdrawal_on_gt();assert(withdrawal.failure==FrameReadFailure::stale_before);
            assert(withdrawal.input_age_ns==102130700);assert(c.existing_action_lifetime_on_gt(life));}
        assert(!h.snapshot());h.invalidate();
        {GT gt;assert(!c.existing_action_lifetime_on_gt(life));}
        ++stale_cases;
    }
    std::printf("Actual HandClient lifetime: %u inherited identity/epoch cases + %u stale-publication cases; no Frame or pose authority\n",inherited,stale_cases);
}
'''
target=P/'source/tools/vr-common/hammerburst_released_lifetime_test.cpp'
target.write_text(source)
cmd=P/'client-test.cmd'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
    f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "client-test.obj"}" /Fe"{P / "client-test.exe"}"\n'+
    f'if errorlevel 1 exit /b %errorlevel%\n"{P / "client-test.exe"}"\nexit /b %errorlevel%\n')
with (P/'client-test.log').open('wb') as log:
    result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'client-test.log').read_text(errors='replace'))
result.check_returncode()
(P/'client-test-result.json').write_text(json.dumps(dict(passed=True,actual_client=True,
    source_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
    header_sha256=hashlib.sha256((target.parent/'weapon_hand_client.h').read_bytes()).hexdigest(),
    stale_single_and_support_cases=2,game_execution=False),indent=2))
