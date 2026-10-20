#include "crosshair_scene_evidence.h"
#include "anchor_observer.h"
#include "submitted_view_lifetime.h"
#include <cassert>
#include <cstdio>
using gears_crosshair_scene::Metadata;
static Metadata evidence(){Metadata v{};v.controller=10;v.pawn=11;v.weapon=12;v.root_frame=13;
    v.world=3;v.tick=4;v.thread=7;v.begin_qpc=1;v.assignment_qpc=2;v.finish_qpc=3;v.scene_qpc=4;
    v.point={1,2,3};v.writer_view.valid=true;v.writer_view.location={4,5,6};return v;}
int main(){
    gears_anchor::Receipt receipt{};receipt.key.local=1;receipt.key.family=receipt.key.owner=2;
    receipt.key.source=3;receipt.invocation=8;receipt.thread=7;receipt.scene_qpc=5;
    receipt.anchor.controller=10;receipt.anchor.pawn=11;
    auto bind=[&](Metadata& v){return gears_crosshair_scene::associate(v,receipt.key,8,7,3,10,11);};
    auto value=evidence();assert(bind(value));assert(!bind(value));
    for(unsigned fault=0;fault<7;++fault){auto v=evidence();
        if(fault==0)++v.world;if(fault==1)++v.thread;if(fault==2)++v.controller;
        if(fault==3)++v.pawn;if(fault==4)v.weapon=0;if(fault==5)v.finish_qpc=5;if(fault==6)v.tick=0;
        assert(!bind(v));}
    for(unsigned fault=0;fault<8;++fault){std::optional<Metadata> pending=value;auto key=receipt.key;
        auto invocation=receipt.invocation;auto thread=receipt.thread;
        if(fault==0)++key.local;if(fault==1)++key.family;if(fault==2)++key.source;
        if(fault==3)++key.owner;if(fault==4)++key.stamp;if(fault==5)++key.matrices[127];
        if(fault==6)++invocation;if(fault==7)++thread;
        assert(!gears_crosshair_scene::take(pending,key,invocation,thread));assert(!pending);
        assert(!gears_crosshair_scene::take(pending,receipt.key,8,7));}
    // Actual Context transfers optional owning evidence exactly once.
    receipt.crosshair=value;gears_anchor::Context context;
    context.begin_scene();assert(context.offer(receipt)==gears_anchor::accepted);context.end_scene();
    gears_anchor::Context::FamilyScope family(context,receipt.key,receipt.anchor,7,6,true,1,0x2c8909);
    assert(family.valid());gears_anchor::Receipt renderer{};
    assert(context.consume(receipt.key,receipt.anchor,7,7,true,1,0x3a1595,renderer)==gears_anchor::accepted);
    assert(context.consume(receipt.key,receipt.anchor,7,7,true,1,0x3a1595,receipt)==gears_anchor::already_consumed);
    auto ticket_value=gears_crosshair_scene::take(renderer.crosshair,renderer.key,renderer.invocation,renderer.thread);
    assert(ticket_value&&ticket_value->point==value.point&&!renderer.crosshair&&
        gears_crosshair_view::equal(ticket_value->writer_view,value.writer_view));
    assert(!gears_crosshair_scene::take(renderer.crosshair,renderer.key,renderer.invocation,renderer.thread));
    gears_submitted::FrameTicket frame{};frame.enabled=true;frame.id=9;frame.crosshair=ticket_value;
    frame.crosshair->ticket=frame.id;gears_submitted::Lifetime lifetime;
    assert(!lifetime.begin(0,{},frame));assert(lifetime.begin(100,{},frame));
    frame.crosshair->point={9,9,9}; // Registered ticket owns its copy.
    gears_submitted::FrameTicket stored{};assert(lifetime.frame(100,stored));
    assert(stored.crosshair&&stored.crosshair->ticket==9&&stored.crosshair->invocation==8&&stored.crosshair->point==value.point);
    assert(!lifetime.begin(100,{},frame));lifetime.retire(100);assert(!lifetime.frame(100,stored));
    // No assignment evidence is also a normal, admissible stereo ticket.
    frame.crosshair.reset();assert(lifetime.begin(101,{},frame));assert(lifetime.frame(101,stored)&&!stored.crosshair);
    std::puts("Crosshair assignment owning scene/anchor/ticket association and one-use rejection tests passed");
}
