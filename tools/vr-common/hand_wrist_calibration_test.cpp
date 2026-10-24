#include "hand_wrist_calibration.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>

#ifdef NDEBUG
#error Wrist calibration CPU assertions must stay active
#endif

namespace {
namespace hw=gears_hand_wrist;
using gears_vr::Vector;
using gears_vr::Quaternion;
using gears_vr::Pose;
constexpr double pi=3.14159265358979323846;
unsigned groups{};
Vector add(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vector sub(Vector a,Vector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vector scaled(Vector a,double factor){return {a.x*factor,a.y*factor,a.z*factor};}
double length(Vector a){return std::hypot(a.x,a.y,a.z);}
bool close(Vector a,Vector b,double tolerance=2e-11){return length(sub(a,b))<=tolerance;}
bool equivalent(Quaternion a,Quaternion b,double tolerance=2e-11){
    a=gears_vr::normalized(a);b=gears_vr::normalized(b);
    const double plus=std::hypot(std::hypot(a.x-b.x,a.y-b.y),std::hypot(a.z-b.z,a.w-b.w));
    const double minus=std::hypot(std::hypot(a.x+b.x,a.y+b.y),std::hypot(a.z+b.z,a.w+b.w));
    return (std::min)(plus,minus)<=tolerance;
}
Quaternion axis(Vector direction,double radians){
    const double n=length(direction);assert(n>0);const double s=std::sin(radians/2)/n;
    return {direction.x*s,direction.y*s,direction.z*s,std::cos(radians/2)};
}
Quaternion multi(){return gears_vr::multiply(axis({0,0,1},.63),gears_vr::multiply(axis({0,1,0},-.37),axis({1,0,0},1.13)));}
Pose transformed(Pose pose,Quaternion turn,Vector shift){
    return {add(gears_vr::transform(gears_vr::rotation_matrix(turn),pose.position),shift),gears_vr::multiply(turn,pose.orientation)};
}
Pose target(Pose const& grip,hw::Calibration const& calibration,double units){
    Pose out{};assert(hw::native_wrist_target(grip,calibration,units,out)==hw::Status::okay);return out;
}
hw::Calibration inferred(Pose const& grip,Pose const& wrist,double units){
    hw::Calibration out{};assert(hw::from_caller_aligned_world_pair(grip,wrist,units,out)==hw::Status::okay);assert(out.known);return out;
}
template<class T>std::array<unsigned char,sizeof(T)> bytes(T const& value){
    static_assert(std::is_trivially_copyable<T>::value);
    std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof(value));return out;
}
void reject_target(Pose const& grip,hw::Calibration const& calibration,double units,hw::Status expected){
    Pose out{{71,72,73},{.1,.2,.3,.4}};const auto before=bytes(out);
    assert(hw::native_wrist_target(grip,calibration,units,out)==expected);
    assert(std::memcmp(before.data(),&out,sizeof(out))==0);
}
void reject_pair(Pose const& grip,Pose const& wrist,double units,hw::Status expected){
    hw::Calibration out{true,{81,82,83},{.2,.3,.4,.5}};const auto before=bytes(out);
    assert(hw::from_caller_aligned_world_pair(grip,wrist,units,out)==expected);
    assert(std::memcmp(before.data(),&out,sizeof(out))==0);
}
template<class Test>void run(char const* name,Test test){test();++groups;std::printf("PASS %s\n",name);}
} // namespace

int main(){
    run("default-unknown-refuses-without-output-change",[]{
        const hw::Calibration unknown{};assert(!unknown.known);
        reject_target({},unknown,100,hw::Status::unknown_calibration);
        reject_target({},unknown,0,hw::Status::unknown_calibration);
    });
    run("explicit-local-offset-90deg-noncommuting-full-quaternion",[]{
        const double h=std::sqrt(.5);
        const Pose grip{{10,20,30},{0,0,h,h}};
        const hw::Calibration known{true,{.1,.2,.3},{h,0,0,h}};
        const auto wrist=target(grip,known,100);
        assert(close(wrist.position,{-10,30,60}));
        assert(equivalent(wrist.orientation,{.5,.5,.5,.5}));
        const auto recovered=inferred(grip,{{-10,30,60},{.5,.5,.5,.5}},100);
        assert(close(recovered.grip_to_wrist_metres,known.grip_to_wrist_metres));
        assert(equivalent(recovered.grip_to_wrist_rotation,known.grip_to_wrist_rotation));
    });
    run("metres-units-scale-exactly-once-and-aligned-pair-roundtrip",[]{
        const hw::Calibration known{true,{.03,-.07,.11},multi()};
        const Pose metric_grip{{1.2,-3.4,5.6},axis({1,2,3},.7)};
        const auto metric_wrist=target(metric_grip,known,1);
        for(double units:{1e-6,.5,1.,100.,1000.,1e6}){
            auto grip=metric_grip;grip.position=scaled(grip.position,units);
            const auto wrist=target(grip,known,units);
            assert(close(scaled(wrist.position,1/units),metric_wrist.position));
            assert(equivalent(wrist.orientation,metric_wrist.orientation));
            const auto recovered=inferred(grip,wrist,units);
            assert(close(recovered.grip_to_wrist_metres,known.grip_to_wrist_metres));
            assert(equivalent(recovered.grip_to_wrist_rotation,known.grip_to_wrist_rotation));
        }
    });
    run("full-roll-and-multi-axis-orientation-retained",[]{
        const Pose grip{{4,5,6},multi()};
        const hw::Calibration no_roll{true,{.02,.04,-.03},{}};
        const hw::Calibration with_roll{true,no_roll.grip_to_wrist_metres,axis({1,0,0},1.7)};
        const auto plain=target(grip,no_roll,100),rolled=target(grip,with_roll,100);
        assert(close(plain.position,rolled.position));
        assert(equivalent(rolled.orientation,gears_vr::multiply(grip.orientation,with_roll.grip_to_wrist_rotation)));
        assert(!equivalent(plain.orientation,rolled.orientation,.1));
        const auto recovered=inferred(grip,rolled,100);
        assert(equivalent(recovered.grip_to_wrist_rotation,with_roll.grip_to_wrist_rotation));
    });
    run("legal-nonunit-quaternion-normalization-including-sign-and-magnitude",[]{
        const double h=std::sqrt(.5),large=(std::numeric_limits<double>::max)()/4;
        const double tiny=(std::numeric_limits<double>::denorm_min)();
        const hw::Calibration known{true,{.01,.02,.03},{tiny,0,0,tiny}};
        const Pose grip{{1,2,3},{0,0,-large,-large}};
        const auto wrist=target(grip,known,100);
        assert(close(wrist.position,{-1,3,6}));assert(equivalent(wrist.orientation,{.5,.5,.5,.5}));
        auto aligned_wrist=wrist;aligned_wrist.orientation={7*.5,7*.5,7*.5,7*.5};
        const auto recovered=inferred(grip,aligned_wrist,100);
        assert(equivalent(recovered.grip_to_wrist_rotation,{h,0,0,h}));
    });
    run("return-to-centre-is-stateless-and-in-place-output-is-supported",[]{
        const hw::Calibration known{true,{.04,-.02,.01},multi()};const auto calibration_before=bytes(known);
        const Pose centre{{30,-40,50},axis({2,1,4},.8)};const auto first=target(centre,known,100);
        auto moving=transformed(centre,axis({1,3,-2},1.2),{9,8,7});
        const auto moved=target(moving,known,100);assert(!close(moved.position,first.position,.1));
        const auto returned=target(centre,known,100);assert(bytes(first)==bytes(returned));
        auto in_place=centre;assert(hw::native_wrist_target(in_place,known,100,in_place)==hw::Status::okay);
        assert(bytes(in_place)==bytes(first));assert(bytes(known)==calibration_before);
        const auto before=bytes(in_place);
        assert(hw::native_wrist_target(in_place,hw::Calibration{},100,in_place)==hw::Status::unknown_calibration);
        assert(bytes(in_place)==before);
    });
    run("same-world-rigid-transform-covariance-and-local-calibration-invariance",[]{
        const hw::Calibration known{true,{.012,-.034,.056},multi()};
        const Pose grip{{21,-34,55},axis({3,-2,1},.9)};const auto wrist=target(grip,known,100);
        for(auto turn:{axis({1,0,0},pi/2),axis({0,1,0},-pi/2),multi()}){
            const Vector shift{123,-456,78};const auto changed_grip=transformed(grip,turn,shift);
            const auto changed_wrist=target(changed_grip,known,100),expected=transformed(wrist,turn,shift);
            assert(close(changed_wrist.position,expected.position));assert(equivalent(changed_wrist.orientation,expected.orientation));
            const auto recovered=inferred(changed_grip,expected,100);
            assert(close(recovered.grip_to_wrist_metres,known.grip_to_wrist_metres));
            assert(equivalent(recovered.grip_to_wrist_rotation,known.grip_to_wrist_rotation));
        }
    });
    run("left-right-distinct-explicit-mounts-remain-independent",[]{
        const hw::Calibration left{true,{.03,.01,-.05},axis({1,2,0},.4)};
        const hw::Calibration right{true,{-.02,.07,.04},axis({0,1,3},-.8)};
        const Pose grip{{2,3,4},multi()};const auto a=target(grip,left,100),b=target(grip,right,100),again=target(grip,left,100);
        assert(!close(a.position,b.position,.1)&&!equivalent(a.orientation,b.orientation,.1));
        assert(bytes(a)==bytes(again));
        assert(close(inferred(grip,a,100).grip_to_wrist_metres,left.grip_to_wrist_metres));
        assert(close(inferred(grip,b,100).grip_to_wrist_metres,right.grip_to_wrist_metres));
    });
    run("invalid-units-finite-positions-and-quaternions-preserve-output-bytes",[]{
        const double nan=(std::numeric_limits<double>::quiet_NaN)(),inf=(std::numeric_limits<double>::infinity)();
        const hw::Calibration known{true,{.1,.2,.3},{}};const Pose valid{{1,2,3},multi()};
        for(double units:{0.,-1.,nan,inf}){
            reject_target(valid,known,units,hw::Status::invalid_units);reject_pair(valid,valid,units,hw::Status::invalid_units);
        }
        for(double invalid:{nan,inf}){
            auto grip=valid;grip.position.y=invalid;reject_target(grip,known,100,hw::Status::nonfinite_input);
            reject_pair(grip,valid,100,hw::Status::nonfinite_input);reject_pair(valid,grip,100,hw::Status::nonfinite_input);
            auto calibration=known;calibration.grip_to_wrist_metres.z=invalid;reject_target(valid,calibration,100,hw::Status::nonfinite_input);
        }
        for(Quaternion invalid:{Quaternion{0,0,0,0},Quaternion{nan,0,0,1},Quaternion{0,inf,0,1}}){
            auto grip=valid;grip.orientation=invalid;reject_target(grip,known,100,hw::Status::invalid_quaternion);
            reject_pair(grip,valid,100,hw::Status::invalid_quaternion);reject_pair(valid,grip,100,hw::Status::invalid_quaternion);
            auto calibration=known;calibration.grip_to_wrist_rotation=invalid;reject_target(valid,calibration,100,hw::Status::invalid_quaternion);
        }
    });
    run("translation-overflow-at-scale-rotation-add-subtract-divide-is-explicit",[]{
        const double maximum=(std::numeric_limits<double>::max)();
        const double tiny=(std::numeric_limits<double>::denorm_min)();
        reject_target({},hw::Calibration{true,{maximum,0,0},{}},2,hw::Status::overflow);
        reject_target(Pose{{maximum,0,0},{}},hw::Calibration{true,{maximum,0,0},{}},1,hw::Status::overflow);
        reject_target(Pose{{},axis({0,0,1},pi/4)},hw::Calibration{true,{maximum,maximum,0},{}},1,hw::Status::overflow);
        reject_pair(Pose{{-maximum,0,0},{}},Pose{{maximum,0,0},{}},1,hw::Status::overflow);
        reject_pair({},Pose{{1,0,0},{}},tiny,hw::Status::overflow);
        reject_pair(Pose{{},axis({0,0,1},pi/4)},Pose{{maximum,maximum,0},{}},1,hw::Status::overflow);
    });
    std::printf("PASS hand_wrist_calibration groups=%u physical_calibration_proved=0 live_integration_proved=0\n",groups);
    return 0;
}
