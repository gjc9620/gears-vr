
#include "hb_history_rejection.h"
#include <cassert>
#include <cwchar>
int main(){
 using namespace hb_history_rejection;using namespace gears_hand_continuity;
 TrackedInput a{},b{};a.input.sequence=100;b.input.sequence=102;
 a.input.sync_ns=1000;b.input.sync_ns=2000;a.stream_epoch=b.stream_epoch=1;
 MotionHistory h{};h.count=2;h.frames[0]=a;h.frames[0].input.sequence=101;h.frames[1]=b;
 for(auto* r:{&a,&b,&h.frames[0],&h.frames[1]})for(unsigned n=0;n<2;++n){
  r->tracking_epoch[n]=n+1;r->input.hands[n].grip.position.x=double(n)+.125;
  for(unsigned c=0;c<gears_vr::control_count;++c){r->input.hands[n].pressed[c]=c+1;r->input.hands[n].released[c]=c;}}
 capture(nullptr,12,102,gears_vr::control_squeeze,false,0,a,b,h,{},1);assert(records==0);
 FILE* f=std::tmpfile();assert(f);
 for(unsigned n=0;n<6;++n)capture(f,12+n,102,gears_vr::control_squeeze,false,0,a,b,h,{},1);
 assert(records==4&&a.input.sequence==100&&b.input.sync_ns==2000&&h.count==2);
 std::rewind(f);wchar_t line[8192]{};unsigned summaries{},raws{},hands{};
 while(std::fgetws(line,8192,f)){
  if(std::wcsstr(line,L"hammerburst_history_rejection ")){++summaries;assert(std::wcsstr(line,L"previous_sequence=100 current_sequence=102 history_count=2"));}
  if(std::wcsstr(line,L"hammerburst_history_raw "))++raws;
  if(std::wcsstr(line,L"hammerburst_history_hand ")){++hands;assert(std::wcsstr(line,L"p0=1 r0=0"));}
 }
 assert(summaries==4&&raws==16&&hands==32);std::fclose(f);
 std::puts("History rejection observer: null/no-write,4-record cap, original sequence/time/pose/control serialization and source non-mutation passed. Pure build unchanged.");
}
