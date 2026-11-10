"""Exact inserted scheduling block, real identity comparison, local call spies."""
from pathlib import Path
import hashlib,json,subprocess
import prepare_reload_priority as prepare
P=prepare.P;COMMON=P/'source/tools/vr-common'
text=(P/'source/tools/playground/weapon_equipment_backend.inl').read_text()
assert text.count(prepare.BLOCK)==1
assert prepare.PREDICATE in (P/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
assert text.index(prepare.BLOCK)<text.index('    const auto timing_follow=clock_ns(nullptr);')
source=r'''
#include "weapon_hand_participant.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <vector>
namespace abi=gears_weapon_hand;
FILE* log_file{};std::vector<unsigned> order;unsigned hb_calls{};bool hb_result=true;
namespace nativelancerfollow {bool bound_is_hammerburst=true,bound_key_known=true;
 struct Key{abi::ObjectIdentity weapon{};std::uint64_t hold_generation{1};}bound_key;}
namespace vrequip {struct Model{std::uint64_t hold_generation(){return 1;}}model;}
namespace hammerburst_runtime {namespace hand=abi;enum class Stage{idle,active};Stage stage=Stage::idle;
'''+prepare.PREDICATE+r'''
 bool blocks_fire(abi::ObjectIdentity const&){return stage!=Stage::idle;}
 bool tick(std::uint64_t){++hb_calls;order.push_back(4);return hb_result;}}
int main(){unsigned cases{};
 for(unsigned variant=0;variant<9;++variant){
  abi::Frame frame;frame.weapon.address=1;frame.left_grip_down=1;const std::uint64_t pc=1;
  bool reload_ticked_early=false;hb_calls=0;order.clear();hb_result=true;
  hammerburst_runtime::stage=hammerburst_runtime::Stage::idle;
  nativelancerfollow::bound_is_hammerburst=nativelancerfollow::bound_key_known=true;
  nativelancerfollow::bound_key.weapon=frame.weapon;nativelancerfollow::bound_key.hold_generation=1;
  switch(variant){
   case 1:reload_ticked_early=true;break;
   case 2:nativelancerfollow::bound_is_hammerburst=false;break;
   case 3:nativelancerfollow::bound_key_known=false;break;
   case 4:++frame.weapon.address;break;
   case 5:++nativelancerfollow::bound_key.hold_generation;break;
   case 6:frame.left_grip_down=0;break;
   case 7:frame.left_grip_down=0;hammerburst_runtime::stage=hammerburst_runtime::Stage::active;break;
   case 8:hb_result=false;break;
  }
  order.push_back(1); // Existing fire occurs before this block.
'''+prepare.BLOCK+r'''
  order.push_back(2);order.push_back(3); // Follow tick/apply.
  if(!reload_ticked_early){++hb_calls;order.push_back(4);} // Original late path.
  if(variant==1){assert(hb_calls==0&&(order==std::vector<unsigned>{1,2,3}));}
  else if(variant==0||variant==7||variant==8){assert(hb_calls==1&&(order==std::vector<unsigned>{1,4,2,3}));}
  else {assert(hb_calls==1&&(order==std::vector<unsigned>{1,2,3,4}));}
  ++cases;
 }
 std::printf("HB exact reload-priority block: %u local schedule cases; failed attempt once, other kind/hold/order unchanged; no game\n",cases);
}
'''
target=P/'source/tools/playground/hammerburst_reload_priority_test.cpp';target.write_text(source)
cmd=P/'reload-priority-test.cmd';exe=P/'reload-priority-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
 f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{COMMON}" "{target}" /Fo"{P / "priority-test.obj"}" /Fe"{exe}"\n'+
 f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'priority-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'priority-test.log').read_text(errors='replace'));result.check_returncode()
(P/'priority-test-result.json').write_text(json.dumps(dict(passed=True,cases=9,exact_block=True,local_call_spies=True,
 game_execution=False,source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
