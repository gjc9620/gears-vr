#pragma once
#include "weapon_lancer_feed_anchor.h"
#include "weapon_pose_solver.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace gears_weapon {

// Private, observation-only Lancer trial. Limits are provisional raw UE units;
// actual controller-to-palm calibration and rendered insertion need headset QA.
enum class LancerInsertResult : std::uint8_t { rejected, approaching, inserted };
struct LancerInsertionSample {
    std::uint64_t weapon{},component{},world{},session{},space_epoch{},
                  input_epoch{},world_generation{},sequence{},created_ms{};
    std::array<float,16> gun_mesh{};
    gears_vr::Viewpoint magazine{};
    bool tracked{},grip_down{};
};
struct LancerInsertionGeometry {
    double depth_raw{},lateral_raw{},axis_alignment{},clock_alignment{};
    gears_vr::Vector magazine_in_gun{};
};
inline bool lancer_insertion_geometry(LancerInsertionSample const& sample,
                                      LancerInsertionGeometry& out) noexcept {
    LancerFeedWorld feed{};
    if(!lancer_feed_world(sample.gun_mesh,feed)||
       !gears_vr::finite(sample.magazine.position)||
       !std::isfinite(sample.magazine.rotation.pitch)||
       !std::isfinite(sample.magazine.rotation.yaw)||
       !std::isfinite(sample.magazine.rotation.roll))return false;
    const auto& gun=sample.gun_mesh;
    gears_vr::Vector in_gun{};
    gears_vr::Vector gun_axes[3]{};
    const gears_vr::Vector from_gun{
        sample.magazine.position.x-gun[12],
        sample.magazine.position.y-gun[13],
        sample.magazine.position.z-gun[14]};
    double local[3]{};
    for(unsigned axis=0;axis<3;++axis){
        const auto row=axis*4;
        const double scale=std::hypot(std::hypot(double(gun[row]),double(gun[row+1])),
                                      double(gun[row+2]));
        gun_axes[axis]={gun[row]/scale,gun[row+1]/scale,gun[row+2]/scale};
        local[axis]=(from_gun.x*gun[row]+from_gun.y*gun[row+1]+
                     from_gun.z*gun[row+2])/scale;
    }
    in_gun={local[0],local[1],local[2]};
    const auto offset=difference(sample.magazine.position,feed.point_raw);
    const double depth=dot(offset,feed.outward_axis);
    const double lateral=length(difference(offset,scaled(feed.outward_axis,depth)));
    const auto magazine_axes=gears_vr::ue_matrix(sample.magazine.rotation);
    const gears_vr::Vector magazine_up{
        magazine_axes[0][2],magazine_axes[1][2],magazine_axes[2][2]};
    const gears_vr::Vector magazine_forward{
        magazine_axes[0][0],magazine_axes[1][0],magazine_axes[2][0]};
    const gears_vr::Vector magazine_right{
        magazine_axes[0][1],magazine_axes[1][1],magazine_axes[2][1]};
    const double alignment=dot(magazine_up,feed.outward_axis);
    // Original seated Lancer magazine: local X follows gun +Y, local Y
    // follows gun +X, local Z points out of the underside of the receiver.
    const double clock_alignment=(std::min)(dot(magazine_forward,gun_axes[1]),
                                            dot(magazine_right,gun_axes[0]));
    if(!gears_vr::finite(in_gun)||!std::isfinite(depth)||!std::isfinite(lateral)||
       !std::isfinite(alignment)||!std::isfinite(clock_alignment))return false;
    out={depth,lateral,alignment,clock_alignment,in_gun};return true;
}

class LancerMagazineInsertionStroke {
    std::uint64_t weapon_{},component_{},world_{},session_{},space_epoch_{},
                  input_epoch_{},world_generation_{},last_sequence_{},last_ms_{};
    gears_vr::Vector last_in_gun_{};
    double last_depth_{};
    bool approaching_{},inserted_{};
public:
    void reset() noexcept {*this={};}
    bool arm(LancerInsertionSample const& sample) noexcept {
        reset();
        if(!sample.weapon||!sample.component||!sample.world||!sample.session||
           !sample.space_epoch||!sample.input_epoch||!sample.world_generation)return false;
        weapon_=sample.weapon;component_=sample.component;world_=sample.world;
        session_=sample.session;space_epoch_=sample.space_epoch;
        input_epoch_=sample.input_epoch;world_generation_=sample.world_generation;
        return true;
    }
    bool inserted() const noexcept {return inserted_;}
    LancerInsertResult observe(LancerInsertionSample const& sample,
                               LancerInsertionGeometry* geometry=nullptr) noexcept {
        if(!weapon_||sample.weapon!=weapon_||sample.component!=component_||
           sample.world!=world_||sample.session!=session_||
           sample.space_epoch!=space_epoch_||sample.input_epoch!=input_epoch_||
           sample.world_generation!=world_generation_||
           !sample.tracked||!sample.grip_down||!sample.sequence||!sample.created_ms){
            reset();return LancerInsertResult::rejected;
        }
        if(inserted_||sample.sequence==last_sequence_)return LancerInsertResult::rejected;
        if(last_sequence_&&(sample.sequence<last_sequence_||sample.created_ms<=last_ms_||
                           sample.created_ms-last_ms_>100)){
            reset();return LancerInsertResult::rejected;
        }
        LancerInsertionGeometry current{};
        if(!lancer_insertion_geometry(sample,current)){
            reset();return LancerInsertResult::rejected;
        }
        if(geometry)*geometry=current;
        const double previous_depth=last_depth_;
        const double travel=last_sequence_?length(difference(current.magazine_in_gun,
                                                            last_in_gun_)):0;
        const double dt=last_sequence_?double(sample.created_ms-last_ms_)/1000.:0;
        last_sequence_=sample.sequence;last_ms_=sample.created_ms;
        last_in_gun_=current.magazine_in_gun;last_depth_=current.depth_raw;
        // An original held-magazine sample converges to about 1.5 units of
        // depth and 1.8 units of lateral offset. These generous candidates
        // accept a continuous approach, never a single-frame overlap.
        const bool aligned=current.axis_alignment>=.85&&
            current.clock_alignment>=.8&&current.lateral_raw<=4.;
        const bool plausible_speed=!dt||travel<=400.*dt;
        if(!aligned||!plausible_speed||current.depth_raw<0||
           current.depth_raw>25.){
            approaching_=false;return LancerInsertResult::rejected;
        }
        if(!approaching_){
            if(current.depth_raw>=4.5){
                approaching_=true;return LancerInsertResult::approaching;
            }
            return LancerInsertResult::rejected;
        }
        if(current.depth_raw>previous_depth+.5){
            approaching_=false;return LancerInsertResult::rejected;
        }
        if(current.depth_raw<=2.5){
            inserted_=true;approaching_=false;return LancerInsertResult::inserted;
        }
        return LancerInsertResult::approaching;
    }
};

} // namespace gears_weapon
