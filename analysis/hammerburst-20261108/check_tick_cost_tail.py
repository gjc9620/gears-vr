from pathlib import Path
import json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'tick-cost-tail-r1';S=P/'source/tools/playground';BASE=A/'extraction-history-r1/source/tools/playground'
cpp=(S/'playground.cpp').read_text();r2=(S/'weapon_lancer_r2_native.inl').read_text()
start=cpp.index('        if(vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&nativelancerfollow::bound_is_hammerburst){')
end=cpp.index('        static unsigned equipment_work_logs{},equipment_work_observed{};',start);insert=cpp[start:end]
assert not any(x in insert for x in ('copy_frame','read_game','observe_source','execute(','invoke','controllers.','sync_ns='))
assert cpp[:start]+cpp[end:]==(BASE/'playground.cpp').read_text().replace('#include <array>','#include <array>\n#include "hb_tick_cost_tail.h"')
old='        else mode.store(stopped,std::memory_order_release);';new='        else {hb_tick_cost_tail::dump(log_file);mode.store(stopped,std::memory_order_release);}'
assert r2.replace(new,old)==(BASE/'weapon_lancer_r2_native.inl').read_text()
assert 'const bool quiet=weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready();' in r2
assert 'if(!lr::tick_admitted(quiet,pc,0,0))return;' in r2
code=r'''
#include "hb_tick_cost_tail.h"
#include <cassert>
#include <cwchar>
int main(){
 using namespace hb_tick_cost_tail;
 dump(nullptr);assert(total==0);
 for(unsigned n=1;n<=200;++n){Row r{};r.begin=1000*n;r.original_begin=r.begin+10;r.original_end=r.begin+20;
  r.equipment_begin=r.begin+30;r.equipment_end=r.begin+40;r.end=r.begin+50;
  r.last_input_sync=n;r.last_input_observed=n+1;r.captured=n+2;r.event=n;
  r.first_debt=n%2;r.cancel=n%3;r.count=n%7;r.owned=n%2;r.armed=1;
  r.fire_early=n%2;r.reload_early=n%3;r.pre_reload=n%4;record(r);}
 assert(total==200&&rows.size()==128);
 for(unsigned n=72;n<200;++n){auto const& r=rows[n%128];assert(r.event==n+1&&r.begin==1000*(n+1));
  assert(r.last_input_sync==n+1&&r.captured==n+3&&r.end-r.begin==50);}
 FILE* f=std::tmpfile();assert(f);dump(f);assert(total==200);std::rewind(f);
 wchar_t line[2048]{};assert(std::fgetws(line,2048,f)&&std::wcsstr(line,L"total=200 retained=128"));
 unsigned seen{};while(std::fgetws(line,2048,f)){++seen;
  assert(std::wcsstr(line,L"pre_original_ns=10 original_ns=10 service_ns=10 equipment_ns=10 follow_ns=10 total_ns=50 action=0"));
  if(seen==1)assert(std::wcsstr(line,L"ordinal=73 "));if(seen==128)assert(std::wcsstr(line,L"ordinal=200 "));}
 assert(seen==128);std::fclose(f);std::puts("Passive HB GT tail: original scalar retention,128-row wrap, ordering, phase decomposition, null dump and200-write/128-row export passed. No game or timing authority.");
}
'''
target=P/'tick-cost-test.cpp';target.write_text(code);cmd=P/'tick-cost-test.cmd';exe=P/'tick-cost-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /D_CRT_SECURE_NO_WARNINGS /EHsc /I"{S}" "{target}" /Fo"{P / "tick-cost-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'tick-cost-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'tick-cost-test.log').read_text(errors='replace'));r.check_returncode()
v=dict(passed=True,capacity=128,writes=200,exported=128,exact_removed_insertions_equal_original=True,gt_only_dump_before_stopped=True,new_engine_source_input_calls=0,new_action_or_gate_clock_uses=0,game_started=False,fixture_sha256=hashlib.sha256(code.encode()).hexdigest(),limits='Ring/scalar export and exact source delta checks, not runtime cost or race-detector evidence.')
(P/'tick-cost-test-result.json').write_text(json.dumps(v,indent=2))
proof=json.loads((P/'prepared.json').read_text())
with (P/'build-self-contained-header.log').open('wb') as log:subprocess.run(proof['build_command'],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
proof['dll_sha256']=hashlib.sha256(Path(proof['dll']).read_bytes()).hexdigest();proof['passive_ring_checks_passed']=True;(P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
