// Read-only eye placement from the same certified Marcus reference geometry
// used by the body planner. Pawn EyeHeight is a gameplay camera pivot, not
// the model's eyes. While the body planner has no committed view, follow the
// actual head's authored eye mount so an animated weapon stance cannot leave
// the fallback camera inside the neck. A committed body view replaces this
// fallback as one complete basis. No pose, scale or game memory is set.
#include "body_head_authored_mount.h"
#include "cover_pose_phase.h"
#include <bcrypt.h>
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace first_person_live {
namespace eye_body=gears_full_body_c0;
namespace eye_marker=gears_body_head_authored;
namespace eye_phase=gears_cover_pose_phase;
struct AuthoredEyeCache {
    gears_anchor::Identity asset{};
    eye_phase::ArrayHeader reference{};
    std::uint64_t world{};
    eye_body::Model model{};
    eye_body::Frame frame{};
    eye_marker::Workspace work{};
    eye_marker::Marker marker{};
    bool ready{};
};
AuthoredEyeCache authored_cache;
bool authored_digest(void const* p,size_t n,eye_body::rig::Signature& result){
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};
    bool ok=n<=ULONG_MAX&&BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(p)),ULONG(n),0)>=0;
    if(ok)ok=BCryptFinishHash(h,result.data(),ULONG(result.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;
}
bool authored_eye(gears_anchor::Receipt const* anchor,gears_vr::Vector& output){
    namespace binding=weapon_hand_scene_binding;
    if(!anchor)return false;
    binding::Binding owner{},closing{};
    // Inventory/Instigator can be in a legitimate handoff during original
    // pickup animation. The eye observes the current local avatar, and grants
    // no weapon action, so it must not depend on that changing weapon binding.
    if(!binding::sample_visual(anchor,owner))return false;
    gears_vr::Vector eye{};
    {
        std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current())return false;
        auto read=[](void* out,void const* in,size_t size){return safe_copy(out,in,size);};
        auto const mesh=owner.mesh.address;
        uintptr_t asset{},master{},again_asset{};
        eye_phase::ArrayHeader ref{},again_ref{},space{},again_space{};
        eye_phase::Matrix component{},again_component{},head{},again_head{};
        gears_anchor::Identity asset_id{},again_id{};
        if(!eye_phase::field(read,asset,mesh,0x218)||!asset||
           !eye_phase::field(read,master,mesh,0x29c)||master||
           !binding::identity(asset,asset_id)||
           !eye_phase::field(read,ref,asset,0xa4)||ref.count!=int(eye_body::bones)||
           !eye_phase::header_valid(ref,sizeof(eye_body::rig::ReferenceRecord68))||
           !eye_phase::field(read,component,mesh,0xa0)||!eye_phase::finite(component)||
           !eye_phase::field(read,space,mesh,0x26c)||space.count!=int(eye_body::bones)||
           !eye_phase::header_valid(space,sizeof(eye_phase::Matrix))||
           !eye_phase::field(read,head,space.data,eye_marker::head_bone*sizeof head)||!eye_phase::finite(head))return false;
        auto& cache=authored_cache;
        if(!cache.ready||cache.world!=owner.world_generation||
           !binding::same(cache.asset,asset_id)||!eye_phase::header_equal(cache.reference,ref)){
            cache.ready=false;cache.frame={};cache.frame.count=eye_body::bones;
            cache.frame.units_per_metre=stereo_units_per_metre;cache.frame.model_scale=1.;
            if(!eye_phase::bytes(read,cache.frame.reference.data(),ref.data,sizeof cache.frame.reference)||
               eye_phase::repeat(read,cache.frame.reference.data(),ref.data,sizeof cache.frame.reference)!=eye_phase::Reject::none||
               !eye_body::prepare_model(cache.frame.reference,authored_digest,cache.model)||
               eye_marker::prepare(cache.model,cache.frame,eye_marker::Basis::marcus_mesh_face_forward_y,cache.work,cache.marker)!=eye_marker::Status::okay)return false;
            cache.asset=asset_id;cache.reference=ref;cache.world=owner.world_generation;cache.ready=true;
        }
        const auto local=cache.marker.head_to_eye_local().position;
        const auto& h=head.values;
        if(std::abs(h[3])>1e-4||std::abs(h[7])>1e-4||std::abs(h[11])>1e-4||std::abs(h[15]-1)>1e-4)return false;
        const eye_body::Vec p{local.x*h[0]+local.y*h[4]+local.z*h[8]+h[12],
             local.x*h[1]+local.y*h[5]+local.z*h[9]+h[13],
             local.x*h[2]+local.y*h[6]+local.z*h[10]+h[14]};
        const auto& m=component.values;
        if(std::abs(m[3])>1e-4||std::abs(m[7])>1e-4||std::abs(m[11])>1e-4||std::abs(m[15]-1)>1e-4)return false;
        eye={p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],
             p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],
             p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};
        const auto& a=anchor->anchor;
        // Pawn.Location is the capsule pivot, not the mesh's ground/root.
        // A valid low-cover head can be BELOW that pivot. Check its lower
        // extent in the same actual mesh space that supplied the head mount;
        // retain the independent world/Pawn horizontal and upper bounds.
        if(!std::isfinite(eye.x)||!std::isfinite(eye.y)||!std::isfinite(eye.z)||
           std::abs(eye.x-a.location[0])>stereo_units_per_metre||
           std::abs(eye.y-a.location[1])>stereo_units_per_metre||
           p.z<stereo_units_per_metre*.1||eye.z-a.location[2]>stereo_units_per_metre*1.5)return false;
        if(!eye_phase::field(read,again_asset,mesh,0x218)||again_asset!=asset||
           !eye_phase::field(read,again_ref,asset,0xa4)||!eye_phase::header_equal(ref,again_ref)||
           !eye_phase::field(read,again_component,mesh,0xa0)||std::memcmp(&component,&again_component,sizeof component)||
           !eye_phase::field(read,again_space,mesh,0x26c)||!eye_phase::header_equal(space,again_space)||
           !eye_phase::field(read,again_head,space.data,eye_marker::head_bone*sizeof head)||std::memcmp(&head,&again_head,sizeof head)||
           !binding::identity(asset,again_id)||!binding::same(asset_id,again_id)||!binding::metadata_current())return false;
    }
    if(!binding::sample_visual(anchor,closing)||!binding::same_visual_key(owner,closing))return false;
    output=eye;authored_eye_last={eye.x,eye.y,eye.z};++authored_eye_applied;return true;
}
}
#else
namespace first_person_live {bool authored_eye(gears_anchor::Receipt const*,gears_vr::Vector&){return false;}}
#endif
