#pragma once
#include "weapon_lancer_feed_anchor.h"
#include "weapon_pose_solver.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace gears_weapon {

// This bounds consecutive already-authenticated geometry observations, not
// the age of a controller pose or permission to write inventory. A 13 Hz game
// can publish actual hands about 77 ms apart and miss one render publication
// during animation ownership changes. Treating that ~154 ms interval as loss
// made an ordinary short pull restart midway. Current live input, tracking,
// Grip, identity and geometry admission retain their independent deadlines.
// Longer discontinuities still discard travel and require a new approach.
inline constexpr std::uint64_t lancer_gesture_observation_interval_ms=250;
// A real held approach may engage the last6cm of smooth Magwell seating.
// Side/rotation fitting and final seating never provide input/item ownership.
inline constexpr double lancer_insertion_assist_metres=.14;
inline constexpr double lancer_insertion_assist_radians=1.4835298641951802; //85deg
inline constexpr double lancer_magwell_snap_metres=.06;
inline constexpr double lancer_magwell_approach_metres=.02;
inline constexpr std::uint64_t lancer_magwell_snap_ms=180;

// Private, observation-only Lancer trial. Limits are provisional raw UE units;
// actual controller-to-palm calibration and rendered insertion need headset QA.
enum class LancerInsertResult : std::uint8_t { rejected, approaching, inserted };
struct LancerInsertionSample {
    std::uint64_t weapon{},component{},world{},session{},space_epoch{},
                  input_epoch{},world_generation{},sequence{},created_ms{};
    std::array<float,16> gun_mesh{};
    gears_vr::Viewpoint magazine{};
    bool tracked{},grip_down{};
    // Optional bounded insertion assistance. The original hand-derived pose
    // remains the motion witness; guidance cannot invent axial travel.
    double units_per_metre{100.};
    gears_vr::Viewpoint unassisted_magazine{};
    bool assisted{};
    // Internal guide output only. The stroke independently observes the raw
    // approach; this flag cannot substitute for real motion or a new grasp.
    bool snap_active{};
    double snap_progress{};
};
struct LancerInsertionGeometry {
    double depth_raw{},lateral_raw{},axis_alignment{},clock_alignment{};
    gears_vr::Vector magazine_in_gun{};
};
inline bool lancer_extraction_alignment(LancerInsertionGeometry const& g,double units) noexcept {
    return std::isfinite(units)&&units>0&&g.lateral_raw<=.08*units&&
        g.axis_alignment>=.5&&g.clock_alignment>=.35;
}
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
    double raw_approach_max_{};
    bool raw_approach_known_{};
    std::uint64_t snap_started_ms_{};
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
        if(last_sequence_&&(sample.sequence<last_sequence_||sample.created_ms<=last_ms_)){
            reset();return LancerInsertResult::rejected;
        }
        if(last_sequence_&&sample.created_ms-last_ms_>lancer_gesture_observation_interval_ms){
            // A still-tracked, same-identity sample may establish a NEW
            // approach after a publication gap. Discard all previous travel;
            // landing inside the port cannot count as crossing it. Explicit
            // tracking/Grip loss above still disarms this stroke entirely.
            if(!arm(sample))return LancerInsertResult::rejected;
        }
        LancerInsertionGeometry current{};
        if(!lancer_insertion_geometry(sample,current)){
            reset();return LancerInsertResult::rejected;
        }
        auto motion=current;
        if(sample.assisted){
            auto original=sample;original.magazine=sample.unassisted_magazine;
            const auto before=gears_vr::ue_matrix(original.magazine.rotation);
            const auto after=gears_vr::ue_matrix(sample.magazine.rotation);
            double trace{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)trace+=before[i][j]*after[i][j];
            if(!std::isfinite(sample.units_per_metre)||sample.units_per_metre<=0||
               !lancer_insertion_geometry(original,motion)||
               !std::isfinite(sample.snap_progress)||sample.snap_progress<0||sample.snap_progress>1||
               (!sample.snap_active&&std::abs(motion.depth_raw-current.depth_raw)>1e-4)||
               (sample.snap_active&&(!std::isfinite(motion.depth_raw)||
                 motion.depth_raw<-.04*sample.units_per_metre||motion.depth_raw>(lancer_magwell_snap_metres+.000001)*sample.units_per_metre||
                 std::abs(motion.depth_raw-current.depth_raw)>(lancer_magwell_snap_metres+.000001)*sample.units_per_metre))||
               length(difference(original.magazine.position,sample.magazine.position))>
                 (std::hypot(lancer_insertion_assist_metres,sample.snap_active?lancer_magwell_snap_metres:0.)+.000001)*sample.units_per_metre||
               trace<1.+2.*std::cos(lancer_insertion_assist_radians+.000001)){
                reset();return LancerInsertResult::rejected;
            }
        }
        if(geometry)*geometry=current;
        const double previous_depth=last_depth_;
        const double travel=last_sequence_?length(difference(motion.magazine_in_gun,
                                                            last_in_gun_)):0;
        const double dt=last_sequence_?double(sample.created_ms-last_ms_)/1000.:0;
        last_sequence_=sample.sequence;last_ms_=sample.created_ms;
        last_in_gun_=motion.magazine_in_gun;last_depth_=motion.depth_raw;
        // An original held-magazine sample converges to about 1.5 units of
        // depth and 1.8 units of lateral offset. These generous candidates
        // accept a continuous approach, never a single-frame overlap.
        const bool aligned=current.axis_alignment>=.85&&
            current.clock_alignment>=.8&&current.lateral_raw<=4.;
        const bool plausible_speed=!dt||travel<=400.*dt;
        const double raw_contact_cos=.08715574274765817; //cos85deg
        if(plausible_speed&&motion.depth_raw>=-.04*sample.units_per_metre&&
           motion.depth_raw<=.20*sample.units_per_metre&&motion.lateral_raw<=.14*sample.units_per_metre&&
           motion.axis_alignment>=raw_contact_cos&&motion.clock_alignment>=raw_contact_cos){
            const double distance=std::hypot(motion.depth_raw,motion.lateral_raw);
            raw_approach_max_=raw_approach_known_?(std::max)(raw_approach_max_,distance):distance;
            raw_approach_known_=true;
        }else raw_approach_known_=false;
        if(!aligned||!plausible_speed||current.depth_raw<-.02*sample.units_per_metre||
           current.depth_raw>25.){
            approaching_=false;return LancerInsertResult::rejected;
        }
        if(sample.snap_active){
            // A guide flag/elapsed blend is insufficient. THIS observer must
            // also have seen at least2cm of real inward motion of this exact
            // held item. A stationary overlap or a new item starts from zero.
            if(!sample.assisted||!raw_approach_known_||raw_approach_max_-std::hypot(motion.depth_raw,motion.lateral_raw)<
               (lancer_magwell_approach_metres-.000001)*sample.units_per_metre){
                approaching_=false;return LancerInsertResult::rejected;
            }
            if(!snap_started_ms_)snap_started_ms_=sample.created_ms;
            approaching_=true;
            if(sample.snap_progress<1.||sample.created_ms-snap_started_ms_<lancer_magwell_snap_ms||
               std::abs(current.depth_raw)>.001||current.lateral_raw>.001)
                return LancerInsertResult::approaching;
            inserted_=true;approaching_=false;return LancerInsertResult::inserted;
        }
        snap_started_ms_=0;
        if(!approaching_){
            if(current.depth_raw>=4.5){
                approaching_=true;return LancerInsertResult::approaching;
            }
            return LancerInsertResult::rejected;
        }
        if(motion.depth_raw>previous_depth+.5){
            approaching_=false;return LancerInsertResult::rejected;
        }
        if(current.depth_raw<=2.5){
            inserted_=true;approaching_=false;return LancerInsertResult::inserted;
        }
        return LancerInsertResult::approaching;
    }
};

