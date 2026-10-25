#pragma once
#include "anchor_observer.h"
#include "first_person_view.h"
#include <cstdint>

// Pure render-base proposal. The caller certifies all typed fields, current
// registry slots/links and its one-use Scene receipt; this header reads no
// process memory and assumes no offsets. Serialize one Kernel on the Scene GT.
// Apply BEFORE stereo/HMD composition. There is deliberately no HMD, weapon,
// native camera orientation, animated bone or game-write interface here.
namespace gears_run_camera {
struct Key {
    gears_anchor::Identity world{},pawn{},mesh{},controller{};
    std::uint64_t session{},world_generation{},reference_generation{},space_epoch{};
};
inline bool same(Key const& a,Key const& b) noexcept {
    return a.session==b.session&&a.world_generation==b.world_generation&&
        a.reference_generation==b.reference_generation&&a.space_epoch==b.space_epoch&&
        gears_anchor::equal_identity(a.world,b.world)&&gears_anchor::equal_identity(a.pawn,b.pawn)&&
        gears_anchor::equal_identity(a.mesh,b.mesh)&&gears_anchor::equal_identity(a.controller,b.controller);
}
struct Sample {
    Key key{};std::uint64_t frame{},sample_ns{};
    gears_vr::Vector pawn_location{};
    double eye_height{},controller_yaw_radians{},controller_pitch_radians{},units_per_metre{};
    unsigned special_move{};
    // Caller certificates, never inferred from an SM number alone. Standing
    // proof must exclude crouch, cover, nonstanding posture and transitions.
    bool context_verified{},eligible{},standing_sm0_verified{};
};
enum class Status {inactive,calibrating,applied,rejected};
enum class Reason {none,context,identity,time,duplicate,calibration,height,orientation,numerical};
struct Result {
    gears_vr::StereoMatrix view{};gears_vr::Vector origin{};
    double protected_height{};std::uint64_t frame{},sample_ns{};
};
class Kernel {
    static constexpr std::uint64_t ms=1000000;
    Key key_{};bool bound_{},calibrated_{},sprinting_{};
    std::uint64_t frame_{},stamp_{},calibration_start_{},calibration_last_{};
    unsigned calibration_samples_{};double first_height_{},height_{},units_{};
    Reason reason_{Reason::none};
    static bool identity(gears_anchor::Identity const& id) noexcept {
        return id.address&&id.cls&&id.name[0]&&id.name.back()==0;
    }
    static bool valid_key(Key const& k) noexcept {
        return k.session&&k.world_generation&&k.reference_generation&&k.space_epoch&&
            identity(k.world)&&identity(k.pawn)&&identity(k.mesh)&&identity(k.controller);
    }
    Status reject(Reason r,bool discard=true) noexcept {
        if(discard){reset();}reason_=r;return Status::rejected;
    }
public:
    // Max source age 50 ms; cadence gap 250 ms invalidates the calibration.
    // Calibrate >=200 ms / >=3 fresh standing-SM0 samples, within 2 seconds,
    // with <=0.01 m total drift. Entry calibration is valid for 2 seconds;
    // sustained fresh same-context SM4 remains stable until exit/context loss.
    void reset() noexcept {*this=Kernel{};}
    Reason reason() const noexcept {return reason_;}
    bool calibrated() const noexcept {return calibrated_;}
    double protected_height() const noexcept {return height_;}
    Status update(Sample const& s,std::uint64_t now_ns,Result& out) noexcept {
        reason_=Reason::none;
        if(!s.context_verified||!s.eligible)return reject(Reason::context);
        if(!valid_key(s.key))return reject(Reason::identity);
        if(!s.frame||!s.sample_ns||now_ns<s.sample_ns||now_ns-s.sample_ns>50*ms)return reject(Reason::time);
        if(!gears_vr::finite(s.pawn_location)||!std::isfinite(s.units_per_metre)||
           s.units_per_metre<10||s.units_per_metre>500)return reject(Reason::numerical);
        if(bound_&&!same(key_,s.key))reset();
        if(bound_&&units_!=s.units_per_metre)return reject(Reason::context);
        if(!bound_){key_=s.key;units_=s.units_per_metre;bound_=true;}
        if(s.frame<=frame_||s.sample_ns<=stamp_)return reject(Reason::duplicate,false);
        if(stamp_&&s.sample_ns-stamp_>250*ms)return reject(Reason::time);
        frame_=s.frame;stamp_=s.sample_ns;
        if(s.special_move!=4){
            sprinting_=false;
            if(s.special_move!=0||!s.standing_sm0_verified){calibrated_=false;calibration_samples_=0;return Status::inactive;}
            if(!std::isfinite(s.eye_height)||s.eye_height<.2*s.units_per_metre||s.eye_height>2.5*s.units_per_metre)
                return reject(Reason::height);
            if(!calibration_samples_||(!calibrated_&&s.sample_ns-calibration_start_>2000*ms)||
               std::abs(s.eye_height-first_height_)>.01*s.units_per_metre){
                calibration_start_=s.sample_ns;first_height_=s.eye_height;calibration_samples_=0;calibrated_=false;
            }
            ++calibration_samples_;calibration_last_=s.sample_ns;
            if(calibration_samples_>=3&&s.sample_ns-calibration_start_>=200*ms){height_=first_height_;calibrated_=true;}
            return Status::calibrating; // Never override any SM0/other stance.
        }
        if(!calibrated_||!calibration_last_||(!sprinting_&&s.sample_ns-calibration_last_>2000*ms))
            return reject(Reason::calibration);
        sprinting_=true;
        if(!std::isfinite(s.controller_yaw_radians)||!std::isfinite(s.controller_pitch_radians)||
           std::abs(s.controller_pitch_radians)>1.57079632679489661923)return reject(Reason::orientation);
        const double yaw=std::remainder(s.controller_yaw_radians,6.2831853071795864769);
        const double cp=std::cos(s.controller_pitch_radians),sp=std::sin(s.controller_pitch_radians);
        const double cy=std::cos(yaw),sy=std::sin(yaw);
        // UE/world forward +X, horizontal right +Y, world up +Z. No camera roll.
        gears_vr::StereoMatrix basis{float(-sy),float(-sp*cy),float(cp*cy),0,
            float(cy),float(-sp*sy),float(cp*sy),0,0,float(cp),float(sp),0,0,0,0,1};
        const gears_vr::Vector origin{s.pawn_location.x+.35*s.units_per_metre*cy,
            s.pawn_location.y+.35*s.units_per_metre*sy,s.pawn_location.z+height_+.10*s.units_per_metre};
        gears_vr::StereoMatrix candidate{};
        if(!gears_vr::view_with_origin(basis,origin,candidate))return reject(Reason::numerical);
        out={candidate,origin,height_,s.frame,s.sample_ns};return Status::applied;
    }
};
// rejected never mutates Result: decline this optional override and never reuse
// an old-context proposal. It does not request stopping stereo/the session.
// Normal rendering, fallback and smooth handoff remain caller responsibilities.
// Exit SM4 returns inactive/calibrating and leaves stance policy to caller.
}
