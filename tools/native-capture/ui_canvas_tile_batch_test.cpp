#include "ui_canvas_tile_batch.h"
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>

using gears_ui_batch::detail::Array;
struct Fixture {
    gears_ui_batch::Config config{0x10000000};
    static constexpr std::uintptr_t canvas=0x1000,fcanvas=0x2000,keys=0x3000,elements=0x4000,
        old_items=0x5000,first=0x6000,foreign=0x6100,foreign_vtable=0xbeef0000,
        new_items=0x9000,new_item=0xa000,payload=0xb000,vertices=0xc000;
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    std::function<void(std::uintptr_t,std::size_t)> before_read;
    unsigned forbidden_reads{},freed_reads{};bool appended{},foreign_type=true;
    template<class T> void put(std::uintptr_t address,T const& value){
        for(auto& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&sizeof(value)<=b.second.size()-(address-b.first)){
            std::memcpy(b.second.data()+address-b.first,&value,sizeof(value));return;}assert(false);
    }
    Fixture(){
        blocks[canvas].resize(0x200);blocks[fcanvas].resize(0x100);blocks[keys].resize(8);
        blocks[elements].resize(60);blocks[old_items].resize(16);blocks[first].resize(16);blocks[foreign].resize(16);
        blocks[config.base+0x133cc98].resize(16);blocks[config.base+0x1a44e0].resize(4);
        put(config.base+0x133cc98+8,config.base+0x1a44e0);
        put(config.base+0x1a44e0,std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3});
        put(canvas+0x9c,fcanvas);put(fcanvas+0x28,Array{keys,2,2});put(keys,std::int32_t(3));put(keys+4,std::int32_t(7));
        put(fcanvas+0x48,Array{elements,3,3});
        for(unsigned i=0;i<3;++i){put(elements+i*20,std::int32_t(i==1?7:10+i));put(elements+i*20+4,Array{});}
        put(elements+24,Array{old_items,2,2});put(old_items,first);put(old_items+8,foreign);
        put(first,config.base+0x133cc98);put(first+8,payload);put(foreign,foreign_vtable);
    }
    void append(unsigned item_delta=1,unsigned vertex_count=4){
        blocks[new_items].resize(32);put(new_items,first);put(new_items+8,foreign);put(new_items+16,new_item);
        put(new_items+24,new_item);put(elements+24,Array{new_items,int(2+item_delta),4});
        blocks.erase(old_items);appended=true;
        blocks[new_item].resize(16);put(new_item,config.base+0x133cc98);put(new_item+8,payload);
        blocks[payload].resize(0x80);blocks[vertices].resize(8*gears_ui_batch::vertex_stride);
        put(payload+0x48,Array{vertices,int(vertex_count),8});
        for(unsigned i=0;i<8;++i){gears_ui_batch::Vertex v{};
            for(unsigned j=0;j<v.size();++j)v[j]=static_cast<unsigned char>(i*48+j);
            put(vertices+i*48,v);put(vertices+i*48+44,std::array<unsigned char,4>{0xde,0xad,0xbe,0xef});}
    }
    bool read(void* out,const void* source,std::size_t size){
        const auto address=reinterpret_cast<std::uintptr_t>(source);
        if(foreign_type&&(address==foreign+8||address==foreign_vtable)){++forbidden_reads;return false;}
        if(appended&&address>=old_items&&address<old_items+64*8){++freed_reads;return false;}
        if(before_read)before_read(address,size);
        for(auto const& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&size<=b.second.size()-(address-b.first)){
            std::memcpy(out,b.second.data()+address-b.first,size);return true;}return false;
    }
    auto reader(){return [this](void* out,const void* address,std::size_t size){return read(out,address,size);};}
    auto before(){gears_ui_tile_batch::Before out{};assert(gears_ui_tile_batch::capture_before(config,reader(),canvas,out));return out;}
};
static bool same(gears_ui_batch::Receipt const& a,gears_ui_batch::Receipt const& b){
    return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&a.item==b.item&&a.payload==b.payload&&
        a.sort_key==b.sort_key&&a.begin==b.begin&&a.end==b.end&&a.vertices==b.vertices;
}
int main(){
    Fixture f;gears_ui_batch::Snapshot strict{};
    assert(!gears_ui_batch::snapshot(f.config,f.reader(),f.canvas,strict));
    auto before=f.before();assert(before.foreign_valid&&!before.strict_valid&&before.foreign.prefix_count==2);
    assert(before.foreign.vtable==f.foreign_vtable&&before.foreign.prefix[1]==f.foreign);
    f.append();gears_ui_batch::Receipt result{};
    assert(!gears_ui_batch::appended_quad(f.config,f.reader(),strict,f.canvas,result)); // Strict API still rejects this before state.
    assert(gears_ui_tile_batch::appended_quad(f.config,f.reader(),before,f.canvas,result));
    assert(result.payload==f.payload&&result.item==f.new_item&&result.begin==0&&result.end==4);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<44;++j)assert(result.vertices[i][j]==static_cast<unsigned char>(i*48+j));
    assert(f.forbidden_reads==0&&f.freed_reads==0); // Includes old array realloc/free and canary padding.
    // Strict success still delegates to its original range-in-same-payload path.
    auto strict_before=f.before();assert(strict_before.strict_valid&&!strict_before.foreign_valid);
    f.put(f.payload+0x48,Array{f.vertices,8,8});
    assert(gears_ui_tile_batch::appended_quad(f.config,f.reader(),strict_before,f.canvas,result)&&result.begin==4);
    const auto kept=result;
    for(unsigned scenario=0;scenario<6;++scenario){Fixture n;auto b=n.before();n.append();
        switch(scenario){
        case 0:b.foreign.prefix_count=0;break;
        case 1:b.foreign.prefix_count=65;break;
        case 2:b.foreign.items.count=3;break;
        case 3:b.foreign.prefix[1]=n.first;break;
        case 4:b.foreign.prefix[0]=n.foreign;break;
        case 5:b.strict_valid=true;break;
        }
        result=kept;assert(!gears_ui_tile_batch::appended_quad(n.config,n.reader(),b,n.canvas,result));
        assert(same(result,kept)&&n.forbidden_reads==0&&n.freed_reads==0);
    }
    // Maximum64-pointer before prefix permits exactly one new65th item; all
    // prefix values are read from the reallocated POST array, never the freed one.
    Fixture boundary;boundary.blocks[boundary.old_items].resize(64*8);
    for(unsigned i=0;i<63;++i)boundary.put(boundary.old_items+i*8,std::uintptr_t(0x200000+i*16));
    boundary.put(boundary.old_items+63*8,boundary.foreign);
    boundary.put(boundary.elements+24,Array{boundary.old_items,64,64});auto maximum=boundary.before();
    boundary.append();boundary.blocks[boundary.new_items].resize(65*8);
    for(unsigned i=0;i<64;++i)boundary.put(boundary.new_items+i*8,maximum.foreign.prefix[i]);
    boundary.put(boundary.new_items+64*8,boundary.new_item);
    boundary.put(boundary.elements+24,Array{boundary.new_items,65,65});
    assert(gears_ui_tile_batch::appended_quad(boundary.config,boundary.reader(),maximum,boundary.canvas,result));
    assert(result.begin==0&&result.end==4&&boundary.forbidden_reads==0&&boundary.freed_reads==0);
    for(unsigned scenario=0;scenario<13;++scenario){Fixture n;auto b=n.before();
        if(scenario!=0)n.append(scenario==1?2:1,scenario==2?0:scenario==3?8:4);
        switch(scenario){
        case 4:n.put(n.new_items,n.foreign);break; // Prefix replaced/duplicated.
        case 5:n.put(n.new_items,n.foreign);n.put(n.new_items+8,n.first);break; // Prefix reorder.
        case 6:n.put(n.new_items+16,n.first);break; // New simple tail aliases a prior item.
        case 7:n.put(n.foreign,n.config.base+0x133cc98);n.foreign_type=false;break; // Prior foreign type flips.
        case 8:n.put(n.new_item,n.foreign_vtable);break;
        case 9:n.put(n.keys+4,std::int32_t(8));break;
        case 10:n.put(n.canvas+0x9c,std::uintptr_t(0x2010));break;
        case 11:n.put(n.config.base+0x1a44e0,std::array<unsigned char,4>{});break;
        case 12:b.foreign_valid=false;break;
        }
        result=kept;assert(!gears_ui_tile_batch::appended_quad(n.config,n.reader(),b,n.canvas,result));
        assert(same(result,kept)&&n.forbidden_reads==0&&n.freed_reads==0);
    }
    // Mutations after the quad copy starts must fail final post/prefix/type checks.
    for(unsigned scenario=0;scenario<4;++scenario){Fixture n;auto b=n.before();n.append();bool fired=false;
        n.before_read=[&](std::uintptr_t address,std::size_t size){if(fired||address!=n.vertices||size!=44)return;fired=true;
            switch(scenario){
            case 0:n.put(n.new_items,n.foreign);break;
            case 1:n.put(n.payload+0x48,Array{n.vertices,3,8});break;
            case 2:n.put(n.foreign,std::uintptr_t(0xcafe0000));break;
            case 3:n.put(n.new_item,std::uintptr_t(0xcafe0000));break;
            }
        };
        result=kept;assert(!gears_ui_tile_batch::appended_quad(n.config,n.reader(),b,n.canvas,result));
        assert(fired&&same(result,kept)&&n.forbidden_reads==0&&n.freed_reads==0);
    }
    // The fallback cannot turn a bad simple tail, empty chain, duplicate key or
    // oversized prefix into an empty-before/new-batch authorization.
    for(unsigned scenario=0;scenario<4;++scenario){Fixture n;gears_ui_tile_batch::Before target=before;
        switch(scenario){
        case 0:n.put(n.foreign,n.config.base+0x133cc98);n.foreign_type=false;break;
        case 1:n.put(n.elements+24,Array{});break;
        case 2:n.put(n.elements,std::int32_t(7));break;
        case 3:n.put(n.elements+24,Array{n.old_items,65,65});break;
        }
        // A genuinely empty chain remains handled strictly, without fallback.
        if(scenario==1){assert(gears_ui_tile_batch::capture_before(n.config,n.reader(),n.canvas,target)&&target.strict_valid&&!target.foreign_valid);continue;}
        assert(!gears_ui_tile_batch::capture_before(n.config,n.reader(),n.canvas,target));
        assert(target.foreign_valid&&target.foreign.tail==before.foreign.tail&&n.forbidden_reads==0);
    }
    // A before prefix mutation between the two traversals is not certified.
    Fixture unstable;unsigned item_reads=0;
    unstable.before_read=[&](std::uintptr_t address,std::size_t){
        // The failed strict snapshot reads only the tail, not prefix[0].
        if(address==unstable.old_items&&++item_reads==2)unstable.put(unstable.old_items,unstable.foreign);
    };
    gears_ui_tile_batch::Before rejected{};
    assert(!gears_ui_tile_batch::capture_before(unstable.config,unstable.reader(),unstable.canvas,rejected));
    assert(unstable.forbidden_reads==0);
    std::puts("Canvas Tile typed transition: foreign-before/new-simple quad, realloc, prefix/type/alias/mutation rejection passed");
}
