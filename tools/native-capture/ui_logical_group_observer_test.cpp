#include "ui_observer.h"
#include "ui_logical_group_observer.h"
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cassert>
#include <map>
#include <iostream>
struct Memory {
    std::map<uintptr_t,unsigned char> bytes;
    template<class T> void put(uintptr_t at,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);
        for(size_t i=0;i<sizeof(value);++i)bytes[at+i]=p[i];}
    bool read(void* out,void const* source,size_t n){auto at=reinterpret_cast<uintptr_t>(source);auto p=static_cast<unsigned char*>(out);
        for(size_t i=0;i<n;++i){auto found=bytes.find(at+i);if(found==bytes.end())return false;p[i]=found->second;}return true;}
};
Memory memory;uintptr_t base=0x10000000;bool ui_observer_enabled=true;std::atomic<bool> recording{true};
long long mock_time=10000,qpc_frequency=1000;
long long now(){return mock_time;}
unsigned GetCurrentThreadId(){return 42;}
bool safe_copy(void* out,void const* at,size_t n){return memory.read(out,at,n);}
#define GEARS_UI_LOGICAL_GROUP_OBSERVER 1
#include "ui_logical_group_observer_live.inl"
#include "ui_scene_logical_layout.h"
// Compile the candidate against the real observer Row as well as its fixtures.
template bool gears_ui_scene_layout::fit_invocation(ui_logical_group_live::Row const&,uint64_t,
    gears_ui_group::Bounds,double,gears_ui_scene_layout::Candidate&);
using namespace gears_ui_logical_group;
static std::array<unsigned char,80> transform(){std::array<unsigned char,80> bytes{};
    const float m[16]{2,0,0,0,0,-2,0,0,0,0,1,0,-1,1,0,1};std::memcpy(bytes.data(),m,sizeof(m));return bytes;}
static gears_ui_scene_batch::Range quad(){gears_ui_scene_batch::Range r{};r.item=0x4000;r.payload=0x5000;
    r.key=7;r.begin=0;r.end=4;r.transform=transform();
    for(auto p: {std::array<float,4>{.2f,.1f,0,1},std::array<float,4>{.8f,.1f,0,1},
        std::array<float,4>{.2f,.3f,0,1},std::array<float,4>{.8f,.3f,0,1}}){gears_ui_batch::Vertex v{};
        std::memcpy(v.data(),p.data(),sizeof(p));r.vertices.push_back(v);}return r;}
