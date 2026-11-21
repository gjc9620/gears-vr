#pragma once
#include "core.h"
#include "game_rules/hand_authored_mount_profile.h"
#include "game_rules/weapon_lancer_wrist_mount.h"
#include "game_rules/weapon_lancer_held_contact.h"
#include "game_rules/weapon_lancer_owner_grabposes.h"
#include "game_rules/weapon_lancer_main_hand.h"
#include "game_rules/weapon_lancer_natural_hand.h"
#include "game_rules/weapon_lancer_part_hand.h"
#include "game_rules/weapon_lancer_feed_anchor.h"
#include "game_rules/weapon_lancer_sight_zero.h"

namespace workbench {
inline constexpr v::Matrix asset_to_xr{{{-1,0,0},{0,0,1},{0,-1,0}}};
inline constexpr v::Matrix grip_to_xr{{{0,1,0},{0,0,1},{-1,0,0}}};
inline constexpr v::Matrix wrist_reflection{{{-1,0,0},{0,1,0},{0,0,1}}};
inline V from_raw(V a){return mul(v::transform(asset_to_xr,a),.01);}
inline V to_raw(V a){return mul(v::transform(v::transpose(asset_to_xr),a),100.);}
inline P from_raw_wrist(P a){return {from_raw(a.position),matrix_quaternion(v::multiply(v::multiply(asset_to_xr,v::rotation_matrix(a.orientation)),wrist_reflection))};}
inline P to_raw_wrist(P a){return {to_raw(a.position),matrix_quaternion(v::multiply(v::multiply(v::transpose(asset_to_xr),v::rotation_matrix(a.orientation)),wrist_reflection))};}
inline Q from_raw_joint(Q q){return {q.x,-q.y,-q.z,q.w};}
inline Q mix(Q a,Q b,double t){t=std::clamp(t,0.,1.);if(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0)b={-b.x,-b.y,-b.z,-b.w};return v::normalized({a.x*(1-t)+b.x*t,a.y*(1-t)+b.y*t,a.z*(1-t)+b.z*t,a.w*(1-t)+b.w*t});}
inline P mix(P a,P b,double t){return {add(mul(a.position,1-t),mul(b.position,t)),mix(a.orientation,b.orientation,t)};}
inline Q degrees(V d){constexpr double r=3.141592653589793/180;return v::normalized(v::multiply(v::multiply(axis({0,0,1},d.z*r),axis({0,1,0},d.y*r)),axis({1,0,0},d.x*r)));}
inline double rotation_error(Q a,Q b){return 2*std::acos(std::clamp(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w),0.,1.));}
inline P starter_delta(double angle,double pull){constexpr V pivot{-.04685962200164795,.10670499801635742,-.32844940185546875};auto q=axis({0,-1,0},angle);return {add(sub(pivot,rotate(q,pivot)),{0,0,-pull}),q};}
inline P magazine_seat(){return {from_raw(gears_weapon::lancer_magazine_upper_anchor_raw),{}};}
inline P part_delta(unsigned part,Frame const& f){return part==4?starter_delta(f.starter_angle,f.starter_pull):part==3?P{{0,0,f.charging_distance},{}}:part==2?f.magazine_delta:P{};}
inline P saved_delta(unsigned part,GrabPose const& p){if(p.part_delta_known)return p.part_delta;return part==4?starter_delta(p.starter_angle,0):part==3?P{{0,0,p.charging_distance},{}}:P{};}
inline void captured_geometry(unsigned part,GrabPose const& p,Frame& f){if(part==4){f.starter_angle=p.starter_angle;f.starter_pull=p.part_delta_known?std::clamp(starter_delta(p.starter_angle,0).position.z-p.part_delta.position.z,0.,.08):0;}else if(part==3)f.charging_distance=p.charging_distance;else if(part==2)f.magazine_delta=saved_delta(part,p);}
inline P effective_wrist(Part const& part,unsigned side,Frame const& f){auto const& p=part.poses[side];unsigned index=part.id=="main_grip"?0:part.id=="support"?1:part.id=="magazine"?2:part.id=="charging"?3:4;P wrist=compose(part_delta(index,f),relative(saved_delta(index,p),p.wrist_in_object));wrist.orientation=v::normalized(v::multiply(wrist.orientation,degrees(part.wrist_degrees[side])));wrist.position=add(wrist.position,rotate(wrist.orientation,part.wrist_offset[side]));return wrist;}
inline P controller_wrist_mount(unsigned side,bool holding_main){
    gears_hand_authored_mount::Mount mount{};
    if(holding_main&&side==1){gears_weapon_lancer_wrist_mount::Profile p{};gears_weapon_lancer_wrist_mount::marcus115(gears_hand_authored_mount::marcus115_signature,1,100,p);mount=p.mount;}
    else gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,side?gears_hand_authored_mount::Side::right:gears_hand_authored_mount::Side::left,1,100,mount);
    auto r=v::multiply(v::multiply(grip_to_xr,v::rotation_matrix(mount.calibration.grip_to_wrist_rotation)),wrist_reflection);
    return {v::transform(grip_to_xr,mount.calibration.grip_to_wrist_metres),matrix_quaternion(r)};
}
inline void seed_game_poses(std::vector<Part>& parts){
    if(parts.size()!=5)return;std::array<P,5> raw{};
    V palm{},marker{};v::Matrix basis{};gears_weapon_lancer_contact::authored(palm,basis);gears_weapon_lancer_contact::palm_marker_in_wrist(marker);raw[0]={sub(palm,v::transform(basis,marker)),matrix_quaternion(basis)};
    raw[1]=gears_lancer_owner_grabposes::support_wrist_in_gun;
    auto item=gears_lancer_owner_grabposes::magazine_wrist_in_item;v::Matrix item_basis{{{0,1,0},{1,0,0},{0,0,-1}}};raw[2]={add(gears_weapon::lancer_magazine_upper_anchor_raw,v::transform(item_basis,item.position)),v::normalized(v::multiply(matrix_quaternion(item_basis),item.orientation))};
    raw[3]=gears_lancer_owner_grabposes::loading_wrist_in_gun;raw[4]=gears_lancer_owner_grabposes::starter_wrist_closed;
    auto set=[&](unsigned part,unsigned side,auto const& fingers){if(parts[part].known[side])return;auto& p=parts[part].poses[side];p={};p.side=side;p.wrist_in_object=from_raw_wrist(raw[part]);p.mapped=p.authored_default=p.part_delta_known=true;for(unsigned i=0;i<15;++i)p.local[i]=from_raw_joint(fingers[i]);parts[part].known[side]=true;};
    set(0,1,gears_lancer_main_hand::rest);set(1,0,gears_lancer_natural_hand::support_closed);set(2,0,gears_lancer_part_hand::magazine);set(3,0,gears_lancer_part_hand::loading);set(4,0,gears_lancer_part_hand::starter);
}
// Quest Touch aim already supplies the device's pointing calibration. Keep the
// authored palm at Grip, and fit the cosmetic wrist to the gun, not vice versa.
inline P quest_main_wrist(P grip,P aim,P wrist_in_gun){auto mount=controller_wrist_mount(1,true);auto q=v::normalized(v::multiply(aim.orientation,wrist_in_gun.orientation));auto offset=rotate(v::inverse(mount.orientation),mount.position);return {add(grip.position,rotate(q,offset)),q};}
}
