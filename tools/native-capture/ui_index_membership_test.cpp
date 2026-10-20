#include "ui_index_membership.h"
#include <cassert>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <cstdio>

using namespace gears_ui_indices;
using Array=gears_ui_batch::detail::Array;
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    std::function<void(std::uintptr_t,std::size_t)> before_read;
    void block(std::uintptr_t address,std::size_t size){blocks[address].resize(size);}
    template<class T> void put(std::uintptr_t address,T const& value){
        for(auto& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&sizeof(value)<=b.second.size()-(address-b.first)){
            std::memcpy(b.second.data()+address-b.first,&value,sizeof(value));return;}assert(false);
    }
    bool read(void* out,const void* source,std::size_t size){auto address=reinterpret_cast<std::uintptr_t>(source);
        if(before_read)before_read(address,size);
        for(auto const& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&size<=b.second.size()-(address-b.first)){
            std::memcpy(out,b.second.data()+address-b.first,size);return true;}return false;
    }
};
struct Fixture {
    Memory m;const std::uintptr_t payload=0x1000,segments=0x2000,vertices=0x3000;
    unsigned count{};
    Fixture(){m.block(payload,0x80);m.block(segments,segment_capacity*36);m.block(vertices,256*gears_ui_batch::vertex_stride);
        m.put(payload+0x38,Array{segments,0,64});m.put(payload+0x48,Array{vertices,256,256});}
    void segment(unsigned ordinal,int minimum,int maximum,std::vector<unsigned> const& indices){
        auto data=std::uintptr_t(0x10000+ordinal*0x10000);m.block(data,std::max<std::size_t>(1,indices.size()*4));
        for(unsigned i=0;i<indices.size();++i)m.put(data+i*4,indices[i]);
        auto address=segments+ordinal*36;m.put(address,std::int32_t(minimum));m.put(address+4,std::int32_t(maximum));
        m.put(address+8,Array{indices.empty()?0:data,int(indices.size()),int(indices.size())});
        m.put(address+24,std::uintptr_t(0x777));m.put(address+32,std::uint32_t(2));
        count=std::max(count,ordinal+1);m.put(payload+0x38,Array{segments,int(count),64});
    }
    auto reader(){return [this](void* out,const void* address,std::size_t size){return m.read(out,address,size);};}
};
static gears_ui_receipts::Claim claim(std::uintptr_t payload){
    auto store=std::make_unique<gears_ui_receipts::Store>();
    for(unsigned i=0;i<2;++i){gears_ui_batch::Receipt r{};r.payload=payload;r.canvas=0x500000;r.fcanvas=0x500100;r.item=0x500200;
        r.begin=i?108:100;r.end=r.begin+4;r.sort_key=-7;assert(store->publish(r,i?3:1,i+1));}
    gears_ui_receipts::Claim out{};assert(store->claim(payload,out));return out;
}
static bool same(Snapshot const& a,Snapshot const& b){
    if(a.payload!=b.payload||a.generation!=b.generation||a.vertex_count!=b.vertex_count||a.segment_count!=b.segment_count||
       a.total_indices!=b.total_indices||a.indices!=b.indices||a.global_vertices!=b.global_vertices)return false;
    for(unsigned i=0;i<segment_capacity;++i){auto const& x=a.segments[i];auto const& y=b.segments[i];
        if(x.minimum!=y.minimum||x.maximum!=y.maximum||x.texture!=y.texture||x.blend!=y.blend||x.index_offset!=y.index_offset||
           x.index_count!=y.index_count||x.kind_mask!=y.kind_mask||x.entry_mask!=y.entry_mask||x.known!=y.known||x.unknown!=y.unknown||!gears_ui_indices::same(x.expected,y.expected))return false;
    }return true;
}
static void saturation_membership(){
    using namespace gears_ui_receipts;
    auto store=std::make_unique<Store>();
    auto publish=[&](std::uintptr_t payload,unsigned begin){gears_ui_batch::Receipt r{};
        r.payload=payload;r.canvas=0x500000;r.fcanvas=0x500100;r.item=0x500200;
        r.begin=begin;r.end=begin+4;return store->publish(r,1,77);};
    for(unsigned i=0;i<group_capacity;++i)assert(publish(0x400000+i,0));
    Fixture f;assert(!publish(f.payload,0)&&!publish(f.payload,4));
    Claim omitted{};assert(!store->claim(f.payload,omitted));
    Claim retained{};assert(store->claim(0x400000,retained));
    assert(publish(f.payload,8));Claim partial{};assert(store->claim(f.payload,partial));
    assert(partial.unknown_vertices==8&&partial.generation>retained.generation);
    f.segment(0,0,11,{0,1,2,2,1,3}); // All dropped prefix.
    f.segment(1,0,11,{0,8,9});       // Mixed dropped and recorded.
    f.segment(2,8,11,{0,1,2,2,1,3}); // Only certified suffix.
    Snapshot out{};assert(capture(f.reader(),f.payload,partial,out));
    assert(out.segments[0].kind_mask==0&&out.segments[0].unknown==6);
    assert(out.segments[1].kind_mask==1&&out.segments[1].unknown==1);
    assert(out.segments[2].kind_mask==1&&out.segments[2].unknown==0&&out.segments[2].known[0]==6);
    assert(!store->stats().disabled&&store->stats().groups_full==2);
}
int main(){
    saturation_membership();
    Fixture f;f.segment(0,100,111,{11,0,11,4,1,4});f.segment(1,100,111,{});f.segment(2,100,111,{0,1,2});
    auto certified=claim(f.payload);Snapshot out{};assert(capture(f.reader(),f.payload,certified,out));
    assert(out.segment_count==3&&out.total_indices==9&&out.vertex_count==256);
    const std::array<unsigned,6> expected_global{111,100,111,104,101,104};
    for(unsigned i=0;i<6;++i)assert(out.global_vertices[i]==expected_global[i]);
    assert((out.segments[0].known==std::array<unsigned,3>{2,0,2})&&out.segments[0].unknown==2);
    assert(out.segments[0].kind_mask==5&&out.segments[0].entry_mask==3);
    assert(out.segments[1].index_count==0&&out.segments[1].kind_mask==0);
    assert(out.segments[2].known[0]==3&&out.segments[2].unknown==0);
    Cursor correct(out);unsigned ordinal=99;
    assert(correct.consume({6,1,0,0,0},ordinal)&&ordinal==0);
    assert(correct.consume({0,1,0,0,0},ordinal)&&ordinal==1); // Zero draw is not skipped.
    assert(correct.consume({3,1,0,0,0},ordinal)&&ordinal==2);
    assert(!correct.completed()&&correct.finish()&&correct.completed());
    assert(!correct.consume({3,1,0,0,0},ordinal)&&ordinal==2&&correct.poisoned()&&!correct.finish());
    Cursor missing(out);assert(missing.consume({6,1,0,0,0},ordinal));assert(!missing.finish()&&missing.poisoned());
    Cursor order(out);ordinal=55;assert(!order.consume({3,1,0,0,0},ordinal)&&ordinal==55);
    assert(!order.consume({6,1,0,0,0},ordinal)&&ordinal==55&&!order.finish());
    for(unsigned field=0;field<5;++field){Cursor mismatch(out);DrawArgs args{6,1,0,0,0};
        if(field==0)args.index_count=3;if(field==1)args.instance_count=2;if(field==2)args.start_index=1;
        if(field==3)args.base_vertex=-1;if(field==4)args.first_instance=1;
        ordinal=77;assert(!mismatch.consume(args,ordinal)&&ordinal==77&&mismatch.poisoned());}

    // TRUE counterexample to range-intersection tagging: upload includes the
    // HUD interval108..112, but no actually referenced index reaches it.
    Fixture intersects;intersects.segment(0,100,111,{0,1,2,2,1,3});Snapshot range{};
    assert(capture(intersects.reader(),intersects.payload,certified,range));
    assert(range.segments[0].kind_mask==1&&range.segments[0].known[0]==6&&range.segments[0].known[2]==0);
    intersects.segment(0,104,111,{0,1,2});
    assert(capture(intersects.reader(),intersects.payload,certified,range)&&range.segments[0].unknown==3&&range.segments[0].kind_mask==0);
    gears_ui_receipts::Claim absent{};assert(capture(f.reader(),f.payload,absent,range));
    assert(range.segments[0].unknown==6&&range.segments[0].kind_mask==0);
    Fixture empty;empty.m.put(empty.payload+0x38,Array{});empty.m.put(empty.payload+0x48,Array{});
    assert(capture(empty.reader(),empty.payload,absent,range));Cursor no_draws(range);assert(no_draws.finish()&&no_draws.completed());

    auto saved=out;
    for(unsigned failure=0;failure<9;++failure){Fixture bad;bad.segment(0,100,111,{0,1,2});
        switch(failure){
        case 0:bad.m.put(bad.segments,std::int32_t(-1));break;
        case 1:bad.m.put(bad.segments+4,std::int32_t(99));break;
        case 2:bad.m.put(bad.segments+4,std::int32_t(256));break;
        case 3:bad.m.put(0x10000,std::uint32_t(12));break;
        case 4:bad.m.put(bad.segments+8,Array{0x10000,2,3});break;
        case 5:bad.m.put(bad.segments+8,Array{0,3,3});break;
        case 6:bad.m.put(bad.payload+0x38,Array{bad.segments,65,65});break;
        case 7:bad.m.put(bad.segments+8,Array{(std::numeric_limits<std::uintptr_t>::max)()-4,3,3});break;
        case 8:bad.m.blocks.erase(0x10000);break;}
        assert(!capture(bad.reader(),bad.payload,certified,out)&&same(out,saved));}
    Fixture overflow;overflow.segment(0,0,0,std::vector<unsigned>(8190,0));overflow.segment(1,0,0,{0,0,0});
    assert(!capture(overflow.reader(),overflow.payload,absent,out)&&same(out,saved));
    Fixture changing;changing.segment(0,100,111,{0,1,2});unsigned reads=0;
    changing.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==0x10000&&++reads==2)changing.m.put(0x10000,std::uint32_t(1));};
    assert(!capture(changing.reader(),changing.payload,certified,out)&&same(out,saved));
    changing.m.before_read={};changing.segment(0,100,111,{0,1,2});reads=0;
    changing.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==changing.segments&&++reads==2)
        changing.m.put(changing.segments+32,std::uint32_t(9));};
    assert(!capture(changing.reader(),changing.payload,certified,out)&&same(out,saved));
    changing.m.before_read={};changing.segment(0,100,111,{0,1,2});reads=0;
    changing.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==changing.payload+0x38&&++reads==2)
        changing.m.put(changing.payload+0x38,Array{changing.segments,2,64});};
    assert(!capture(changing.reader(),changing.payload,certified,out)&&same(out,saved));
    changing.m.before_read={};changing.segment(0,100,111,{0,1,2});reads=0;
    changing.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==changing.payload+0x48&&++reads==2)
        changing.m.put(changing.payload+0x48,Array{changing.vertices,255,256});};
    assert(!capture(changing.reader(),changing.payload,certified,out)&&same(out,saved));
    auto badclaim=certified;badclaim.entries[1].receipt.begin=102;badclaim.entries[1].receipt.end=106;
    assert(!capture(f.reader(),f.payload,badclaim,out)&&same(out,saved));
    badclaim=certified;badclaim.payload=0xdead;assert(!capture(f.reader(),f.payload,badclaim,out)&&same(out,saved));
    auto badsnapshot=saved;badsnapshot.segment_count=65;Cursor invalid(badsnapshot);assert(invalid.poisoned()&&!invalid.finish());
    badsnapshot=saved;badsnapshot.total_indices=8193;Cursor too_many(badsnapshot);assert(too_many.poisoned());
    badsnapshot=saved;badsnapshot.segments[0].kind_mask=1;Cursor bad_membership(badsnapshot);assert(bad_membership.poisoned());
    // A variable-size text range participates in actual-index membership as a
    // single interval. Neither quad splitting nor 32-entry truncation is used.
    auto store=std::make_unique<gears_ui_receipts::Store>();std::vector<gears_ui_receipts::Vertex> text_vertices(256);
    gears_ui_receipts::RangeMetadata text{0x500000,0x500100,0x500200,f.payload,-7,0,256};
    assert(store->publish_range(text,text_vertices.data(),256,3,2000,gears_ui_receipts::Source::CanvasText));
    gears_ui_receipts::Claim long_text{};assert(store->claim(f.payload,long_text));
    Fixture text_indices;text_indices.segment(0,0,255,{0,255,128,128,255,0});
    assert(capture(text_indices.reader(),text_indices.payload,long_text,out));
    assert(out.segments[0].known[2]==6&&out.segments[0].unknown==0&&out.segments[0].entry_mask==1);
    text.canvas=0;assert(store->publish_range(text,text_vertices.data(),256,3,2001,gears_ui_receipts::Source::NativeSubtitle));
    assert(store->claim(f.payload,long_text)&&capture(text_indices.reader(),text_indices.payload,long_text,out));
    auto bad_range=long_text;bad_range.entries[0].vertex_offset=1;auto stable=out;
    assert(!capture(text_indices.reader(),text_indices.payload,bad_range,out)&&same(out,stable));
    bad_range=long_text;bad_range.vertex_count=255;assert(!capture(text_indices.reader(),text_indices.payload,bad_range,out)&&same(out,stable));
    bad_range=long_text;bad_range.vertices.reset();assert(!capture(text_indices.reader(),text_indices.payload,bad_range,out)&&same(out,stable));
    printf("Index membership: min-relative/reordered/repeated indices, unknown/mixed references, stable copies, zero draws and fail-closed cursor completion passed\n");
}
