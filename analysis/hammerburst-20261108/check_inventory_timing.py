from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'inventory-timing-r1';COMMON=P/'source/tools/vr-common'
source=r'''
#include "weapon_hand_participant.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
namespace abi=gears_weapon_hand;
FILE* log_file{};const wchar_t* inventory_stage=L"current_after";
unsigned clock_reads{},withdrawal_reads{};std::int64_t tick=1000;
std::int64_t clock_ns(void*){++clock_reads;return tick+=100;}
namespace weaponhand {
 struct W{std::uint64_t frame{1},sequence{2};unsigned failure{5},game_difference{};std::int64_t input_age_ns{101000000};};
 struct Client{W value;W withdrawal_on_gt(){++withdrawal_reads;return value;}}client;
}
#include "weapon_hammerburst_inventory_timing.inl"
int main(){unsigned cases{};abi::Frame f;f.key.frame=1;f.controllers.sequence=2;f.controllers.sync_ns=500;
 {HammerburstInventoryTiming t(f);t.mark(1);}assert(!clock_reads&&!withdrawal_reads);++cases;
 assert(tmpfile_s(&log_file)==0&&log_file);
 for(unsigned i=0;i<4;++i){HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);}
 assert(clock_reads==20&&withdrawal_reads==4);++cases;
 for(unsigned i=0;i<8;++i){HammerburstInventoryTiming t(f);t.mark(1);}
 assert(clock_reads==20&&withdrawal_reads==4);++cases;
 f.controllers.hands[1].squeeze_active=1;f.controllers.hands[1].squeeze=1;
 for(unsigned i=0;i<12;++i){HammerburstInventoryTiming t(f);t.mark(1);t.mark(2);t.mark(3);t.mark(4);t.mark(99);}
 assert(clock_reads==92&&withdrawal_reads==16);++cases;
 for(unsigned i=0;i<12;++i){HammerburstInventoryTiming t(f);t.mark(1);}
 assert(clock_reads==92&&withdrawal_reads==16);++cases;
 std::rewind(log_file);wchar_t line[2048]{};unsigned rows{};
 while(std::fgetws(line,2048,log_file)){if(rows==0){assert(std::wcsstr(line,L"phase=3 stage=current_after"));
  assert(std::wcsstr(line,L"snapshot_ns=100 binding_ns=100 current_after_ns=100"));
  assert(std::wcsstr(line,L"withdrawal_same=1 withdrawal_failure=5"));}++rows;}
 assert(rows==16);++cases;std::fclose(log_file);
 std::printf("HB exact inventory timing observer: %u local cases;4 idle+12 down rows, disabled clocks/withdrawals0; no game\n",cases);
}
'''
target=P/'source/tools/playground/hammerburst_inventory_timing_test.cpp';target.write_text(source)
cmd=P/'inventory-timing-test.cmd';exe=P/'inventory-timing-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
 f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{COMMON}" "{target}" /Fo"{P / "inventory-timing-test.obj"}" /Fe"{exe}"\n'+
 f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'timing-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'timing-test.log').read_text(errors='replace'));result.check_returncode()
(P/'timing-test-result.json').write_text(json.dumps(dict(passed=True,cases=6,exact_observer=True,local_clock_and_client_fixture=True,
 game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
