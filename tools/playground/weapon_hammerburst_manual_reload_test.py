"""Compile the actual manual-permission classifier; no engine/action mock grant."""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[2];A=R/'analysis/hammerburst-20261108';D=A/'manual-permission-test';D.mkdir(exist_ok=True)
source=Path(sys.argv[1]) if len(sys.argv)>1 else R/'tools/playground/weapon_lancer_manual_reload.inl';text=source.read_text();start=text.index('bool requires_permission(');opening=text.index('{',start);depth=0
for i in range(opening,len(text)):
    if text[i]=='{':depth+=1
    elif text[i]=='}':
        depth-=1
        if not depth:classifier=text[start:i+1];break
else:raise AssertionError('Unclosed actual requires_permission')
host=r'''
#include "weapon_hand_participant.h"
#include <cstdio>
#include <cwchar>
namespace hand=gears_weapon_hand;
bool hb_enabled{},bol_enabled{},snub_enabled{};
namespace hammerburst_runtime {bool manual_enabled() noexcept{return hb_enabled;}}
namespace boltok_runtime {bool manual_enabled() noexcept{return bol_enabled;}}
namespace snub_runtime {bool manual_enabled() noexcept{return snub_enabled;}}
namespace playground {
bool instance(std::uint64_t id,wchar_t const* name) noexcept {
 static const wchar_t* classes[]={L"Unknown",L"Weap_AssaultRifle",L"Weap_Shotgun",L"Weap_Bow_base",L"Weap_Boomer_base",L"Weap_SniperRifle",L"Weap_LocustPistol",L"Weap_COGPistol",L"Weap_LocustAssaultRifle"};
 return id<9&&!std::wcscmp(classes[id],name);
}}
'''
tests=r'''
int main(){unsigned cases{},failed{};
 for(unsigned flags=0;flags<8;++flags){hb_enabled=bool(flags&1);bol_enabled=bool(flags&2);snub_enabled=bool(flags&4);
  for(unsigned family=0;family<9;++family){hand::Frame frame{};frame.weapon.address=family;
   const bool expected=(family>=1&&family<=5)||(family==6&&bol_enabled)||(family==7&&snub_enabled)||(family==8&&hb_enabled);
   const bool actual=requires_permission(frame);++cases;
   if(expected!=actual){++failed;std::printf("mismatch family=%u flags=%u expected=%u actual=%u\n",family,flags,unsigned(expected),unsigned(actual));}
  }
 }
 std::printf("cases=%u mismatches=%u; classifier only, no runtime or skin acceptance\n",cases,failed);return failed?1:0;
}
'''
(D/'main.cpp').write_text(host+classifier+'\n'+tests);headers=R/'analysis/hammerburst-20261108/c0-mixed-skin-probe/source/tools/vr-common';(D/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(HBManualPermission LANGUAGES CXX)\nadd_executable(HBManualPermission main.cpp)\ntarget_compile_features(HBManualPermission PRIVATE cxx_std_17)\ntarget_compile_options(HBManualPermission PRIVATE /W4 /WX /EHsc)\ntarget_include_directories(HBManualPermission PRIVATE "'+headers.as_posix()+'")\n')
commands=[['E:/CMake/bin/cmake.exe','-S',str(D),'-B',str(D/'build-vs'),'-G','Visual Studio 18 2026','-A','x64','-DCMAKE_GENERATOR_INSTANCE=E:/vs'],['E:/CMake/bin/cmake.exe','--build',str(D/'build-vs'),'--config','Release','--target','HBManualPermission','--parallel','2']]
for label,command in zip(['configure','build'],commands):
    with (D/(label+'.log')).open('wb') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    assert result.returncode==0,(label,result.returncode)
result=subprocess.run([str(D/'build-vs/Release/HBManualPermission.exe')],capture_output=True,text=True);(D/'test.log').write_text(result.stdout+result.stderr);report=dict(actual_source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),classifier_sha256=hashlib.sha256(classifier.encode()).hexdigest(),configure_exit=0,build_exit=0,test_exit=result.returncode,output=result.stdout,engine_queries=False,game_started=False,acceptance='Classifier membership only; real CanReload callback and Steam gameplay remain unverified');(D/'result.json').write_text(json.dumps(report,indent=2));print(result.stdout,flush=True);raise SystemExit(result.returncode)
