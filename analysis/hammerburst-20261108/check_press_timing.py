"""Exact observer fixture; verify no disabled clock reads and bounded output."""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'press-timing-r1'
source=r'''
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cassert>
namespace wh {struct Frame{struct{std::uint64_t frame{1};}key;struct{std::int64_t sync_ns{500};}controllers;};}
FILE* log_file{};
namespace weaponhand {unsigned calls{};std::int64_t tick{1000};std::int64_t clock_ns(void*){++calls;return tick+=1000;}}
#include "weapon_hammerburst_press_timing.inl"
int main(){unsigned cases{};wh::Frame f;
 {HammerburstPressTiming t(true,&f);t.mark(2);}assert(weaponhand::calls==0);++cases;
 assert(tmpfile_s(&log_file)==0&&log_file);
 {HammerburstPressTiming t(false,&f);t.mark(2);}assert(weaponhand::calls==0);++cases;
 {HammerburstPressTiming t(true,nullptr);t.mark(2);}assert(weaponhand::calls==0);++cases;
 {HammerburstPressTiming t(true,&f);for(unsigned n:{2u,3u,4u,5u,6u,7u})t.mark(n);t.invoked=true;}
 assert(weaponhand::calls==7);++cases;
 for(unsigned i=1;i<12;++i){HammerburstPressTiming t(true,&f);t.mark(2);t.mark(99);}
 assert(HammerburstPressTiming::rows==12&&weaponhand::calls==29);++cases;
 for(unsigned i=0;i<20;++i){HammerburstPressTiming t(true,&f);t.mark(2);}
 assert(HammerburstPressTiming::rows==12&&weaponhand::calls==29);++cases;
 std::rewind(log_file);wchar_t line[2048]{};unsigned lines{};
 while(std::fgetws(line,2048,log_file)){if(!lines){assert(std::wcsstr(line,L"phase=7 invoked=1"));
  assert(std::wcsstr(line,L"asset_ns=1000 contract_ns=1000 button_schema_ns=1000 final_reserve_ns=1000 owned_and_dispatch_ns=1000"));}
  ++lines;}
 assert(lines==12);++cases;std::fclose(log_file);
 std::printf("HB exact first-press timing observer: %u fixture cases, max12 rows, no disabled clock reads; no game\n",cases);
}
'''.replace('#include <cassert>','#include <cassert>\n#include <initializer_list>')
target=P/'source/tools/playground/hammerburst_press_timing_test.cpp';target.write_text(source)
cmd=P/'press-timing-test.cmd';exe=P/'press-timing-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
 f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "timing-test.obj"}" /Fe"{exe}"\n'+
 f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'timing-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'timing-test.log').read_text(errors='replace'));result.check_returncode()
(P/'timing-test-result.json').write_text(json.dumps(dict(passed=True,cases=7,exact_observer=True,clock_fixture=True,
 game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
