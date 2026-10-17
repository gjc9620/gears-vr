#include "path_choice_position_trial.h"
#include <cassert>
#include <cstdio>
#include <map>
using namespace gears_path_choice_position;
struct Engine {
    Config c;Context ctx;std::map<std::uintptr_t,unsigned char> memory;unsigned calls{};int fail_call{};bool mutate_on_failure{};
    Engine(){c.nonce=1;c.pid=1;c.creation_time=2;c.started_qpc=3;c.world_generation=4;c.base=0x140000000;c.client=0x1000;
        c.scene=0x2000;c.panel=0x3000;c.panel_vtable=0x4000;c.duration_ms=3000;c.top_bits=bits(.0374f);c.bottom_bits=bits(.9626f);
        ctx={1,2,3,4,c.base,c.client,c.scene};put(c.panel,c.panel_vtable);put(c.panel_vtable+0x3d8,c.base+0x2988d8);
        put(c.panel_vtable+0x3b8,c.base+0x297338);put(c.panel+0x70,std::uint32_t(0x04040404));
        put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        c.checks.push_back({0x5000,{0xaa}});memory[0x5000]=0xaa;}
    template<class T> void put(std::uintptr_t address,T v){auto p=reinterpret_cast<unsigned char const*>(&v);for(std::size_t i=0;i<sizeof(v);++i)memory[address+i]=p[i];}
    bool read(void* out,void const* from,std::size_t size){auto p=reinterpret_cast<std::uintptr_t>(from);auto q=static_cast<unsigned char*>(out);
        for(std::size_t i=0;i<size;++i){auto it=memory.find(p+i);if(it==memory.end())return false;q[i]=it->second;}return true;}
    auto reader(){return [this](auto out,auto from,auto size){return read(out,from,size);};}
    auto setter(){return [this](auto panel,float v,unsigned face,unsigned input,int origin){
        assert(panel==c.panel&&(face==1||face==3)&&input==0&&origin==0);++calls;
        bool fail=int(calls)==fail_call;if(!fail||mutate_on_failure)put(panel+0x60+face*4,bits(v));return !fail;};}
    std::array<std::uint32_t,2> pos(){std::array<std::uint32_t,2> p{};assert(positions(c,reader(),p));return p;}
};
static std::string ini(){std::string s="[PathChoicePositionTrial]\nschema=1\nnonce=7\npid=1\ncreation_time=2\nstarted_qpc=3\nworld_generation=4\nbase=0x140000000\nclient=0x1000\nscene=0x2000\npanel=0x3000\npanel_vtable=0x4000\ntop_bits="+std::to_string(bits(.0374f))+"\nbottom_bits="+std::to_string(bits(.9626f))+"\nduration_ms=3000\ncheck_count=16\n";
    for(unsigned i=0;i<16;++i)s+="check_"+std::to_string(i)+"=0x5000:aa\n";return s;}
static void parse_checks(){Config c;auto text=ini();assert(parse(text,c));assert(c.checks.size()==16&&c.panel==0x3000);
    for(auto const& pair:std::vector<std::pair<std::string,std::string>>{{"schema=1","schema=2"},{"nonce=7","nonce=0"},
        {"duration_ms=3000","duration_ms=9000"},{"panel=0x3000","panel=0x2000"},{"check_15=0x5000:aa","check_15=0x5000:ab"},
        {"check_15=0x5000:aa","check_15=0x3064:aa"},{"check_15=0x5000:aa","check_15=0x306c:aa"},
        {"check_15=0x5000:aa","check_15=0x3074:aa"},{"check_15=0x5000:aa","check_15=0xffffffffffffffff:aa"}}){
        auto bad=text;bad.replace(bad.find(pair.first),pair.first.size(),pair.second);assert(!parse(bad,c));}
    assert(!parse(text+"unknown=1\n",c));assert(!parse(text+std::string(1,'\0'),c));}
static void lifecycle(){Engine e;State s;auto before=e.pos();assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::applied);
    auto after=e.pos();assert(after[0]==bits(value(before[0])+delta)&&after[1]==bits(value(before[1])+delta));
    for(unsigned frame=0;frame<100;++frame){assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::holding);assert(e.pos()==after&&e.calls==2);}
    assert(restore(s,e.ctx,e.reader(),e.setter())==Result::restored&&e.pos()==before&&e.calls==4);
    assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::rejected&&e.calls==4);
    ++e.c.nonce;assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::applied);}
static void failed_admission(){for(unsigned kind=0;kind<7;++kind){Engine e;State s;
    if(kind==0)++e.ctx.world_generation;if(kind==1)++e.ctx.scene;if(kind==2)++e.ctx.creation_time;
    if(kind==3)e.memory[0x5000]^=1;if(kind==4)e.put(e.c.panel,std::uintptr_t(0x9000));
    if(kind==5)e.put(e.c.panel+0x70,std::uint32_t(0x05050505));if(kind==6)e.put(e.c.panel+0x64,bits(.2f));
    assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::rejected&&e.calls==0);}}
static void lost_ownership(){for(unsigned kind=0;kind<5;++kind){Engine e;State s;assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::applied);
    if(kind==0)++e.ctx.world_generation;if(kind==1)e.memory[0x5000]^=1;if(kind==2)e.put(e.c.panel,std::uintptr_t(0x9000));
    if(kind==3)e.put(e.c.panel+0x70,std::uint32_t(0x05050505));if(kind==4)e.put(e.c.panel+0x64,bits(.3f));
    auto before=e.pos();auto r=restore(s,e.ctx,e.reader(),e.setter());assert(r==(kind==4?Result::ownership_lost:Result::identity_lost));
    assert(e.calls==2&&e.pos()==before&&!s.lease.active);}}
static void partial_failure(){for(unsigned fail:{1u,2u})for(bool mutated:{false,true}){Engine e;State s;auto before=e.pos();e.fail_call=fail;e.mutate_on_failure=mutated;
    assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::setter_failed);assert(e.pos()==before&&!s.lease.active);
    assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::rejected);}
    Engine e;State s;assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::applied);e.fail_call=3;
    assert(restore(s,e.ctx,e.reader(),e.setter())==Result::setter_failed&&s.lease.active);
    e.fail_call=0;assert(restore(s,e.ctx,e.reader(),e.setter())==Result::restored);}
int main(){parse_checks();lifecycle();failed_admission();lost_ownership();partial_failure();std::puts("PathChoicePositionTrialTest passed");}
