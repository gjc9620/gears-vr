#pragma once
#include "body_head_authored_mount.h"
#include <cfloat>

// A local render-palette operation on the certified Marcus115 profile only.
// Never changes the skeleton, physics, tracking, hands or an asset. Collapse
// head AND neck at the neck base: leaving the neck rendered around an eye
// inside the animated head exposes its interior when the player looks down.
namespace gears_first_person_visibility {
namespace body=gears_full_body_c0;
inline constexpr unsigned neck_bone=54;
inline constexpr std::array<unsigned,2> shoulder_armor_bones{{7,31}};
// Original WarPawn weapon pickup owns its arms during SM53 (the recorded
// AR_Idle_Ready_Kickup path). This is a display distinction, never hand input,
// C0 composition, inventory or firing authority.
inline bool original_pickup(unsigned special_move) noexcept {return special_move==53;}
// Display-only animation policy. The caller independently authenticates the
// current local avatar and actual Lancer class before setting lancer. SM33/34
// are original saw hold/execution, not general special moves. No hand solver,
// weapon action or retained tracking pose is authorized by this predicate.
inline bool original_lancer_animation(unsigned special_move,bool lancer) noexcept {
    return lancer&&(special_move==33||special_move==34);
}
// Geometry comparison only, never a source receipt or action permission.
// UE row-vector skinning: reference-space * inverse-reference/current-space.
template<class Matrix>
bool matches_completed_wrist(Matrix const& reference,Matrix const& skin,Matrix const& completed) noexcept {
    for(unsigned i=0;i<16;++i)
        if(!std::isfinite(reference.values[i])||!std::isfinite(skin.values[i])||!std::isfinite(completed.values[i]))return false;
    for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k){
        double value=0;for(unsigned n=0;n<4;++n)value+=double(reference.values[r*4+n])*skin.values[n*4+k];
        if(!std::isfinite(value)||std::abs(value-completed.values[r*4+k])>.002)return false;
    }
    return true;
}
inline bool descendant(std::array<std::int32_t,body::bones> const& parents,unsigned index,unsigned root) noexcept {
    for(unsigned n=0;n<body::bones;++n){
        if(index==root)return true;
        if(!index||index>=body::bones||parents[index]<0||unsigned(parents[index])>=index)return false;
        index=unsigned(parents[index]);
    }
    return false;
}
inline bool neck_descendant(std::array<std::int32_t,body::bones> const& parents,unsigned index) noexcept {
    return descendant(parents,index,neck_bone);
}
template<class Matrix>
bool mask(std::array<std::int32_t,body::bones> const& parents,body::Vec reference_neck,
    std::array<Matrix,body::bones> const& before,std::array<Matrix,body::bones>& after,unsigned& hidden) noexcept {
    hidden=0;
    if(&before==&after||parents[gears_body_head_authored::head_bone]!=int(neck_bone)||parents[neck_bone]!=5||
       !body::d::finite_vector(reference_neck))return false;
    const auto& neck=before[neck_bone].values;
    for(float v:neck)if(!std::isfinite(v))return false;
    const body::Vec centre{reference_neck.x*neck[0]+reference_neck.y*neck[4]+reference_neck.z*neck[8]+neck[12],
        reference_neck.x*neck[1]+reference_neck.y*neck[5]+reference_neck.z*neck[9]+neck[13],
        reference_neck.x*neck[2]+reference_neck.y*neck[6]+reference_neck.z*neck[10]+neck[14]};
    if(!body::d::finite_vector(centre)||std::abs(centre.x)>FLT_MAX||std::abs(centre.y)>FLT_MAX||std::abs(centre.z)>FLT_MAX)return false;
    after=before;
    for(unsigned i=0;i<body::bones;++i){
        if(!neck_descendant(parents,i))continue;
        auto& m=after[i].values;std::fill(std::begin(m),std::end(m),0.f);
        m[12]=float(centre.x);m[13]=float(centre.y);m[14]=float(centre.z);m[15]=1.f;++hidden;
    }
    return hidden>1;
}
// The original, signature-checked Marcus LOD0 has two separate rigid shoulder
// pads: bones7/31, each562 vertices /814 triangles, with no mixed influences or
// triangles crossing into the arms. Hide only those pads from the local first
// person palette. Upper arms8/32 are siblings, never descendants or targets.
// The mesh asset, reference pose, animation, physics and C0 remain untouched.
template<class Matrix>
bool hide_shoulder_armor(std::array<std::int32_t,body::bones> const& parents,
    std::array<body::Vec,2> const& reference,
    std::array<Matrix,body::bones> const& before,std::array<Matrix,body::bones>& staged) noexcept {
    if(&before==&staged||parents[7]!=6||parents[31]!=30||parents[8]!=6||parents[32]!=30)return false;
    for(unsigned i=1;i<body::bones;++i)if(parents[i]==7||parents[i]==31)return false;
    std::array<body::Vec,2> centres{};
    for(unsigned j=0;j<2;++j){
        const auto& m=before[shoulder_armor_bones[j]].values;const auto p=reference[j];
        if(!body::d::finite_vector(p))return false;
        for(float value:m)if(!std::isfinite(value))return false;
        centres[j]={p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],
            p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};
        const auto q=centres[j];
        if(!body::d::finite_vector(q)||std::abs(q.x)>FLT_MAX||std::abs(q.y)>FLT_MAX||std::abs(q.z)>FLT_MAX)return false;
    }
    // Validate the pair before changing either entry. All other staged bytes,
    // including the existing neck mask and both entire arms, remain exact.
    for(unsigned j=0;j<2;++j){
        auto& m=staged[shoulder_armor_bones[j]].values;std::fill(std::begin(m),std::end(m),0.f);
        m[12]=float(centres[j].x);m[13]=float(centres[j].y);m[14]=float(centres[j].z);m[15]=1.f;
    }
    return true;
}
// Called only when the SAME current completed right-hand receipt explicitly
// says the left grip is unavailable. Hide that local render branch rather
// than exposing the original weapon animation as a fake tracked left hand.
// The already staged head mask and every non-left entry remain unchanged.
// No model/skeleton/physics/pose/availability or gameplay data is edited.
template<class Matrix>
bool hide_untracked_left(std::array<std::int32_t,body::bones> const& parents,body::Vec reference_shoulder,
    std::array<Matrix,body::bones> const& before,std::array<Matrix,body::bones>& staged,unsigned& hidden) noexcept {
    hidden=0;constexpr unsigned shoulder=8;
    if(&before==&staged||parents[shoulder]!=6||!body::d::finite_vector(reference_shoulder))return false;
    const auto& m=before[shoulder].values;for(float v:m)if(!std::isfinite(v))return false;
    const body::Vec centre{reference_shoulder.x*m[0]+reference_shoulder.y*m[4]+reference_shoulder.z*m[8]+m[12],
        reference_shoulder.x*m[1]+reference_shoulder.y*m[5]+reference_shoulder.z*m[9]+m[13],
        reference_shoulder.x*m[2]+reference_shoulder.y*m[6]+reference_shoulder.z*m[10]+m[14]};
    if(!body::d::finite_vector(centre)||std::abs(centre.x)>FLT_MAX||std::abs(centre.y)>FLT_MAX||std::abs(centre.z)>FLT_MAX)return false;
    for(unsigned i=0;i<body::bones;++i)if(descendant(parents,i,shoulder)){
        auto& a=staged[i].values;std::fill(std::begin(a),std::end(a),0.f);
        a[12]=float(centre.x);a[13]=float(centre.y);a[14]=float(centre.z);a[15]=1.f;++hidden;
    }
    return hidden>2;
}
}
