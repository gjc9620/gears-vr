#include "ui_scene_batch.h"
#include <cassert>
#include <map>
#include <functional>
#include <iostream>
using namespace gears_ui_scene_batch;
struct Memory {
    std::map<std::uintptr_t,unsigned char> bytes;
    std::function<void(std::uintptr_t,std::size_t)> before;
    template<class T> void put(std::uintptr_t at,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);
        for(std::size_t i=0;i<sizeof(value);++i)bytes[at+i]=p[i];}
    void zero(std::uintptr_t at,std::size_t n){for(std::size_t i=0;i<n;++i)bytes[at+i]=0;}
    bool read(void* out,void const* source,std::size_t n){auto at=reinterpret_cast<std::uintptr_t>(source);
        if(before)before(at,n);auto p=static_cast<unsigned char*>(out);
        for(std::size_t i=0;i<n;++i){auto it=bytes.find(at+i);if(it==bytes.end())return false;p[i]=it->second;}return true;}
};
struct Fixture {
    Config c{0x10000000};Memory m;std::uintptr_t fc=0x1000,stack=0x2000,elements=0x3000;
    std::uintptr_t next=0x100000;std::map<int,std::vector<std::uintptr_t>> keys;
    struct Native {std::uintptr_t payload{},storage{};unsigned count{};};std::map<std::uintptr_t,Native> native;
    Fixture(){
        m.put(c.base+0x133cc98+8,c.base+0x1a44e0);m.put(c.base+0x1a44e0,std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3});
        m.put(c.base+0x159fcc,gears_ui_material::factory_bytes);
        m.put(c.base+0x15a3ca,gears_ui_material::mesh_flag_and_bytes);m.put(c.base+0x15a3d6,gears_ui_material::mesh_flag_or_bytes);
        m.put(c.base+0x136c5f0,std::array<std::uintptr_t,3>{c.base+0x3a514c,c.base+0x1ea9e0,c.base+0x1a44e0});
        m.put(fc+0x28,d::Array{stack,2,2});m.put(stack,std::int32_t(3));m.put(stack+4,std::int32_t(7));
        simple(3,4);material(7);sync();
    }
    std::uintptr_t alloc(){auto at=next;next+=0x10000;return at;}
    void sync(){unsigned index=0;for(auto const& key:keys){auto data=0x10000+index*0x1000;
        m.put(elements+index*20,std::int32_t(key.first));m.put(elements+index*20+4,d::Array{data,int(key.second.size()),int(key.second.size())});
        for(std::size_t i=0;i<key.second.size();++i)m.put(data+i*8,key.second[i]);++index;}
        m.put(fc+0x48,d::Array{elements,int(keys.size()),int(keys.size())});}
    std::uintptr_t simple(int key,unsigned count){auto item=alloc(),payload=alloc(),storage=alloc();
        m.put(item,c.base+0x133cc98);m.put(item+8,payload);m.zero(payload+0x80,80);
        native[item]={payload,storage,count};fill(item,count);keys[key].push_back(item);return item;}
    void fill(std::uintptr_t item,unsigned count){auto& n=native[item];n.count=count;
        m.put(n.payload+0x48,d::Array{n.storage,int(count),int(count)});
        for(unsigned i=0;i<count;++i){std::array<unsigned char,48> v{};
            for(unsigned j=0;j<44;++j)v[j]=static_cast<unsigned char>(i+j);m.put(n.storage+i*48,v);}}
    std::uintptr_t material(int key){auto item=alloc(),payload=alloc(),records=alloc();
        m.put(item,c.base+0x136c5f0);m.put(item+8,payload);m.put(item+16,unsigned(0));m.zero(payload,112);
        m.put(payload,std::uintptr_t(0x777));m.put(payload+0x60,d::Array{records,1,1});m.zero(records,36);
        native[item]={payload,records,0};keys[key].push_back(item);return item;}
    auto reader(){return [this](void* out,void const* at,std::size_t n){return m.read(out,at,n);};}
};
static void success(){Fixture f;Inventory before;assert(inventory(f.c,f.reader(),f.fc,before));
    assert(before.keys.size()==2&&before.keys[0].items[0].vertices.size()==4&&before.keys[1].items[0].kind==Kind::material);
    const auto old_simple=f.keys[3][0];f.fill(old_simple,8);f.simple(7,4);f.material(11);f.simple(11,8);f.sync();
    Delta change;assert(delta(f.c,f.reader(),before,f.fc,change));assert(change.simple.size()==3&&change.material.size()==1);
    assert(change.simple[0].key==3&&change.simple[0].begin==4&&change.simple[0].end==8);
    assert(change.simple[1].key==7&&change.simple[1].begin==0&&change.simple[1].vertices.size()==4);
    assert(change.simple[2].key==11&&change.material[0].key==11&&!change.material[0].canvas);
    assert(change.simple[0].vertices[0][0]==4&&sizeof(Vertex)==44);
    // Sort elements may reorder; delta matches keys by value, not array slot.
    std::array<unsigned char,20> first{},last{};assert(f.m.read(first.data(),reinterpret_cast<void const*>(f.elements),20));
    assert(f.m.read(last.data(),reinterpret_cast<void const*>(f.elements+40),20));f.m.put(f.elements,last);f.m.put(f.elements+40,first);
    assert(delta(f.c,f.reader(),before,f.fc,change));
    // Owned old bytes are independent; four padding bytes are not evidence.
    f.m.put(f.native[old_simple].storage+44,unsigned(123));assert(delta(f.c,f.reader(),before,f.fc,change));
}
static void relocated_vertex_append(){
    Fixture f;Inventory before;assert(inventory(f.c,f.reader(),f.fc,before));
    auto item=f.keys[3][0];auto old=f.native[item].storage;
    f.native[item].storage=f.alloc();f.fill(item,8);
    for(unsigned i=0;i<4*48;++i)f.m.bytes.erase(old+i); // Old allocation is unreadable.
    Delta out;assert(delta(f.c,f.reader(),before,f.fc,out));
    assert(out.simple.size()==1&&out.simple[0].begin==4&&out.simple[0].end==8&&out.simple[0].vertices[0][0]==4);
}
static void rejects(){
    for(unsigned failure=0;failure<15;++failure){Fixture f;f.simple(3,4);f.sync();Inventory before;
        assert(inventory(f.c,f.reader(),f.fc,before));const auto a=f.keys[3][0],b=f.keys[3][1],mat=f.keys[7][0];
        Delta out;out.fcanvas=99;Range marker;marker.key=98;out.simple.push_back(marker);
        switch(failure){
        case 0:f.m.put(f.native[a].storage,unsigned(99));break; // Meaningful old prefix edit.
        case 1:f.keys[3].pop_back();f.sync();break;
        case 2:std::swap(f.keys[3][0],f.keys[3][1]);f.sync();break;
        case 3:f.keys.erase(7);f.sync();break;
        case 4:f.m.put(f.native[mat].storage,float(1));break; // Existing material changed.
        case 5:f.m.put(a,std::uintptr_t(0xdead));break;
        case 6:f.keys[7].push_back(a);f.sync();break;
        case 7:f.m.put(b+8,f.native[a].payload);break;
        case 8:f.m.put(f.elements+20,std::int32_t(3));break;
        case 9:f.m.put(f.stack+4,std::int32_t(9));break;
        case 10:f.m.put(mat+16,unsigned(1));break;
        case 11:f.m.put(f.native[a].payload+0x80,unsigned(1));break;
        case 12:f.fill(a,0);break;
        case 13:f.m.bytes.erase(f.native[a].storage);break;
        case 14:f.m.put(f.native[mat].payload+8,unsigned(1));break; // Full material header remains frozen.
        }
        assert(!delta(f.c,f.reader(),before,f.fc,out));assert(out.fcanvas==99&&out.simple.size()==1&&out.simple[0].key==98);
    }
}
static void caller_inventory_validation(){
    Fixture f;Inventory original;assert(inventory(f.c,f.reader(),f.fc,original));
    for(unsigned failure=0;failure<10;++failure){auto before=original;auto& m=before.keys[1].items[0].material;
        switch(failure){
        case 0:before.stack_array.count=1;break;
        case 1:before.element_array.capacity=65;break;
        case 2:before.keys[0].item_array.count=0;break;
        case 3:before.keys[0].items[0].vertex_array.count=0;break;
        case 4:m.fcanvas+=8;break;
        case 5:m.key+=1;break;
        case 6:m.item+=8;break;
        case 7:m.payload+=8;break;
        case 8:m.proxy+=8;break;
        case 9:before.keys[1].items[0].material_header[0x68]=0;break;
        }
        Delta out;out.fcanvas=99;assert(!delta(f.c,f.reader(),before,f.fc,out)&&out.fcanvas==99);
    }
}
static void limits_and_races(){
    for(unsigned failure=0;failure<4;++failure){Fixture f;
        if(failure==0)f.m.put(f.fc+0x48,d::Array{f.elements,65,65});
        if(failure==1){for(unsigned i=0;i<128;++i)f.material(7);f.sync();}
        if(failure==2)f.fill(f.keys[3][0],1028);
        if(failure==3){f.fill(f.keys[3][0],1024);f.simple(7,4);f.sync();}
        Inventory out;out.fcanvas=99;assert(!inventory(f.c,f.reader(),f.fc,out)&&out.fcanvas==99);
    }
    Fixture spare;auto item=spare.keys[3][0];
    spare.m.put(spare.native[item].payload+0x48,d::Array{spare.native[item].storage,4,2048});
    Inventory roomy;assert(inventory(spare.c,spare.reader(),spare.fc,roomy));
    Delta no_change;assert(delta(spare.c,spare.reader(),roomy,spare.fc,no_change)&&no_change.simple.empty());
    Fixture edge;edge.fill(edge.keys[3][0],1024);Inventory accepted;assert(inventory(edge.c,edge.reader(),edge.fc,accepted));
    Fixture item_edge;for(unsigned i=0;i<126;++i)item_edge.material(7);item_edge.sync();
    assert(inventory(item_edge.c,item_edge.reader(),item_edge.fc,accepted));
    Fixture key_edge;for(int k=20;k<82;++k)key_edge.keys[k]={};key_edge.sync();
    assert(inventory(key_edge.c,key_edge.reader(),key_edge.fc,accepted)&&accepted.keys.size()==64);
    for(unsigned target=0;target<8;++target){Fixture f;auto simple=f.keys[3][0],material=f.keys[7][0];unsigned hits=0;
        const auto address=target==0?f.stack:target==1?f.elements:target==2?f.native[simple].storage:
                           target==3?f.native[simple].payload+0x80:target==4?f.native[material].storage:
                           target==5?std::uintptr_t(0x10000):f.native[material].payload;
        const auto mutate_address=target==6?address+0x10:target==7?address+8:address;
        f.m.before=[&](std::uintptr_t at,std::size_t){if(at==address&&++hits==2)f.m.put(mutate_address,unsigned(0x123));};
        Inventory out;out.fcanvas=99;assert(!inventory(f.c,f.reader(),f.fc,out)&&out.fcanvas==99);
    }
}
int main(){success();relocated_vertex_append();rejects();caller_inventory_validation();limits_and_races();std::cout<<"UiSceneBatchTest passed\n";}
