#pragma once
#include "weapon_torque_bow_feedback.h"
#include "weapon_torque_bow_interaction.h"
#include "weapon_torque_bow_arrow_mount.h"
#include <algorithm>
// Cosmetic data only. No input, clock, Engine call, lease, mechanical volume,
// inventory setter or reconstructed Charge timer is available here.
namespace gears_torque_bow_guide {
namespace vr=gears_vr;namespace hand=gears_weapon_hand;namespace mount=gears_torque_bow_arrow_mount;
enum class Step:unsigned {none,shoulder,feed,locked,charging,cancelled,fired,no_ammo};
struct Receipts {std::uint64_t loaded{},cancelled{},projectile{};};
struct Sample {
    hand::Frame source{};std::uint64_t hold{},config{};std::int64_t now{};
    gears_weapon::TorqueBowStage stage{};Receipts receipts{};
    unsigned loaded{},reserve{};bool held_arrow{},charging{},progress_known{};
    double charge_count{},charge_max{};
};
struct View {Step step{};double progress{};};
class Selector {
    hand::Frame source_{};std::uint64_t hold_{},config_{};Receipts receipts_{};
    std::int64_t time_{},until_{};Step event_{};bool known_{};
    bool cohort(Sample const& s) const noexcept {
        auto a=source_.key,b=s.source.key;a.frame=b.frame=0;
        return hold_==s.hold&&config_==s.config&&hand::same(a,b)&&
            hand::same(source_.world,s.source.world)&&hand::same(source_.controller,s.source.controller)&&
            hand::same(source_.pawn,s.source.pawn)&&hand::same(source_.mesh,s.source.mesh)&&hand::same(source_.weapon,s.source.weapon)&&
            source_.reference_generation==s.source.reference_generation&&vr::stereo_same_pose(source_.reference,s.source.reference)&&
            source_.controllers.source==s.source.controllers.source&&source_.controllers.session==s.source.controllers.session&&
            source_.controllers.space_epoch==s.source.controllers.space_epoch&&source_.controllers.input_epoch==s.source.controllers.input_epoch&&
            source_.controllers.profile_generation==s.source.controllers.profile_generation;
    }
public:
    void lost() noexcept {known_=false;event_=Step::none;until_=0;}
    View advance(Sample const& s) noexcept {
        View out{};
        if(!s.hold||!s.config||s.loaded>1||s.reserve>1000000||!hand::frame_active(s.source,s.now)||
           s.now<s.source.controllers.sync_ns||s.now-s.source.controllers.sync_ns>100000000){lost();return out;}
        const bool same=cohort(s);
        if(same&&(s.source.key.frame<source_.key.frame||s.source.controllers.sequence<source_.controllers.sequence||
           s.source.controllers.sync_ns<source_.controllers.sync_ns||s.now<time_)){lost();return out;}
        const bool continuing=same&&known_&&s.now-time_<=100000000;
        if(!continuing){event_=Step::none;until_=0;}
        else if(s.receipts.loaded<receipts_.loaded||s.receipts.cancelled<receipts_.cancelled||s.receipts.projectile<receipts_.projectile){lost();return out;}
        else {
            // Original projectile has priority over any simultaneous cleanup.
            Step event=Step::none;
            if(s.receipts.projectile>receipts_.projectile)event=Step::fired;
            else if(s.receipts.cancelled>receipts_.cancelled)event=Step::cancelled;
            else if(s.receipts.loaded>receipts_.loaded&&s.loaded==1)event=Step::locked;
            if(event!=Step::none){event_=event;until_=s.now>INT64_MAX-700000000?INT64_MAX:s.now+700000000;}
        }
        source_=s.source;hold_=s.hold;config_=s.config;receipts_=s.receipts;time_=s.now;known_=true;
        using Stage=gears_weapon::TorqueBowStage;
        if(s.stage==Stage::interrupted||s.stage==Stage::unbound){lost();return out;}
        if(s.stage==Stage::charge_abort)return out;
        if(event_==Step::fired&&s.now<until_)return {event_,0};
        if(s.charging){
            if(!s.progress_known||!std::isfinite(s.charge_count)||!std::isfinite(s.charge_max)||s.charge_count<0||s.charge_max<=0)return out;
            return {Step::charging,std::clamp(s.charge_count/s.charge_max,0.,1.)};
        }
        if(s.stage==Stage::reload_steps)return {s.held_arrow?Step::feed:Step::shoulder,0};
        if(s.stage==Stage::reload_commit||s.stage==Stage::charge_begin||
           s.stage==Stage::charge_end||s.stage==Stage::await_original)return out;
        if(event_!=Step::none&&s.now<until_)return {event_,0};
        if(s.loaded==0)return {s.reserve?Step::shoulder:Step::no_ammo,0};
        return out;
    }
};
struct Line {vr::Vector start{},end{};};
struct Shape {std::array<Line,40> lines{};unsigned count{};};
inline bool line(Shape& s,vr::Vector a,vr::Vector b) noexcept {
    if(s.count==s.lines.size()||!vr::finite(a)||!vr::finite(b))return false;
    s.lines[s.count++]={a,b};return true;
}
inline vr::Vector point(vr::Pose const& p,vr::Vector v) noexcept {return mount::add(p.position,mount::rotate(p.orientation,v));}
inline bool ring(Shape& s,vr::Pose const& p,double radius,unsigned segments=12) noexcept {
    if(!mount::pose_valid(p)||!std::isfinite(radius)||radius<=0||segments<3||segments>16)return false;
    for(unsigned i=0;i<segments;++i){const double a=6.2831853071795864769*i/segments,b=6.2831853071795864769*(i+1)/segments;
        if(!line(s,point(p,{0,radius*std::cos(a),radius*std::sin(a)}),point(p,{0,radius*std::cos(b),radius*std::sin(b)})))return false;
    }return true;
}
// root is original b_TB_Root in world UE units. scale is the REAL original
// component scale, units the authenticated RawRig units/metre. The feed ring
// is the original Muzzle mouth, not the final arrow HEAD or a larger hit box.
inline bool make(View view,vr::Pose const& root,double scale,double units,vr::Pose const& shoulder,Shape& out) noexcept {
    out={};if(view.step==Step::none||!mount::pose_valid(root)||!std::isfinite(scale)||scale<=.1||scale>=10||
        !std::isfinite(units)||units<10||units>500)return false;
    Shape s{};vr::Pose port{point(root,mount::mul({58.00706100463867,.10888981819152832,8.750000953674316},scale)),root.orientation};
    const double u=units;
    const auto local=[&](vr::Vector v){return point(port,mount::mul(v,u));};
    if(view.step==Step::shoulder||view.step==Step::no_ammo){
        if(!ring(s,shoulder,.055*u))return false;
        const auto at=[&](vr::Vector v){return point(shoulder,mount::mul(v,u));};
        if(view.step==Step::no_ammo){
            if(!line(s,at({0,-.025,-.025}),at({0,.025,.025}))||!line(s,at({0,-.025,.025}),at({0,.025,-.025})))return false;
        }else if(!line(s,at({0,-.025,0}),at({0,.025,0}))||!line(s,at({0,0,-.025}),at({0,0,.025})))return false;
    }else if(view.step==Step::feed){
        if(!ring(s,port,1.2*scale)||!line(s,local({.075,.025,0}),local({.015,.025,0}))||
           !line(s,local({.015,.025,0}),local({.03,.015,0}))||!line(s,local({.015,.025,0}),local({.03,.035,0})))return false;
    }else {
        // Keep status marks above the mouth, away from the moving bow string.
        port.position=local({0,0,.055});
        const auto at=[&](double y,double z){return point(port,{0,y*u,z*u});};
        if(view.step==Step::charging){
            if(!std::isfinite(view.progress)||view.progress<0||view.progress>1)return false;
            constexpr unsigned n=16;const double radius=.025*u;
            if(!ring(s,port,radius,n))return false;
            const unsigned count=unsigned(std::floor(view.progress*n));
            for(unsigned i=0;i<count;++i){const double a=6.2831853071795864769*(i+.5)/n;
                if(!line(s,point(port,{0,.35*radius*std::cos(a),.35*radius*std::sin(a)}),
                     point(port,{0,.8*radius*std::cos(a),.8*radius*std::sin(a)})))return false;
            }
        }else if(view.step==Step::cancelled){
            if(!line(s,at(-.02,-.02),at(.02,.02))||!line(s,at(-.02,.02),at(.02,-.02)))return false;
        }else if(view.step==Step::locked){
            if(!line(s,at(-.025,0),at(-.005,-.018))||!line(s,at(-.005,-.018),at(.025,.025)))return false;
        }else if(view.step==Step::fired){
            // Status glyph shares the readable Y/Z plane with check/cross;
            // a barrel-axis line would collapse when aiming straight ahead.
            if(!line(s,at(-.03,0),at(.03,0))||!line(s,at(.03,0),at(.01,.018))||
               !line(s,at(.03,0),at(.01,-.018)))return false;
        }else return false;
    }
    out=s;return s.count!=0;
}
}
