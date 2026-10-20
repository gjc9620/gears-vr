#include "ui_canvas_tail_diagnostic.h"
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>

using gears_ui_batch::detail::Array;
using gears_ui_batch_diagnostic::Tail;
struct Fixture {
    static constexpr std::uintptr_t canvas=0x1000,fcanvas=0x2000,keys=0x3000,elements=0x4000,
        items=0x5000,item=0x6000,payload=0x7000,foreign_vtable=0xdead0000;
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    std::function<void(std::uintptr_t,std::size_t)> before_read;
    unsigned forbidden_reads{};
    template<class T> void put(std::uintptr_t address,T const& value){
        for(auto& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&sizeof(value)<=b.second.size()-(address-b.first)){
            std::memcpy(b.second.data()+address-b.first,&value,sizeof(value));return;}assert(false);
    }
    Fixture(){
        blocks[canvas].resize(0x200);blocks[fcanvas].resize(0x100);blocks[keys].resize(8);
        blocks[elements].resize(60);blocks[items].resize(16);blocks[item].resize(16);
        put(canvas+0x9c,fcanvas);put(fcanvas+0x28,Array{keys,2,2});put(keys,std::int32_t(3));put(keys+4,std::int32_t(7));
        put(fcanvas+0x48,Array{elements,3,3});
        for(unsigned i=0;i<3;++i){put(elements+i*20,std::int32_t(i==1?7:10+i));put(elements+i*20+4,Array{});}
        put(elements+24,Array{items,1,2});put(items,item);put(item,foreign_vtable);put(item+8,payload);
    }
    bool read(void* out,const void* source,std::size_t size){
        const auto address=reinterpret_cast<std::uintptr_t>(source);
        if(address==item+8||(address>=payload&&address<payload+0x100)||address==foreign_vtable){++forbidden_reads;return false;}
        if(before_read)before_read(address,size);
        for(auto const& b:blocks)if(address>=b.first&&address-b.first<=b.second.size()&&size<=b.second.size()-(address-b.first)){
            std::memcpy(out,b.second.data()+address-b.first,size);return true;}return false;
    }
    auto reader(){return [this](void* out,const void* address,std::size_t size){return read(out,address,size);};}
};
int main(){
    Fixture known;Tail out{};
    assert(gears_ui_batch_diagnostic::observe_tail(known.reader(),known.canvas,out));
    assert(!out.empty&&out.item==known.item&&out.vtable==known.foreign_vtable&&out.sort_key==7);
    assert(known.forbidden_reads==0); // No payload or foreign-vtable dereference, even with foreign type.
    const auto kept=out;
    // Each mutation happens on the second independent traversal. Every selected
    // chain identity/header/current key and exact tail identity must remain equal.
    for(unsigned scenario=0;scenario<8;++scenario){
        Fixture f;unsigned visits=0;
        const std::uintptr_t targets[]{f.canvas+0x9c,f.fcanvas+0x28,f.keys+4,f.fcanvas+0x48,
            f.elements+20,f.items,f.item,f.elements};
        f.before_read=[&](std::uintptr_t address,std::size_t){
            if(address!=targets[scenario]||++visits!=2)return;
            switch(scenario){
            case 0:f.put(f.canvas+0x9c,std::uintptr_t(0x2010));break;
            case 1:f.put(f.fcanvas+0x28,Array{f.keys,1,2});break;
            case 2:f.put(f.keys+4,std::int32_t(8));break;
            case 3:f.put(f.fcanvas+0x48,Array{f.elements,2,3});break;
            case 4:f.put(f.elements+24,Array{f.items,2,2});break;
            case 5:f.put(f.items,std::uintptr_t(0x6010));break;
            case 6:f.put(f.item,std::uintptr_t(0xbeef0000));break;
            case 7:f.put(f.elements,std::int32_t(7));break;
            }
        };
        out=kept;assert(!gears_ui_batch_diagnostic::observe_tail(f.reader(),f.canvas,out));
        assert(gears_ui_batch_diagnostic::same(out,kept)&&f.forbidden_reads==0);
    }
    for(unsigned scenario=0;scenario<5;++scenario){Fixture f;
        switch(scenario){
        case 0:f.put(f.fcanvas+0x48,Array{f.elements,4097,4097});break;
        case 1:f.put(f.elements+24,Array{f.items,65537,65537});break;
        case 2:f.put(f.fcanvas+0x28,Array{});break;
        case 3:f.put(f.elements,std::int32_t(7));break;
        case 4:f.blocks.erase(f.items);break;
        }
        out=kept;assert(!gears_ui_batch_diagnostic::observe_tail(f.reader(),f.canvas,out));
        assert(gears_ui_batch_diagnostic::same(out,kept)&&f.forbidden_reads==0);
    }
    Fixture empty;empty.put(empty.elements+24,Array{});
    assert(gears_ui_batch_diagnostic::observe_tail(empty.reader(),empty.canvas,out)&&out.empty&&!out.item);
    std::puts("Canvas tail diagnostic: bounded two-read chain, mutations and foreign tail without payload reads passed");
}