static void setup(){
    using gears_ui_batch::detail::Array;
    memory={};memory.put(base+0x133cc98+8,base+0x1a44e0);
    memory.put(base+0x1a44e0,std::array<unsigned char,4>{0x48,0x8b,0xc1,0xc3});
    memory.put(base+0x159fcc,gears_ui_material::factory_bytes);
    memory.put(base+0x15a3ca,gears_ui_material::mesh_flag_and_bytes);memory.put(base+0x15a3d6,gears_ui_material::mesh_flag_or_bytes);
    memory.put(base+0x136c5f0,std::array<uintptr_t,3>{base+0x3a514c,base+0x1ea9e0,base+0x1a44e0});
    memory.put(0x1000+0x28,Array{0x2000,1,1});memory.put(0x2000,int32_t(7));memory.put(0x1000+0x48,Array{});
}
static void append_quad(){using gears_ui_batch::detail::Array;auto r=quad();
    memory.put(0x1000+0x48,Array{0x3000,1,1});memory.put(0x3000,int32_t(7));memory.put(0x3004,Array{0x3100,1,1});
    memory.put(0x3100,r.item);memory.put(r.item,base+0x133cc98);memory.put(r.item+8,r.payload);
    memory.put(r.payload+0x48,Array{0x6000,4,4});memory.put(r.payload+0x80,r.transform);
    for(unsigned i=0;i<4;++i)memory.put(0x6000+i*48,r.vertices[i]);
}
static void geometry(){
    gears_ui_scene_batch::Delta delta{};delta.fcanvas=0x1000;delta.simple.push_back(quad());
    gears_ui_material::Receipt m{};m.fcanvas=delta.fcanvas;m.item=0x9000;m.payload=0xa000;m.proxy=1;m.transform=transform();
    // Mirrored material quad is part of the same complete bounds.
    const float record[8]{.9f,.8f,-.2f,-.4f,0,0,1,1};std::memcpy(m.record.data(),record,sizeof(record));delta.material.push_back(m);
    gears_ui_group::Bounds b{};assert(bounds(delta,b));
    assert(std::abs(b.left-.2)<1e-6&&std::abs(b.top-.1)<1e-6&&std::abs(b.right-.9)<1e-6&&std::abs(b.bottom-.8)<1e-6);
    const auto sentinel=b;delta.material[0].proxy=0;assert(!bounds(delta,b));assert(b.left==sentinel.left&&b.bottom==sentinel.bottom);
    Member good{delta.fcanvas,0x4000,0x5000,7,0,4,false};assert(contains(delta,good));
    auto bad=good;bad.end=8;assert(!contains(delta,bad));bad=good;bad.fcanvas=99;assert(!contains(delta,bad));
    Call call{};call.append_complete=true;call.member_count=1;call.members[0]=good;assert(mapped(delta,call));
    call.members[0]=bad;assert(!mapped(delta,call));call.member_count=0;call.append_complete=false;assert(!mapped(delta,call));
    delta.material.clear();delta.simple[0].vertices.pop_back();assert(!bounds(delta,b));
    delta.simple[0]=quad();float w=0;std::memcpy(delta.simple[0].vertices[0].data()+12,&w,4);assert(!bounds(delta,b));
    delta.simple.clear();assert(!bounds(delta,b));
}
static void frame_chain(){
    auto put=[](uintptr_t at,gears_ui_call_boundary::Frame f){memory.put(at+0x18,f.node);memory.put(at+0x20,f.object);
        memory.put(at+0x28,f.code);memory.put(at+0x38,f.previous);};
    put(0x7000,{11,12,13,0x7100});put(0x7100,{21,22,23,0});Call c{};c.draw=3;
    assert(ancestry(safe_copy,0x7000,0x7100,c)&&c.count==2&&c.frames[1].fields.node==21);
    unsigned reads=0;auto racing=[&](void* out,void const* at,size_t n){
        if(reinterpret_cast<uintptr_t>(at)==0x7000+0x18&&++reads==2)memory.put(0x7000+0x18,uintptr_t(99));
        return memory.read(out,at,n);};
    Call raced{};assert(!ancestry(racing,0x7000,0x7100,raced)&&!raced.count);put(0x7000,{11,12,13,0x7100});
    Call sentinel{};sentinel.draw=99;assert(!ancestry(safe_copy,0x7000,0x7200,sentinel)&&sentinel.draw==99&&!sentinel.count);
    put(0x7100,{21,22,23,0x7000});assert(!ancestry(safe_copy,0x7000,0x7200,sentinel));
}
static void object_names(){
    gears_ui::Config c{};c.objects_header=0x9000;c.names_header=0x9100;c.object_internal_index=0x38;
    c.object_name=0x40;c.object_class=0x48;c.object_outer=0x50;c.name_entry_index=0;c.name_entry_text=0x10;
    memory.put(c.objects_header,gears_ui::Registry{0x1000000,1001,2000});
    memory.put(c.names_header,gears_ui::Registry{0x1100000,1001,2000});
    const uintptr_t object=0x30000,name_entry=0x40000;
    memory.put(object,std::array<unsigned char,96>{});memory.put(object+0x38,unsigned(5));
    memory.put(object+0x40,unsigned(7));memory.put(object+0x48,uintptr_t(0x50000));memory.put(object+0x50,uintptr_t(0x60000));
    memory.put(0x1000000+5*8,object);memory.put(0x1100000+7*8,name_entry);memory.put(name_entry,unsigned(7));
    std::array<wchar_t,64> text{};wcscpy_s(text.data(),text.size(),L"DrawActionInfo");memory.put(name_entry+0x10,text);
    gears_ui::Identity id{};assert(identity(c,safe_copy,object,id)&&id.index==5&&id.name_index==7&&
        std::wcscmp(id.name.data(),L"DrawActionInfo")==0);
    memory.put(0x1000000+5*8,uintptr_t(0xdead));gears_ui::Identity out{};out.address=99;
    assert(!identity(c,safe_copy,object,out)&&out.address==99);
}
static void unique_children(){using namespace ui_logical_group_live;
    setup();object_names();memory.put(0x1000000+5*8,uintptr_t(0x30000));
    gears_ui::Config c{};c.objects_header=0x9000;c.names_header=0x9100;c.object_internal_index=0x38;
    c.object_name=0x40;c.object_class=0x48;c.object_outer=0x50;c.name_entry_text=0x10;
    auto named=[&](uintptr_t at,unsigned slot,unsigned name_index,uintptr_t entry,wchar_t const* text){
        memory.put(at,std::array<unsigned char,96>{});memory.put(at+0x38,slot);memory.put(at+0x40,name_index);
        memory.put(at+0x48,uintptr_t(0x777));memory.put(at+0x50,uintptr_t(0x888));
        memory.put(0x1000000+slot*8,at);memory.put(0x1100000+name_index*8,entry);memory.put(entry,name_index);
        std::array<wchar_t,64> value{};wcscpy_s(value.data(),value.size(),text);memory.put(entry+0x10,value);};
    named(0x50000,6,8,0x42000,L"Function");named(0x60000,7,9,0x43000,L"WarHUD");
    std::array<wchar_t,64> function{};wcscpy_s(function.data(),function.size(),L"DrawWeaponInfo");memory.put(0x40010,function);
    memory.put(0x30000+0x8c,gears_ui::Registry{0x90000,8,8});memory.put(0x30000+0x114,base+gears_ui::binding_rvas[0]);
    auto frame=[](uintptr_t at,uintptr_t node,uintptr_t object,uintptr_t previous){memory.put(at+0x18,node);
        memory.put(at+0x20,object);memory.put(at+0x28,uintptr_t(0x90001));memory.put(at+0x38,previous);};
    frame(0x7000,0xabc,0x4444,0);frame(0x7100,0x30000,0x4444,0x7000);frame(0x7200,0x999,0x8888,0x7100);
    {Observe group(2,98,0x4444,0xabc,0,0x1000,0x7000);assert(group.observing());
        group.row.kind=1;group.config=&c;
        gears_ui::ScopeContext selected{3,0,0xabc,0x4444,0};unsigned original_calls=0;
        uint64_t ids[2]{};
        for(unsigned n=0;n<2;++n){ChildScope child(reinterpret_cast<void*>(0x4444),reinterpret_cast<void*>(0x7000),reinterpret_cast<void*>(0x30000),true);
            assert(child.owner==&group&&current_child==&child);ids[n]=group.row.children[child.index].id;
            const auto returned=child.invoke([&]{++original_calls;canvas(1,reinterpret_cast<void*>(0x8888),reinterpret_cast<void*>(0x7200),selected,0x7000);
                return uintptr_t(0xfedcba9876543210ull);});assert(returned==0xfedcba9876543210ull);
        }
        assert(original_calls==2&&ids[0]!=ids[1]&&!current_child&&group.row.child_count==2);
        assert(group.row.calls[0].logical_count==1&&group.row.calls[0].logical_ids[0]==ids[0]);
        assert(group.row.calls[1].logical_ids[0]==ids[1]&&group.row.children[0].returned&&group.row.children[1].body_calls==1);
        try {ChildScope child(reinterpret_cast<void*>(0x4444),reinterpret_cast<void*>(0x7000),reinterpret_cast<void*>(0x30000),true);
            child.invoke([]()->uintptr_t {throw 17;});assert(false);
        }catch(int value){assert(value==17);}
        assert(!current_child&&!group.row.children[2].returned);
        memory.put(0x30000+0x114,uintptr_t(99));ChildScope rejected(reinterpret_cast<void*>(0x4444),reinterpret_cast<void*>(0x7000),reinterpret_cast<void*>(0x30000),true);
        assert(!rejected.owner&&!current_child);
    }
    mock_time+=1000;
}
static void live_lifecycle(char const* output_path){using namespace ui_logical_group_live;
    setup();{Observe group(2,123,0x4444,0x5555,0,0x1000,0);assert(group.observing()&&group.row.before_ok);
        gears_ui::ScopeContext selected{3,0,0x99,0x4444,0};
        canvas(1,reinterpret_cast<void*>(0x8888),reinterpret_cast<void*>(0x7000),selected,0x7000);
        CallMark outer(reinterpret_cast<void*>(0x7000),1);
        CallMark rejected(reinterpret_cast<void*>(0x7000),1,false);assert(!rejected.observing());
        canvas(3,reinterpret_cast<void*>(0x8888),reinterpret_cast<void*>(0x7100),selected,0x7100);
        CallMark inner(reinterpret_cast<void*>(0x7100),3);
        struct EmptyText {uintptr_t fcanvas=0x1000;int32_t sort_key=7;
            std::vector<gears_ui_scene_batch::Range> ranges;};inner.text(EmptyText{});
        append_quad();gears_ui_batch::Receipt receipt{};receipt.fcanvas=0x1000;receipt.item=0x4000;
        receipt.payload=0x5000;receipt.sort_key=7;receipt.end=4;outer.tile(receipt);
        group.finish(true);assert(current==&group);}
    assert(!current&&completed[1].count==1);auto const& good=completed[1].oldest(0);
    assert(good.delta_ok&&good.bounds_ok&&good.returned&&good.root_current&&good.delta.simple.size()==1);
    assert(good.call_count==2&&good.calls[0].member_count==1&&good.calls[1].member_count==0&&
        mapped(good.delta,good.calls[0])&&mapped(good.delta,good.calls[1]));
    // Published copies survive subsequent source mutation.
    float changed=.99f;memory.put(0x6000,changed);assert(completed[1].oldest(0).delta.simple[0].vertices[0]==quad().vertices[0]);
    {Observe throttled_group(2,124,1,2,0,0x1000,0);assert(!throttled_group.observing());}
    assert(completed[1].count==1);
    for(unsigned failure=0;failure<5;++failure){mock_time+=1000;setup();
        {Observe group(2,200+failure,1,2,0,0x1000,0);append_quad();
            if(failure==0)executing(1);if(failure==1)executing(2);
            if(failure==4)canvas_rejected();
            if(failure<3)group.finish(failure!=2); // Failure3 models exceptional original return.
            if(failure==4)group.finish(true);
        }
        assert(!current);auto const& r=completed[1].oldest(completed[1].count-1);
        if(failure==4)assert(r.delta_ok&&r.unclassified_canvas==1);
        else {assert(!r.delta_ok&&!r.bounds_ok);if(failure==3)assert(!r.returned);}}
    // Disabled observation neither reads nor changes the current chain.
    ui_observer_enabled=false;{Observe disabled(2,777,1,2,0,0x1000,0);assert(!disabled.observing()&&!current);}ui_observer_enabled=true;
    Ring<unsigned,3> ring;for(unsigned i=0;i<7;++i)ring.append(i);
    assert(ring.count==3&&ring.overwritten==4&&ring.oldest(0)==4&&ring.oldest(2)==6);
    FILE* out=nullptr;
    if(output_path)assert(fopen_s(&out,output_path,"wb")==0&&out);else assert(tmpfile_s(&out)==0&&out);
    print(out);fclose(out);
}
int main(int argc,char** argv){geometry();frame_chain();object_names();unique_children();live_lifecycle(argc>1?argv[1]:nullptr);std::cout<<"UiLogicalGroupObserverTest passed\n";}
