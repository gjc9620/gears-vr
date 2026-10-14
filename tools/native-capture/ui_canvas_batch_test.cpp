#include "ui_canvas_batch.h"
#include <cassert>
#include <map>
#include <vector>
#include <functional>
#include <cstdio>

using namespace gears_ui_batch;
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
    Config c{0x10000000};Memory m;
    const std::uintptr_t canvas=0x1000,fcanvas=0x2000,keys=0x3000,elements=0x4000,items=0x5000,item=0x6000,payload=0x7000,vertices=0x8000;
    Fixture(){
        m.block(canvas,0x200);m.block(fcanvas,0x100);m.block(keys,8);m.block(elements,60);
        m.block(items,16);m.block(item,16);m.block(payload,0x80);m.block(vertices,8*vertex_stride);
        m.block(c.base+0x133cc98,16);m.block(c.base+0x1a44e0,4);
        m.put(c.base+0x133cc98+8,c.base+0x1a44e0);m.put(c.base+0x1a44e0,std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3});
        m.put(canvas+0x9c,fcanvas);m.put(fcanvas+0x28,detail::Array{keys,2,2});m.put(keys,std::int32_t(3));m.put(keys+4,std::int32_t(7));
        m.put(fcanvas+0x48,detail::Array{elements,3,3});
        for(unsigned i=0;i<3;++i){m.put(elements+i*20,std::int32_t(i==1?7:10+i));m.put(elements+i*20+4,detail::Array{});}
        m.put(elements+20+4,detail::Array{items,1,2});m.put(items,item);
        m.put(item,c.base+0x133cc98);m.put(item+8,payload);m.put(payload+0x48,detail::Array{vertices,4,8});fill(vertices,8);
    }
    void fill(std::uintptr_t address,unsigned count){for(unsigned i=0;i<count;++i){Vertex v{};
        for(unsigned j=0;j<v.size();++j)v[j]=static_cast<unsigned char>(i*vertex_stride+j);
        m.put(address+i*vertex_stride,v);m.put(address+i*vertex_stride+v.size(),std::array<unsigned char,4>{0xde,0xad,0xbe,0xef});}}
    auto reader(){return [this](void* out,const void* address,std::size_t size){return m.read(out,address,size);};}
};
static bool same(Receipt const& a,Receipt const& b){return a.canvas==b.canvas&&a.fcanvas==b.fcanvas&&a.item==b.item&&a.payload==b.payload&&
    a.sort_key==b.sort_key&&a.begin==b.begin&&a.end==b.end&&a.vertices==b.vertices;}
