#include "ui_payload_receipts.h"
#include <cassert>
#include <memory>
#include <thread>
#include <cstdio>
#include <vector>

using namespace gears_ui_receipts;
static gears_ui_batch::Receipt receipt(std::uintptr_t payload,unsigned begin){
    gears_ui_batch::Receipt r{};r.canvas=0x1000;r.fcanvas=0x2000;r.item=0x3000;r.payload=payload;r.sort_key=-7;
    r.begin=begin;r.end=begin+4;
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<44;++j)r.vertices[i][j]=static_cast<unsigned char>(begin+i*gears_ui_batch::vertex_stride+j);
    return r;
}
static bool same(Claim const& a,Claim const& b){
    if(a.generation!=b.generation||a.payload!=b.payload||a.canvas!=b.canvas||a.fcanvas!=b.fcanvas||a.item!=b.item||
       a.sort_key!=b.sort_key||a.count!=b.count||a.observed_end!=b.observed_end||a.unknown_vertices!=b.unknown_vertices||a.kind_mask!=b.kind_mask||
       a.vertex_count!=b.vertex_count||bool(a.vertices)!=bool(b.vertices))return false;
    if(a.vertices&&a.vertices->data!=b.vertices->data)return false;
    for(unsigned i=0;i<entry_capacity;++i){auto const& x=a.entries[i];auto const& y=b.entries[i];
        if(x.kind!=y.kind||x.root_invocation!=y.root_invocation||x.receipt.canvas!=y.receipt.canvas||
           x.receipt.fcanvas!=y.receipt.fcanvas||x.receipt.item!=y.receipt.item||x.receipt.payload!=y.receipt.payload||
           x.receipt.sort_key!=y.receipt.sort_key||x.receipt.begin!=y.receipt.begin||x.receipt.end!=y.receipt.end||x.vertex_offset!=y.vertex_offset||x.source!=y.source)return false;
    }return true;
}
static std::shared_ptr<VertexBuffer> no_memory(){return {};}
static std::shared_ptr<VertexBuffer> throw_no_memory(){throw std::bad_alloc();}
static std::shared_ptr<VertexBuffer> shared_buffer;
static std::shared_ptr<VertexBuffer> aliased_memory(){return shared_buffer;}
int main(){
    auto s=std::make_unique<Store>();Claim out{};out.generation=999;out.payload=0xdead;auto unchanged=out;
    assert(!s->claim(0xdead,out)&&same(out,unchanged));
    auto first=receipt(0x5000,4);assert(s->publish(first,1,17));first.vertices[0][0]=255;
    assert(s->publish(receipt(0x5000,12),3,18));
    assert(s->claim(0x5000,out));assert(out.count==2&&out.observed_end==16&&out.unknown_vertices==8&&out.kind_mask==5);
    assert(out.vertices->data[out.entries[0].vertex_offset][0]==4&&out.entries[0].root_invocation==17&&out.entries[1].kind==3&&out.vertex_count==8);
    auto detached=out;auto previous_generation=out.generation;
    assert(!s->claim(0x5000,out)&&same(out,detached));
    assert(s->publish(receipt(0x5000,0),2,19));assert(s->claim(0x5000,out)&&out.generation>previous_generation);
    assert(detached.vertices->data[detached.entries[0].vertex_offset][0]==4&&detached.count==2); // Immutable owning Claim copy.

    // World retirement preserves queued keys, even text rollback cannot erase
    // them. Only consumption permits a later genuine producer at that address.
    assert(s->publish(receipt(0x5100,0),1,20));
    s->retire_all();assert(s->stats().poisoned_groups==1);
    assert(detached.vertices->data[0][0]==4); // Borrowed owning copy unchanged.
    assert(!s->publish(receipt(0x5100,0),2,21));
    assert(!s->retire(0x5100)); // Failed multi-atlas rollback keeps tombstone.
    assert(s->publish(receipt(0x5200,0),3,22)); // Other new world key works.
    assert(s->claim(0x5200,out));auto fresh_generation=out.generation;
    auto kept=out;assert(!s->claim(0x5100,out)&&same(out,kept));
    assert(s->publish(receipt(0x5100,0),2,23));
    assert(s->claim(0x5100,out)&&out.generation>fresh_generation&&out.entries[0].root_invocation==23);
    s->retire_all();s->retire_all(); // Empty/idempotent retirement.

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

    // Saturation drops only the new range. Retained data and tombstones stay
    // intact; an eventual partial claim cannot label the dropped prefix.
    for(unsigned i=0;i<group_capacity;++i)assert(s->publish(receipt(0xa000+i,0),1,i+1));
    assert(!s->publish(receipt(0xb000,0),1,100));assert(!s->stats().disabled&&!s->stats().poisoned_groups);
    before=out;assert(!s->claim(0xb000,out)&&same(out,before));
    assert(s->publish(receipt(0xa000,4),1,102)); // Existing key works while full.
    assert(s->claim(0xa000,out)&&out.count==2&&out.unknown_vertices==0);
    assert(s->publish(receipt(0xb000,4),1,101));
    assert(s->claim(0xb000,out)&&out.count==1&&out.entries[0].receipt.begin==4&&out.unknown_vertices==4);
    assert(s->stats().groups_full==1&&!s->stats().disabled);
    // Existing poison cannot be resurrected by unrelated saturation/draining.
    assert(!s->publish(receipt(0xa001,0),1,103));
    assert(!s->publish(receipt(0xa001,4),1,104));
    before=out;assert(!s->claim(0xa001,out)&&same(out,before));
    for(unsigned i=2;i<group_capacity;++i)assert(s->claim(0xa000+i,out)&&out.count==1);
    auto pre_reset_generation=out.generation;s->reset();assert(!s->stats().disabled&&s->stats().active_groups==0);
    assert(s->publish(receipt(0xc000,0),3,200));assert(s->claim(0xc000,out)&&out.generation>pre_reset_generation);
    // Explicit reset is exercised only by this CPU test, never runtime here.
    std::thread left([&](){for(unsigned i=0;i<entry_capacity;++i)assert(s->publish(receipt(0xd000,i*4),1,i+1));});
    std::thread right([&](){for(unsigned i=0;i<entry_capacity;++i)assert(s->publish(receipt(0xe000,i*4),2,i+1));});
    left.join();right.join();assert(s->claim(0xd000,out)&&out.count==entry_capacity&&out.unknown_vertices==0);
    assert(s->claim(0xe000,out)&&out.count==entry_capacity&&out.kind_mask==2);
    assert(s->stats().capacity_rejected==2&&s->stats().unknown_vertices_seen>=8&&s->stats().active_groups==0);
    // One 64-glyph range stores 256 vertices as ONE range, not 64 quad entries.
    std::vector<Vertex> glyphs(256);for(unsigned i=0;i<glyphs.size();++i)glyphs[i].fill(static_cast<unsigned char>(i));
    auto text=range_metadata(receipt(0xf000,4));text.end=260;
    assert(s->publish_range(text,glyphs.data(),glyphs.size(),3,1000,Source::CanvasText));
    glyphs[0][0]=99;assert(s->publish(receipt(0xf000,260),1,1001));
    assert(s->claim(0xf000,out)&&out.count==2&&out.vertex_count==260&&out.entries[1].vertex_offset==256);
    assert(out.vertices->data[0][0]==0&&out.vertices->data[255][0]==255&&out.unknown_vertices==4&&out.kind_mask==5);
    auto full_text=out;assert(s->publish(receipt(0xf000,0),2,1002));assert(s->retire(0xf000));
    assert(full_text.vertices->data[0][0]==0&&full_text.entries[0].source==Source::CanvasText);
    for(unsigned atlas=0;atlas<2;++atlas){auto range=text;range.payload=0xf100+atlas;range.item+=atlas;range.begin=0;range.end=256;
        assert(s->publish_range(range,glyphs.data(),glyphs.size(),3,1100+atlas,Source::CanvasText));}
    for(unsigned atlas=0;atlas<2;++atlas)assert(s->claim(0xf100+atlas,out)&&out.vertex_count==256&&out.count==1);
    std::vector<Vertex> maximum(vertex_capacity);auto all=text;all.payload=0xf200;all.begin=0;all.end=unsigned(vertex_capacity);
    assert(s->publish_range(all,maximum.data(),maximum.size(),3,1200,Source::CanvasText));
    assert(!s->publish(receipt(0xf200,unsigned(vertex_capacity)),3,1201));
    before=out;assert(!s->claim(0xf200,out)&&same(out,before));
    assert(s->publish_range(all,maximum.data(),maximum.size(),3,1202,Source::CanvasText));
    assert(s->claim(0xf200,out)&&out.vertex_count==vertex_capacity&&out.count==1);
    auto partial=text;partial.payload=0xf250;partial.begin=0;partial.end=256;
    assert(s->publish_range(partial,glyphs.data(),glyphs.size(),3,1250,Source::CanvasText));
    partial.begin=256;partial.end=1280;
    assert(!s->publish_range(partial,maximum.data(),maximum.size(),3,1251,Source::CanvasText));
    before=out;assert(!s->claim(0xf250,out)&&same(out,before));
    auto overlap=text;overlap.payload=0xf300;overlap.begin=0;overlap.end=256;
    assert(s->publish_range(overlap,glyphs.data(),glyphs.size(),3,1300,Source::CanvasText));
    assert(!s->publish(receipt(0xf300,4),3,1301));assert(s->retire(0xf300));
    // Explicit native subtitle provenance permits missing UCanvas; no FCanvas
    // address is forged into that field and no new semantic kind is invented.
    auto subtitle=text;subtitle.canvas=0;subtitle.payload=0xf400;subtitle.begin=0;subtitle.end=256;
    assert(!s->publish_range(subtitle,glyphs.data(),glyphs.size(),3,1400,Source::CanvasText));
    assert(s->publish_range(subtitle,glyphs.data(),glyphs.size(),3,1401,Source::NativeSubtitle));
    assert(s->claim(0xf400,out)&&!out.canvas&&out.entries[0].source==Source::NativeSubtitle&&!out.entries[0].whole_text_group);
    assert(s->publish_range(subtitle,glyphs.data(),glyphs.size(),3,1402,Source::NativeSubtitle,true));
    assert(s->claim(0xf400,out)&&out.entries[0].whole_text_group);
    assert(s->publish_range(subtitle,glyphs.data(),glyphs.size(),3,1402,Source::NativeSubtitle));
    auto not_native=subtitle;not_native.canvas=0x1000;not_native.begin=256;not_native.end=512;
    assert(!s->publish_range(not_native,glyphs.data(),glyphs.size(),3,1403,Source::CanvasText));assert(s->retire(0xf400));
    for(auto allocator:{no_memory,throw_no_memory}){auto failing=std::make_unique<Store>(allocator);
        assert(!failing->publish(receipt(0xf500,0),1,1500));assert(failing->stats().allocation_rejected==1&&failing->stats().poisoned_groups==1);
        assert(!failing->publish(receipt(0xf500,4),1,1501));before=out;
        assert(!failing->claim(0xf500,out)&&same(out,before));}
    shared_buffer=std::make_shared<VertexBuffer>();shared_buffer->data[0][0]=73;
    auto aliased=std::make_unique<Store>(aliased_memory);
    assert(!aliased->publish(receipt(0xf600,0),1,1600)&&aliased->stats().allocation_rejected==1);
    assert(shared_buffer->data[0][0]==73);shared_buffer.reset();
    printf("Payload receipts: bounded copy claims, monotonic tokens, gaps, poison tombstones, recoverable saturation and concurrent publishers passed\n");
}
