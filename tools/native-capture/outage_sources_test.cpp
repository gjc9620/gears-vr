#include "outage_sources.h"
#include <cstdio>
using namespace gears_outage_sources;
#define CHECK(x) do{if(!(x)){std::printf("failed line %d: %s\n",__LINE__,#x);return __LINE__;}}while(0)
static Interval interval(std::int64_t qpc,bool published=false){
    Interval row{};row.qpc=row.end_qpc=qpc;row.published=published;
    if(published)row.published_qpc=qpc;return row;
}
int main(){
    Recorder trace;trace.reset(1000);
    trace.observe(interval(10000));CHECK(trace.count()==0&&!trace.has_longest()); // Unarmed startup.
    trace.observe(interval(10000,true));
    Draw first{};first.qpc=10001;first.list=17;first.generation=3;first.target=19;
    trace.unknown(first);
    auto material=first;material.qpc=10002;material.material_sequence=42;material.material_payload=71;
    trace.unknown(material);
    auto overlay=first;overlay.qpc=10003;overlay.overlay={5,256,0};trace.unknown(overlay);
    auto row=trace.begin(10004,{41,42});row.end_qpc=10004;trace.observe(row);
    CHECK(row.unknowns==3&&row.first.qpc==10001&&row.material.material_sequence==42&&row.overlay.overlay.rejection==256);
    CHECK(row.eye_ticket[0]==41&&row.eye_ticket[1]==42);
    first.list=999;material.material_payload=999;
    CHECK(row.first.list==17&&row.material.material_payload==71); // Values, no borrowed receipt memory.
    auto empty=trace.begin(10005,{0,0});CHECK(!empty.unknowns&&!empty.first.qpc&&!empty.material.qpc&&!empty.overlay.qpc);
    trace.observe(interval(10499));CHECK(trace.count()==0);
    trace.observe(interval(10500));CHECK(trace.count()==1&&trace.total()==1);
    auto const& gap=trace.at(0);CHECK(gap.trigger_qpc==10500&&!gap.recovered);
    CHECK(gap.first_unknown.first.list==17&&gap.first_unknown.eye_ticket[1]==42);
    CHECK(gap.trigger.end_qpc==10500&&gap.recent_count==4);
    for(int i=0;i<100;++i)trace.observe(interval(10501+i));
    CHECK(trace.count()==1&&trace.total()==1&&trace.at(0).intervals==101);
    trace.observe(interval(12000,true));CHECK(trace.at(0).recovered&&trace.at(0).last.published_qpc==12000);
    CHECK(trace.has_longest()&&trace.longest().previous_publish==10000&&trace.longest().recovered);
    CHECK(trace.at(0).first_unknown.first.qpc==10001); // Not lost on recovery or recent ring wrap.
    trace.observe(interval(12001));CHECK(trace.count()==1);
    // A recovery publication itself can reveal a gap without an intermediate Present.
    trace.observe(interval(12500,true));CHECK(trace.count()==2&&trace.at(1).recovered&&!trace.at(1).first_unknown.unknowns);
    CHECK(trace.at(1).recent_count==16);
    CHECK(trace.at(1).recent[15].end_qpc==12001);
    trace.observe(interval(13000,true));trace.observe(interval(13500,true));
    CHECK(trace.count()==4&&trace.total()==4&&trace.at(0).previous_publish==10000);
    trace.observe(interval(14000)); // Overwrite oldest with an active fifth gap.
    CHECK(trace.count()==4&&trace.total()==5&&trace.at(0).previous_publish==12000);
    CHECK(trace.at(3).previous_publish==13500&&!trace.at(3).recovered);
    trace.observe(interval(14500,true));CHECK(trace.at(3).recovered);
    trace.observe(interval(15000,true));CHECK(trace.total()==6&&trace.at(0).previous_publish==12500&&trace.at(3).previous_publish==14500);
    CHECK(trace.longest().previous_publish==10000); // Longest survives recent-slot overwrites.
    trace.observe(interval(18000));CHECK(trace.longest().previous_publish==15000&&!trace.longest().recovered);
    trace.observe(interval(18001,true));CHECK(trace.longest().previous_publish==15000&&trace.longest().recovered);
    trace.reset(1000);CHECK(trace.count()==0&&trace.total()==0&&!trace.has_longest());
    trace.observe(interval(100,true));trace.observe(interval(600));
    CHECK(!trace.at(0).first_unknown.unknowns);
    trace.unknown(first);auto late=trace.begin(650,{17,17});late.end_qpc=650;trace.observe(late);
    CHECK(trace.at(0).first_unknown.first.list==999&&trace.at(0).first_unknown.eye_ticket[0]==17);
    trace.unknown(first);trace.reset(1000);CHECK(!trace.begin(700,{0,0}).unknowns);
    trace.observe(interval(2000,true));trace.observe(interval(1999));CHECK(!trace.count()); // No negative-age gap.
    trace.reset(0);trace.observe(interval(100,true));trace.observe(interval(10000));CHECK(!trace.count());
    std::puts("outage source threshold/first evidence/recovery/bounded wrap checks passed");
}
