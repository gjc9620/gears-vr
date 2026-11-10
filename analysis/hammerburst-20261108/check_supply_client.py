"""Run the actual frozen HandClient across HB's proposal contribution switch.

The Host supplies synthetic game observations; no Native/game acceptance.
"""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'supply-frame-r1'
source=(P/'source/tools/vr-common/weapon_hand_client_test.cpp').read_text()
def replace(old,new):
    global source
    assert source.count(old)==1,old[:80]
    source=source.replace(old,new)
replace('#include "weapon_hand_client.h"','#include "weapon_hand_client.h"\n#include "weapon_hammerburst_supply_publication.h"')
replace('    Frame frame{},live{};GripGameState game{};',
    '    Frame frame{},live{};GripGameState game{};\n    gears_hammerburst_supply_publication::Snapshot hb_publication;Lease hb_lease{};bool hb_supply_held{};')
replace('        out=h.game;', '''        out=h.game;
        // Model ONLY HB's proposal contributor before/after its gameplay
        // callback. The actual shared Client performs all comparison below.
        if(h.hb_supply_held&&!h.hb_publication.same_acquisition_source(f,h.hb_lease,h.hb_lease.id)){
            out.request_ammo=false;out.object_incumbents[0]=h.hb_lease;
        }''')
source=source[:source.index('int main(){')]+r'''int main(){
    unsigned cases{};
    for(unsigned variant=0;variant<4;++variant){
        Host h;HandClient c(h.adapter());h.connect(c);h.game.mount_known=false;
        h.step();h.next();h.step();h.game.request_ammo=true;h.next(true);
        assert(h.step()==Grip::single);
        Frame f{};GripGameState game{};Decision d{};
        {GT gt;assert(c.copy_frame_on_gt(f,game,d));}
        assert(d.status==DecisionStatus::granted&&d.new_grant&&d.lease.kind==LeaseKind::ammo);
        h.hb_supply_held=true;h.hb_lease=d.lease;
        if(variant)assert(h.hb_publication.remember(f,d));
        if(variant==2)h.game.busy=true;
        if(variant==3)h.invalidate();
        FrameReadObservation observation{};bool copied{};
        {GT gt;copied=c.copy_frame_on_gt(f,game,d,&observation);}
        if(variant==0){
            assert(!copied&&!c.armed());
            {GT gt;const auto w=c.withdrawal_on_gt();assert(w.failure==FrameReadFailure::game_changed&&w.game_difference==5);}
        }else if(variant==1){
            assert(copied&&c.armed()&&game.request_ammo&&game.object_incumbents[0].kind==LeaseKind::none);
            assert(d.new_grant&&d.status==DecisionStatus::granted);
            h.next(true);assert(h.step()==Grip::single);
            {GT gt;assert(c.copy_frame_on_gt(f,game,d));}
            assert(!d.new_grant&&d.status==DecisionStatus::retained&&same(d.lease,h.hb_lease));
            assert(!game.request_ammo&&same(game.object_incumbents[0],h.hb_lease));
        }else{
            assert(!copied&&!c.armed());
            if(variant==2){GT gt;assert(c.withdrawal_on_gt().game_difference==8);}
        }
        ++cases;
    }
    std::printf("HB actual HandClient supply handoff: %u cases; reproduce difference5, preserve first Frame and next strict retained Frame, reject busy/invalidation; local Host only\n",cases);
}
'''
target=P/'source/tools/vr-common/hammerburst_supply_client_test.cpp';target.write_text(source)
cmd=P/'supply-client-test.cmd';exe=P/'supply-client-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
    f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "supply-client-test.obj"}" /Fe"{exe}"\n'+
    f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'supply-client-test.log').open('wb') as log:
    result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'supply-client-test.log').read_text(errors='replace'))
result.check_returncode()
(P/'supply-client-test-result.json').write_text(json.dumps(dict(passed=True,cases=4,actual_client=True,
    source_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
    header_sha256=hashlib.sha256((target.parent/'weapon_hand_client.h').read_bytes()).hexdigest(),
    game_observations='Synthetic local Host',game_execution=False),indent=2))
