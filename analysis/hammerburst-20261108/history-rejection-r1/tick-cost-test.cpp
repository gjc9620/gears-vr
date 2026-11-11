
#include "hb_tick_cost_tail.h"
#include <cassert>
#include <cwchar>
int main(){
 using namespace hb_tick_cost_tail;
 dump(nullptr);assert(total==0);
 for(unsigned n=1;n<=200;++n){Row r{};r.begin=1000*n;r.original_begin=r.begin+10;r.original_end=r.begin+20;
  r.equipment_begin=r.begin+30;r.equipment_end=r.begin+40;r.end=r.begin+50;
  r.last_input_sync=n;r.last_input_observed=n+1;r.captured=n+2;r.event=n;
  r.first_debt=n%2;r.cancel=n%3;r.count=n%7;r.owned=n%2;r.armed=1;
  r.fire_early=n%2;r.reload_early=n%3;r.pre_reload=n%4;record(r);}
 assert(total==200&&rows.size()==128);
 for(unsigned n=72;n<200;++n){auto const& r=rows[n%128];assert(r.event==n+1&&r.begin==1000*(n+1));
  assert(r.last_input_sync==n+1&&r.captured==n+3&&r.end-r.begin==50);}
 FILE* f=std::tmpfile();assert(f);dump(f);assert(total==200);std::rewind(f);
 wchar_t line[2048]{};assert(std::fgetws(line,2048,f)&&std::wcsstr(line,L"total=200 retained=128"));
 unsigned seen{},protected_seen{};while(std::fgetws(line,2048,f)){
  if(std::wcsstr(line,L"hammerburst_first_tick_cost ")){++protected_seen;
   if(protected_seen==1)assert(std::wcsstr(line,L"ordinal=1 "));
   if(protected_seen==64)assert(std::wcsstr(line,L"ordinal=127 "));continue;}
  if(std::wcsstr(line,L"hammerburst_first_tick_summary ")){assert(std::wcsstr(line,L"seen=100 retained=64 capacity=64 truncated=1"));continue;}
  ++seen;
  assert(std::wcsstr(line,L"pre_original_ns=10 original_ns=10 service_ns=10 equipment_ns=10 follow_ns=10 total_ns=50 action=0"));
  if(seen==1)assert(std::wcsstr(line,L"ordinal=73 "));if(seen==128)assert(std::wcsstr(line,L"ordinal=200 "));}
 assert(seen==128&&protected_seen==64&&first_seen==100&&first_kept==64);std::fclose(f);
 Row no_event{};no_event.first_debt=1;no_event.captured=1;record(no_event);
 no_event.event=1;no_event.captured=0;record(no_event);assert(first_seen==100&&first_kept==64);std::puts("Passive HB GT tail: original scalar retention,128-row wrap, ordering, phase decomposition, null dump and200-write/128-row export passed. No game or timing authority.");
}
