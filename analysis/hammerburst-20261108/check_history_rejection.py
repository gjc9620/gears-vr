from pathlib import Path
import json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'history-rejection-r1';S=P/'source/tools/playground';BASE=A/'tick-cost-tail-r1/source/tools'
# Run the original passive-tail fixture against this extended header, keeping
# the exact original source-reversal checks and adding protected-first checks.
script=(A/'check_tick_cost_tail.py').read_text().split('proof=json.loads')[0]
script=script.replace("P=A/'tick-cost-tail-r1'","P=A/'history-rejection-r1'")
script=script.replace("'#include <array>\\n#include \"hb_tick_cost_tail.h\"'", "'#include <array>\\n#include \"hb_tick_cost_tail.h\"\\n#include \"hb_history_rejection.h\"'")
script=script.replace('unsigned seen{};while(std::fgetws(line,2048,f)){++seen;', '''unsigned seen{},protected_seen{};while(std::fgetws(line,2048,f)){
  if(std::wcsstr(line,L"hammerburst_first_tick_cost ")){++protected_seen;
   if(protected_seen==1)assert(std::wcsstr(line,L"ordinal=1 "));
   if(protected_seen==64)assert(std::wcsstr(line,L"ordinal=127 "));continue;}
  if(std::wcsstr(line,L"hammerburst_first_tick_summary ")){assert(std::wcsstr(line,L"seen=100 retained=64 capacity=64 truncated=1"));continue;}
  ++seen;''')
script=script.replace('assert(seen==128);std::fclose(f);','''assert(seen==128&&protected_seen==64&&first_seen==100&&first_kept==64);std::fclose(f);
 Row no_event{};no_event.first_debt=1;no_event.captured=1;record(no_event);
 no_event.event=1;no_event.captured=0;record(no_event);assert(first_seen==100&&first_kept==64);''')
exec(compile(script,str(__file__),'exec'),{'__file__':str(__file__),'__name__':'__main__'})
source=(S/'weapon_hammerburst_latch_history.inl').read_text();old=(BASE/'playground/weapon_hammerburst_latch_history.inl').read_text()
start=source.index('    const bool built=relative::build(');end=source.index('    std::array<gears_weapon::HammerburstLatchSample,9> points{};',start)
replacement='    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;\n'
assert source[:start]+replacement+source[end:]==old
assert source[start:end].count('relative::build(')==1
assert (P/'source/tools/vr-common/controller_relative_motion.h').read_bytes()==(BASE/'vr-common/controller_relative_motion.h').read_bytes()
code=r'''
#include "hb_history_rejection.h"
#include <cassert>
#include <cwchar>
int main(){
 using namespace hb_history_rejection;using namespace gears_hand_continuity;
 TrackedInput a{},b{};a.input.sequence=100;b.input.sequence=102;
 a.input.sync_ns=1000;b.input.sync_ns=2000;a.stream_epoch=b.stream_epoch=1;
 MotionHistory h{};h.count=2;h.frames[0]=a;h.frames[0].input.sequence=101;h.frames[1]=b;
 for(auto* r:{&a,&b,&h.frames[0],&h.frames[1]})for(unsigned n=0;n<2;++n){
  r->tracking_epoch[n]=n+1;r->input.hands[n].grip.position.x=double(n)+.125;
  for(unsigned c=0;c<gears_vr::control_count;++c){r->input.hands[n].pressed[c]=c+1;r->input.hands[n].released[c]=c;}}
 capture(nullptr,12,102,gears_vr::control_squeeze,false,0,a,b,h,{},1);assert(records==0);
 FILE* f=std::tmpfile();assert(f);
 for(unsigned n=0;n<6;++n)capture(f,12+n,102,gears_vr::control_squeeze,false,0,a,b,h,{},1);
 assert(records==4&&a.input.sequence==100&&b.input.sync_ns==2000&&h.count==2);
 std::rewind(f);wchar_t line[8192]{};unsigned summaries{},raws{},hands{};
 while(std::fgetws(line,8192,f)){
  if(std::wcsstr(line,L"hammerburst_history_rejection ")){++summaries;assert(std::wcsstr(line,L"previous_sequence=100 current_sequence=102 history_count=2"));}
  if(std::wcsstr(line,L"hammerburst_history_raw "))++raws;
  if(std::wcsstr(line,L"hammerburst_history_hand ")){++hands;assert(std::wcsstr(line,L"p0=1 r0=0"));}
 }
 assert(summaries==4&&raws==16&&hands==32);std::fclose(f);
 std::puts("History rejection observer: null/no-write,4-record cap, original sequence/time/pose/control serialization and source non-mutation passed. Pure build unchanged.");
}
'''
target=P/'history-observer-test.cpp';target.write_text(code);cmd=P/'history-observer-test.cmd';exe=P/'history-observer-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /D_CRT_SECURE_NO_WARNINGS /EHsc /I"{S}" "{target}" /Fo"{P / "history-observer-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'history-observer-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'history-observer-test.log').read_text(errors='replace'));r.check_returncode()
v=dict(passed=True,rejection_cap=4,protected_first_capacity=64,tail_capacity=128,original_pure_build_unchanged=True,original_build_called_once=True,exact_history_rejection_boolean_preserved=True,new_source_queries=0,new_gate_clocks=0,game_started=False,limits='Passive serialization and source-delta/kernel identity checks; actual failed history and first-debt costs still need game evidence.')
(P/'history-observer-test-result.json').write_text(json.dumps(v,indent=2))