// Physical extraction precedes inventory mutation. Provisional geometry uses
// the same model-space port as insertion; no timer can complete this stroke.
class LancerMagazineExtractionStroke {
    LancerInsertionSample previous_{};
    LancerInsertionGeometry geometry_{};
    double completion_depth_{4.5};
    bool active_{},rebase_pending_{};
public:
    struct Failure {unsigned reason{};std::uint64_t previous_sequence{},sequence{},previous_ms{},ms{};
        double depth{},lateral{},axis{},clock{},travel{};};
private:
    Failure failure_{};
    bool reject(unsigned reason) noexcept {active_=false;failure_.reason=reason;return false;}
public:
    Failure const& failure() const noexcept {return failure_;}
    void reset() noexcept {*this={};}
    bool active() const noexcept {return active_;}
    bool begin(LancerInsertionSample const& s,bool grip_pressed) noexcept {
        reset();LancerInsertionGeometry g{};
        if(!grip_pressed||!s.tracked||!s.grip_down||!s.weapon||!s.component||!s.world||
           !s.session||!s.space_epoch||!s.input_epoch||!s.world_generation||!s.sequence||!s.created_ms||
           !lancer_insertion_geometry(s,g)||g.depth_raw<0||g.depth_raw>2.5||
           !lancer_extraction_alignment(g,s.units_per_metre))return false;
        previous_=s;geometry_=g;active_=true;return true;
    }
    bool observe(LancerInsertionSample const& s) noexcept {
        if(!active_)return false;
        const auto& p=previous_;
        failure_={0,p.sequence,s.sequence,p.created_ms,s.created_ms};
        if(s.weapon!=p.weapon||s.component!=p.component||s.world!=p.world||s.session!=p.session||
           s.space_epoch!=p.space_epoch||s.input_epoch!=p.input_epoch||s.world_generation!=p.world_generation||
           !s.tracked||!s.grip_down)return reject(1);
        if(s.sequence==p.sequence)return false;
        LancerInsertionGeometry g{};
        if(s.sequence<p.sequence)return reject(2);
        if(s.created_ms<=p.created_ms)return reject(3);
        if(s.created_ms-p.created_ms>lancer_gesture_observation_interval_ms){
            // A delayed consumer must not integrate travel across the gap.
            // At the seated port, restart the original stroke. Further out,
            // keep the SAME still-held identity but require a whole NEW 4.5
            // unit outward stroke from this fresh sample. Merely reappearing
            // outside the port never completes extraction. Tracking/Grip or
            // identity loss above still disarms; no inventory action occurs.
            if(!lancer_insertion_geometry(s,g))return reject(5);
            previous_=s;geometry_=g;
            failure_.depth=g.depth_raw;failure_.lateral=g.lateral_raw;
            failure_.axis=g.axis_alignment;failure_.clock=g.clock_alignment;
            rebase_pending_=!lancer_extraction_alignment(g,s.units_per_metre)||g.depth_raw>20.5;
            if(!rebase_pending_)completion_depth_=g.depth_raw<=2.5?4.5:g.depth_raw+4.5;
            // No lost interval is motion evidence. A fresh inward position
            // remains held; an unsuitable recovery pose waits for a new valid
            // baseline, then requires the complete new outward stroke.
            return false;
        }
        if(!lancer_insertion_geometry(s,g))return reject(5);
        failure_.depth=g.depth_raw;failure_.lateral=g.lateral_raw;
        failure_.axis=g.axis_alignment;failure_.clock=g.clock_alignment;
        failure_.travel=length(difference(g.magazine_in_gun,geometry_.magazine_in_gun));
        if(failure_.travel>.4*double(s.created_ms-p.created_ms))return reject(9);
        previous_=s;geometry_=g;
        if(rebase_pending_){
            if(lancer_extraction_alignment(g,s.units_per_metre)&&g.depth_raw<=20.5){
                completion_depth_=g.depth_raw<=2.5?4.5:g.depth_raw+4.5;rebase_pending_=false;
            }
            return false;
        }
        // Acquisition/alignment limits cannot end a genuine still-held grasp.
        // Owner13780 made11 ordinary inward adjustments (up to3.92units)
        // while the index trigger and tracking remained valid. The former
        // per-observation .5unit reversal cutoff discarded those grasps.
        // Keep observing the SAME authenticated hold outside the mechanical
        // envelope, but grant no extraction until it actually returns to a
        // valid outward pose. Identity, tracking, release, ordering, gaps and
        // excessive speed above retain their independent protections.
        if(!lancer_extraction_alignment(g,s.units_per_metre)||g.depth_raw>25)return false;
        if(g.depth_raw>=completion_depth_){reset();return true;}
        return false;
    }
};

// A seated magazine cannot be pushed through the receiver. This bounded
// display-only stop leaves the original sample unchanged for the stroke.
inline bool lancer_extraction_display_target(LancerInsertionSample const& s,
    gears_vr::Viewpoint& target) noexcept {
    LancerInsertionGeometry g{};LancerFeedWorld feed{};
    if(!lancer_insertion_geometry(s,g)||!lancer_feed_world(s.gun_mesh,feed)||
       !std::isfinite(s.units_per_metre)||s.units_per_metre<10||s.units_per_metre>500)return false;
    target=s.magazine;
    if(g.depth_raw<0){
        const double stop=(std::min)(-g.depth_raw,.06*s.units_per_metre);
        target.position={target.position.x+feed.outward_axis.x*stop,
            target.position.y+feed.outward_axis.y*stop,target.position.z+feed.outward_axis.z*stop};
    }
    return true;
}
} // namespace gears_weapon
