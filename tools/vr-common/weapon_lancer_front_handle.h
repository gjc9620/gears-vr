#pragma once
#include "weapon_snub_slide.h"
#include <array>
#include <limits>

namespace gears_weapon::lancer_front {
// Owner-marked front protrusion, original hash df356ff3... LOD0. These64
// ROOT vertices form exactly54 triangles. This is distinct from the old18
// vertices beside the starter (2915..2932) and from receiver bones5/6.
inline constexpr unsigned first_vertex=4133,vertices=64,triangles=54;
inline constexpr gears_vr::Vector minimum_raw{2.1331593990,57.8399238586,12.7702350616};
inline constexpr gears_vr::Vector maximum_raw{4.3696632385,58.9116554260,13.7571115494};
inline constexpr gears_vr::Vector contact_raw{4.52,58.35,13.25};
inline constexpr double acquisition_radius_metres=.18;
inline constexpr gears_vr::Vector pull_axis_raw{0,-1,0};
// Original longitudinal guide-slot rear stop: ROOT vertex10401. The front
// moving tab's rear edge is57.83992385864258. Stop its rear edge at the actual
// slot end rather than inventing the previous3.5raw-unit short stroke.
// This remains AUTHORED VR travel, not a measured stock-animation translation.
inline constexpr gears_vr::Vector slot_rear_raw{2.7596969604492188,34.79851150512695,12.67965316772461};
inline constexpr double travel_raw=23.041412353515625;
inline constexpr char delta_sha256[]="24062bd3de919aa61f2f7110d628d733d5b08db16eaa283b0e23f44ad4c5f6d7";
inline bool raw_distance(double units,double metres,double& out) noexcept {
    if(!std::isfinite(units)||units<10||units>500||!std::isfinite(metres)||metres<0)return false;
    const double maximum=travel_raw/units;
    const double rounding=8*std::numeric_limits<double>::epsilon();
    if(metres>maximum+rounding)return false;
    // Metres-to-model-unit conversion can round a closed interval endpoint
    // slightly upward. Clamp only that representation error; Native still
    // checks this same model-measured travel and the certified delta payload.
    out=std::clamp(metres*units,0.,travel_raw);return true;
}
inline bool geometry(double units,SnubSlideGeometry& out) noexcept {
    if(!std::isfinite(units)||units<10||units>500)return false;
    out={};out.closed_contact={contact_raw.x/units,contact_raw.y/units,contact_raw.z/units};
    out.axis=pull_axis_raw;out.travel=travel_raw/units;
    out.contact_radius=acquisition_radius_metres;out.lateral_tolerance=.12;out.maximum_speed=4.;
    // Physical13780 contains full held/release routes peaking at13..15cm
    // in the current35deg basis. The prior6cm assist still required17cm,
    // beyond those real gestures. Gradual bounded10cm end assistance now
    // completes the long slot after13.04cm actual pull (over half its travel).
    // A12cm partial cannot finish. Observed upload/release/return remain
    // mandatory; the original raw controller path is never rewritten.
    out.overtravel=.25;out.reverse_tolerance=.08;out.retain_grasp_outside_bounds=true;
    out.sliding_grip_socket=true;
    out.endpoint_assistance=std::min(.10,out.travel*.45);
    out.current_instance_verified=true;return out.travel>=.01&&out.travel<=.40;
}
inline bool weights(double distance_raw,std::array<float,2>& out) noexcept {
    if(!std::isfinite(distance_raw)||distance_raw<0||distance_raw>travel_raw)return false;
    const float half=float(distance_raw/(2*travel_raw));out={half,half};return true;
}
// Reuse the common observed linear-stroke kernel. Only current observed visible
// part displacement may acknowledge travel/return; controller targets, time,
// reload UI and insertion cannot substitute for this feedback.
class Stroke {
    SnubSlideStroke stroke_;
public:
    using Phase=SnubSlideStroke::Phase;using Rejection=SnubSlideStroke::Rejection;
    void reset() noexcept{stroke_.reset();}
    Phase phase() const noexcept{return stroke_.phase();}
    Rejection last_rejection() const noexcept{return stroke_.last_rejection();}
    bool actual_full_pull() const noexcept{return stroke_.actual_full_pull();}
    double wanted_distance() const noexcept{return stroke_.wanted_distance();}
    bool begin(SnubSlideGeometry const& g,HammerburstLatchSample const& s,double actual) noexcept{return stroke_.begin(g,s,actual);}
    bool move(HammerburstLatchSample const& s) noexcept{return stroke_.move(s);}
    bool release(double game_time) noexcept{return stroke_.release(game_time);}
    bool cancel(double game_time) noexcept{return stroke_.cancel(game_time);}
    bool advance_return(double game_time) noexcept{return stroke_.advance_return(game_time);}
    bool acknowledge_visible_part(double distance,std::uint64_t upload_serial,double game_time) noexcept {
        return stroke_.acknowledge_bone(distance,upload_serial,game_time);
    }
};
}
