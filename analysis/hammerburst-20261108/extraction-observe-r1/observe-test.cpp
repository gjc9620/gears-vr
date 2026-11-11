
#include "weapon_hand_client.h"
#include "weapon_hammerburst_magazine.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
gears_weapon::FeedPortMount feed{};
FILE* log_file{};
struct Proxy {
 gears_weapon::HammerburstMagazineExtraction value;unsigned calls{};
 bool observe(gears_weapon::FeedSample const& s,gears_weapon::FeedPortMount const& m){++calls;return value.observe(s,m);}
 bool complete()const{return value.complete();}
} extraction;
void observe_original_extraction(hand::Frame const& f,std::uint64_t item,
    gears_weapon::FeedSample const& sample,std::int64_t gap,double depth,double lateral,double aligned) noexcept {
    const bool accepted=extraction.observe(sample,feed); // Exact original call, once.
    static unsigned rows{};
    if(log_file&&rows<32){++rows;
        fwprintf(log_file,L"hammerburst_extraction_observed frame=%llu sequence=%llu item=%llu accepted=%u complete=%u gap_ns=%lld depth=%.9g lateral=%.9g aligned=%.9g sync_ns=%lld\n",
            f.key.frame,f.controllers.sequence,item,unsigned(accepted),unsigned(extraction.complete()),gap,
            depth,lateral,aligned,f.controllers.sync_ns);fflush(log_file);
    }
}

int main(){
 assert(tmpfile_s(&log_file)==0&&log_file);gears_weapon::HammerburstMagazineExtraction oracle;
 hand::Frame frame{};unsigned checks{};
 for(unsigned i=1;i<=45;++i){
  gears_weapon::FeedSample sample{1,2,3,4,i,1000000000+std::int64_t(i)*20000000,
    {{0,0,0},{0,0,0,1}},{{0,0,0},{0,0,0,1}},true,true};
  if(i>=2&&i<=3)sample.left_grip.position.z=.025*(i-1);
  if(i==4)sample.tracked=false;
  if(i==6)sample.monotonic_ns+=101000000;
  if(i==8)sample.left_grip.position.x=.1;
  frame.key.frame=i;frame.controllers.sequence=i;frame.controllers.sync_ns=sample.monotonic_ns;
  const auto expected=oracle.observe(sample,feed);(void)expected;
  observe_original_extraction(frame,2,sample,20000000,sample.left_grip.position.z,sample.left_grip.position.x,1);
  assert(extraction.calls==i&&extraction.complete()==oracle.complete());++checks;
 }
 rewind(log_file);unsigned lines{};wchar_t line[1024];while(fgetws(line,1024,log_file))++lines;
 assert(lines==32);fclose(log_file);log_file=nullptr;
 std::printf("Exact extraction wrapper: %u call/result checks, original continuous/invalid samples and32-row cap\n",checks);
}
