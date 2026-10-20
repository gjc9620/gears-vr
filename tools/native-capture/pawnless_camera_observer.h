#pragma once
#include "anchor_observer.h"

// Diagnostic observations only: no Anchor, ticket, scene or rendering authority.
// Read must be a nonthrowing bool(void*, const void*, size_t) reader.
namespace gears_pawnless_camera {
enum Role : unsigned {local,controller,camera,level,role_count};
enum Link : unsigned {controller_pawn,local_controller,player_camera,pc_owner,player,view_target,link_count};
inline constexpr unsigned objects_bit=0,names_bit=1,header_bit=2,object_slot_bit=6,name_slot_bit=10,
    link_bit=14,timestamp_bit=20,pov_bit=21;
// Config has no certified local.controller offset. Do not guess one or follow
// an arbitrary target. The existing certified pc.player -> local is sampled.
inline constexpr std::uint32_t unsupported_mask=1u<<(link_bit+local_controller);
inline constexpr std::uint32_t required_mask=((1u<<22)-1)&~unsupported_mask;
struct Pass {
    gears_anchor::Registry objects{},names{};
    std::array<std::array<unsigned char,96>,role_count> headers{};
    std::array<std::uintptr_t,role_count> object_slots{},name_slots{};
    std::array<std::uintptr_t,link_count> links{};
    std::uint32_t timestamp{};std::array<unsigned char,28> pov{};
    std::array<float,3> location{};float fov{},time{};
    std::uint32_t read_mask{},range_rejected_mask{};
    unsigned registry_valid_mask{},identity_match_mask{},link_match_mask{};
    bool finite_location{},valid_fov{},finite_timestamp{};
};
struct Snapshot {
    std::array<std::uintptr_t,role_count> expected{};
    std::uintptr_t initial_pawn{};
    bool config_valid{},initial_pawn_read{},initial_pawn_range_rejected{},sampled{};
    Pass before{},after{};
    std::uint32_t stable_mask{};
    bool reads_complete{},reads_stable{};
};
static_assert(sizeof(gears_anchor::Registry)==16&&sizeof(std::uintptr_t)==8,"Certified x64 registry layout");
static_assert(sizeof(Snapshot)<=2048,"Diagnostic snapshot must remain bounded");
namespace detail {
inline bool add(std::uintptr_t base,std::size_t offset,std::uintptr_t& out)noexcept{
    if(!base||offset>(std::numeric_limits<std::uintptr_t>::max)()-base)return false;out=base+offset;return true;
}
inline bool range(std::uintptr_t address,std::size_t size)noexcept{
    return address&&size&&size<=(std::numeric_limits<std::uintptr_t>::max)()-address;
}
inline bool registry(gears_anchor::Registry const& r)noexcept{
    return r.data&&r.count>1000&&r.count<=r.capacity&&r.capacity<4000000;
}
template<class T> bool field(std::array<unsigned char,96> const& raw,unsigned offset,T expected)noexcept{
    if(offset>raw.size()||sizeof(T)>raw.size()-offset)return false;T actual{};
    std::memcpy(&actual,raw.data()+offset,sizeof(actual));return actual==expected;
}
template<class Read> void capture(gears_anchor::Config const& c,Read read,
        std::array<unsigned,role_count> const& roles,Pass& p)noexcept{
    auto part=[&](void* dest,std::uintptr_t base,std::size_t offset,std::size_t size,unsigned bit){
        std::uintptr_t address{};if(!add(base,offset,address)||!range(address,size)){p.range_rejected_mask|=1u<<bit;return;}
        if(read(dest,reinterpret_cast<void const*>(address),size))p.read_mask|=1u<<bit;
    };
    part(&p.objects,c.objects_header,0,sizeof(p.objects),objects_bit);
    part(&p.names,c.names_header,0,sizeof(p.names),names_bit);
    if((p.read_mask&1)&&registry(p.objects))p.registry_valid_mask|=1;
    if((p.read_mask&2)&&registry(p.names))p.registry_valid_mask|=2;
    for(unsigned i=0;i<role_count;++i){auto const& id=c.identities[roles[i]];
        part(p.headers[i].data(),id.address,0,p.headers[i].size(),header_bit+i);
        auto slot=[&](gears_anchor::Registry const& r,unsigned index,std::uintptr_t& value,unsigned bit,unsigned valid){
            if(!(p.registry_valid_mask&valid)||index>=unsigned(r.count))return;
            if(std::uintptr_t(index)>(std::numeric_limits<std::uintptr_t>::max)()/8){p.range_rejected_mask|=1u<<bit;return;}
            part(&value,r.data,std::size_t(index)*8,sizeof(value),bit);
        };
        slot(p.objects,id.index,p.object_slots[i],object_slot_bit+i,1);
        slot(p.names,id.name_index,p.name_slots[i],name_slot_bit+i,2);
        const auto needed=(1u<<(header_bit+i))|(1u<<(object_slot_bit+i))|(1u<<(name_slot_bit+i));
        auto const& h=p.headers[i];
        if((p.read_mask&needed)==needed&&id.cls&&p.object_slots[i]==id.address&&p.name_slots[i]&&
           field(h,c.object_internal_index,id.index)&&field(h,c.object_name,id.name_index)&&
           field(h,c.object_name+4,id.number)&&field(h,c.object_class,id.cls)&&field(h,c.object_outer,id.outer))p.identity_match_mask|=1u<<i;
    }
    const auto pc=c.identities[c.role_controller].address,cam=c.identities[c.role_camera].address;
    const std::uintptr_t owners[]{pc,0,pc,cam,pc,cam};
    const unsigned offsets[]{c.controller_pawn,0,c.player_camera,c.pc_owner,c.player,c.camera_view_target};
    const std::uintptr_t expected[]{0,0,cam,pc,c.identities[c.role_local].address,0};
    for(unsigned i=0;i<link_count;++i)if(i!=local_controller){
        part(&p.links[i],owners[i],offsets[i],sizeof(p.links[i]),link_bit+i);
        if(i!=view_target&&(p.read_mask&(1u<<(link_bit+i)))&&p.links[i]==expected[i])p.link_match_mask|=1u<<i;
    }
    part(&p.timestamp,cam,c.camera_timestamp,sizeof(p.timestamp),timestamp_bit);
    part(p.pov.data(),cam,c.camera_pov,p.pov.size(),pov_bit);
    if(p.read_mask&(1u<<timestamp_bit)){std::memcpy(&p.time,&p.timestamp,4);p.finite_timestamp=std::isfinite(p.time);}
    if(p.read_mask&(1u<<pov_bit)){std::memcpy(p.location.data(),p.pov.data(),12);std::memcpy(&p.fov,p.pov.data()+24,4);
        p.finite_location=std::all_of(p.location.begin(),p.location.end(),[](float v){return std::isfinite(v);});
        p.valid_fov=std::isfinite(p.fov)&&p.fov>=1&&p.fov<=179;}
}
}
template<class Read> bool observe(gears_anchor::Config const& c,Read read,Snapshot& out)noexcept{
    out={};const std::array<unsigned,role_count> roles{c.role_local,c.role_controller,c.role_camera,c.role_level};
    if(!c.identity_count||c.identity_count>c.identities.size()||c.role_pawn>=c.identity_count||
       c.object_internal_index>92||c.object_name>88||c.object_class>88||c.object_outer>88||
       c.controller_pawn>65528||c.player_camera>65528||c.pc_owner>65528||c.player>65528||
       c.camera_view_target>65528||c.camera_timestamp>65532||c.camera_pov>65508)return false;
    for(unsigned i=0;i<role_count;++i){if(roles[i]>=c.identity_count)return false;auto const& id=c.identities[roles[i]];
        if(!id.address||!id.cls||!id.name[0]||id.name.back()||id.index>=4000000||id.name_index>=4000000)return false;
        out.expected[i]=id.address;
        for(unsigned j=0;j<i;++j)if(roles[i]==roles[j]||out.expected[i]==out.expected[j])return false;
    }
    out.config_valid=true;std::uintptr_t pawn_address{};
    if(!detail::add(out.expected[controller],c.controller_pawn,pawn_address)||!detail::range(pawn_address,8)){
        out.initial_pawn_range_rejected=true;return false;}
    out.initial_pawn_read=read(&out.initial_pawn,reinterpret_cast<void const*>(pawn_address),8);
    if(!out.initial_pawn_read||out.initial_pawn)return false;
    out.sampled=true;detail::capture(c,read,roles,out.before);detail::capture(c,read,roles,out.after);
    auto stable=[&](unsigned bit,void const* a,void const* b,std::size_t n){const auto mask=1u<<bit;
        if((out.before.read_mask&mask)&&(out.after.read_mask&mask)&&!std::memcmp(a,b,n))out.stable_mask|=mask;};
    stable(objects_bit,&out.before.objects,&out.after.objects,16);stable(names_bit,&out.before.names,&out.after.names,16);
    for(unsigned i=0;i<role_count;++i){stable(header_bit+i,out.before.headers[i].data(),out.after.headers[i].data(),96);
        stable(object_slot_bit+i,&out.before.object_slots[i],&out.after.object_slots[i],8);
        stable(name_slot_bit+i,&out.before.name_slots[i],&out.after.name_slots[i],8);}
    for(unsigned i=0;i<link_count;++i)stable(link_bit+i,&out.before.links[i],&out.after.links[i],8);
    stable(timestamp_bit,&out.before.timestamp,&out.after.timestamp,4);stable(pov_bit,out.before.pov.data(),out.after.pov.data(),28);
    out.reads_complete=(out.before.read_mask&out.after.read_mask&required_mask)==required_mask;
    out.reads_stable=(out.stable_mask&required_mask)==required_mask;return true;
}
// identity_match_mask compares numeric header identity and registered slots only;
// name text/classes/superchains are not reauthenticated here. Stable reads and a
// null Pawn never bypass an anchor guard or authorize rendering/scene ownership.
}
