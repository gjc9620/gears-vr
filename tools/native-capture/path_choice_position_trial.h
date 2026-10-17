#pragma once
#include "ui_scene_transform_trial.h"
#include <cstring>
#include <limits>

// Private CPU layout experiment. This admits only one authenticated SafePanel;
// it supplies no proof of GPU clipping, input alignment or headset acceptance.
namespace gears_path_choice_position {
using Check=gears_ui_scene_trial::Check;
struct Config {
    std::uint64_t nonce{},pid{},creation_time{},started_qpc{},world_generation{},base{},client{},scene{},panel{},panel_vtable{};
    std::uint32_t top_bits{},bottom_bits{};unsigned duration_ms{};std::vector<Check> checks;
};
inline std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
inline float value(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}
inline constexpr float delta=16.f/1080.f;
inline bool overlap(std::uintptr_t a,std::size_t n,std::uintptr_t b,std::size_t m){return a<b+m&&b<a+n;}
inline bool parse(std::string text,Config& out){
    if(text.empty()||text.size()>262144||text.find('\0')!=std::string::npos)return false;
    std::map<std::string,std::string> fields;bool section=false;
    for(std::size_t begin=0;begin<text.size();){auto end=text.find('\n',begin);if(end==std::string::npos)end=text.size();
        auto line=text.substr(begin,end-begin);begin=end+1;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;if(line=="[PathChoicePositionTrial]"){if(section)return false;section=true;continue;}
        auto split=line.find('=');if(!section||split==std::string::npos||!split||line.size()>1024||
            !fields.emplace(line.substr(0,split),line.substr(split+1)).second)return false;}
    Config c;unsigned schema{},count{};
    auto u=[&](char const* key,auto& v){auto it=fields.find(key);return it!=fields.end()&&gears_ui_scene_trial::decimal(it->second,v);};
    auto a=[&](char const* key,auto& v){auto it=fields.find(key);std::uintptr_t p{};if(it==fields.end()||!gears_ui_scene_trial::address(it->second,p))return false;v=p;return true;};
    if(!u("schema",schema)||schema!=1||!u("nonce",c.nonce)||!c.nonce||!u("pid",c.pid)||!c.pid||c.pid>0xffffffffull||
       !u("creation_time",c.creation_time)||!c.creation_time||!u("started_qpc",c.started_qpc)||!c.started_qpc||
       !u("world_generation",c.world_generation)||!c.world_generation||!a("base",c.base)||!a("client",c.client)||
       !a("scene",c.scene)||!a("panel",c.panel)||!a("panel_vtable",c.panel_vtable)||
       c.panel==c.scene||c.panel==c.client||c.scene==c.client||c.panel>UINTPTR_MAX-0x1000||
       c.panel_vtable>UINTPTR_MAX-0x400||c.base>UINTPTR_MAX-0x2000000||
       !u("top_bits",c.top_bits)||!u("bottom_bits",c.bottom_bits)||
       !std::isfinite(value(c.top_bits))||!std::isfinite(value(c.bottom_bits))||
       value(c.top_bits)<0||value(c.bottom_bits)>1||value(c.top_bits)>=value(c.bottom_bits)||
       !u("duration_ms",c.duration_ms)||c.duration_ms<100||c.duration_ms>8000||
       !u("check_count",count)||count<16||count>2048||fields.size()!=15+count)return false;
    std::map<std::uintptr_t,unsigned char> seen;std::size_t total{};
    for(unsigned i=0;i<count;++i){auto it=fields.find("check_"+std::to_string(i));if(it==fields.end())return false;
        auto split=it->second.find(':');Check check;
        if(split==std::string::npos||!gears_ui_scene_trial::address(std::string_view(it->second).substr(0,split),check.address))return false;
        auto hex=it->second.substr(split+1);if(hex.empty()||hex.size()%2||hex.size()>512)return false;
        auto size=hex.size()/2;total+=size;if(total>32768||check.address>UINTPTR_MAX-size||
            overlap(check.address,size,c.panel+0x64,4)||overlap(check.address,size,c.panel+0x6c,4)||
            overlap(check.address,size,c.panel+0x74,4))return false;
        auto nib=[](char ch){return ch>='0'&&ch<='9'?ch-'0':ch>='a'&&ch<='f'?ch-'a'+10:-1;};
        for(std::size_t j=0;j<hex.size();j+=2){int hi=nib(hex[j]),lo=nib(hex[j+1]);if(hi<0||lo<0)return false;
            auto b=static_cast<unsigned char>(hi*16+lo);auto p=seen.emplace(check.address+j/2,b);if(!p.second&&p.first->second!=b)return false;check.bytes.push_back(b);}
        c.checks.push_back(std::move(check));}
    out=std::move(c);return true;
}
struct Context {std::uint64_t pid{},creation_time{},started_qpc{},world_generation{},base{},client{},scene{};};
inline bool same(Config const& c,Context const& x){return c.pid==x.pid&&c.creation_time==x.creation_time&&
    c.started_qpc==x.started_qpc&&c.world_generation==x.world_generation&&c.base==x.base&&c.client==x.client&&c.scene==x.scene;}
template<class Read> bool authenticate(Config const& c,Context const& x,Read read){
    if(!same(c,x))return false;
    std::array<unsigned char,256> buf{};
    for(auto const& check:c.checks)if(check.bytes.empty()||check.bytes.size()>buf.size()||
        !read(buf.data(),reinterpret_cast<void const*>(check.address),check.bytes.size())||
        !std::equal(check.bytes.begin(),check.bytes.end(),buf.begin()))return false;
    std::uintptr_t vt{},invalidate{},notify{};std::uint32_t units{};
    return read(&vt,reinterpret_cast<void const*>(c.panel),8)&&vt==c.panel_vtable&&
        read(&invalidate,reinterpret_cast<void const*>(vt+0x3d8),8)&&invalidate==c.base+0x2988d8&&
        read(&notify,reinterpret_cast<void const*>(vt+0x3b8),8)&&notify==c.base+0x297338&&
        read(&units,reinterpret_cast<void const*>(c.panel+0x70),4)&&units==0x04040404;
}
template<class Read> bool positions(Config const& c,Read read,std::array<std::uint32_t,2>& p){
    return read(&p[0],reinterpret_cast<void const*>(c.panel+0x64),4)&&read(&p[1],reinterpret_cast<void const*>(c.panel+0x6c),4);}
enum class Result { idle,applied,holding,restored,rejected,identity_lost,ownership_lost,setter_failed };
struct Lease {Config config;std::array<std::uint32_t,2> owned{};std::array<bool,2> touched{};bool active{};};
struct State {Lease lease;std::uint64_t consumed_nonce{};Result last=Result::idle;};
// set(panel,float,face,inputType,zeroOrigin) returns success and may fail after a
// mutation. Read back after every call, including failed calls, before rollback.
template<class Read,class Set> Result restore(State& s,Context const& x,Read read,Set set){
    auto& l=s.lease;if(!l.active)return s.last;
    if(!authenticate(l.config,x,read)){l.active=false;return s.last=Result::identity_lost;}
    std::array<std::uint32_t,2> p{};if(!positions(l.config,read,p)){l.active=false;return s.last=Result::identity_lost;}
    for(unsigned i=0;i<2;++i)if(l.touched[i]&&p[i]!=l.owned[i]){l.active=false;return s.last=Result::ownership_lost;}
    bool ok=true;const std::uint32_t before[]={l.config.top_bits,l.config.bottom_bits};
    for(unsigned i=0;i<2;++i)if(l.touched[i]){
        // Revalidate before each native call; no stale pointer write on rollback.
        if(!authenticate(l.config,x,read)||!positions(l.config,read,p)||p[i]!=l.owned[i]){l.active=false;return s.last=Result::identity_lost;}
        bool called=false;try{called=set(l.config.panel,value(before[i]),i?3:1,0,0);}catch(...){called=false;}
        if(!positions(l.config,read,p)||p[i]!=before[i]){ok=false;break;}
        l.touched[i]=false;l.owned[i]=before[i];ok&=called;
    }
    l.active=l.touched[0]||l.touched[1];return s.last=ok&&!l.active?Result::restored:Result::setter_failed;
}
template<class Read,class Set> Result apply(State& s,Config const& c,Context const& x,Read read,Set set){
    if(s.lease.active)return s.last=Result::holding;
    if(c.nonce<=s.consumed_nonce)return s.last=Result::rejected;
    s.consumed_nonce=c.nonce;
    if(!authenticate(c,x,read))return s.last=Result::rejected;
    std::array<std::uint32_t,2> p{};if(!positions(c,read,p)||p[0]!=c.top_bits||p[1]!=c.bottom_bits)return s.last=Result::rejected;
    const auto before=p;s.lease={c,before, {},true};auto& l=s.lease;
    for(unsigned i=0;i<2;++i){
        if(!authenticate(c,x,read)||!positions(c,read,p)){l.active=false;return s.last=Result::identity_lost;}
        for(unsigned j=0;j<2;++j)if(p[j]!=(l.touched[j]?l.owned[j]:before[j])){l.active=false;return s.last=Result::ownership_lost;}
        const auto target=bits(value(before[i])+delta);bool called=false;
        try{called=set(c.panel,value(target),i?3:1,0,0);}catch(...){called=false;}
        if(!positions(c,read,p)){l.active=false;return s.last=Result::identity_lost;}
        if(p[i]==target){l.touched[i]=true;l.owned[i]=target;}
        else if(p[i]!=before[i]){l.active=false;return s.last=Result::ownership_lost;}
        if(!called||!l.touched[i]){restore(s,x,read,set);return s.last=Result::setter_failed;}
    }
    return s.last=Result::applied;
}
} // namespace gears_path_choice_position
