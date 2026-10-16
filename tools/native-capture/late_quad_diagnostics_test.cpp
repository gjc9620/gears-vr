#include "late_quad_diagnostics.h"
#include <cassert>
int main(){
    using gears_late_quad::observe_target;
    assert(observe_target(0x1234,true,false,0));assert(observe_target(0x1234,true,false,9));
    assert(observe_target(0x1234,true,true,0));assert(!observe_target(0x1234,true,true,9));
    assert(!observe_target(0x1234,false,false,0));assert(!observe_target(0x1234,false,true,9));
    assert(observe_target(0x2b11a0,false,true,9));
    assert(observe_target(0xd83cbf,false,false,9));
    assert(observe_target(0xd83cbf,false,false,0));
    assert(!observe_target(0xd83cbe,false,false,9));
    assert(!observe_target(0xd83cc0,false,false,9));
    struct Payload {unsigned queries{};};
    gears_late_quad::Recorder<Payload> recorder;
    gears_late_quad::Epoch epoch{1,1,9};gears_late_quad::Key key{};
    key.args={0,0,1920,1081,0,0,1,1};key.dimensions={1920,1081,1920,1081};
    auto* row=recorder.admit(epoch,key,0,10);assert(row);row->payload.queries=4;
    // Tickets and recording generations are intentionally outside the key.
    assert(!recorder.admit(epoch,key,12,20));assert(recorder.size==1);
    assert(row->count==2&&row->last_ticket==12&&row->first_qpc==10&&row->last_qpc==20&&row->payload.queries==4);
    for(unsigned i=1;i<8;++i){key.objects[0]=i;assert(recorder.admit(epoch,key,0,30));}
    key.objects[0]=99;assert(!recorder.admit(epoch,key,0,40));assert(recorder.dropped==1);
    // A full epoch still accepts repeats and leaves room for resize/world epochs.
    key.objects[0]=0;assert(!recorder.admit(epoch,key,0,50));assert(row->count==3&&recorder.dropped==1);
    for(unsigned e=2;e<=4;++e){epoch.inventory=e;for(unsigned i=0;i<8;++i){key.objects[0]=i;assert(recorder.admit(epoch,key,0,60));}}
    epoch.world=2;assert(!recorder.admit(epoch,key,0,70));assert(recorder.size==32&&recorder.dropped==2);
    // Source, viewport and known-buffer differences are distinct observations.
    gears_late_quad::Recorder<Payload> changes;epoch={3,4,9};key={};
    assert(changes.admit(epoch,key,0,1));key.known_buffer=true;assert(changes.admit(epoch,key,0,2));
    key.rectangles.viewport.width=1920;assert(changes.admit(epoch,key,0,3));
    key.args[4]=.5f;assert(changes.admit(epoch,key,0,4));
    key.dimensions[3]=1081;assert(changes.admit(epoch,key,0,5));
    key.objects[3]=7;assert(changes.admit(epoch,key,0,6));
    key.quad_caller=0xd83b1c;assert(changes.admit(epoch,key,0,7));
    key.quad_caller=0x2c1234;assert(changes.admit(epoch,key,0,8));
    assert(changes.size==8&&changes.rows[6].key.quad_caller==0xd83b1c&&changes.rows[7].key.quad_caller==0x2c1234);
    return 0;
}
