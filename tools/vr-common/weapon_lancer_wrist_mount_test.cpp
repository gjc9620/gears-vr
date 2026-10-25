#include "weapon_lancer_wrist_mount.h"
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
    CHECK(std::strcmp(lancer::profile_id,"marcus115-lancer-held-grip-v1")==0);
    CHECK(std::strcmp(lancer::supported_mesh,"COGAssaultRifle")==0);
    const Quaternion grips[]={Quaternion{},axis({0,0,1},.8),axis({1,0,0},1.3),
        gears_vr::multiply(axis({0,0,1},-.7),gears_vr::multiply(axis({0,1,0},.4),axis({1,0,0},1.1)))};
    Vector unit_scale_offset{};
    for(double model:{.5,1.,1.7,3.})for(double units:{10.,100.,500.}){
        authored::Mount free{};CHECK(authored::marcus115(authored::marcus115_signature,authored::Side::right,model,units,free));
        lancer::Profile held{};CHECK(lancer::marcus115(authored::marcus115_signature,model,units,held));
        CHECK(held.revision==1&&held.provenance==authored::Provenance::designer_authored);
        CHECK(held.mount.model==authored::Model::marcus115&&held.mount.side==authored::Side::right);
        CHECK(held.mount.provenance==authored::Provenance::designer_authored);
        CHECK(held.mount.input_space==authored::Space::ue_grip_forward_right_up);
        CHECK(held.mount.geometry_signature==authored::marcus115_signature);
        CHECK(held.mount.bone_count==115&&held.mount.hand_bone==35&&held.mount.chain==free.chain);
        CHECK(held.mount.model_to_world_scale==model&&held.mount.units_per_metre==units);
        const auto free_marker=marker_in_wrist(free.calibration),held_marker=marker_in_wrist(held.mount.calibration);
        CHECK(close(free_marker,held_marker));
        CHECK(close(rotated(held.mount.calibration.grip_to_wrist_rotation,{1,0,0}),{1,0,0}));
        for(auto q:grips){
            const Pose grip{{12,-34,56},q};
            const auto fw=target(grip,free.calibration,units),hw=target(grip,held.mount.calibration,units);
            // Both authored Wrist poses retain the SAME physical marker at Grip.
            CHECK(close(add(fw.position,rotated(fw.orientation,scale(free_marker,units))),grip.position));
            CHECK(close(add(hw.position,rotated(hw.orientation,scale(free_marker,units))),grip.position));
            CHECK(close(hw.position,add(grip.position,rotated(q,scale(held.mount.calibration.grip_to_wrist_metres,units)))));
            for(Vector basis:{Vector{1,0,0},Vector{0,1,0},Vector{0,0,1}})
                CHECK(close(rotated(hw.orientation,basis),rotated(q,basis)));
            const auto gun_world=gears_vr::multiply(hw.orientation,held.gun_in_wrist_rotation);
            CHECK(close(rotated(gun_world,{0,1,0}),rotated(q,{1,0,0})));
            CHECK(close(rotated(gun_world,{0,0,1}),rotated(q,{0,0,1})));
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
    std::printf("WeaponLancerWristMountTest: %u checks passed (synthetic CPU data only)\n",checks);return 0;
}
