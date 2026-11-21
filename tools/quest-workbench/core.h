#pragma once
#include "game_rules/pose_math.h"
#include "third_party/json.hpp"
#include <array>
#include <vector>
#include <deque>
#include <fstream>
#include <string>
#include <cstdint>
#include <limits>

// No Android, OpenXR handles, game pointers or action grants in project data.
// Both the Quest application and desktop conversion/checks use this core.
namespace workbench {
namespace v=gears_vr;
using Json=nlohmann::json;
using V=v::Vector;using Q=v::Quaternion;using P=v::Pose;
inline V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V mul(V a,double s){return {a.x*s,a.y*s,a.z*s};}
inline double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(V a){return std::sqrt(dot(a,a));}
inline V unit(V a){const auto n=length(a);return n>1e-9?mul(a,1/n):V{};}
inline V rotate(Q q,V a){return v::transform(v::rotation_matrix(q),a);}
inline P compose(P a,P b){return {add(a.position,rotate(a.orientation,b.position)),v::normalized(v::multiply(a.orientation,b.orientation))};}
inline P inverse(P a){auto q=v::inverse(a.orientation);return {rotate(q,mul(a.position,-1)),q};}
inline P relative(P parent,P child){return compose(inverse(parent),child);}
inline Q axis(V a,double radians){a=unit(a);const auto s=std::sin(radians*.5);return {a.x*s,a.y*s,a.z*s,std::cos(radians*.5)};}
inline Q arc(V a,V b){a=unit(a);b=unit(b);double d=std::clamp(dot(a,b),-1.,1.);if(d<-.999999){auto n=unit(cross(a,std::abs(a.x)<.8?V{1,0,0}:V{0,1,0}));return {n.x,n.y,n.z,0};}auto c=cross(a,b);return v::normalized({c.x,c.y,c.z,1+d});}
inline Q matrix_quaternion(v::Matrix const& m){
    Q q{};double t=m[0][0]+m[1][1]+m[2][2];
    if(t>0){double s=std::sqrt(t+1)*2;q={ (m[2][1]-m[1][2])/s,(m[0][2]-m[2][0])/s,(m[1][0]-m[0][1])/s,s*.25};}
    else {unsigned i=m[1][1]>m[0][0]?1:0;if(m[2][2]>m[i][i])i=2;unsigned j=(i+1)%3,k=(i+2)%3;double a[4]{};double s=std::sqrt(1+m[i][i]-m[j][j]-m[k][k])*2;a[i]=s*.25;a[j]=(m[j][i]+m[i][j])/s;a[k]=(m[k][i]+m[i][k])/s;a[3]=(m[k][j]-m[j][k])/s;q={a[0],a[1],a[2],a[3]};}return v::normalized(q);
}
inline bool valid(P p){double n=p.orientation.x*p.orientation.x+p.orientation.y*p.orientation.y+p.orientation.z*p.orientation.z+p.orientation.w*p.orientation.w;return v::finite(p.position)&&length(p.position)<100&&std::isfinite(n)&&std::abs(n-1)<.01;}
inline Json json(V a){return Json::array({a.x,a.y,a.z});}
inline Json json(Q a){return Json::array({a.x,a.y,a.z,a.w});}
inline Json json(P a){return {{"position_metres",json(a.position)},{"rotation_xyzw",json(a.orientation)}};}
inline V vector(Json const& a){if(!a.is_array()||a.size()!=3)throw std::runtime_error("Expected vector3");V r{a[0].get<double>(),a[1].get<double>(),a[2].get<double>()};if(!v::finite(r))throw std::runtime_error("Nonfinite vector");return r;}
inline Q quaternion(Json const& a){if(!a.is_array()||a.size()!=4)throw std::runtime_error("Expected quaternion");Q q{a[0].get<double>(),a[1].get<double>(),a[2].get<double>(),a[3].get<double>()};if(!valid({{},q}))throw std::runtime_error("Invalid quaternion");return q;}
inline P pose(Json const& a){P p{vector(a.at("position_metres")),quaternion(a.at("rotation_xyzw"))};if(!valid(p))throw std::runtime_error("Invalid pose");return p;}
struct Joint {P pose{};double radius{};std::uint64_t flags{};};
struct Hand {std::array<Joint,26> joints{};bool active{};bool aim_valid{},system_gesture{};unsigned data_source{};P aim{};double pinch{};};
// flags deliberately match OpenXR VALID=1|2 and TRACKED=4|8. Valid data can be inferred.
inline bool valid(Hand const& h){if(!h.active)return false;for(auto const& j:h.joints)if((j.flags&3)!=3||!valid(j.pose)||!std::isfinite(j.radius)||j.radius<0||j.radius>.1)return false;return true;}
inline bool tracked(Hand const& h){if(!valid(h))return false;for(auto const& j:h.joints)if((j.flags&12)!=12)return false;return true;}
inline double pinch(Hand const& h){if(!valid(h))return 0;return 1-std::clamp((length(sub(h.joints[5].pose.position,h.joints[10].pose.position))-.014)/.032,0.,1.);}
struct Frame {std::int64_t time_ns{};std::uint64_t epoch{1};P head{},object{};std::array<Hand,2> hands{};double starter_angle{},charging_distance{},starter_pull{};P magazine_delta{};};
struct Volume {std::string shape{"sphere"};P local{};V half{.06,.06,.06};double radius{.08},half_length{.06},snap_distance{.1},snap_angle{1.48},release_distance{.18};};
inline bool contains(Volume const& a,P object,V point){auto p=compose(inverse(compose(object,a.local)),{point,{}}).position;
    if(a.shape=="box")return std::abs(p.x)<=a.half.x&&std::abs(p.y)<=a.half.y&&std::abs(p.z)<=a.half.z;
    if(a.shape=="capsule")p.y-=std::clamp(p.y,-a.half_length,a.half_length);
    return dot(p,p)<=a.radius*a.radius;}
inline bool snap(Volume const& a,P object,V palm,P wrist,P target,bool held){
    if(!valid(wrist)||!valid(target))return false;
    const double distance=length(sub(wrist.position,target.position));auto p=wrist.orientation,q=target.orientation;
    const double angle=2*std::acos(std::clamp(std::abs(p.x*q.x+p.y*q.y+p.z*q.z+p.w*q.w),0.,1.));
    if(held)return distance<=std::max(a.release_distance,a.snap_distance)&&angle<=std::min(3.141592653589793,a.snap_angle+.35);
    return contains(a,object,palm)&&distance<=a.snap_distance&&angle<=a.snap_angle;
}
inline Json json(Volume const& a){return {{"shape",a.shape},{"local",json(a.local)},{"half_extents_metres",json(a.half)},{"radius_metres",a.radius},{"half_length_metres",a.half_length},{"snap_distance_metres",a.snap_distance},{"snap_angle_radians",a.snap_angle},{"release_distance_metres",a.release_distance}};}
struct GrabPose {unsigned side{},data_source{};P wrist_in_object{};std::array<Q,15> local{};std::array<Joint,26> raw_in_object{};bool mapped{},fully_tracked{},authored_default{},part_delta_known{};P part_delta{};std::int64_t source_time{};double starter_angle{},charging_distance{};};
struct Part {std::string id{"main_grip"};Volume volume{};std::array<GrabPose,2> poses{};std::array<bool,2> known{};std::array<V,2> wrist_offset{},wrist_degrees{};std::array<std::array<V,15>,2> finger_degrees{};};
inline constexpr unsigned source_joint[15]={7,8,9,12,13,14,17,18,19,22,23,24,2,3,4};
struct Rig {std::array<P,16> reference{};std::array<int,16> parent{};};
inline std::array<P,16> fk(Rig const& r,std::array<Q,15> const& local){std::array<P,16> out{};out[0]={};for(unsigned i=1;i<16;++i){auto p=r.reference[i];p.orientation=local[i-1];out[i]=compose(out[r.parent[i]],p);}return out;}
struct Calibration {bool known{};Q wrist_basis{};std::array<Q,15> xr_local{},axes{};};
inline Q hand_basis(V forward,V across){forward=unit(forward);across=unit(sub(across,mul(forward,dot(across,forward))));auto normal=unit(cross(forward,across));v::Matrix m{{{forward.x,across.x,normal.x},{forward.y,across.y,normal.y},{forward.z,across.z,normal.z}}};return matrix_quaternion(m);}
inline bool calibrate(Rig const& r,Hand const& h,Calibration& out){
    if(!valid(h))return false;Calibration c{};std::array<Q,15> refs{};for(unsigned i=0;i<15;++i)refs[i]=r.reference[i+1].orientation;const auto bind=fk(r,refs);
    auto const wrist=h.joints[1].pose;
    auto hf=rotate(v::inverse(wrist.orientation),sub(h.joints[12].pose.position,wrist.position));
    auto ha=rotate(v::inverse(wrist.orientation),sub(h.joints[22].pose.position,h.joints[7].pose.position));
    auto gf=bind[4].position;auto ga=sub(bind[10].position,bind[1].position);
    if(length(hf)<.02||length(ha)<.02||length(cross(hf,ha))<.0003)return false;
    c.wrist_basis=v::normalized(v::multiply(hand_basis(hf,ha),v::inverse(hand_basis(gf,ga))));
    for(unsigned i=0;i<15;++i){const auto j=source_joint[i];auto parent=i%3?h.joints[source_joint[i-1]].pose:wrist;
        c.xr_local[i]=relative(parent,h.joints[j].pose).orientation;
        const auto xr_in_wrist=relative(wrist,h.joints[j].pose).orientation;
        c.axes[i]=v::normalized(v::multiply(v::inverse(bind[i+1].orientation),v::multiply(v::inverse(c.wrist_basis),xr_in_wrist)));
    }c.known=true;out=c;return true;
}
inline bool retarget(Rig const& r,Calibration const& c,Hand const& h,std::array<Q,15>& local,P& wrist){
    if(!c.known||!valid(h))return false;wrist=h.joints[1].pose;wrist.orientation=v::normalized(v::multiply(wrist.orientation,c.wrist_basis));
    for(unsigned i=0;i<15;++i){const auto parent=i%3?h.joints[source_joint[i-1]].pose:h.joints[1].pose;
        auto now=relative(parent,h.joints[source_joint[i]].pose).orientation;
        auto delta=v::multiply(v::inverse(c.xr_local[i]),now);auto converted=v::multiply(v::multiply(c.axes[i],delta),v::inverse(c.axes[i]));
        local[i]=v::normalized(v::multiply(r.reference[i+1].orientation,converted));
    }return true;
}
inline Json json(GrabPose const& p){Json joints=Json::array(),raw=Json::array();for(auto q:p.local)joints.push_back(json(q));for(auto const& j:p.raw_in_object)raw.push_back({{"pose",json(j.pose)},{"radius_metres",j.radius},{"flags",j.flags}});return {{"side",p.side?"right":"left"},{"wrist_in_object",json(p.wrist_in_object)},{"local_fingers_xyzw",joints},{"retargeted",p.mapped},{"fully_tracked",p.fully_tracked},{"source_time_ns",p.source_time},{"hand_data_source",p.data_source},{"authored_default",p.authored_default},{"part_delta_known",p.part_delta_known},{"part_delta_in_object",json(p.part_delta)},{"starter_angle_radians",p.starter_angle},{"charging_distance_metres",p.charging_distance},{"raw_joints_in_object",raw}};}
inline GrabPose capture(Frame const& f,unsigned side,Rig const& r,Calibration const& c){if(side>1||!valid(f.hands[side])||f.hands[side].data_source==2||!valid(f.object))throw std::runtime_error("Selected hand is not valid");GrabPose p{};p.side=side;p.data_source=f.hands[side].data_source;p.starter_angle=f.starter_angle;p.charging_distance=f.charging_distance;p.source_time=f.time_ns;p.fully_tracked=tracked(f.hands[side]);P wrist=f.hands[side].joints[1].pose;p.mapped=retarget(r,c,f.hands[side],p.local,wrist);p.wrist_in_object=relative(f.object,wrist);for(unsigned i=0;i<26;++i){p.raw_in_object[i]=f.hands[side].joints[i];p.raw_in_object[i].pose=relative(f.object,p.raw_in_object[i].pose);}return p;}
struct GrabState {int owner{-1};P object_in_wrist{};bool secondary{};V support_in_object{};bool locked{true};};
inline bool begin_grab_at(GrabState& s,P object,P wrist,unsigned side){if(side>1||!valid(wrist)||!valid(object))return false;s.owner=int(side);s.object_in_wrist=relative(wrist,object);s.secondary=false;s.locked=false;return true;}
inline bool begin_grab(GrabState& s,P object,Hand const& h,unsigned side){return valid(h)&&begin_grab_at(s,object,h.joints[1].pose,side);}
inline bool follow_at(GrabState const& s,std::array<P,2> const& wrists,std::array<bool,2> const& known,P& object){
    if(s.locked||s.owner<0||s.owner>1||!known[s.owner]||!valid(wrists[s.owner]))return false;
    auto proposed=compose(wrists[s.owner],s.object_in_wrist);
    if(s.secondary&&known[1-s.owner]&&valid(wrists[1-s.owner])){
        const auto anchor=wrists[s.owner].position;
        const auto current=compose(proposed,{s.support_in_object,{}}).position;
        const auto target=wrists[1-s.owner].position;
        if(length(sub(current,anchor))>.05&&length(sub(target,anchor))>.05){auto a=arc(sub(current,anchor),sub(target,anchor));proposed.position=add(anchor,rotate(a,sub(proposed.position,anchor)));proposed.orientation=v::normalized(v::multiply(a,proposed.orientation));}
    }object=proposed;return true;
}
inline bool follow(GrabState const& s,std::array<Hand,2> const& h,P& object){return follow_at(s,{h[0].joints[1].pose,h[1].joints[1].pose},{valid(h[0]),valid(h[1])},object);}
// Source changes cannot turn an already-held trigger into a new click.
struct ButtonEdge {unsigned source{};bool down{},armed{};bool update(unsigned current,double value,double press=.65,double release=.3){
    if(!current||!std::isfinite(value)){source=0;down=armed=false;return false;}
    if(source!=current){source=current;down=value>=press;armed=!down;return false;}
    if(value<=release){down=false;armed=true;return false;}
    if(value>=press&&!down){down=true;if(armed)return true;}return false;}
};
inline double ray_rectangle(P ray,P plane,double width,double height){
    if(!valid(ray)||!valid(plane)||width<=0||height<=0)return -1;
    auto local=relative(plane,ray);auto d=rotate(local.orientation,{0,0,-1});if(std::abs(d.z)<1e-8)return -1;
    double t=-local.position.z/d.z;if(t<0||t>3)return -1;auto p=add(local.position,mul(d,t));
    return std::abs(p.x)<=width*.5&&std::abs(p.y)<=height*.5?t:-1;
}
}
