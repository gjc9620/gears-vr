#pragma once
#include "weapon_hand_participant.h"
#include "weapon_lancer_front_handle.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Exact native render layouts, checked against the supported retail consumer
// and serializer. This is visual deformation only, never charging authority.
namespace gears_lancer_handle_morph {
namespace hand=gears_weapon_hand;
// The original main material's parent supports MorphTargets; the separate
// blade material does not (run67112 fatal buffer). Native must reserve both
// certified factory lookup hooks and pin the actual original material pair
// before binding. The unaffected blade chunk uses original regular factories.
// V2 selects only the owner's FRONT handle (4133..4196); no receiver motion.
// The instance-specific factory table closes the inlined consumer bypass.
// V2 physical adapter and original-vertex inputs are compiled together.
// Native still requires the exact material/factory/identity certificates.
inline constexpr bool retail_material_route_available=true;
struct Array {std::uintptr_t data{};std::int32_t count{},capacity{};};
#pragma pack(push,4)
struct Active {std::uintptr_t target{};float weight{};};
struct Delta {float position[3]{};std::uint32_t normal{0x80808080};std::uint16_t vertex{},padding{};};
struct Lod {Array vertices{};std::uint32_t base_vertices{};};
#pragma pack(pop)
static_assert(sizeof(Active)==12&&offsetof(Active,weight)==8);
static_assert(sizeof(Delta)==20&&offsetof(Delta,vertex)==16);
static_assert(sizeof(Lod)==20&&offsetof(Lod,base_vertices)==16);
inline constexpr unsigned vertices=gears_weapon::lancer_front::vertices,first_vertex=gears_weapon::lancer_front::first_vertex;
// These are lookup-only stack copies. The certified original lookup methods
// inspect the92-byte GPU header and only the active-morph count in its68-byte
// payload. The actual GPU, queued payload, arrays and materials are not set.
struct RegularLookup {
    alignas(16) std::array<std::uint8_t,92> gpu{};
    std::array<std::uint8_t,4> gpu_padding{};
    alignas(16) std::array<std::uint8_t,68> payload{};
    std::array<std::uint8_t,12> payload_padding{};
};
template<class T,std::size_t N>T field(std::array<std::uint8_t,N> const& bytes,unsigned at) noexcept {
    T value{};std::memcpy(&value,bytes.data()+at,sizeof value);return value;
}
template<class T,std::size_t N>void field(std::array<std::uint8_t,N>& bytes,unsigned at,T value) noexcept {
    std::memcpy(bytes.data()+at,&value,sizeof value);
}
template<class Read>bool regular_blade_lookup(Read&& read,std::uintptr_t gpu,std::uintptr_t owned_gpu,
    int lod,int chunk,std::uintptr_t first,std::uintptr_t second,RegularLookup& out) noexcept {
    if(!gpu||gpu!=owned_gpu||lod!=0||chunk!=1||!first||!second||first==second)return false;
    RegularLookup candidate{};
    if(!read(gpu,candidate.gpu.data(),candidate.gpu.size()))return false;
    const auto payload=field<std::uintptr_t>(candidate.gpu,80);
    const auto lods=field<Array>(candidate.gpu,44);
    if(!payload||!lods.data||lods.count<1||lods.count>8||lods.capacity<lods.count||lods.capacity>32||
       !read(payload,candidate.payload.data(),candidate.payload.size())||
       field<std::int32_t>(candidate.payload,40)!=0||field<std::int32_t>(candidate.payload,60)<=0)return false;
    const auto active=field<Array>(candidate.payload,44);
    std::array<Active,2> actual{};
    if(active.count!=2||active.capacity<2||active.capacity>65536||!active.data||
       !read(active.data,actual.data(),sizeof actual)||actual[0].target!=first||actual[1].target!=second)return false;
    for(auto const& value:actual)if(!std::isfinite(value.weight)||value.weight<0||value.weight>1)return false;
    // Reobserve the actual installed header/payload; these visual copies do
    // not renew tracking, manufacture a pose, or authorize any game action.
    std::array<std::uint8_t,92> closing_gpu{};std::array<std::uint8_t,68> closing_payload{};
    if(!read(gpu,closing_gpu.data(),closing_gpu.size())||closing_gpu!=candidate.gpu||
       !read(payload,closing_payload.data(),closing_payload.size())||closing_payload!=candidate.payload)return false;
    field(candidate.payload,60,std::int32_t(0));
    out=candidate;field(out.gpu,80,reinterpret_cast<std::uintptr_t>(out.payload.data()));return true;
}
struct V {double x{},y{},z{};};
inline V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V scale(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
inline V sub(V a,V b) noexcept {return add(a,scale(b,-1));}
inline double dot(V a,V b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b) noexcept {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool finite(V v) noexcept {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool weights(double distance_raw,std::array<float,2>& out) noexcept {
    return gears_weapon::lancer_front::weights(distance_raw,out);
}
// Re-observe a currently installed upload without manufacturing an upload
// serial. A static neutral part need not upload again just because the user
// spends time reaching for it. The caller binds the original actual upload
// identity; this only brackets its still-installed payload and current weights.
template<class Reader>
bool installed_distance(Reader read,std::uintptr_t gpu,std::uintptr_t payload,
    std::uintptr_t first,std::uintptr_t second,double& distance) noexcept {
    if(!gpu||gpu>UINTPTR_MAX-88||!payload||payload>UINTPTR_MAX-60||!first||!second||first==second)return false;
    std::uintptr_t installed{},closing{};Array a{},after{};
    if(!read(gpu+80,&installed,8)||installed!=payload||!read(payload+44,&a,sizeof a)||
       a.count<0||a.capacity<a.count||a.capacity>34||(a.count!=0&&a.count!=2))return false;
    double actual{};std::array<Active,2> pair{},again{};
    if(a.count==2){
        if(!a.data||a.data>UINTPTR_MAX-sizeof pair||!read(a.data,pair.data(),sizeof pair)||
           pair[0].target!=first||pair[1].target!=second||
           !std::isfinite(pair[0].weight)||!std::isfinite(pair[1].weight)||
           pair[0].weight<0||pair[0].weight>.5f||pair[1].weight<0||pair[1].weight>.5f||
           std::abs(pair[0].weight-pair[1].weight)>=.00001f)return false;
        actual=(double(pair[0].weight)+double(pair[1].weight))*gears_weapon::lancer_front::travel_raw;
        if(!read(a.data,again.data(),sizeof again)||std::memcmp(pair.data(),again.data(),sizeof pair))return false;
    }
    if(!read(payload+44,&after,sizeof after)||std::memcmp(&a,&after,sizeof a)||
       !read(gpu+80,&closing,8)||closing!=payload)return false;
    distance=actual;return true;
}
enum class Result:std::uint32_t {rejected,ready,revoking,retired,unavailable};
struct Request {
    std::uint32_t size{sizeof(Request)},version{2};hand::Frame source{};
    hand::ObjectIdentity component{},asset{};std::array<hand::ObjectIdentity,2> targets{};
};
struct Control {
    std::uint32_t size{sizeof(Control)},version{2};std::uint64_t handle{},cookie{};
    hand::Frame source{};double distance_raw{};
};
struct Receipt {
    std::uint32_t size{sizeof(Receipt)},version{2};Result result{Result::rejected};
    std::uint64_t handle{},submitted{},rendered{};double last_distance_raw{};bool fenced{};
    // actual_at_ns may be a bracketed current read of the same installed
    // upload. Only the actual RT upload callback advances the upload serial.
    double actual_distance_raw{};std::int64_t actual_at_ns{};std::uint64_t actual_upload_serial{};bool actual_known{};
    bool neutral_prepared{};
    std::uint64_t matched_updates{},angle_rejections{},admission_rejections{};
    std::uint64_t regular_factory_routes{},regular_secondary_routes{};
    unsigned binding_gate{};
};
inline constexpr char begin_export[]="GearsVrBindLancerFrontHandleMorphV2";
inline constexpr char update_export[]="GearsVrUpdateLancerFrontHandleMorphV2";
inline constexpr char revoke_export[]="GearsVrRevokeLancerFrontHandleMorphV2";
inline constexpr char poll_export[]="GearsVrPollLancerFrontHandleMorphV2";
#if defined(_WIN32)
using Begin=Result(__cdecl*)(Request const*,Receipt*) noexcept;
using Revoke=Result(__cdecl*)(Control const*,Receipt*) noexcept;
#endif
}
