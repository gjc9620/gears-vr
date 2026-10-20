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
static Check header_check(uintptr_t address,RegistryHeader h){Check c;c.address=address;c.bytes.resize(16);std::memcpy(c.bytes.data(),&h,16);return c;}
static std::string encoded(Check const& c){constexpr char hex[]="0123456789abcdef";std::string text;
    char address[32]{};std::snprintf(address,sizeof(address),"0x%llx:",static_cast<unsigned long long>(c.address));text=address;
    for(auto b:c.bytes){text+=hex[b>>4];text+=hex[b&15];}return text;}
static std::string prefix_ini(){auto text=ini();text.replace(text.find("schema=1"),8,"schema=2\nregistry_header_policy=fixed_prefix_v1");
    for(unsigned i=0;i<2;++i){auto key="check_"+std::to_string(i)+"=0x5000:aa";
        auto check=header_check(0x140000000+registry_header_rvas[i],{0x700000+i*0x100000,100,200});
        text.replace(text.find(key),key.size(),"check_"+std::to_string(i)+"="+encoded(check));}return text;}
static void prefix_parser(){Config c;auto text=prefix_ini();assert(parse(text,c)&&c.schema==2&&c.registry_header_policy==RegistryHeaderPolicy::fixed_prefix_v1);
    auto replace=[&](std::string const& from,std::string const& to){auto bad=text;bad.replace(bad.find(from),from.size(),to);assert(!parse(bad,c));};
    replace("fixed_prefix_v1","monotonic");replace("schema=2","schema=1");
    auto h=header_check(0x140000000+registry_header_rvas[0],{0x700000,100,200});
    for(auto invalid:std::vector<RegistryHeader>{{0,100,200},{0x700000,0,200},{0x700000,-1,200},{0x700000,201,200},
            {0x700000,100,0},{0x700000,100,-1},{0x700000,100,4000000},{UINT64_MAX-1599,100,200}})
        replace("check_0="+encoded(h),"check_0="+encoded(header_check(h.address,invalid)));
    replace("check_1="+encoded(header_check(0x140000000+registry_header_rvas[1],{0x800000,100,200})),"check_1=0x5000:aa");
    for(unsigned kind=0;kind<3;++kind){auto alias=h;if(kind==1){alias.address+=8;alias.bytes.erase(alias.bytes.begin(),alias.bytes.begin()+8);}
        if(kind==2){alias.address-=8;alias.bytes.insert(alias.bytes.begin(),8,0);}
        replace("check_2=0x5000:aa","check_2="+encoded(alias));}
    replace("check_1="+encoded(header_check(0x140000000+registry_header_rvas[1],{0x800000,100,200})),
            "check_1="+encoded(header_check(0x140000000+registry_header_rvas[1],{0x700000,100,200})));
}
static void prefix_authentication_and_restore(){Engine e;e.c.schema=2;e.c.registry_header_policy=RegistryHeaderPolicy::fixed_prefix_v1;
    for(unsigned i=0;i<2;++i){auto h=RegistryHeader{0x700000+i*0x100000,100,200};e.c.checks.push_back(header_check(e.c.base+registry_header_rvas[i],h));e.put(e.c.base+registry_header_rvas[i],h);}
    auto slot=header_check(0x700000+19*8,{0x3000,0,0});slot.bytes.resize(8);e.c.checks.push_back(slot);e.put(slot.address,uintptr_t(0x3000));
    assert(authenticate(e.c,e.ctx,e.reader()));State state;assert(apply(state,e.c,e.ctx,e.reader(),e.setter())==Result::applied);
    for(int count:{180,120,100,150}){for(unsigned i=0;i<2;++i)e.put(e.c.base+registry_header_rvas[i],RegistryHeader{0x700000+i*0x100000,count,200});
        assert(authenticate(e.c,e.ctx,e.reader()));}
    assert(restore(state,e.ctx,e.reader(),e.setter())==Result::restored&&e.calls==4);
    for(unsigned i=0;i<2;++i){auto address=e.c.base+registry_header_rvas[i];
        for(auto bad:std::vector<RegistryHeader>{{0x700000+i*0x100000,99,200},{0x700008+i*0x100000,150,200},{0x700000+i*0x100000,150,201},
                {0x700000+i*0x100000,201,200},{0x700000+i*0x100000,-1,200},{0x700000+i*0x100000,150,4000000},{UINT64_MAX-1599,150,200}}){
            e.put(address,bad);assert(!authenticate(e.c,e.ctx,e.reader()));}
        e.put(address,RegistryHeader{0x700000+i*0x100000,150,200});}
    e.put(slot.address,uintptr_t(0x3008));assert(!authenticate(e.c,e.ctx,e.reader()));e.put(slot.address,uintptr_t(0x3000));
    auto unknown=header_check(0x900000,{0x1000000,100,200});e.c.checks.push_back(unknown);e.put(unknown.address,RegistryHeader{0x1000000,101,200});
    assert(!authenticate(e.c,e.ctx,e.reader()));e.put(unknown.address,RegistryHeader{0x1000000,100,200});assert(authenticate(e.c,e.ctx,e.reader()));
    e.c.schema=1;e.c.registry_header_policy=RegistryHeaderPolicy::exact;assert(!authenticate(e.c,e.ctx,e.reader()));
}
static void offset_parse_and_apply(){Config c;auto text=prefix_ini();assert(parse(text,c)&&c.vertical_offset_reference_pixels==16&&delta_for(c)==delta);
    for(unsigned offset:{1u,16u,374u,400u}){assert(parse(text+"vertical_offset_reference_pixels="+std::to_string(offset)+"\n",c));
        assert(c.vertical_offset_reference_pixels==offset&&delta_for(c)==static_cast<float>(offset)/1080.f);}
    for(auto bad:{"0","401","-1","16.0","true","4294967296"})assert(!parse(text+"vertical_offset_reference_pixels="+bad+"\n",c));
    assert(!parse(text+"vertical_offset_reference_pixels=374\nvertical_offset_reference_pixels=374\n",c));
    assert(!parse(text+"vertical_offset_reference_pixel=374\n",c));
    assert(!parse(ini()+"vertical_offset_reference_pixels=16\n",c));
    assert(!parse(ini()+"vertical_offset_reference_pixels=374\n",c));
    Engine e;e.c.schema=2;e.c.registry_header_policy=RegistryHeaderPolicy::fixed_prefix_v1;e.c.vertical_offset_reference_pixels=374;
    for(unsigned i=0;i<2;++i){auto h=RegistryHeader{0x700000+i*0x100000,100,200};e.c.checks.push_back(header_check(e.c.base+registry_header_rvas[i],h));e.put(e.c.base+registry_header_rvas[i],h);}
    State s;auto before=e.pos();assert(apply(s,e.c,e.ctx,e.reader(),e.setter())==Result::applied);
    assert((e.pos()==std::array<std::uint32_t,2>{bits(value(before[0])+374.f/1080.f),bits(value(before[1])+374.f/1080.f)}));
    assert(restore(s,e.ctx,e.reader(),e.setter())==Result::restored&&e.pos()==before);
    e.c.schema=1;e.c.registry_header_policy=RegistryHeaderPolicy::exact;State legacy;assert(apply(legacy,e.c,e.ctx,e.reader(),e.setter())==Result::rejected);
}
static void group_fit_parse_and_apply(){
    const std::array<float,6> source{.28663f,.06703f,.71394f,.2411013f,.25f,.75f};
    std::string text=prefix_ini()+"group_fit=vertical_v1\n";
    const char* keys[]{"fit_left_bits","fit_top_bits","fit_right_bits","fit_bottom_bits","fit_target_top_bits","fit_target_bottom_bits"};
    for(unsigned i=0;i<6;++i)text+=std::string(keys[i])+"="+std::to_string(bits(source[i]))+"\n";
    Config c;assert(parse(text,c)&&c.group_fit.enabled);const float shift=delta_for(c);
    assert(std::abs(shift-(.5f-(source[1]+source[3])/2))<1e-7f);
    assert(!parse(text+"vertical_offset_reference_pixels=374\n",c));
    assert(parse(text,c));auto changed=c;changed.group_fit.bounds[4]=bits(.49f);changed.group_fit.bounds[5]=bits(.51f);
    assert(!std::isfinite(delta_for(changed)));changed=c;changed.group_fit.bounds[4]=bits(.1f);
    assert(!same_offset(c,changed));
    changed=c;changed.group_fit.bounds[4]=bits(-.01f);assert(!std::isfinite(delta_for(changed)));
    changed=c;changed.group_fit.bounds[5]=bits(1.01f);assert(!std::isfinite(delta_for(changed)));
    auto outside=text;const auto field="fit_target_top_bits="+std::to_string(bits(.25f));
    outside.replace(outside.find(field),field.size(),"fit_target_top_bits="+std::to_string(bits(-.01f)));
    Config rejected;assert(!parse(outside,rejected));
    Engine e;e.c.schema=c.schema;e.c.registry_header_policy=c.registry_header_policy;e.c.checks=c.checks;
    e.c.group_fit=c.group_fit;
    for(auto const& check:c.checks)for(unsigned i=0;i<check.bytes.size();++i)e.memory[check.address+i]=check.bytes[i];
    State state;assert(apply(state,e.c,e.ctx,e.reader(),e.setter())==Result::applied);
    assert(e.pos()[0]==bits(value(e.c.top_bits)+shift));
    assert(restore(state,e.ctx,e.reader(),e.setter())==Result::restored&&e.pos()[0]==e.c.top_bits&&e.pos()[1]==e.c.bottom_bits);
}
int main(){parse_checks();lifecycle();failed_admission();lost_ownership();partial_failure();prefix_parser();prefix_authentication_and_restore();offset_parse_and_apply();group_fit_parse_and_apply();std::puts("PathChoicePositionTrialTest passed");}
