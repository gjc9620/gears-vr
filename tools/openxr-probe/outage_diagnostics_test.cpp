#include "outage_diagnostics.h"
#include <cassert>
#include <cstring>
#include <string>
using namespace outage_diagnostics;
static Row frame(std::int64_t ns,bool submitted=false,bool fresh=false){
    Row r;r.ns=ns;r.submitted=submitted;r.new_image=fresh;r.new_image_valid=true;r.status=submitted?"submitted":"stale_ticket";return r;
}
int main(int argc,char** argv){
    Recorder disabled;disabled.observe(frame(1,true));assert(disabled.count==0&&disabled.longest_gap().duration_ns==0);
    Recorder before;before.enabled=true;before.observe(frame(1));before.observe(frame(900000001));assert(!before.first_started);
    Recorder exact;exact.enabled=true;exact.observe(frame(1,true,true));exact.observe(frame(100000001));assert(!exact.first_started);
    exact.observe(frame(100000002));assert(exact.first_started&&exact.count==2&&!exact.complete());
    exact.observe(frame(110000001,true,true));assert(exact.complete()&&exact.longest_gap().end_valid);
    Recorder cache;cache.enabled=true;auto cached=frame(1,true,false);cached.image.receiver_valid=true;cached.image.ticket_valid=true;cached.image.ticket_id=9;
    cached.image.received_ns=1;cached.image.copies=2;cached.image.acquired_serial=7;
    cache.observe(cached);auto second=cached;second.ns=50000001;cache.observe(second);cache.observe(frame(160000002));
    assert(cache.rows[0].submitted&&!cache.rows[0].new_image&&cache.rows[0].image.acquired_serial==7);
    Recorder overflow;overflow.enabled=true;overflow.observe(frame(1,true));
    for(int i=0;i<400;++i)overflow.observe(frame(100000002+i));
    overflow.observe(frame(200000001,true,true));assert(overflow.count==249&&overflow.dropped>0&&!overflow.complete());
    assert(overflow.longest_gap().end_valid&&overflow.longest_gap().end.new_image);
    overflow.observe(frame(400000002));overflow.observe(frame(410000002,true));assert(overflow.later_windows==1&&!overflow.complete());
    assert(overflow.first_start.ns==1&&overflow.longest_gap().start.ns==200000001);
    Recorder front;front.enabled=true;front.observe(frame(1,true));
    for(int i=1;i<=20;++i)front.observe(frame(1+i*6000000));
    assert(front.first_start.ns==1&&front.rows[0].ns>1);
    Recorder ongoing;ongoing.enabled=true;ongoing.observe(frame(1,true));ongoing.observe(frame(100000002));
    const auto gap=ongoing.longest_gap().duration_ns;auto stop=frame(900000001);stop.status="producer_stopped";stop.terminal=true;stop.new_image_valid=false;
    ongoing.observe(stop);assert(ongoing.longest_gap().duration_ns==gap&&!ongoing.longest_gap().end_valid&&!ongoing.complete());
    Recorder leap;leap.enabled=true;leap.observe(frame(1,true));leap.observe(frame(150000001,true,true));
    assert(leap.complete()&&leap.first_started&&!leap.longest_gap().first_missing_valid);
    FILE* file{};assert(tmpfile_s(&file)==0&&file);ongoing.dump(file);ongoing.dump(file);rewind(file);std::string text;char buf[2048];
    while(fgets(buf,sizeof(buf),file))text+=buf;fclose(file);
    assert(text.find("\"ticket_valid\":false")!=std::string::npos);
    assert(text.find("\"ticket_id\"")==std::string::npos);
    assert(text.find("termination_excluded_from_gap")!=std::string::npos);
    assert(text.find("\"complete\":false")!=std::string::npos);
    if(argc==2&&!strcmp(argv[1],"--dump-fixtures")){cache.dump();overflow.dump();leap.dump();}
    printf("outage diagnostics boundary/cache/overflow/recovery/ongoing/termination tests passed; no XR/GPU opened\n");
}
