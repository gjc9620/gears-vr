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
    Recorder provenance;row={};row.geometry.quad.ticket=1;row.geometry.provenance={32,{1,1,7,7,0,123,0}};
    row.present_resources[0]=456;provenance.record(row);row.present_resources[0]=789;
    provenance.record(row);assert(provenance.size==1&&provenance.rows[0].present_resources[0]==456);
    // Repetitive copies coalesce without losing their first serial snapshot.
    row.geometry.provenance.trace.write_serial=2;row.geometry.provenance.trace.source_write_serial=99;
    provenance.record(row);assert(provenance.size==1&&provenance.rows[0].count==3&&
        provenance.rows[0].geometry.provenance.trace.write_serial==1&&provenance.rows[0].geometry.provenance.trace.source_write_serial==0);
    row.geometry.provenance.trace.origin_create_serial=2;provenance.record(row);assert(provenance.size==2);
    row.geometry.provenance.trace.create_epoch=8;provenance.record(row);assert(provenance.size==3);
    std::puts("final eye diagnostics bounds, coalescing and epoch sampling passed");
}
