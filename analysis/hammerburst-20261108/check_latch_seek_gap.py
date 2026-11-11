from pathlib import Path
import json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'latch-seek-gap-r1'
s=(P/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
helper=s[s.index('bool expired_before_latch_dispatch('):s.index('void step(hand::Frame const&',s.index('bool expired_before_latch_dispatch('))]
start=s.index('            if(!sought){',s.index('const auto ended='));end=s.index('{',start);depth=1;i=end+1
while depth:
 depth+=(s[i]=='{')-(s[i]=='}');i+=1
block=s[start:i]
code=r'''
#include "weapon_hand_client.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
namespace anim {struct Owner {bool active=true,stopped=true,seek_pending=false;unsigned serial=1,inventory=1;} ownership;}
struct Opening {unsigned identity=1;} opening;
unsigned animation_serial(){return 1;}
@HELPER@
FILE* log_file{};unsigned cleaned{};unsigned stage=2;double angle{};
struct Stroke {double value=1;double angle()const{return value;}}stroke;
namespace latch_history {unsigned previous=2;}
void cleanup(unsigned){++cleaned;}
void attempt(hand::Frame const& f,std::int64_t began,std::int64_t ended,bool sought=false){
 const auto stroke_before=stroke;const auto history_before=latch_history::previous;
 stroke.value=99;latch_history::previous=88;
 @BLOCK@
}
int main(){
 hand::Frame f{};f.now_ns=1000000000;f.controllers.received_ns=f.stereo.received_ns=f.now_ns;
 unsigned checks{};
 for(unsigned change=0;change<12;++change){
  anim::ownership={};opening={};stroke.value=1;latch_history::previous=2;cleaned=0;
  auto source=f;std::int64_t begin=f.now_ns+90000000,end=f.now_ns+100000001;
  if(change==1)anim::ownership.seek_pending=true;
  if(change==2)end=f.now_ns+100000000;
  if(change==3)anim::ownership.active=false;
  if(change==4)anim::ownership.stopped=false;
  if(change==5)++anim::ownership.serial;
  if(change==6)++anim::ownership.inventory;
  if(change==7)end=begin-1;
  if(change==8)begin=0;
  if(change==9)source.now_ns=end+1;
  if(change==10)source.controllers.received_ns=source.stereo.received_ns=0;
  if(change==11)source.controllers.received_ns=source.stereo.received_ns=end+1;
  attempt(source,begin,end);
  if(change==0)assert(!cleaned&&stroke.value==1&&latch_history::previous==2);
  else assert(cleaned==1&&stroke.value==99&&latch_history::previous==88);
  ++checks;
 }
 std::printf("Exact latch pre-dispatch rollback block: %u expiry/pending/identity/clock cases passed; no engine calls\n",checks);
}
'''.replace('@HELPER@',helper).replace('@BLOCK@',block)
(P/'seek-gap-test.cpp').write_text(code);cmd=P/'seek-gap-test.cmd';exe=P/'seek-gap-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{P / "source/tools/vr-common"}" "{P / "seek-gap-test.cpp"}" /Fo"{P / "seek-gap-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'seek-gap-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'seek-gap-test.log').read_text(errors='replace'));r.check_returncode()
(P/'seek-gap-test-result.json').write_text(json.dumps(dict(exit_code=0,cases=12,exact_helper_and_caller_block=True,game_started=False,source_sha256=hashlib.sha256(code.encode()).hexdigest(),limits='Animation/trajectory holders and cleanup are fakes. Actual predicate and rollback caller text compiled; not full engine/geometry or gameplay proof.'),indent=2))
