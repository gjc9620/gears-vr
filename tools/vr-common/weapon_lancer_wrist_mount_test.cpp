#include "hand_finger_pose.h" // Must compile without a prior stereo header.
#include "weapon_lancer_wrist_mount.h"
#include "hand_contact_equivalence.h"
#include "weapon_mesh_follow.h"
#include "weapon_two_hand_hold.h"
#include "weapon_lancer_foregrip.h"
#include "weapon_lancer_natural_hand.h"
#include "weapon_lancer_main_hand.h"
#include "weapon_lancer_part_hand.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>

#ifdef NDEBUG
#error WeaponLancerWristMountTest requires active checks
#endif
namespace {
namespace authored=gears_hand_authored_mount;
namespace lancer=gears_weapon_lancer_wrist_mount;
namespace wrist=gears_hand_wrist;
using gears_vr::Vector;using gears_vr::Quaternion;using gears_vr::Pose;
unsigned checks{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);std::abort();}} while(false)
Vector scale(Vector v,double s){return {v.x*s,v.y*s,v.z*s};}
Vector add(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
bool close(Vector a,Vector b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<2e-10;}
Vector rotated(Quaternion q,Vector v){return gears_vr::transform(gears_vr::rotation_matrix(q),v);}
Quaternion axis(Vector v,double radians){const double h=radians*.5,s=std::sin(h);return {v.x*s,v.y*s,v.z*s,std::cos(h)};}
template<class T>std::array<unsigned char,sizeof(T)> bytes(T const& value){
    static_assert(std::is_trivially_copyable_v<T>);std::array<unsigned char,sizeof(T)> out{};
    std::memcpy(out.data(),&value,sizeof(value));return out;
}
Vector marker_in_wrist(wrist::Calibration const& c){
    return scale(gears_vr::transform(gears_vr::transpose(gears_vr::rotation_matrix(c.grip_to_wrist_rotation)),
        c.grip_to_wrist_metres),-1);
}
Pose target(Pose const& grip,wrist::Calibration const& c,double units){
    Pose out{};CHECK(wrist::native_wrist_target(grip,c,units,out)==wrist::Status::okay);return out;
}
void reject(authored::Signature const& signature,double model,double units){
    lancer::Profile out{};CHECK(lancer::marcus115(authored::marcus115_signature,1.7,100,out));
    const auto before=bytes(out);CHECK(!lancer::marcus115(signature,model,units,out));CHECK(bytes(out)==before);
}
static_assert(noexcept(lancer::marcus115(authored::Signature{},1,100,std::declval<lancer::Profile&>())));
}
int main(){
    {
        namespace fore=gears_weapon_lancer_foregrip;
        // Original LOD0 side-panel contact sits ABOVE the chainsaw/guard.
        // The former Root3096 is a real vertex but belongs to that lower
        // strip; being Root-bound alone never certifies a handguard socket.
        CHECK(fore::surface_raw.z>7.&&fore::surface_raw.z<13.&&
              fore::surface_raw.y>45.&&fore::surface_raw.y<62.&&fore::surface_raw.x>2.5);
        Vector grip{};CHECK(fore::grip_in_mesh(grip));
        authored::Mount left{};CHECK(authored::marcus115(authored::marcus115_signature,authored::Side::left,1,100,left));
        const auto orientation=gears_vr::multiply(fore::wrist_in_mesh(),gears_vr::inverse(left.calibration.grip_to_wrist_rotation));
        const auto hand=target({grip,orientation},left.calibration,100);
        // Reconstruct the authored palm marker after calibration. This
        // checks the contact transform only; actual45596's whole palm still
        // crosses the gun despite marker coincidence. Full original skin
        // fitting is a separate requirement, not proved by this CPU check.
        CHECK(close(add(hand.position,rotated(hand.orientation,fore::palm_surface_raw)),fore::surface_raw));
        // These are authored mount regression bounds, not a hand-skin test.
        // Review the thumb on its actual posed branch; wrist-local -Z alone
        // is the knuckle-row side and cannot prove a forward-facing thumb.
        const auto longitudinal=rotated(hand.orientation,{1,0,0});
        CHECK(longitudinal.y>.85&&longitudinal.z<-.2&&longitudinal.z>-.55&&std::abs(longitudinal.x)<.15);
        const auto finger_row=rotated(hand.orientation,{0,0,1});
        CHECK(finger_row.z<-.85&&finger_row.y<-.2&&std::abs(finger_row.x)<.15);
        const auto normal=rotated(hand.orientation,{0,-1,0});
        CHECK(normal.x<-.95&&std::abs(normal.y)<.15&&std::abs(normal.z)<.15);
        CHECK(close(hand.position,{7.7614252890634017,42.787216568792928,11.352242872682027}));
        namespace natural=gears_lancer_natural_hand;
        const auto thumb_base=gears_vr::multiply(hand.orientation,natural::support_closed[12]);
        const auto thumb_middle=gears_vr::multiply(thumb_base,natural::support_closed[13]);
        const auto thumb_direction=add(rotated(thumb_base,{4.162128448486328,-7.62939453125e-6,-1.9073486328125e-6}),
            rotated(thumb_middle,{3.3369216918945312,7.62939453125e-6,-3.814697265625e-6}));
        const auto thumb_length=std::hypot(thumb_direction.x,thumb_direction.y,thumb_direction.z);
        CHECK(thumb_direction.y>.8*thumb_length&&thumb_direction.z>0);

        for(unsigned i=0;i<15;++i){
            const auto reference=natural::support_reference[i];Quaternion q{};
            CHECK(gears_hand_fingers::rotation(0,i,1.,reference,q,gears_hand_fingers::Shape::authored,1.,0.,natural::support_closed));
            const auto expected=natural::support_closed[i];
            CHECK(std::abs(q.x*expected.x+q.y*expected.y+q.z*expected.z+q.w*expected.w)>.999999999);
            CHECK(gears_hand_fingers::rotation(0,i,0.,reference,q,gears_hand_fingers::Shape::authored,1.,0.,natural::support_closed));
            CHECK(std::abs(q.x*reference.x+q.y*reference.y+q.z*reference.z+q.w*reference.w)>.999999999);
            const auto before=bytes(q);
            CHECK(!natural::support_rotation(1,i,reference,q));
            CHECK(bytes(q)==before);
        }
    }
    {
        const Pose tracking{{-4096,512,600},{0,0,0,1}};
        auto rendered=tracking;rendered.position.x+=.0003;
        CHECK(gears_hand_contact_equivalence::same_contact(tracking,rendered,100));
        rendered.orientation.w=-1;
        CHECK(gears_hand_contact_equivalence::same_contact(tracking,rendered,100));
        rendered.position.x+=.01; // A real hand projection is not roundoff.
        CHECK(!gears_hand_contact_equivalence::same_contact(tracking,rendered,100));
        rendered=tracking;rendered.orientation=axis({0,0,1},.001);
        CHECK(!gears_hand_contact_equivalence::same_contact(tracking,rendered,100));
        rendered=tracking;rendered.orientation={0,0,0,0};
        CHECK(!gears_hand_contact_equivalence::same_contact(tracking,rendered,100));
        CHECK(!gears_hand_contact_equivalence::same_contact(tracking,tracking,0));
    }
    CHECK(std::strcmp(lancer::profile_id,"marcus115-lancer-held-grip-v5-owner-down55")==0);
    CHECK(std::strcmp(lancer::supported_mesh,"COGAssaultRifle")==0);
    const Quaternion grips[]={Quaternion{},axis({0,0,1},.8),axis({1,0,0},1.3),
        gears_vr::multiply(axis({0,0,1},-.7),gears_vr::multiply(axis({0,1,0},.4),axis({1,0,0},1.1)))};
    Vector unit_scale_offset{};
    for(double model:{.5,1.,1.7,3.})for(double units:{10.,100.,500.}){
        authored::Mount free{};CHECK(authored::marcus115(authored::marcus115_signature,authored::Side::right,model,units,free));
        lancer::Profile held{};CHECK(lancer::marcus115(authored::marcus115_signature,model,units,held));
        CHECK(held.revision==5&&held.provenance==authored::Provenance::designer_authored);
        CHECK(held.mount.model==authored::Model::marcus115&&held.mount.side==authored::Side::right);
        CHECK(held.mount.provenance==authored::Provenance::designer_authored);
        CHECK(held.mount.input_space==authored::Space::ue_grip_forward_right_up);
        CHECK(held.mount.geometry_signature==authored::marcus115_signature);
        CHECK(held.mount.bone_count==115&&held.mount.hand_bone==35&&held.mount.chain==free.chain);
        CHECK(held.mount.model_to_world_scale==model&&held.mount.units_per_metre==units);
        const auto& a=held.mount.calibration.grip_to_wrist_metres;
        const auto& b=free.calibration.grip_to_wrist_metres;
        CHECK(std::abs(std::hypot(std::hypot(a.x,a.y),a.z)-std::hypot(std::hypot(b.x,b.y),b.z))<1e-12);
        const auto free_marker=marker_in_wrist(free.calibration),held_marker=marker_in_wrist(held.mount.calibration);
        CHECK(close(free_marker,held_marker));
        constexpr double c=.57357643635104609611,s=.81915204428899178968;
        CHECK(close(rotated(held.mount.calibration.grip_to_wrist_rotation,{1,0,0}),{c,0,-s}));
        for(auto q:grips){
            const auto previous_bore=rotated(gears_vr::multiply(q,Quaternion{0,.30070579950427311950,0,.95371695074822692114}),{1,0,0});
            const auto corrected_bore=rotated(gears_vr::multiply(q,held.mount.calibration.grip_to_wrist_rotation),{1,0,0});
            const double separation=std::acos(std::clamp(previous_bore.x*corrected_bore.x+previous_bore.y*corrected_bore.y+previous_bore.z*corrected_bore.z,-1.,1.));
            CHECK(std::abs(separation-20.*3.14159265358979323846/180.)<1e-12);
            const Pose grip{{12,-34,56},q};
            const auto fw=target(grip,free.calibration,units),hw=target(grip,held.mount.calibration,units);
            // Contextual held-hand fitting may rotate the wrist, while BOTH
            // poses must keep the exact same physical palm marker at Grip.
            // Both authored Wrist poses retain the SAME physical marker at Grip.
            CHECK(close(add(fw.position,rotated(fw.orientation,scale(free_marker,units))),grip.position));
            CHECK(close(add(hw.position,rotated(hw.orientation,scale(free_marker,units))),grip.position));
            Pose endpoint{grip.position,hw.orientation},decoded{};
            CHECK(lancer::grip_from_palm_endpoint(endpoint,decoded)&&close(decoded.position,grip.position));
            for(Vector basis:{Vector{1,0,0},Vector{0,1,0},Vector{0,0,1}})
                CHECK(close(rotated(decoded.orientation,basis),rotated(grip.orientation,basis)));
            CHECK(close(hw.position,add(grip.position,rotated(q,scale(held.mount.calibration.grip_to_wrist_metres,units)))));
            for(Vector basis:{Vector{1,0,0},Vector{0,1,0},Vector{0,0,1}})
                CHECK(close(rotated(hw.orientation,basis),rotated(gears_vr::multiply(q,held.mount.calibration.grip_to_wrist_rotation),basis)));
            const auto gun_world=gears_vr::multiply(hw.orientation,held.gun_in_wrist_rotation);
            const auto corrected=gears_vr::multiply(q,lancer::gun_pitch_down);
            CHECK(close(rotated(gun_world,{0,1,0}),rotated(corrected,{1,0,0})));
            CHECK(close(rotated(gun_world,{0,0,1}),rotated(corrected,{0,0,1})));
            if(model==1&&units==100){
                // Cross the actual Native-palm -> PG-grip -> weapon-mount
                // boundary. The old path fed Wrist axes straight to the
                // follower: it produced an extra20degrees and made the
                // rendered foregrip disagree with acquisition geometry.
                const std::array<float,16> initial{{1,0,0,0,0,1,0,0,0,0,1,0,17,-23,41,1}};
                const Vector contact{-2.8,5.4,.2},foregrip{9,49,0};
                gears_weapon::WeaponMeshFollow follower;
                const gears_vr::Viewpoint view{decoded.position,gears_vr::ue_angles(gears_vr::rotation_matrix(decoded.orientation))};
                CHECK(follower.bind_mount(view,initial,contact,gears_vr::ue_matrix({0,1.5707963267948966,0}),
                    gears_vr::rotation_matrix(lancer::gun_pitch_down)));
                std::array<float,16> actual{};CHECK(follower.model_matrix(view,actual));
                const auto point=[](std::array<float,16> const& m,Vector p){return Vector{
                    m[12]+m[0]*p.x+m[4]*p.y+m[8]*p.z,m[13]+m[1]*p.x+m[5]*p.y+m[9]*p.z,
                    m[14]+m[2]*p.x+m[6]*p.y+m[10]*p.z};};
                const auto near=[](Vector a,Vector b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<.0001;};
                CHECK(near(point(actual,contact),grip.position));
                CHECK(near({actual[4],actual[5],actual[6]},rotated(corrected,{1,0,0})));
                std::array<float,16> canonical{};CHECK(follower.model_matrix({{},{}},canonical));
                Pose support{scale(point(canonical,foregrip),.01),{}};
                const Pose physical_left{point(actual,foregrip),{}};
                gears_weapon_two_hand::Solution paired{};
                CHECK(gears_weapon_two_hand::solve(decoded,physical_left,support,100,paired));
                CHECK(near(paired.right.position,grip.position)&&near(paired.left.position,physical_left.position));
                // A current offhand motion steers this same weapon matrix;
                // contact and muzzle direction cannot use different mounts.
                auto moved=physical_left;moved.position=add(moved.position,rotated(q,{0,3,2}));
                CHECK(gears_weapon_two_hand::solve(decoded,moved,support,100,paired));
                const gears_vr::Viewpoint supported{paired.right.position,gears_vr::ue_angles(gears_vr::rotation_matrix(paired.right.orientation))};
                CHECK(follower.model_matrix(supported,actual));
                CHECK(near(point(actual,contact),grip.position)&&near(point(actual,foregrip),paired.left.position));
                CHECK(near({actual[4],actual[5],actual[6]},rotated(gears_vr::multiply(paired.right.orientation,lancer::gun_pitch_down),{1,0,0})));
            }
        }
        // Calibration divides by units once; target multiplies by units once.
        const auto world_offset=scale(held.mount.calibration.grip_to_wrist_metres,units/model);
        if(model==.5&&units==10)unit_scale_offset=world_offset;
        CHECK(close(world_offset,unit_scale_offset));
    }
    auto wrong=authored::marcus115_signature;
    for(unsigned i=0;i<wrong.size();++i){wrong=authored::marcus115_signature;wrong[i]^=1;reject(wrong,1,100);}
    const auto nan=(std::numeric_limits<double>::quiet_NaN)(),inf=(std::numeric_limits<double>::infinity)();
    for(double model:{0.,-1.,nan,inf})reject(authored::marcus115_signature,model,100);
    for(double units:{0.,-1.,9.999,500.001,nan,inf})reject(authored::marcus115_signature,1,units);
    for(auto bad:std::array<Pose,3>{{{{nan,0,0},{}},{{},{0,0,0,0}},{{},{0,0,0,2}}}}){
        Pose untouched{{17,23,41},{}};const auto before=bytes(untouched);
        CHECK(!lancer::grip_from_palm_endpoint(bad,untouched)&&bytes(untouched)==before);}
    // The Lancer-only main preset has the authenticated model reference and
    // moves only the index when the actual trigger changes. It cannot supply
    // a new wrist transform, action lease or altered non-index grip.
    {
        namespace main=gears_lancer_main_hand;namespace api=gears_hand_authored_grasp;
        const auto q=main::request({});CHECK(api::structural(q));
        api::Binding binding{gears_hand_authored_mount::marcus115_signature,
            gears_hand_marcus115_reference::right};
        api::Joints open{},pressed{},middle{};
        CHECK(api::rotations(q,binding,0,open)&&api::rotations(q,binding,1,pressed)&&api::rotations(q,binding,.5,middle));
        for(unsigned i=3;i<15;++i)CHECK(bytes(open[i])==bytes(pressed[i])&&bytes(open[i])==bytes(middle[i]));
        CHECK(bytes(open[1])!=bytes(pressed[1]));
        auto bad=binding;bad.geometry_signature[0]^=1;CHECK(!api::rotations(q,bad,0,middle));
        bad=binding;bad.reference[0]={0,0,0,1};CHECK(!api::rotations(q,bad,0,middle));
    }
    {
        namespace part=gears_lancer_part_hand;namespace api=gears_hand_authored_grasp;
        api::Binding binding{gears_hand_authored_mount::marcus115_signature,
            gears_hand_marcus115_reference::left};
        for(auto kind:{part::Kind::magazine,part::Kind::loading,part::Kind::starter}){
            api::Request q{};CHECK(part::request(kind,{},{{1,2,3},{}},q));
            api::Joints a{},b{};CHECK(api::rotations(q,binding,0,a)&&api::rotations(q,binding,1,b));
            CHECK(bytes(a)==bytes(b)&&!q.has_trigger_endpoint&&q.side==api::Side::left);
            auto wrong_binding=binding;wrong_binding.geometry_signature[3]^=1;CHECK(!api::rotations(q,wrong_binding,0,b));
        }
        api::Request q{};const auto before=bytes(q);
        CHECK(!part::request(part::Kind::none,{},{},q)&&bytes(q)==before);
        CHECK(!part::request(part::Kind::support,{},{},q)&&bytes(q)==before);
    }
    std::printf("WeaponLancerWristMountTest: %u checks passed (synthetic CPU data only)\n",checks);return 0;
}
