"""Exact observer: passthrough, short circuit, missing stages, disabled/caps."""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'press-assets-r1';PG=P/'source/tools/playground'
source=r'''
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <cassert>
#include <initializer_list>
namespace wh {struct Frame{struct{std::uint64_t frame{1};}key;struct{std::int64_t sync_ns{500};}controllers;};}
FILE* log_file{};
namespace weaponhand {unsigned calls{};std::int64_t tick{1000};std::int64_t clock_ns(void*){++calls;return tick+=1000;}}
#include "weapon_hammerburst_press_timing.inl"
int main(){unsigned cases{};wh::Frame f;
 {HammerburstPressTiming t(true,&f);t.mark(2);assert(t.asset(true,0));}assert(!weaponhand::calls);++cases;
 assert(tmpfile_s(&log_file)==0&&log_file);
 {HammerburstPressTiming t(false,&f);t.mark(2);assert(!t.asset(false,1));}assert(!weaponhand::calls);++cases;
 {HammerburstPressTiming t(true,nullptr);t.mark(2);assert(t.asset(true,2));}assert(!weaponhand::calls);++cases;
 {HammerburstPressTiming t(true,&f);t.mark(2);
  assert(!t.asset(false,0));assert(!t.asset(false,1));assert(!t.asset(false,2));
  const auto n=weaponhand::calls;assert(!t.asset(false,99));assert(n==weaponhand::calls);
  for(unsigned phase:{3u,4u,5u,6u,7u})t.mark(phase);t.invoked=true;}
 assert(weaponhand::calls==10);++cases;
 for(unsigned failure=0;failure<3;++failure){
  const auto start=weaponhand::calls;unsigned evaluations{};
  {HammerburstPressTiming t(true,&f);t.mark(2);
   const auto operand=[&](unsigned stage){++evaluations;return stage==failure;};
   assert(t.asset(operand(0),0)||t.asset(operand(1),1)||t.asset(operand(2),2));
   assert(evaluations==failure+1);
   for(unsigned stage=0;stage<3;++stage)assert(bool(t.asset_stamp[stage])==(stage<=failure));}
  assert(weaponhand::calls-start==3+failure);++cases;
 }
 for(unsigned row=4;row<12;++row){HammerburstPressTiming t(true,&f);t.mark(2);}
 assert(HammerburstPressTiming::rows==12&&weaponhand::calls==38);++cases;
 for(unsigned row=0;row<20;++row){HammerburstPressTiming t(true,&f);t.mark(2);assert(t.asset(true,0));}
 assert(HammerburstPressTiming::rows==12&&weaponhand::calls==38);++cases;
 std::rewind(log_file);wchar_t line[2048]{};unsigned lines{};
 while(std::fgetws(line,2048,log_file)){
  if(!lines){assert(std::wcsstr(line,L"phase=7 invoked=1"));assert(std::wcsstr(line,L"asset_ns=4000 contract_ns=1000"));
   assert(std::wcsstr(line,L"asset_read_ns=1000 attachment_ns=1000 post_asset_current_ns=1000"));}
  if(lines==1)assert(std::wcsstr(line,L"asset_read_ns=1000 attachment_ns=-1 post_asset_current_ns=-1"));
  ++lines;}
 assert(lines==12);++cases;std::fclose(log_file);
 std::printf("HB exact press asset timing: %u cases passed, original short-circuit/booleans, max12 rows; no game\n",cases);
}
'''
target=PG/'hammerburst_press_assets_test.cpp';target.write_text(source);cmd=P/'press-assets-test.cmd';exe=P/'press-assets-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "press-assets-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'press-assets-test.log').open('wb') as log:result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'press-assets-test.log').read_text(errors='replace'));result.check_returncode()
(P/'press-assets-test-result.json').write_text(json.dumps(dict(passed=True,cases=10,exact_observer=True,clock_fixture=True,game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
