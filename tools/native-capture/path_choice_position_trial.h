#pragma once
#include "ui_scene_transform_trial.h"
#include "ui_group_placement.h"
#include <cstring>
#include <limits>

// Private CPU layout experiment. This admits only one authenticated SafePanel;
// it supplies no proof of GPU clipping, input alignment or headset acceptance.
namespace gears_path_choice_position {
using Check=gears_ui_scene_trial::Check;
enum class RegistryHeaderPolicy {exact,fixed_prefix_v1};
struct GroupFit {bool enabled{};std::array<std::uint32_t,6> bounds{};};
struct Config {
    std::uint64_t nonce{},pid{},creation_time{},started_qpc{},world_generation{},base{},client{},scene{},panel{},panel_vtable{};
    std::uint32_t top_bits{},bottom_bits{};unsigned duration_ms{};std::vector<Check> checks;
    unsigned schema=1;RegistryHeaderPolicy registry_header_policy=RegistryHeaderPolicy::exact;
    unsigned vertical_offset_reference_pixels=16;
    GroupFit group_fit{};
};
inline std::uint32_t bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
inline float value(std::uint32_t b){float v;std::memcpy(&v,&b,4);return v;}
inline constexpr float delta=16.f/1080.f;
inline float delta_for(Config const& c){
    if(!c.group_fit.enabled)return static_cast<float>(c.vertical_offset_reference_pixels)/1080.f;
    auto const& b=c.group_fit.bounds;
    gears_ui_group::Bounds source{value(b[0]),value(b[1]),value(b[2]),value(b[3]),true};
    gears_ui_group::Bounds target{source.left,value(b[4]),source.right,value(b[5]),true};
    gears_ui_group::Recipe recipe{};
    if(c.vertical_offset_reference_pixels!=16||source.left<0||source.top<0||source.right>1||source.bottom>1||
       !gears_ui_group::fit(source,target,1,recipe)||recipe.scale!=1||recipe.translation_x!=0||
       !std::isfinite(recipe.translation_y)||recipe.translation_y<=0||recipe.translation_y>400./1080.)
        return (std::numeric_limits<float>::quiet_NaN)();
    return static_cast<float>(recipe.translation_y);
}
template<class A,class B> bool same_offset(A const& a,B const& b){return
    a.vertical_offset_reference_pixels==b.vertical_offset_reference_pixels&&a.group_fit.enabled==b.group_fit.enabled&&
    a.group_fit.bounds==b.group_fit.bounds;}
inline bool overlap(std::uintptr_t a,std::size_t n,std::uintptr_t b,std::size_t m){return a<b+m&&b<a+n;}
inline constexpr std::array<std::uintptr_t,2> registry_header_rvas{0x1c24730,0x1c34800};
struct RegistryHeader {std::uint64_t data{};std::int32_t num{},max{};};
static_assert(sizeof(RegistryHeader)==16,"Expected x64 Qii registry header");
inline RegistryHeader registry_header(unsigned char const* bytes){RegistryHeader h;std::memcpy(&h,bytes,16);return h;}
inline bool valid_registry_header(RegistryHeader const& h){return h.data&&h.num>0&&h.num<=h.max&&h.max<4000000&&
    h.data<=UINTPTR_MAX-std::uint64_t(h.max)*8;}
inline int registry_header_index(Config const& c,std::uintptr_t address,std::size_t size){
    for(unsigned i=0;i<registry_header_rvas.size();++i)if(size==16&&address==c.base+registry_header_rvas[i])return int(i);return -1;
}
inline bool valid_registry_policy(Config const& c){
    if(c.schema==1)return c.registry_header_policy==RegistryHeaderPolicy::exact&&c.vertical_offset_reference_pixels==16&&!c.group_fit.enabled;
    if(c.schema!=2||c.registry_header_policy!=RegistryHeaderPolicy::fixed_prefix_v1||c.base>UINTPTR_MAX-0x2000000)return false;
    if(c.vertical_offset_reference_pixels<1||c.vertical_offset_reference_pixels>400)return false;
    if(c.group_fit.enabled&&!std::isfinite(delta_for(c)))return false;
    std::array<unsigned,2> found{};std::array<RegistryHeader,2> headers{};
    for(auto const& check:c.checks){
        if(check.address>UINTPTR_MAX-check.bytes.size())return false;
        for(unsigned i=0;i<registry_header_rvas.size();++i)if(overlap(check.address,check.bytes.size(),c.base+registry_header_rvas[i],16)){
            if(check.address!=c.base+registry_header_rvas[i]||check.bytes.size()!=16||++found[i]!=1)return false;
            headers[i]=registry_header(check.bytes.data());if(!valid_registry_header(headers[i]))return false;
        }
    }
    return found[0]==1&&found[1]==1&&headers[0].data!=headers[1].data;
}
enum class CheckMatch {rejected,exact,accepted_registry_suffix};
inline CheckMatch match_check(Config const& c,Check const& check,unsigned char const* observed,std::size_t size){
    if(size!=check.bytes.size())return CheckMatch::rejected;
    const int header=registry_header_index(c,check.address,size);
    if(c.schema==2&&c.registry_header_policy==RegistryHeaderPolicy::fixed_prefix_v1&&header>=0){
        auto initial=registry_header(check.bytes.data()),live=registry_header(observed);
        if(!valid_registry_header(initial)||!valid_registry_header(live)||live.data!=initial.data||live.max!=initial.max||live.num<initial.num)
            return CheckMatch::rejected;
        return live.num==initial.num?CheckMatch::exact:CheckMatch::accepted_registry_suffix;
    }
    return std::equal(check.bytes.begin(),check.bytes.end(),observed)?CheckMatch::exact:CheckMatch::rejected;
}
inline bool parse(std::string text,Config& out){
    if(text.empty()||text.size()>262144||text.find('\0')!=std::string::npos)return false;
    std::map<std::string,std::string> fields;bool section=false;
    for(std::size_t begin=0;begin<text.size();){auto end=text.find('\n',begin);if(end==std::string::npos)end=text.size();
        auto line=text.substr(begin,end-begin);begin=end+1;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;if(line=="[PathChoicePositionTrial]"){if(section)return false;section=true;continue;}
        auto split=line.find('=');if(!section||split==std::string::npos||!split||line.size()>1024||
            !fields.emplace(line.substr(0,split),line.substr(split+1)).second)return false;}
    Config c;unsigned count{};
    auto u=[&](char const* key,auto& v){auto it=fields.find(key);return it!=fields.end()&&gears_ui_scene_trial::decimal(it->second,v);};
    auto a=[&](char const* key,auto& v){auto it=fields.find(key);std::uintptr_t p{};if(it==fields.end()||!gears_ui_scene_trial::address(it->second,p))return false;v=p;return true;};
    if(!u("schema",c.schema)||(c.schema!=1&&c.schema!=2)||!u("nonce",c.nonce)||!c.nonce||!u("pid",c.pid)||!c.pid||c.pid>0xffffffffull||
       !u("creation_time",c.creation_time)||!c.creation_time||!u("started_qpc",c.started_qpc)||!c.started_qpc||
       !u("world_generation",c.world_generation)||!c.world_generation||!a("base",c.base)||!a("client",c.client)||
       !a("scene",c.scene)||!a("panel",c.panel)||!a("panel_vtable",c.panel_vtable)||
       c.panel==c.scene||c.panel==c.client||c.scene==c.client||c.panel>UINTPTR_MAX-0x1000||
       c.panel_vtable>UINTPTR_MAX-0x400||c.base>UINTPTR_MAX-0x2000000||
       !u("top_bits",c.top_bits)||!u("bottom_bits",c.bottom_bits)||
       !std::isfinite(value(c.top_bits))||!std::isfinite(value(c.bottom_bits))||
       value(c.top_bits)<0||value(c.bottom_bits)>1||value(c.top_bits)>=value(c.bottom_bits)||
       !u("duration_ms",c.duration_ms)||c.duration_ms<100||c.duration_ms>8000||
       !u("check_count",count)||count<16||count>2048)return false;
    const bool explicit_offset=fields.find("vertical_offset_reference_pixels")!=fields.end();
    const bool explicit_fit=fields.find("group_fit")!=fields.end();
    if(fields.size()!=(c.schema==2?16u+unsigned(explicit_offset)+7u*unsigned(explicit_fit):15u)+count)return false;
    if(c.schema==2){auto policy=fields.find("registry_header_policy");
        if(policy==fields.end()||policy->second!="fixed_prefix_v1")return false;c.registry_header_policy=RegistryHeaderPolicy::fixed_prefix_v1;
        if(explicit_offset&&(!u("vertical_offset_reference_pixels",c.vertical_offset_reference_pixels)||c.vertical_offset_reference_pixels<1||c.vertical_offset_reference_pixels>400))return false;}
    if(explicit_fit){
        if(c.schema!=2||explicit_offset||fields.at("group_fit")!="vertical_v1")return false;
        constexpr char const* keys[]{"fit_left_bits","fit_top_bits","fit_right_bits","fit_bottom_bits","fit_target_top_bits","fit_target_bottom_bits"};
        c.group_fit.enabled=true;for(unsigned i=0;i<6;++i)if(!u(keys[i],c.group_fit.bounds[i]))return false;
        if(!std::isfinite(delta_for(c)))return false;
    }
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
    if(!valid_registry_policy(c))return false;out=std::move(c);return true;
}
struct Context {std::uint64_t pid{},creation_time{},started_qpc{},world_generation{},base{},client{},scene{};};
inline bool same(Config const& c,Context const& x){return c.pid==x.pid&&c.creation_time==x.creation_time&&
    c.started_qpc==x.started_qpc&&c.world_generation==x.world_generation&&c.base==x.base&&c.client==x.client&&c.scene==x.scene;}
template<class Read> bool authenticate(Config const& c,Context const& x,Read read){
    if(!same(c,x)||!valid_registry_policy(c))return false;
    std::array<unsigned char,256> buf{};
    for(auto const& check:c.checks)if(check.bytes.empty()||check.bytes.size()>buf.size()||
        !read(buf.data(),reinterpret_cast<void const*>(check.address),check.bytes.size())||
        match_check(c,check,buf.data(),check.bytes.size())==CheckMatch::rejected)return false;
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
        const auto target=bits(value(before[i])+delta_for(c));bool called=false;
        try{called=set(c.panel,value(target),i?3:1,0,0);}catch(...){called=false;}
        if(!positions(c,read,p)){l.active=false;return s.last=Result::identity_lost;}
        if(p[i]==target){l.touched[i]=true;l.owned[i]=target;}
        else if(p[i]!=before[i]){l.active=false;return s.last=Result::ownership_lost;}
        if(!called||!l.touched[i]){restore(s,x,read,set);return s.last=Result::setter_failed;}
    }
    return s.last=Result::applied;
}
} // namespace gears_path_choice_position
