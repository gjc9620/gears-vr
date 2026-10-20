#include "ui_scene_root_candidate.h"
#include <cassert>
#include <map>
#include <functional>
#include <iostream>
using namespace gears_ui_scene_root;
struct Memory {
    std::map<std::uintptr_t,unsigned char> bytes;std::function<void(std::uintptr_t,std::size_t)> before;
    template<class T> void put(std::uintptr_t at,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);
        for(std::size_t i=0;i<sizeof(value);++i)bytes[at+i]=p[i];}
    bool read(void* out,void const* source,std::size_t size){auto at=reinterpret_cast<std::uintptr_t>(source);
        if(before)before(at,size);auto p=static_cast<unsigned char*>(out);
        for(std::size_t i=0;i<size;++i){auto it=bytes.find(at+i);if(it==bytes.end())return false;p[i]=it->second;}return true;}
};
struct Fixture {
    Config c{0x140000000};Memory m;std::uintptr_t client=0x1000,canvas=0x2000,scene=0x3000,transforms=0x4000;
    Fixture(){m.put(c.base+adapter_rva,adapter_bytes);m.put(c.base+caller_site_rva,caller_bytes);
        m.put(client,c.base+client_table_rva);m.put(c.base+client_table_rva+0x2b8,c.base+adapter_rva);
        m.put(scene,c.base+scene_table_rva);m.put(c.base+scene_table_rva+0x4a8,c.base+scene_render_rva);
        m.put(canvas+0x38,d::Array{transforms,2,2});std::array<unsigned char,80> top{};
        for(unsigned i=0;i<80;++i)top[i]=static_cast<unsigned char>(i);m.put(transforms+80,top);}
    auto read(){return [this](void* out,void const* at,std::size_t size){return m.read(out,at,size);};}
    bool get(Candidate& out){return candidate(c,read(),client,canvas,scene,c.base+caller_return_rva,out);}
};
static void valid(){Fixture f;assert(validate_adapter(f.c,f.read()));Candidate out{};assert(f.get(out));
    assert(out.client==f.client&&out.fcanvas==f.canvas&&out.scene==f.scene&&out.client_vtable==f.c.base+client_table_rva);
    // Installed detour bytes may differ; runtime candidate never reads entry.
    f.m.bytes.erase(f.c.base+adapter_rva);assert(!validate_adapter(f.c,f.read())&&f.get(out));
    MatrixSnapshot matrix{};assert(matrix_stack(f.read(),f.canvas,matrix));
    assert(matrix.top_address==f.transforms+80&&matrix.top[79]==79&&matrix.stack.count==2);
}
static void reject_candidates(){
    for(unsigned failure=0;failure<9;++failure){Fixture f;Candidate out{};out.client=99;
        switch(failure){
        case 0:f.m.put(f.c.base+caller_site_rva,unsigned(0));break;
        case 1:f.m.put(f.client,std::uintptr_t(0));break;
        case 2:f.m.put(f.c.base+client_table_rva+0x2b8,std::uintptr_t(0));break;
        case 3:f.m.put(f.scene,std::uintptr_t(0));break;
        case 4:f.m.put(f.c.base+scene_table_rva+0x4a8,std::uintptr_t(0));break;
        case 5:f.m.bytes.erase(f.scene);break;
        case 6:f.canvas=0;break;
        case 7:f.c.base=(std::numeric_limits<std::uintptr_t>::max)()-1;break;
        case 8:f.c.base=0;break;
        }
        assert(!f.get(out)&&out.client==99);
    }
    Fixture f;Candidate out{};out.scene=99;
    assert(!candidate(f.c,f.read(),f.client,f.canvas,f.scene,f.c.base+caller_return_rva+1,out)&&out.scene==99);
    f.m.put(f.c.base+adapter_rva,unsigned(0));assert(!validate_adapter(f.c,f.read()));
    assert(!validate_adapter({(std::numeric_limits<std::uintptr_t>::max)()},f.read()));
    for(unsigned target=0;target<5;++target){Fixture race;unsigned hits=0;
        const auto at=target==0?race.client:target==1?race.scene:target==2?race.c.base+client_table_rva+0x2b8:
                      target==3?race.c.base+scene_table_rva+0x4a8:race.c.base+caller_site_rva;
        race.m.before=[&](std::uintptr_t address,std::size_t){if(address==at&&++hits==2)race.m.put(at,std::uintptr_t(0));};
        assert(!race.get(out)&&out.scene==99);
    }
    Fixture race;unsigned hits=0;race.m.before=[&](std::uintptr_t address,std::size_t){
        if(address==race.c.base+adapter_rva&&++hits==2)race.m.put(address,unsigned(0));};
    assert(!validate_adapter(race.c,race.read()));
}
static void reject_matrix(){
    for(unsigned failure=0;failure<7;++failure){Fixture f;MatrixSnapshot out{};out.fcanvas=99;
        switch(failure){
        case 0:f.m.put(f.canvas+0x38,d::Array{f.transforms,0,2});break;
        case 1:f.m.put(f.canvas+0x38,d::Array{f.transforms,65,65});break;
        case 2:f.m.put(f.canvas+0x38,d::Array{(std::numeric_limits<std::uintptr_t>::max)()-5,2,2});break;
        case 3:f.m.bytes.erase(f.transforms+80);break;
        case 4:f.canvas=(std::numeric_limits<std::uintptr_t>::max)()-1;break;
        case 5:f.m.put(f.canvas+0x38,d::Array{0,2,2});break;
        case 6:f.m.put(f.canvas+0x38,d::Array{f.transforms,3,2});break;
        }
        assert(!matrix_stack(f.read(),f.canvas,out)&&out.fcanvas==99);
    }
    for(unsigned target=0;target<3;++target){Fixture f;unsigned hits=0;MatrixSnapshot out{};out.fcanvas=99;
        const auto at=target==0?f.transforms+80:f.canvas+0x38;
        f.m.before=[&](std::uintptr_t address,std::size_t){if(address==at&&++hits==(target==2?3u:2u)){
            if(target==0)f.m.put(at,unsigned(99));else f.m.put(at,d::Array{f.transforms,3,3});}};
        assert(!matrix_stack(f.read(),f.canvas,out)&&out.fcanvas==99);
    }
}
int main(){valid();reject_candidates();reject_matrix();std::cout<<"UiSceneRootCandidateTest passed\n";}
