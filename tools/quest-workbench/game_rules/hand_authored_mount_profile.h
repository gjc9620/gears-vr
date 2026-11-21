#pragma once
#include "hand_wrist_calibration.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

// Authored asset-local mapping, not physical calibration or Native permission.
// The caller separately certifies current owner/asset, template digest and scale.
// Grip has ALREADY been converted to UE forward/right/up by rig_tracking_math.
namespace gears_hand_authored_mount {
using Signature=std::array<unsigned char,32>;
enum class Side:std::uint32_t {left=0,right=1,unknown=0xffffffffu};
enum class Model:std::uint32_t {unknown,marcus115};
enum class Provenance:std::uint32_t {unknown,designer_authored};
enum class Space:std::uint32_t {unknown,ue_grip_forward_right_up};
inline constexpr char profile_id[]="marcus115-openxr-grip-authored-v3-wrist-down20";
inline constexpr Signature marcus115_signature{{
    0x3b,0x3c,0x97,0x8c,0xb7,0xb6,0xdf,0x3f,0x3b,0xe4,0x8f,0x60,0xac,0x33,0xda,0xaf,
    0x45,0xdd,0x8f,0x30,0x81,0x6a,0x18,0xa3,0x92,0xfc,0x90,0x6b,0xdb,0xc7,0x51,0x93}};
struct Mount {
    Model model{Model::unknown};Side side{Side::unknown};Provenance provenance{Provenance::unknown};
    Space input_space{Space::unknown};Signature geometry_signature{};
    std::uint32_t bone_count{},hand_bone{};std::array<std::uint32_t,3> chain{};
    double model_to_world_scale{},units_per_metre{};
    gears_hand_wrist::Calibration calibration{};
};
inline bool valid_side(Side side) noexcept {return side==Side::left||side==Side::right;}

// Marker origin is halfway between wrist and the mean of the four direct
// non-thumb finger roots. The wrist's longitudinal +X continues forward from
// the forearm at neutral grip; it must not point upward by default. Mirror the
// left hand about that longitudinal axis, retaining the asset-local marker.
// Use the same right-wrist basis before and after taking a weapon. These are
// fixed authoring data, never a calibration fitted to a live animation pose.
inline bool marcus115(Signature const& actual_geometry,Side side,double model_scale,
    double units_per_metre,Mount& out) noexcept {
    if(actual_geometry!=marcus115_signature||!valid_side(side)||!std::isfinite(model_scale)||model_scale<=0||
       !std::isfinite(units_per_metre)||units_per_metre<10||units_per_metre>500)return false;
    const bool left=side==Side::left;
    const gears_vr::Vector raw=left?gears_vr::Vector{-5.4255638122558594,.45673370361328125,.20201516151428223}:
        gears_vr::Vector{-5.4255599975585938,-.45673370361328125,.20201301574707031};
    // Owner's physical check: depress both wrists20degrees in GRIP-local
    // forward/right/up space. Rotate the marker offset with the same mount so
    // the authored palm stays on the real controller; never alter XR input.
    const gears_vr::Quaternion down{0,.17364817766693034885,0,.98480775301220805937};
    const auto pitched=gears_vr::transform(gears_vr::rotation_matrix(down),raw);
    const double scale=model_scale/units_per_metre;
    const gears_vr::Vector metres{pitched.x*scale,pitched.y*scale,pitched.z*scale};
    if(!gears_vr::finite(metres))return false;
    Mount candidate{};candidate.model=Model::marcus115;candidate.side=side;candidate.provenance=Provenance::designer_authored;
    candidate.input_space=Space::ue_grip_forward_right_up;candidate.geometry_signature=actual_geometry;
    candidate.bone_count=115;candidate.hand_bone=left?11u:35u;
    candidate.chain=left?std::array<std::uint32_t,3>{{8,10,11}}:std::array<std::uint32_t,3>{{32,34,35}};
    candidate.model_to_world_scale=model_scale;candidate.units_per_metre=units_per_metre;
    const auto mirrored=left?gears_vr::Quaternion{1,0,0,0}:gears_vr::Quaternion{0,0,0,1};
    candidate.calibration={true,metres,gears_vr::multiply(down,mirrored)};
    out=candidate;return true;
}

// Owned-buffer codec for the supported current Native RefBone68 layout only.
// Offsets were matched mechanically against ALL 115 static q/T/parent records
// and the independently parsed retail RO3 reference. No GNames are decoded.
// Hash this output with the caller's existing SHA256 implementation. Matching
// a digest identifies this rig template; it does not authenticate live memory.
struct ReferenceRecord68 {unsigned char bytes[68]{};};
using CanonicalGeometry=std::array<unsigned char,115*36>;
inline bool canonical_geometry(ReferenceRecord68 const* owned,std::size_t count,CanonicalGeometry& out) noexcept {
    if(!owned||count!=115||sizeof(float)!=4||!std::numeric_limits<float>::is_iec559)return false;
    const std::uint32_t endian=1;if(*reinterpret_cast<unsigned char const*>(&endian)!=1)return false;
    const auto at=reinterpret_cast<std::uintptr_t>(owned),destination=reinterpret_cast<std::uintptr_t>(&out);
    const auto bytes=count*sizeof(ReferenceRecord68),output_bytes=sizeof(out);
    if(at>UINTPTR_MAX-(bytes-1)||destination>UINTPTR_MAX-(output_bytes-1)||
       (at<=destination+output_bytes-1&&destination<=at+bytes-1))return false;
    for(std::size_t i=0;i<count;++i){
        std::int32_t parent{};float qt[7]{};
        std::memcpy(&parent,owned[i].bytes+56,4);std::memcpy(qt,owned[i].bytes+12,28);
        if((i==0&&parent!=0)||(i&&(parent<0||static_cast<std::size_t>(parent)>=i)))return false;
        double norm{};for(unsigned j=0;j<7;++j){if(!std::isfinite(qt[j]))return false;if(j<4)norm+=double(qt[j])*qt[j];}
        if(norm<.98||norm>1.02)return false;
    }
    // All rejecting checks precede output writes; raw float bits are preserved.
    for(std::size_t i=0;i<count;++i){
        const auto index=static_cast<std::uint32_t>(i);auto* p=out.data()+i*36;
        std::memcpy(p,&index,4);std::memcpy(p+4,owned[i].bytes+56,4);std::memcpy(p+8,owned[i].bytes+12,28);
    }
    return true;
}
static_assert(sizeof(ReferenceRecord68)==68&&sizeof(CanonicalGeometry)==4140);
} // namespace gears_hand_authored_mount
