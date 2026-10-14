#include "ui_payload_receipts.h"
#include <cassert>
#include <memory>
#include <thread>
#include <cstdio>

using namespace gears_ui_receipts;
static gears_ui_batch::Receipt receipt(std::uintptr_t payload,unsigned begin){
    gears_ui_batch::Receipt r{};r.canvas=0x1000;r.fcanvas=0x2000;r.item=0x3000;r.payload=payload;r.sort_key=-7;
    r.begin=begin;r.end=begin+4;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<44;++j)r.vertices[i][j]=static_cast<unsigned char>(begin+i*gears_ui_batch::vertex_stride+j);
    return r;
}
static bool same(Claim const& a,Claim const& b){
    if(a.generation!=b.generation||a.payload!=b.payload||a.canvas!=b.canvas||a.fcanvas!=b.fcanvas||a.item!=b.item||
       a.sort_key!=b.sort_key||a.count!=b.count||a.observed_end!=b.observed_end||a.unknown_vertices!=b.unknown_vertices||a.kind_mask!=b.kind_mask)return false;
    for(unsigned i=0;i<entry_capacity;++i){auto const& x=a.entries[i];auto const& y=b.entries[i];
        if(x.kind!=y.kind||x.root_invocation!=y.root_invocation||x.receipt.canvas!=y.receipt.canvas||
           x.receipt.fcanvas!=y.receipt.fcanvas||x.receipt.item!=y.receipt.item||x.receipt.payload!=y.receipt.payload||
           x.receipt.sort_key!=y.receipt.sort_key||x.receipt.begin!=y.receipt.begin||x.receipt.end!=y.receipt.end||x.receipt.vertices!=y.receipt.vertices)return false;
    }return true;
}
int main(){
    auto s=std::make_unique<Store>();Claim out{};out.generation=999;out.payload=0xdead;auto unchanged=out;
    assert(!s->claim(0xdead,out)&&same(out,unchanged));
    auto first=receipt(0x5000,4);assert(s->publish(first,1,17));first.vertices[0][0]=255;
    assert(s->publish(receipt(0x5000,12),3,18));
    assert(s->claim(0x5000,out));assert(out.count==2&&out.observed_end==16&&out.unknown_vertices==8&&out.kind_mask==5);
    assert(out.entries[0].receipt.vertices[0][0]==4&&out.entries[0].root_invocation==17&&out.entries[1].kind==3);
    auto detached=out;auto previous_generation=out.generation;
    assert(!s->claim(0x5000,out)&&same(out,detached));
    assert(s->publish(receipt(0x5000,0),2,19));assert(s->claim(0x5000,out)&&out.generation>previous_generation);
    assert(detached.entries[0].receipt.vertices[0][0]==4&&detached.count==2); // Independent Claim copy.

    // Metadata mismatch poisons the ENTIRE group. Later valid publication
    // cannot silently restart it before the consumer/retire boundary.
    for(unsigned field=0;field<4;++field){assert(s->publish(receipt(0x6000,0),1,1));auto conflict=receipt(0x6000,4);
        if(field==0)++conflict.canvas;if(field==1)++conflict.fcanvas;if(field==2)++conflict.item;if(field==3)++conflict.sort_key;
        assert(!s->publish(conflict,1,2));assert(!s->publish(receipt(0x6000,8),1,3));
        auto before=out;assert(!s->claim(0x6000,out)&&same(out,before));
        assert(s->publish(receipt(0x6000,0),1,4));assert(s->retire(0x6000));}
    for(unsigned begin:{0u,2u}){assert(s->publish(receipt(0x7000,0),1,1));assert(!s->publish(receipt(0x7000,begin),1,2));
        assert(s->stats().poisoned_groups==1);assert(s->retire(0x7000));assert(!s->retire(0x7000));}
    assert(s->publish(receipt(0x7000,8),1,1));assert(!s->publish(receipt(0x7000,0),1,2));assert(s->retire(0x7000));
    // Invalid kind, invocation and quad bounds also fail closed for this key.
    for(unsigned failure=0;failure<4;++failure){assert(s->publish(receipt(0x8000,0),1,1));auto invalid=receipt(0x8000,4);
        if(failure==0)invalid.end=invalid.begin;if(failure==1)invalid.end=invalid.begin+8;
        assert(!s->publish(invalid,failure==2?4:1,failure==3?0:2));assert(!s->publish(receipt(0x8000,8),1,3));assert(s->retire(0x8000));}
    auto invalid=receipt(0,0);assert(!s->publish(invalid,1,1));

    for(unsigned i=0;i<entry_capacity;++i)assert(s->publish(receipt(0x9000,i*4),1,i+1));
    assert(!s->publish(receipt(0x9000,entry_capacity*4),1,100));
    assert(!s->publish(receipt(0x9000,entry_capacity*4+4),1,101));
    auto before=out;assert(!s->claim(0x9000,out)&&same(out,before));
    assert(s->publish(receipt(0x9000,0),1,102));assert(s->claim(0x9000,out)&&out.count==1);

    // Tombstones consume slots. Failure to remember a NEW key latches the
    // whole Store disabled; not even preexisting successful groups may claim.
    for(unsigned i=0;i<group_capacity;++i)assert(s->publish(receipt(0xa000+i,0),1,i+1));
    assert(!s->publish(receipt(0xb000,0),1,100));assert(s->stats().disabled&&s->stats().poisoned_groups==group_capacity);
    before=out;assert(!s->claim(0xa000,out)&&same(out,before));assert(s->retire(0xa001));
    assert(!s->publish(receipt(0xb000,4),1,101));assert(!s->publish(receipt(0xa000,4),1,102));
    auto pre_reset_generation=out.generation;s->reset();assert(!s->stats().disabled&&s->stats().active_groups==0);
    assert(s->publish(receipt(0xc000,0),3,200));assert(s->claim(0xc000,out)&&out.generation>pre_reset_generation);
    // Explicit reset is exercised only by this CPU test, never runtime here.
    std::thread left([&](){for(unsigned i=0;i<entry_capacity;++i)assert(s->publish(receipt(0xd000,i*4),1,i+1));});
    std::thread right([&](){for(unsigned i=0;i<entry_capacity;++i)assert(s->publish(receipt(0xe000,i*4),2,i+1));});
    left.join();right.join();assert(s->claim(0xd000,out)&&out.count==entry_capacity&&out.unknown_vertices==0);
    assert(s->claim(0xe000,out)&&out.count==entry_capacity&&out.kind_mask==2);
    assert(s->stats().capacity_rejected==2&&s->stats().unknown_vertices_seen>=8&&s->stats().active_groups==0);
    printf("Payload receipts: bounded copy claims, monotonic tokens, gaps, poison tombstones, overflow latch and concurrent publishers passed\n");
}
