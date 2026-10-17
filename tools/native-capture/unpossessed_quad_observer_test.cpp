#include "unpossessed_quad_observer.h"
#include "submitted_view_lifetime.h"
#include <cassert>
#include <cstdio>
#include <memory>

int main(){
    using namespace gears_unpossessed_quad;
    struct Payload {unsigned hints{};};
    unsigned eye=99;
    assert(native_eye_index(0x10000,0x10000,2,eye)&&eye==0);
    assert(native_eye_index(0x10d30,0x10000,2,eye)&&eye==1);
    assert(!native_eye_index(0x10d31,0x10000,2,eye)&&eye==0);
    assert(!native_eye_index(0x11a60,0x10000,2,eye));
    assert(!native_eye_index(0xffff,0x10000,2,eye));
    assert(!native_eye_index(0x10001,0x10001,2,eye));
    assert(!native_eye_index(0,0,2,eye));
    assert(!native_eye_index(0x10000,0x10000,1,eye));
    assert(!native_eye_index(0x10000,0x10000,3,eye));
    assert(!native_eye_index(0x10000,0x10000,-1,eye));
    assert(!native_eye_index(UINTPTR_MAX-7,8,2,eye));
    assert(!native_eye_index(8,UINTPTR_MAX-7,2,eye));
    auto recorder=std::make_unique<Recorder<Payload>>();auto& r=*recorder;
    assert(!r.select(false,{1,99})&&!r.select(true,{1,0})&&r.distinct==0);
    // Non-monotonic IDs, duplicates of selected and skipped tickets, same ID
    // in another world, and the exact delayed selection boundary.
    assert(r.select(true,{1,99})->ordinal==1);
    assert(r.select(true,{1,99})->ordinal==1&&r.distinct==1);
    assert(r.select(true,{2,99})->ordinal==2);
    assert(r.select(true,{1,1})->ordinal==3);
    for(unsigned i=4;i<=48;++i){assert(!r.select(true,{1,100+i}));assert(!r.select(true,{1,100+i}));}
    assert(r.distinct==48&&r.size==3);
    auto* delayed=r.select(true,{1,149});assert(delayed&&delayed->ordinal==49&&r.distinct==49);
    for(unsigned i=150;i<2000;++i)assert(!r.select(true,{1,i}));
    assert(r.distinct==49&&r.size==4&&r.select(true,{1,99})==&r.banks[0]);
    assert(!r.select(false,{1,99})&&!r.find({2,149}));
    Key key{};key.call.quad_caller=0x123;key.call.args[2]=100;
    key.raw_list=key.list_identity=77;key.list_generation=3;
    auto* row=delayed->admit(key,10);assert(row);row->payload.hints=4;
    assert(!delayed->admit(key,20)&&row->count==2&&row->last_qpc==20&&row->payload.hints==4);
    // Eye context, list generation and intermediate RT identity all keep
    // distinct CPU states separate, even with identical quad arguments.
    key.final_quad_eye_valid=true;key.final_quad_eye=1;assert(delayed->admit(key,21));
    ++key.list_generation;assert(delayed->admit(key,22));
    key.call.target.resource=456;assert(delayed->admit(key,23));
    key.native_view.current_view=0x10000;assert(delayed->admit(key,24));
    key.native_view.current_view=0x10d30;assert(delayed->admit(key,25));
    for(unsigned i=6;i<64;++i){key.call.draw_caller=i;assert(delayed->admit(key,30+i));}
    assert(delayed->size==64&&delayed->dropped==0);
    const auto full_key=key;
    key.call.draw_caller=999;assert(!delayed->admit(key,100)&&delayed->dropped==1);
    assert(!delayed->admit(full_key,101)&&delayed->dropped==1&&delayed->rows[63].count==2);
    // Capacity belongs to each selected ticket, not the early session history.
    assert(r.banks[0].size==0&&r.banks[0].admit(key,102));
    // Internal metadata is copied through the existing renderer lifetime,
    // unaffected by subsequent producer changes or consumer-local edits.
    auto life=std::make_unique<gears_submitted::Lifetime>();
    gears_submitted::FrameTicket frame{},copy{};frame.enabled=true;frame.id=7;frame.unpossessed=true;
    assert(life->begin(1,{},frame));frame.unpossessed=false;
    assert(life->frame(1,copy)&&copy.unpossessed);copy.unpossessed=false;
    assert(life->frame(1,copy)&&copy.unpossessed);
    assert(!life->retire(1)&&!life->frame(1,copy)&&!copy.unpossessed);
    std::puts("UnpossessedQuadObserverTest passed");
}
