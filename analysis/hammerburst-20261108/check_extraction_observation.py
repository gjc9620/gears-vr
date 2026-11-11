from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'extraction-observe-r1'
s=(P/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
helper=s[s.index('void observe_original_extraction('):s.index('gears_weapon::FeedResult insertion_step(')]
assert helper.count('extraction.observe(sample,feed)')==1
assert 'if(stage==Stage::old_magazine&&!extraction.complete())observe_original_extraction(' in s
code=r'''
#include "weapon_hand_client.h"
#include "weapon_hammerburst_magazine.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
gears_weapon::FeedPortMount feed{};
FILE* log_file{};
struct Proxy {
 gears_weapon::HammerburstMagazineExtraction value;unsigned calls{};
 bool observe(gears_weapon::FeedSample const& s,gears_weapon::FeedPortMount const& m){++calls;return value.observe(s,m);}
 bool complete()const{return value.complete();}
} extraction;
@HELPER@
int main(){
 assert(tmpfile_s(&log_file)==0&&log_file);gears_weapon::HammerburstMagazineExtraction oracle;
 hand::Frame frame{};unsigned checks{};
 for(unsigned i=1;i<=45;++i){
  gears_weapon::FeedSample sample{1,2,3,4,i,1000000000+std::int64_t(i)*20000000,
    {{0,0,0},{0,0,0,1}},{{0,0,0},{0,0,0,1}},true,true};
  if(i>=2&&i<=3)sample.left_grip.position.z=.025*(i-1);
  if(i==4)sample.tracked=false;
  if(i==6)sample.monotonic_ns+=101000000;
  if(i==8)sample.left_grip.position.x=.1;
  frame.key.frame=i;frame.controllers.sequence=i;frame.controllers.sync_ns=sample.monotonic_ns;
  const auto expected=oracle.observe(sample,feed);(void)expected;
  observe_original_extraction(frame,2,sample,20000000,sample.left_grip.position.z,sample.left_grip.position.x,1);
  assert(extraction.calls==i&&extraction.complete()==oracle.complete());++checks;
 }
 rewind(log_file);unsigned lines{};wchar_t line[1024];while(fgetws(line,1024,log_file))++lines;
 assert(lines==32);fclose(log_file);log_file=nullptr;
 std::printf("Exact extraction wrapper: %u call/result checks, original continuous/invalid samples and32-row cap\n",checks);
}
'''.replace('@HELPER@',helper)
(P/'observe-test.cpp').write_text(code)
cmd=P/'observe-test.cmd'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{P / "source/tools/vr-common"}" "{P / "observe-test.cpp"}" /Fo"{P / "observe-test.obj"}" /Fe"{P / "observe-test.exe"}"\nif errorlevel 1 exit /b %errorlevel%\n"{P / "observe-test.exe"}"\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{P / "source/tools/vr-common/weapon_hammerburst_magazine_test.cpp"}" /Fo"{P / "magazine-test.obj"}" /Fe"{P / "magazine-test.exe"}"\nif errorlevel 1 exit /b %errorlevel%\n"{P / "magazine-test.exe"}"\nexit /b %errorlevel%\n')
with (P/'observe-test.log').open('wb') as log:subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
v=dict(exit_code=0,wrapper_checks=45,log_rows=32,original_extraction_test_passed=True,game_started=False,source_sha256=hashlib.sha256(s.encode()).hexdigest(),transcript=(P/'observe-test.log').read_text(errors='replace'))
(P/'observe-test-result.json').write_text(json.dumps(v,indent=2));print(v['transcript'])