int main(){
    Fixture f;auto read=f.reader();Snapshot before{};assert(snapshot(f.c,read,f.canvas,before));
    assert(!before.empty&&before.payload==f.payload&&before.vertex_count==4&&before.sort_key==7);
    Receipt out{};out.begin=999;auto unchanged=out;
    assert(!appended_quad(f.c,read,before,f.canvas,out)&&same(out,unchanged)); // Clipped/no append.
    f.m.put(f.payload+0x48,detail::Array{f.vertices,8,8});assert(appended_quad(f.c,read,before,f.canvas,out));
    assert(out.begin==4&&out.end==8&&out.item==f.item&&out.fcanvas==f.fcanvas);
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<44;++j)assert(out.vertices[i][j]==static_cast<unsigned char>((i+4)*vertex_stride+j));
    // A 44-byte-stride or contiguous-176 read would include these canaries and
    // misalign subsequent vertices. The copied arrays match only known fields.
    std::array<unsigned char,4> padding{};
    assert(read(padding.data(),reinterpret_cast<const void*>(f.vertices+4*vertex_stride+44),padding.size()));
    assert((padding==std::array<unsigned char,4>{0xde,0xad,0xbe,0xef}));

    Fixture realloc;Snapshot old{};assert(snapshot(realloc.c,realloc.reader(),realloc.canvas,old));
    realloc.m.block(0x9000,12*vertex_stride);realloc.fill(0x9000,12);realloc.m.blocks.erase(realloc.vertices);
    realloc.m.put(realloc.payload+0x48,detail::Array{0x9000,8,12});
    assert(appended_quad(realloc.c,realloc.reader(),old,realloc.canvas,out)&&out.begin==4&&out.vertices[0][0]==192);
    // Old data was freed above: success establishes reads use the POST array.
    Fixture fresh;Snapshot baseline{};assert(snapshot(fresh.c,fresh.reader(),fresh.canvas,baseline));
    fresh.m.block(0xa000,16);fresh.m.block(0xb000,0x80);fresh.m.block(0xc000,4*vertex_stride);fresh.fill(0xc000,4);
    fresh.m.put(fresh.items+8,std::uintptr_t(0xa000));fresh.m.put(fresh.elements+24,detail::Array{fresh.items,2,2});
    fresh.m.put(0xa000,fresh.c.base+0x133cc98);fresh.m.put(0xa008,std::uintptr_t(0xb000));fresh.m.put(0xb048,detail::Array{0xc000,4,4});
    assert(appended_quad(fresh.c,fresh.reader(),baseline,fresh.canvas,out)&&out.begin==0&&out.end==4&&out.payload==0xb000);
    auto kept=out;fresh.m.put(0xb048,detail::Array{0xc000,8,8});
    assert(!appended_quad(fresh.c,fresh.reader(),baseline,fresh.canvas,out)&&same(out,kept));
    // A different render item aliasing the same payload is not the same batch.
    fresh.m.put(0xa008,fresh.payload);fresh.m.put(fresh.payload+0x48,detail::Array{fresh.vertices,8,8});
    assert(!appended_quad(fresh.c,fresh.reader(),baseline,fresh.canvas,out)&&same(out,kept));

    for(bool missing_element:{true,false}){Fixture empty;Snapshot start{};
        if(missing_element)empty.m.put(empty.elements+20,std::int32_t(99));
        else empty.m.put(empty.elements+24,detail::Array{});
        assert(snapshot(empty.c,empty.reader(),empty.canvas,start)&&start.empty&&!start.payload);
        empty.m.put(empty.elements+20,std::int32_t(7));empty.m.put(empty.elements+24,detail::Array{empty.items,1,2});
        empty.m.put(empty.payload+0x48,detail::Array{empty.vertices,4,8});
        assert(appended_quad(empty.c,empty.reader(),start,empty.canvas,out)&&out.begin==0);
    }
    Fixture bad;Snapshot good{};assert(snapshot(bad.c,bad.reader(),bad.canvas,good));out.begin=999;unchanged=out;
    for(int count:{3,4,5,12}){bad.m.put(bad.payload+0x48,detail::Array{bad.vertices,count,count>8?12:8});
        assert(!appended_quad(bad.c,bad.reader(),good,bad.canvas,out)&&same(out,unchanged));}
    bad.m.put(bad.payload+0x48,detail::Array{bad.vertices,8,8});bad.m.put(bad.keys+4,std::int32_t(99));
    assert(!appended_quad(bad.c,bad.reader(),good,bad.canvas,out)&&same(out,unchanged));bad.m.put(bad.keys+4,std::int32_t(7));
    bad.m.put(bad.canvas+0x9c,std::uintptr_t(0xdead));assert(!appended_quad(bad.c,bad.reader(),good,bad.canvas,out)&&same(out,unchanged));
    bad.m.put(bad.canvas+0x9c,bad.fcanvas);assert(!appended_quad(bad.c,bad.reader(),good,0xdead,out)&&same(out,unchanged));
    // Bad reads, invalid types and invalid headers cannot become empty-before.
    for(unsigned scenario=0;scenario<7;++scenario){Fixture broken;Snapshot target=good;
        switch(scenario){
        case 0:broken.m.blocks.erase(broken.elements);break;
        case 1:broken.m.put(broken.item,std::uintptr_t(0xdead));break;
        case 2:broken.m.put(broken.item+8,std::uintptr_t(0));break;
        case 3:broken.m.put(broken.fcanvas+0x48,detail::Array{broken.elements,-1,3});break;
        case 4:broken.m.put(broken.payload+0x48,detail::Array{broken.vertices,9,8});break;
        case 5:broken.m.put(broken.payload+0x48,detail::Array{(std::numeric_limits<std::uintptr_t>::max)()-16,4,4});break;
        case 6:broken.m.put(broken.c.base+0x1a44e0,std::array<unsigned char,4>{});break;}
        assert(!snapshot(broken.c,broken.reader(),broken.canvas,target)&&detail::same(target,good));
    }
    Fixture unstable;Snapshot guard{};assert(snapshot(unstable.c,unstable.reader(),unstable.canvas,guard));
    unstable.m.put(unstable.payload+0x48,detail::Array{unstable.vertices,8,8});unsigned header_reads=0;
    unstable.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==unstable.payload+0x48&&++header_reads==2)
        unstable.m.put(unstable.payload+0x48,detail::Array{unstable.vertices,7,8});};
    assert(!appended_quad(unstable.c,unstable.reader(),guard,unstable.canvas,out)&&same(out,unchanged));
    Fixture during_vertices;assert(snapshot(during_vertices.c,during_vertices.reader(),during_vertices.canvas,guard));
    during_vertices.m.put(during_vertices.payload+0x48,detail::Array{during_vertices.vertices,8,8});
    during_vertices.m.before_read=[&](std::uintptr_t address,std::size_t size){if(address==during_vertices.vertices+4*vertex_stride&&size==44)
        during_vertices.m.put(during_vertices.payload+0x48,detail::Array{during_vertices.vertices,7,8});};
    assert(!appended_quad(during_vertices.c,during_vertices.reader(),guard,during_vertices.canvas,out)&&same(out,unchanged));
    Fixture short_vertex;assert(snapshot(short_vertex.c,short_vertex.reader(),short_vertex.canvas,guard));
    short_vertex.m.put(short_vertex.payload+0x48,detail::Array{short_vertex.vertices,8,8});
    short_vertex.m.blocks[short_vertex.vertices].resize(7*vertex_stride+43);
    assert(!appended_quad(short_vertex.c,short_vertex.reader(),guard,short_vertex.canvas,out)&&same(out,unchanged));
    Fixture duplicate;duplicate.m.put(duplicate.elements,std::int32_t(7));assert(!snapshot(duplicate.c,duplicate.reader(),duplicate.canvas,guard));
    Fixture element_key_race;unsigned key_scans=0;
    element_key_race.m.put(element_key_race.elements+20,std::int32_t(99));
    element_key_race.m.before_read=[&](std::uintptr_t address,std::size_t size){if(address==element_key_race.elements+20&&size==4&&++key_scans==1)
        element_key_race.m.put(element_key_race.elements+20,std::int32_t(7));};
    assert(!snapshot(element_key_race.c,element_key_race.reader(),element_key_race.canvas,guard));
    Fixture empty_array;empty_array.m.put(empty_array.fcanvas+0x48,detail::Array{});
    assert(snapshot(empty_array.c,empty_array.reader(),empty_array.canvas,guard)&&guard.empty);
    empty_array.m.put(empty_array.fcanvas+0x28,detail::Array{});
    assert(!snapshot(empty_array.c,empty_array.reader(),empty_array.canvas,guard)); // No current sort key is not a valid empty batch.
    assert(!snapshot(Config{(std::numeric_limits<std::uintptr_t>::max)()},f.reader(),f.canvas,guard));
    printf("Canvas batch: non-last sort element, realloc/new/empty batch, stable current headers and rejection without output mutation passed\n");
}
