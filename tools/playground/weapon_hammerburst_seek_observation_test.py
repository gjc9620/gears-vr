"""Compile actual HB observation reconciliation with the real object identity.

No engine reader or action is executed: fresh live observation and dispatch
remain integration obligations. Rejected observations must not retire a seek.
"""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[2];O=R/'analysis/hammerburst-20261108';O.mkdir(parents=True,exist_ok=True)
source=(Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).with_name('weapon_hammerburst_animation.inl')).resolve()
s=source.read_text(encoding='utf-8')
def block(start):
    begin=s.index(start);opening=s.index('{',begin);depth=0
    for i in range(opening,len(s)):
        if s[i]=='{':depth+=1
        elif s[i]=='}':
            depth-=1
            if not depth:return s[begin:i+1]
    raise AssertionError(start)
observation=block('struct Observation {')+';\n'
reconcile=block('bool reconcile_hammerburst_observation(')
host=r'''
#include "weapon_hand_participant.h"
#include <cmath>
#include <cstdio>
#include <limits>
namespace hand=gears_weapon_hand;
struct Name {unsigned index{},number{};};
'''
owner=r'''
struct Owner {Observation last{};float wanted_time{};bool active{},stopped{},seek_pending{};}ownership;
'''
tests=r'''
int main(){
 Observation base{};base.weapon={0x1000,0x2000,0x3000,1,2,0,1};
 base.component={0x4000,0x1000,0x5000,3,4,0,1};
 base.asset={0x6000,0x7000,0x8000,5,6,0,1};
 base.node={0x9000,0x4000,0xa000,7,8,0,1};
 base.sequence={0xb000,0x6000,0xc000,9,10,0,1};
 base.sequence_name={12,0};base.time=.25f;base.rate=1;base.length=2.5f;base.reload_start=12;
 unsigned cases{};
 const auto reset=[&](){ownership={base,.5f,true,true,false};};
 const auto reject=[&](Observation const& value,char const* label){
  ++cases;reset();const bool accepted=reconcile_hammerburst_observation(value);
  if(accepted||ownership.last.time!=base.time||ownership.seek_pending){std::printf("FAIL %s\n",label);return false;}return true;
 };
 for(auto field:{&Observation::weapon,&Observation::component,&Observation::asset,&Observation::node,&Observation::sequence}){
  for(unsigned part=0;part<7;++part){auto c=base;auto& id=c.*field;
   switch(part){case 0:id.address+=8;break;case 1:id.outer+=8;break;case 2:id.klass+=8;break;
    case 3:++id.registry_index;break;case 4:++id.name_index;break;case 5:++id.name_number;break;default:id.known=0;}
   if(!reject(c,"foreign/recycled object field"))return 1;
  }
 }
 for(unsigned part=0;part<12;++part){auto c=base;
  switch(part){case 0:++c.sequence_name.index;break;case 1:++c.sequence_name.number;break;
   case 2:c.length+=1;break;case 3:c.reload_start+=1;break;case 4:c.playing=true;break;
   case 5:c.rate+=1;break;case 6:c.looping=true;break;case 7:c.time+=.1f;break;
   case 8:c.time=std::numeric_limits<float>::quiet_NaN();break;
   case 9:c.time=std::numeric_limits<float>::infinity();break;case 10:c.time=-1;break;default:c.time=3;}
  if(!reject(c,"foreign clock/content/playback/time"))return 1;
 }
 auto swapped=base;swapped.asset=base.node;if(!reject(swapped,"registered node cannot replace asset"))return 1;
 swapped=base;swapped.node=base.asset;if(!reject(swapped,"registered asset cannot replace selected node"))return 1;
 reset();ownership.active=false;++cases;if(reconcile_hammerburst_observation(base))return 1;
 reset();ownership.stopped=false;++cases;if(reconcile_hammerburst_observation(base))return 1;
 reset();++cases;if(!reconcile_hammerburst_observation(base)||ownership.last.time!=.25f)return 1;
 reset();ownership.seek_pending=true;auto current=base;current.time=.5f;
 ++cases;if(!reconcile_hammerburst_observation(current)||ownership.seek_pending||ownership.last.time!=.5f)return 1;
 reset();ownership.seek_pending=true;current.time=.5002f;
 ++cases;if(reconcile_hammerburst_observation(current)||!ownership.seek_pending||ownership.last.time!=.25f)return 1;
 reset();ownership.seek_pending=true;current.time=std::numeric_limits<float>::quiet_NaN();
 ++cases;if(reconcile_hammerburst_observation(current)||!ownership.seek_pending)return 1;
 std::printf("HB actual seek observation reconciliation: %u identity/content/pending rejection checks passed; CPU only\n",cases);
}
'''
cpp=O/'seek-observation.cpp';exe=O/'seek-observation.exe';cmd=O/'seek-observation.cmd'
cpp.write_text(host+observation+owner+reconcile+tests,encoding='utf-8')
cmd.write_text('@echo off\ncall E:\\vs\\VC\\Auxiliary\\Build\\vcvars64.bat >nul\ncl /nologo /std:c++17 /EHsc /W4 /WX /I"'+str(R/'tools/vr-common')+'" /Fe:"'+str(exe)+'" /Fo:"'+str(O/'seek-observation.obj')+'" "'+str(cpp)+'"\nif errorlevel 1 exit /b %errorlevel%\n"'+str(exe)+'"\n',encoding='ascii')
r=subprocess.run(['cmd','/c',str(cmd)],capture_output=True,encoding='utf-8',errors='replace')
receipt=dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),branch_sha256=hashlib.sha256(reconcile.encode()).hexdigest(),exit_code=r.returncode,output=r.stdout+r.stderr,scope=__doc__)
(O/'seek-observation.log').write_text(r.stdout+r.stderr,encoding='utf-8');(O/'seek-observation.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
print(json.dumps(receipt,indent=2));sys.exit(r.returncode)
