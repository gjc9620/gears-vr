"""Execute exact scheduler with L2/Input/dispatcher call-order spies.

Engine/actual Input policy are not exercised by this fixture. Source parity
separately proves the complete existing Input and delivery blocks are retained.
"""
from pathlib import Path
import hashlib,json,subprocess
from prepare_first_priority import BASE,P,REL,transform
before=(BASE/'source'/REL).read_text();after,block,body,tail=transform(before)
assert (P/'source'/REL).read_text()==after
assert after.index('input.advance(',after.index('hb_first_before_aim='))<after.index('lancerrefire::deliver_ready(frame)',after.index('hb_first_before_aim='))<after.index(tail)
assert before.count('input.advance(')==after.count('input.advance(')
spy_block=block.replace(body,'        calls.push_back(1);aim_clock=aim_now;return !aim_fault;\n')
spy_block=spy_block.replace('    bool part_input_reserved{},aim_allowed{};\n','').replace('    gears_weapon::OriginalL2Status aim_result{};\n','')
assert spy_block!=block
source=r'''
#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
#define GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL 1
std::vector<int> calls;bool owned=true,hb=true,debt=true,aim_fault=false;
std::int64_t aim_clock{};
namespace weaponhand {std::int64_t clock_ns(void*){return calls.empty()?37:99;}}
namespace hammerburst_burst_event {bool targets(std::uint64_t){return hb;}}
namespace lancerrefire {bool has_initial_debt(){return debt;}}
struct Input {bool owns_press(){return owned;}}input;
struct Frame {struct Weapon {std::uint64_t address{1};}weapon;}frame;
void run(bool have_source,bool input_accepts=true,bool input_aborts=false){
'''+spy_block+r'''
    calls.push_back(2); // Unmodified Input policy seam: all gates precede dispatch.
    if(input_aborts)return;
    calls.push_back(3); // Original after_tick seam.
    if(input_accepts)calls.push_back(4); // Original delivery seam.
'''+tail+r'''
    calls.push_back(5);
}
void reset(){calls.clear();owned=hb=debt=true;aim_fault=false;aim_clock=0;}
int main(){unsigned cases{};
 reset();run(true);assert((calls==std::vector<int>{2,3,4,1,5})&&aim_clock==99);++cases;
 for(unsigned gate=0;gate<4;++gate){reset();if(gate==0)owned=false;if(gate==1)hb=false;if(gate==2)debt=false;
 run(gate!=3);assert((calls==std::vector<int>{1,2,3,4,5})&&aim_clock==37);++cases;}
 reset();aim_fault=true;run(true);assert((calls==std::vector<int>{2,3,4,1}));++cases;
 reset();hb=false;aim_fault=true;run(true);assert((calls==std::vector<int>{1}));++cases;
 reset();run(true,false);assert((calls==std::vector<int>{2,3,1,5}));++cases;
 reset();run(true,false,true);assert((calls==std::vector<int>{2}));++cases;
 reset();hb=false;run(true,false,true);assert((calls==std::vector<int>{1,2}));++cases;
 std::printf("HB exact first-priority scheduler: %u call-order cases passed; engine and Input are spies\n",cases);
}
'''
target=P/'first-priority-scheduler-test.cpp';target.write_text(source)
cmd=P/'scheduler-test.cmd';exe=P/'scheduler-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "scheduler-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'scheduler-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'scheduler-test.log').read_text(errors='replace'));result.check_returncode()
(P/'scheduler-test-result.json').write_text(json.dumps(dict(passed=True,cases=10,exact_scheduler=True,unchanged_production_blocks=True,full_input_before_delivery=True,game_execution=False,limits='L2/Input/delivery are call-order spies, not engine/source/anti-replay proof',source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
