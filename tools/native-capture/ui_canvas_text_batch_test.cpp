#include "ui_canvas_text_batch.h"
#include <cassert>
#include <map>
#include <functional>
#include <cstdio>

using namespace gears_ui_text_batch;
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
    const std::uintptr_t canvas=0x1000,fcanvas=0x2000,keys=0x3000,elements=0x4000,item_array=0x5000;
    static std::uintptr_t item(unsigned i){return 0x10000+i*0x1000;}
    static std::uintptr_t payload(unsigned i){return 0x20000+i*0x1000;}
    static std::uintptr_t storage(unsigned i){return 0x40000+i*0x10000;}
    Fixture(){
        m.block(canvas,0x200);m.block(fcanvas,0x100);m.block(keys,8);m.block(elements,60);m.block(item_array,64*8);
        m.block(c.base+0x133cc98,16);m.block(c.base+0x1a44e0,4);
        m.put(c.base+0x133cc98+8,c.base+0x1a44e0);m.put(c.base+0x1a44e0,std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3});
        m.put(canvas+0x9c,fcanvas);m.put(fcanvas+0x28,detail::Array{keys,2,2});m.put(keys,std::int32_t(3));m.put(keys+4,std::int32_t(7));
        m.put(fcanvas+0x48,detail::Array{elements,3,3});
        for(unsigned i=0;i<3;++i){m.put(elements+i*20,std::int32_t(i==1?7:20+i));m.put(elements+i*20+4,detail::Array{});}
        add_item(0,4);add_item(1,8);set_items(2);
    }
    void set_items(unsigned count){m.put(elements+24,detail::Array{item_array,int(count),64});}
    void add_item(unsigned i,unsigned count){m.block(item(i),16);m.block(payload(i),0x80);
        m.put(item_array+i*8,item(i));m.put(item(i),c.base+0x133cc98);m.put(item(i)+8,payload(i));
        const unsigned cap=count>16?count:16;m.block(storage(i),cap*48);fill(storage(i),cap,i);
        m.put(payload(i)+0x48,detail::Array{storage(i),int(count),int(cap)});
    }
    void fill(std::uintptr_t address,unsigned count,unsigned salt=0){
        for(unsigned i=0;i<count;++i){std::array<unsigned char,48> v{};
            for(unsigned j=0;j<44;++j)v[j]=static_cast<unsigned char>(salt*31+i*7+j);
            for(unsigned j=44;j<48;++j)v[j]=0xed;m.put(address+i*48,v);}
    }
    void count(unsigned i,unsigned count){m.put(payload(i)+0x48,detail::Array{storage(i),int(count),16});}
    auto reader(){return [this](void* out,const void* address,std::size_t size){return m.read(out,address,size);};}
};
bool equal(Delta const& a,Delta const& b){
    if(a.canvas!=b.canvas||a.fcanvas!=b.fcanvas||a.sort_key!=b.sort_key||a.ranges.size()!=b.ranges.size()||a.vertices!=b.vertices)return false;
    for(std::size_t i=0;i<a.ranges.size();++i){auto const& x=a.ranges[i];auto const& y=b.ranges[i];
        if(x.item!=y.item||x.payload!=y.payload||x.begin!=y.begin||x.end!=y.end||x.vertex_offset!=y.vertex_offset)return false;}return true;
}
Delta sentinel(){Delta d{};d.canvas=99;d.fcanvas=98;d.sort_key=97;d.ranges.push_back({96,95,94,93,92});d.vertices.resize(1);d.vertices[0].fill(91);return d;}
int main(){
    Fixture f;Snapshot before{};assert(snapshot_fcanvas(f.c,f.reader(),f.fcanvas,before));
    assert(!before.canvas&&before.item_count==2&&before.items[0].vertex_count==4&&before.items[1].vertex_count==8);
    Delta out=sentinel();assert(delta_fcanvas(f.c,f.reader(),before,f.fcanvas,out)&&out.ranges.empty()&&out.vertices.empty());
    f.count(0,8);f.count(1,16);f.add_item(2,8);f.set_items(3);
    assert(delta_fcanvas(f.c,f.reader(),before,f.fcanvas,out));
    assert(out.ranges.size()==3&&out.vertices.size()==20);
    assert(out.ranges[0].begin==4&&out.ranges[0].end==8&&out.ranges[0].vertex_offset==0);
    assert(out.ranges[1].begin==8&&out.ranges[1].end==16&&out.ranges[1].vertex_offset==4);
    assert(out.ranges[2].begin==0&&out.ranges[2].end==8&&out.ranges[2].vertex_offset==12);
    for(unsigned j=0;j<44;++j){assert(out.vertices[0][j]==static_cast<unsigned char>(4*7+j));
        assert(out.vertices[4][j]==static_cast<unsigned char>(31+8*7+j));
        assert(out.vertices[12][j]==static_cast<unsigned char>(62+j));}
    // Each stored prefix is independently read at the real48-byte stride.
    assert(sizeof(out.vertices[0])==44&&gears_ui_batch::vertex_stride==48);

    Fixture realloc;assert(snapshot_fcanvas(realloc.c,realloc.reader(),realloc.fcanvas,before));
    realloc.m.block(0x900000,24*48);realloc.fill(0x900000,24);realloc.m.blocks.erase(Fixture::storage(0));
    realloc.m.put(Fixture::payload(0)+0x48,detail::Array{0x900000,12,24});
    assert(delta_fcanvas(realloc.c,realloc.reader(),before,realloc.fcanvas,out)&&out.vertices.size()==8);
    assert(out.vertices[0][0]==28&&out.vertices[4][0]==56); // No old allocation dereference.

    Fixture adapter;assert(snapshot_canvas(adapter.c,adapter.reader(),adapter.canvas,before));
    adapter.count(1,12);assert(delta_canvas(adapter.c,adapter.reader(),before,adapter.canvas,out)&&out.canvas==adapter.canvas);
    auto kept=out;adapter.m.put(adapter.canvas+0x9c,std::uintptr_t(0xdead));
    assert(!delta_canvas(adapter.c,adapter.reader(),before,adapter.canvas,out)&&equal(out,kept));

    for(bool missing:{true,false}){Fixture empty;
        if(missing)empty.m.put(empty.elements+20,std::int32_t(99));else empty.set_items(0);
        assert(snapshot_fcanvas(empty.c,empty.reader(),empty.fcanvas,before)&&before.item_count==0);
        empty.m.put(empty.elements+20,std::int32_t(7));empty.set_items(2);
        assert(delta_fcanvas(empty.c,empty.reader(),before,empty.fcanvas,out)&&out.vertices.size()==12);
    }
    // Every old item, including unchanged earlier atlases, must survive in order.
    for(unsigned scenario=0;scenario<11;++scenario){Fixture bad;assert(snapshot_fcanvas(bad.c,bad.reader(),bad.fcanvas,before));
        bad.count(1,12);out=sentinel();kept=out;
        switch(scenario){
        case 0:bad.count(0,3);break;
        case 1:bad.count(1,10);break;
        case 2:bad.set_items(1);break;
        case 3:bad.m.put(bad.item_array,Fixture::item(1));bad.m.put(bad.item_array+8,Fixture::item(0));break;
        case 4:bad.m.put(Fixture::item(0)+8,Fixture::payload(1));break;
        case 5:bad.m.put(bad.keys+4,std::int32_t(8));break;
        case 6:bad.m.blocks.erase(Fixture::payload(0));break;
        case 7:bad.m.put(Fixture::item(0),std::uintptr_t(0xdead));break;
        case 8:bad.m.put(bad.elements,std::int32_t(7));break;
        case 9:bad.m.put(Fixture::payload(1)+0x48,detail::Array{Fixture::storage(1),12,8});break;
        case 10:bad.m.put(Fixture::payload(1)+0x48,detail::Array{(std::numeric_limits<std::uintptr_t>::max)()-16,12,16});break;}
        assert(!delta_fcanvas(bad.c,bad.reader(),before,bad.fcanvas,out)&&equal(out,kept));
    }
    Fixture too_large;assert(snapshot_fcanvas(too_large.c,too_large.reader(),too_large.fcanvas,before));
    too_large.m.block(Fixture::storage(0),1040*48);too_large.fill(Fixture::storage(0),1040);
    too_large.m.put(Fixture::payload(0)+0x48,detail::Array{Fixture::storage(0),1040,1040});
    out=sentinel();kept=out;assert(!delta_fcanvas(too_large.c,too_large.reader(),before,too_large.fcanvas,out)&&equal(out,kept));
    too_large.m.put(Fixture::payload(0)+0x48,detail::Array{Fixture::storage(0),1028,1040});
    assert(delta_fcanvas(too_large.c,too_large.reader(),before,too_large.fcanvas,out)&&out.vertices.size()==max_added_vertices);
    assert(out.ranges.size()==1&&out.ranges[0].begin==4&&out.ranges[0].end==1028);
    out=sentinel();kept=out;
    for(unsigned scenario=0;scenario<4;++scenario){Fixture bad;Snapshot guard=before;
        if(scenario==0)bad.set_items(65);
        if(scenario==1)bad.m.put(bad.fcanvas+0x48,detail::Array{bad.elements,65,65});
        if(scenario==2)bad.m.put(bad.fcanvas+0x28,detail::Array{});
        if(scenario==3)bad.m.put(bad.c.base+0x1a44e0,std::array<unsigned char,4>{});
        assert(!snapshot_fcanvas(bad.c,bad.reader(),bad.fcanvas,guard)&&detail::same(guard,before));
    }
    Fixture header_race;assert(snapshot_fcanvas(header_race.c,header_race.reader(),header_race.fcanvas,before));
    header_race.count(1,12);unsigned reads=0;
    header_race.m.before_read=[&](std::uintptr_t address,std::size_t){if(address==Fixture::payload(1)+0x48&&++reads==2)header_race.count(1,11);};
    out=sentinel();kept=out;assert(!delta_fcanvas(header_race.c,header_race.reader(),before,header_race.fcanvas,out)&&equal(out,kept));
    Fixture copy_race;assert(snapshot_fcanvas(copy_race.c,copy_race.reader(),copy_race.fcanvas,before));copy_race.count(1,12);
    copy_race.m.before_read=[&](std::uintptr_t address,std::size_t size){if(address==Fixture::storage(1)+8*48&&size==44)copy_race.count(1,11);};
    assert(!delta_fcanvas(copy_race.c,copy_race.reader(),before,copy_race.fcanvas,out)&&equal(out,kept));
    Fixture key_race;unsigned scans=0;Snapshot guard=before;
    key_race.m.put(key_race.elements+20,std::int32_t(99));
    key_race.m.before_read=[&](std::uintptr_t address,std::size_t size){if(address==key_race.elements+20&&size==4&&++scans==1)
        key_race.m.put(key_race.elements+20,std::int32_t(7));};
    assert(!snapshot_fcanvas(key_race.c,key_race.reader(),key_race.fcanvas,guard)&&detail::same(guard,before));
    auto forged=before;forged.item_count=max_items+1;
    assert(!delta_fcanvas(f.c,f.reader(),forged,f.fcanvas,out)&&equal(out,kept));
    assert(!snapshot_fcanvas(f.c,[](void*,const void*,std::size_t){return false;},f.fcanvas,guard));
    puts("Canvas text: multi-atlas/suffix/new item, real48-byte stride, realloc/empty and fail-closed bounded deltas passed");
}
