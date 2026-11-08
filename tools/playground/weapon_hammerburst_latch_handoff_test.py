"""Actual production release/opening blocks, real stroke and grant predicates.

Engine calls are faked. This tests interruption/anti-replay, not live timing.
"""
from pathlib import Path
import subprocess, json, hashlib, sys
R=Path(__file__).resolve().parents[2]; O=R/'analysis/hammerburst-20261108'; O.mkdir(parents=True,exist_ok=True)
source=(Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('weapon_hammerburst_runtime.inl')).resolve(); text=source.read_text(encoding='utf-8')
tag=sys.argv[2] if len(sys.argv)>2 else 'latch-handoff'
assert tag.isascii() and all(c.isalnum() or c in '-_' for c in tag)
def block(start):
    begin=text.index(start); opening=text.index('{',begin); depth=0
    for i in range(opening,len(text)):
        if text[i]=='{':depth+=1
        elif text[i]=='}':
            depth-=1
            if not depth:return text[begin:i+1]
    raise AssertionError(start)
grant=block('bool granted(')
release=block('    if(!f.left_grip_down){\n        released=true;held={};stroke.reset();')
opening=block('    if(stage==Stage::opening||stage==Stage::closing){')
host=r'''
#include "weapon_hand_participant.h"
#include "weapon_hammerburst_latch.h"
#include <cstdio>
namespace hand=gears_weapon_hand;
enum class Stage {opening,closing,opened,old_magazine,new_magazine,waiting_ammo,completing,recovering};
Stage stage=Stage::opening;
hand::Lease held{};bool released{};double angle{};unsigned seeks{},latch_rejection_logs{},latch_stationary_holds{};
std::uint64_t serial{1},old_item{1},new_item{};FILE* log_file{};
std::int64_t last_latch_seek_elapsed_ns{};
namespace weaponhand {std::int64_t clock_ns(void*){return 0;}}
gears_weapon::HammerburstLatchStroke stroke;
struct Geometry {gears_weapon::HammerburstLatchGeometry latch;}geometry;
gears_weapon::HammerburstLatchSample input{};
bool hand_sample(hand::Frame const&,gears_weapon::HammerburstLatchSample& out){out=input;return true;}
void cleanup(unsigned){stage=Stage::recovering;}
std::uint64_t animation_serial(){return serial;}
namespace anim {bool seek_latch_angle(hand::Frame const&,std::uint64_t,double){++seeks;return true;}}
namespace visual {bool stow(){return true;}}
struct Extraction {bool complete(){return true;}}extraction;
struct Transaction {
 bool stow_original(std::uint64_t){return true;} bool release_item(std::uint64_t){return true;}
 bool confirm_closed(std::int64_t,std::int64_t){return true;} bool take_request(unsigned&){return true;}
 unsigned inserted(){return 0;} unsigned target(){return 0;}
}transaction;unsigned completion{};
namespace lancerreload_state {
 struct Lease {std::int64_t perfect_end_ms{},accepted_begin_ms{};}lease;
 bool closure_time(hand::Frame const&,std::uint64_t,std::int64_t&){return false;}
}
'''
step='void step(hand::Frame const& f,hand::Decision const& decision){\nif(!f.left_grip_known)return;\n'+release+'\n'+opening+'\n}\n'
tests=r'''
int main(){
 unsigned cases{};
 const auto check=[&](bool yes,char const* label){++cases;if(!yes)std::printf("FAIL %s\n",label);return yes;};
 hand::Frame f{};f.key.lifecycle=7;f.press_token=1;f.left_grip_known=f.left_grip_down=1;
 hand::Decision d{};d.key=f.key;d.release_required=0;d.status=hand::DecisionStatus::retained;
 d.lease={hand::LeaseKind::magazine,0,7,1};held=d.lease;
 geometry.latch={{0,0,0},{1,0,0},{0,.07,0},.84,.035,.035,.03,true,100};
 const auto sample=[&](double a,std::uint64_t ms,std::uint64_t token){
  input={{0,.07*std::cos(a),.07*std::sin(a)},ms,ms,token,true,true};
 };
 angle=.5;sample(angle,1000,1);if(!stroke.begin(geometry.latch,input,angle))return 2;
 sample(.55,1101,1);step(f,d);
 if(!check(!stroke.active()&&!held.id&&!released&&angle==.5&&!seeks&&stroke.last_rejection()==gears_weapon::HammerburstLatchRejection::gap,"101ms gap revokes without advancing"))return 1;
 sample(.6,1150,1);step(f,d);
 if(!check(!stroke.active()&&!seeks,"same retained press cannot resume"))return 1;
 f.left_grip_known=0;f.left_grip_down=0;step(f,d);
 if(!check(!released,"unknown UP is not release"))return 1;
 f.left_grip_known=1;step(f,d);
 if(!check(released&&!stroke.active()&&!seeks,"actual known release rearms only"))return 1;
 f.left_grip_down=1;f.press_token=2;d.lease.id=2;d.new_grant=0;
 sample(.5,1200,2);step(f,d);
 if(!check(!stroke.active()&&!seeks,"retained new token is not fresh grant"))return 1;
 d.status=hand::DecisionStatus::granted;d.new_grant=1;d.release_required=1;step(f,d);
 if(!check(!stroke.active(),"release-required cannot regrab"))return 1;
 d.release_required=0;sample(1.2,1210,2);step(f,d);
 if(!check(!stroke.active(),"fresh grant away from actual mechanism cannot regrab"))return 1;
 sample(.5,1220,2);step(f,d);
 if(!check(stroke.active()&&held.id==2&&!released&&!seeks&&angle==.5,"fresh actual-position regrab does not seek"))return 1;
 d.status=hand::DecisionStatus::retained;d.new_grant=0;sample(.6,1300,2);step(f,d);
 if(!check(stroke.active()&&std::abs(angle-.6)<1e-9&&seeks==1,"later continuous retained motion resumes"))return 1;
 sample(.84,1380,2);step(f,d);
 if(!check(stage==Stage::opened&&!stroke.active()&&seeks==2,"actual resumed endpoint reaches opened"))return 1;
 const auto stationary=[&](){
  stage=Stage::opening;angle=.5;released=false;seeks=0;latch_stationary_holds=0;
  f.left_tracked=f.left_grip_known=f.left_grip_down=1;f.press_token=2;
  d.key=f.key;d.release_required=0;d.status=hand::DecisionStatus::retained;d.new_grant=0;
  d.lease={hand::LeaseKind::magazine,0,f.key.lifecycle,2};held=d.lease;
  sample(.5,1500,2);if(!stroke.begin(geometry.latch,input,angle))return false;
  sample(.5,1617,2);return true;
 };
 if(!stationary())return 2;step(f,d);
 if(!check(stroke.active()&&held.id==2&&!released&&angle==.5&&!seeks&&stage==Stage::opening&&latch_stationary_holds==1,"same current retained lease renews stationary baseline without motion"))return 1;
 sample(.6,1697,2);step(f,d);
 if(!check(std::abs(angle-.6)<1e-9&&seeks==1,"only later timely motion can advance"))return 1;
 if(!stationary())return 2;d.status=hand::DecisionStatus::granted;d.new_grant=1;step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"fresh grant cannot borrow retained stationary exception"))return 1;
 if(!stationary())return 2;held.id=1;step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"different held lease cannot renew baseline"))return 1;
 if(!stationary())return 2;input.hand.x=.0001;step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"even0.1mm movement cannot cross the100ms gap"))return 1;
 if(!stationary())return 2;input.hand.x=.000001;step(f,d);
 if(!check(stroke.active()&&angle==.5&&!seeks,"rounding-level position noise never advances motion"))return 1;
 if(!stationary())return 2;stroke.reset();held={};step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"explicit withdrawal cannot revive from an old retained press"))return 1;
 if(!stationary())return 2;f.left_tracked=0;step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"untracked frame cannot retain stationary baseline"))return 1;
 if(!stationary())return 2;--held.lifecycle;step(f,d);
 if(!check(!stroke.active()&&!held.id&&!seeks,"foreign lifecycle cannot retain stationary baseline"))return 1;
 if(!stationary())return 2;d.release_required=1;step(f,d);
 if(!check(stroke.last_observation_ms()==1500&&!seeks&&!latch_stationary_holds,"release-required decision cannot renew observation"))return 1;
 if(!stationary())return 2;++d.key.lifecycle;step(f,d);
 if(!check(stroke.last_observation_ms()==1500&&!seeks&&!latch_stationary_holds,"foreign frame cannot renew observation"))return 1;
 std::printf("HB exact opening/release blocks: %u recovery and anti-replay checks passed; engine calls faked\n",cases);
}
'''
cpp=O/(tag+'.cpp');exe=O/(tag+'.exe');cmd=O/(tag+'.cmd')
cpp.write_text(host+grant+'\n'+step+tests,encoding='utf-8')
cmd.write_text('@echo off\ncall E:\\vs\\VC\\Auxiliary\\Build\\vcvars64.bat >nul\ncl /nologo /std:c++17 /EHsc /W4 /WX /I"'+str(R/'tools/vr-common')+'" /Fe:"'+str(exe)+'" /Fo:"'+str(O/(tag+'.obj'))+'" "'+str(cpp)+'"\nif errorlevel 1 exit /b %errorlevel%\n"'+str(exe)+'"\n',encoding='ascii')
r=subprocess.run(['cmd','/c',str(cmd)],capture_output=True,encoding='utf-8',errors='replace')
receipt=dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),branch_sha256=hashlib.sha256((grant+step).encode()).hexdigest(),exit_code=r.returncode,output=r.stdout+r.stderr,scope=__doc__)
(O/(tag+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
(O/(tag+'.json')).write_text(json.dumps(receipt,indent=2),encoding='utf-8');print(json.dumps(receipt,indent=2));sys.exit(r.returncode)

