#include "outage_trace.h"
#include <cstdio>
#include <thread>
#define CHECK(c) do{if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return __LINE__;}}while(0)
using namespace gears_outage;
static Sample sample(std::int64_t begin,std::int64_t end,bool published=false){Sample s{};s.enter_qpc=s.original_begin_qpc=begin;s.original_end_qpc=end;s.published=published;s.published_qpc=published?begin:0;return s;}
int main(){
    Window w;w.reset(1000);w.observe(sample(100,500));CHECK(!w.triggered()); // no first publication, not armed.
    w.observe(sample(600,610,true));w.observe(sample(690,700));CHECK(!w.triggered()); // exactly100ms.
    w.observe(sample(701,702));CHECK(w.triggered()&&w.count()==2&&w.at(0).enter_qpc==690&&!w.finished());
    w.observe(sample(710,720,true));CHECK(w.finished()&&w.end()==recovered&&w.count()==3);
    w.observe(sample(999,999));CHECK(w.count()==3); // first window frozen.
    w.reset(1000);w.observe(sample(100,110,true));w.observe(sample(250,260,true));
    CHECK(w.triggered()&&w.finished()&&w.count()==2&&w.trigger()==publication_gap); // recovery record itself triggers.
    w.reset(1000);w.observe(sample(100,250,true));
    CHECK(w.triggered()&&!w.finished()&&w.trigger()==original_present_gap); // published then long original Present.
    w.observe(sample(260,270,true));CHECK(w.finished()&&w.count()==2&&w.end()==recovered);
    w.reset(1000);w.observe(sample(100,101,true));w.observe(sample(201,202));
    for(unsigned i=0;i<400;++i)w.observe(sample(210+i,211+i));
    CHECK(w.count()==256&&w.finished()&&w.end()==capacity_full);
    w.reset(0);w.observe(sample(100,1000,true));CHECK(!w.triggered());
    w.reset(1001);w.observe(sample(1,2,true));w.observe(sample(101,101));CHECK(!w.triggered());
    w.observe(sample(102,102));CHECK(w.triggered()); // nonintegral threshold rounding.
    ActivitySnapshot snapshot;Activity output{999};CHECK(!snapshot.read(output)&&output.ticket==999);
    snapshot.write({1,2,3,4,5});CHECK(snapshot.read(output)&&output.ticket==1&&output.count==2&&output.received_ns==5);
    snapshot.write({0,0,0,0,0});bool coherent=true;
    std::atomic<bool> finished{};std::thread writer([&]{for(std::uint64_t n=1;n<30000;++n)snapshot.write({n,n,n,std::int64_t(n),std::int64_t(n)});finished=true;});
    while(!finished){Activity a{};if(snapshot.read(a))coherent=coherent&&a.ticket==a.count&&a.count==a.sequence&&a.sequence==std::uint64_t(a.qpc)&&a.qpc==a.received_ns;}
    writer.join();CHECK(coherent);std::printf("outage window threshold/predecessor/recovery/post-publish Present/capacity/atomic snapshot CPU tests passed\n");return 0;
}
