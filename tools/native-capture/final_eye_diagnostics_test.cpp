#include "final_eye_diagnostics.h"
#include <cassert>
#include <cstdio>
using namespace gears_final_eye_diagnostics;
int main(){
    Recorder recorder;Row row{};row.epoch.world=1;row.geometry.quad.ticket=1;row.first_qpc=10;
    recorder.record(row);row.geometry.quad.ticket=2;row.list=999;row.list_generation=2;row.first_qpc=20;recorder.record(row);
    assert(recorder.size==1&&recorder.rows[0].count==2&&recorder.rows[0].geometry.quad.ticket==1&&
        recorder.rows[0].last_ticket==2&&recorder.rows[0].first_qpc==10&&recorder.rows[0].last_qpc==20);
    for(unsigned i=1;i<=4;++i){row.geometry.reason=i;recorder.record(row);}
    assert(recorder.size==4&&recorder.dropped==1);
    row.epoch.resize=1;recorder.record(row);assert(recorder.size==5);
    row.epoch.world=2;recorder.record(row);assert(recorder.size==6);
    row.epoch.inventory=1;recorder.record(row);assert(recorder.size==7);
    row.geometry.quad.ticket=0;recorder.record(row);assert(recorder.size==8);
    row.geometry.reason=99;recorder.record(row);assert(recorder.size==8&&recorder.dropped==2);
    row.geometry.quad.ticket=3;
    while(recorder.size<Recorder::capacity){++row.epoch.world;recorder.record(row);}
    ++row.epoch.world;recorder.record(row);assert(recorder.size==32&&recorder.dropped==3);
    row=recorder.rows[0];row.first_qpc=30;recorder.record(row);
    assert(recorder.rows[0].count==3&&recorder.rows[0].last_qpc==30&&recorder.dropped==3);
    std::puts("final eye diagnostics bounds, coalescing and epoch sampling passed");
}
