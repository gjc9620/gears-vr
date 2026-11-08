#include "weapon_lancer_wrist_mount.h"
#include "hand_contact_equivalence.h"
#include "weapon_mesh_follow.h"
#include "weapon_two_hand_hold.h"
#include "weapon_lancer_foregrip.h"
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
        Vector grip{};CHECK(fore::grip_in_mesh(grip));
        authored::Mount left{};CHECK(authored::marcus115(authored::marcus115_signature,authored::Side::left,1,100,left));
        const auto orientation=gears_vr::multiply(fore::wrist_in_mesh(),gears_vr::inverse(left.calibration.grip_to_wrist_rotation));
        const auto hand=target({grip,orientation},left.calibration,100);
        // Reconstruct the authored palm marker after calibration. This
        // checks the contact transform only; actual45596's whole palm still
        // crosses the gun despite marker coincidence. Full original skin
        // fitting is a separate requirement, not proved by this CPU check.
        CHECK(close(add(hand.position,rotated(hand.orientation,fore::palm_surface_raw)),fore::surface_raw));
        // The wrist approaches diagonally forward in the horizontal plane,
        // not vertically/upward or backwards into the forearm.
        // Palm faces upward from underneath; fixed marker reaches
        // the actual original fore-end surface after calibration.
        const auto longitudinal=rotated(hand.orientation,{1,0,0});
        CHECK(std::abs(longitudinal.z)<1e-8&&longitudinal.y>.3&&longitudinal.x<-.3);
        // Original index-to-little knuckle row is wrist-local Z. It must
        // extend along the handguard, not span its narrow cross-section.
        const auto finger_row=rotated(hand.orientation,{0,0,1});
        CHECK(std::abs(finger_row.z)<1e-8&&finger_row.y<-.7&&finger_row.x<0);
        // Marcus LEFT thumb lies on wrist-local -Z. The owner5112dfd7
        // reference requires that side toward the muzzle, with the other
        // fingers wrapping the far side. An upward palm alone accepts the
        // reversed hand; the old signed row check even demanded that error.
        const auto thumb_side=rotated(hand.orientation,{0,0,-1});
        CHECK(thumb_side.y>.7);
        // Owner's underhand reference: upward palm, forward forearm,
        // wrist below the contact. A side-facing fist is not acceptable even
        // when the palm marker happens to meet the contact position.
        const auto normal=rotated(hand.orientation,{0,-1,0});
        CHECK(std::abs(normal.x)<1e-8&&std::abs(normal.y)<1e-8&&normal.z>.99);
        CHECK(hand.position.z<fore::surface_raw.z-1.&&hand.position.z>fore::surface_raw.z-2.);
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
    CHECK(std::strcmp(lancer::profile_id,"marcus115-lancer-held-grip-v4-psvr-down35")==0);
    CHECK(std::strcmp(lancer::supported_mesh,"COGAssaultRifle")==0);
    const Quaternion grips[]={Quaternion{},axis({0,0,1},.8),axis({1,0,0},1.3),
        gears_vr::multiply(axis({0,0,1},-.7),gears_vr::multiply(axis({0,1,0},.4),axis({1,0,0},1.1)))};
    Vector unit_scale_offset{};
    for(double model:{.5,1.,1.7,3.})for(double units:{10.,100.,500.}){
        authored::Mount free{};CHECK(authored::marcus115(authored::marcus115_signature,authored::Side::right,model,units,free));
        lancer::Profile held{};CHECK(lancer::marcus115(authored::marcus115_signature,model,units,held));
        CHECK(held.revision==4&&held.provenance==authored::Provenance::designer_authored);
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
        constexpr double c=.81915204428899178968,s=.57357643635104609611;
        CHECK(close(rotated(held.mount.calibration.grip_to_wrist_rotation,{1,0,0}),{c,0,-s}));
        for(auto q:grips){
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
    std::printf("WeaponLancerWristMountTest: %u checks passed (synthetic CPU data only)\n",checks);return 0;
}
