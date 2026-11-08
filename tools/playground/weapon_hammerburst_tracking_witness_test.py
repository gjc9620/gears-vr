"""Actual withdrawal/release blocks with a real magazine extraction witness.

Engine cleanup/stow calls are bounded fakes. This establishes conditional
adapter behavior, not whether a particular live Native lifecycle reaches it.
"""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[2];O=R/'analysis/hammerburst-20261108';O.mkdir(parents=True,exist_ok=True)
source=(Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('weapon_hammerburst_runtime.inl')).resolve()
tag=sys.argv[2] if len(sys.argv)>2 else 'tracking-witness-after';text=source.read_text(encoding='utf-8')
def block(start):
    begin=text.index(start);opening=text.index('{',begin);depth=0
    for i in range(opening,len(text)):
        if text[i]=='{':depth+=1
        elif text[i]=='}':
            depth-=1
            if not depth:return text[begin:i+1]
    raise AssertionError(start)
lost=block('    if(!f.left_tracked){')
unavailable=block('        if(stage!=Stage::idle){held={};stroke.reset();insertion.reset();')
release=block('    if(!f.left_grip_down){\n        released=true;held={};stroke.reset();')
host=r'''
#include "weapon_hand_participant.h"
#include "weapon_hammerburst_magazine.h"
#include <cstdio>
namespace hand=gears_weapon_hand;
enum class Stage {idle,old_magazine,new_magazine,waiting_ammo,recovering};Stage stage;
hand::Lease held{};bool released{};std::uint64_t old_item{1},new_item{2};
gears_weapon::HammerburstMagazineExtraction extraction;
struct Reset {void reset(){}}stroke,insertion;
unsigned cleanups{},stows{},visual_stows{},item_releases{};
struct Opening {struct Identity {hand::ObjectIdentity weapon{};}identity;}opening;
namespace lancerreload_runtime {bool weapon_retired(hand::ObjectIdentity const&){return false;}}
void cleanup(unsigned){++cleanups;stage=Stage::recovering;}
struct Transaction {bool stow_original(std::uint64_t){++stows;return true;}
 bool release_item(std::uint64_t){++item_releases;return true;}}transaction;
namespace visual {bool stow(){++visual_stows;return true;}}
'''
functions='bool lost_tracking(hand::Frame const& f){\n'+lost+'\nreturn false;\n}\n'
functions+='bool unavailable_frame(){\n'+unavailable+'\nreturn false;\n}\n'
functions+='void release_step(hand::Frame const& f){if(!f.left_grip_known)return;\n'+release+'\n}\n'
tests=r'''
int main(){
 unsigned cases{};
 const auto check=[&](bool yes,char const* label){++cases;if(!yes)std::printf("FAIL %s\n",label);return yes;};
 const auto reset=[&](){stage=Stage::old_magazine;held={hand::LeaseKind::magazine,0,1,1};released=false;
  cleanups=stows=visual_stows=item_releases=0;new_item=2;extraction.reset();
  gears_weapon::FeedPortMount mount{};
  gears_weapon::FeedSample sample{1,1,1,1,1,1000000000,{{0,0,0},{0,0,0,1}},{{0,0,0},{0,0,0,1}},true,true};
  if(!extraction.observe(sample,mount))return false;
  sample.sequence=2;sample.monotonic_ns+=40000000;sample.left_grip.position.z=.06;
  return extraction.observe(sample,mount)&&extraction.complete();
 };
 hand::Frame f{};f.left_grip_known=1;
 if(!reset())return 2;f.left_tracked=0;lost_tracking(f);
 if(!check(!extraction.complete()&&!held.id&&!released,"tracking loss clears completed extraction proof"))return 1;
 release_step(f);
 if(!check(cleanups==1&&!stows&&!visual_stows&&stage==Stage::recovering,"later UP cannot stow from lost proof"))return 1;
 if(!reset())return 2;unavailable_frame();
 if(!check(!extraction.complete()&&!held.id&&!released,"unavailable current frame clears proof"))return 1;
 release_step(f);
 if(!check(cleanups==1&&!stows&&!visual_stows,"later UP cannot stow after unavailable frame"))return 1;
 if(!reset())return 2;f.left_tracked=1;lost_tracking(f);
 if(!check(extraction.complete()&&held.id==1&&!cleanups,"valid tracking does not withdraw"))return 1;
 release_step(f);
 if(!check(stows==1&&visual_stows==1&&!cleanups&&stage==Stage::waiting_ammo,"continuous extraction followed by real UP still stows"))return 1;
 if(!reset())return 2;stage=Stage::new_magazine;f.left_tracked=0;lost_tracking(f);
 if(!check(!extraction.complete()&&item_releases==1&&visual_stows==1&&stage==Stage::waiting_ammo&&!new_item,"replacement loss retains existing reservation release behavior"))return 1;
 std::printf("HB actual withdrawal/release blocks: %u extraction invalidation checks passed; engine calls faked\n",cases);
}
'''
cpp=O/(tag+'.cpp');exe=O/(tag+'.exe');cmd=O/(tag+'.cmd');cpp.write_text(host+functions+tests,encoding='utf-8')
cmd.write_text('@echo off\ncall E:\\vs\\VC\\Auxiliary\\Build\\vcvars64.bat >nul\ncl /nologo /std:c++17 /EHsc /W4 /WX /I"'+str(R/'tools/vr-common')+'" /Fe:"'+str(exe)+'" /Fo:"'+str(O/(tag+'.obj'))+'" "'+str(cpp)+'"\nif errorlevel 1 exit /b %errorlevel%\n"'+str(exe)+'"\n',encoding='ascii')
r=subprocess.run(['cmd','/c',str(cmd)],capture_output=True,encoding='utf-8',errors='replace')
receipt=dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),branches_sha256=hashlib.sha256(functions.encode()).hexdigest(),exit_code=r.returncode,output=r.stdout+r.stderr,scope=__doc__)
(O/(tag+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8');(O/(tag+'.json')).write_text(json.dumps(receipt,indent=2),encoding='utf-8');print(json.dumps(receipt,indent=2));sys.exit(r.returncode)
