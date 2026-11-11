
#include "weapon_hand_client.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
namespace anim {struct Owner {bool active=true,stopped=true,seek_pending=false;unsigned serial=1,inventory=1;} ownership;}
struct Opening {unsigned identity=1;} opening;
unsigned animation_serial(){return 1;}
bool expired_before_latch_dispatch(hand::Frame const& f,std::int64_t begin,std::int64_t end) noexcept {
    if(!anim::ownership.active||!anim::ownership.stopped||anim::ownership.seek_pending||
       anim::ownership.serial!=animation_serial()||!(anim::ownership.inventory==opening.identity)||
       begin<=0||end<begin||end<f.now_ns)return false;
    const auto expired=[&](std::int64_t stamp) noexcept {return stamp>0&&end>=stamp&&end-stamp>100000000;};
    return expired(f.controllers.received_ns)||expired(f.stereo.received_ns);
}

FILE* log_file{};unsigned cleaned{};unsigned stage=2;double angle{};
struct Stroke {double value=1;double angle()const{return value;}}stroke;
namespace latch_history {unsigned previous=2;}
void cleanup(unsigned){++cleaned;}
void attempt(hand::Frame const& f,std::int64_t began,std::int64_t ended,bool sought=false){
 const auto stroke_before=stroke;const auto history_before=latch_history::previous;
 stroke.value=99;latch_history::previous=88;
             if(!sought){
                const bool wait=expired_before_latch_dispatch(f,began,ended);
                static unsigned rows{};
                if(log_file&&rows<16){++rows;
                    fwprintf(log_file,L"hammerburst_latch_seek_unavailable frame=%llu sequence=%llu stage=%u wait=%u seek_pending=%u previous_angle=%.9g wanted_angle=%.9g begin_ns=%lld end_ns=%lld sync_ns=%lld no_dispatch=%u\n",
                        f.key.frame,f.controllers.sequence,unsigned(stage),unsigned(wait),unsigned(anim::ownership.seek_pending),
                        angle,stroke.angle(),began,ended,f.controllers.sync_ns,unsigned(wait));fflush(log_file);}
                if(wait){stroke=stroke_before;latch_history::previous=history_before;return;}
                cleanup(__LINE__);return;
            }
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
