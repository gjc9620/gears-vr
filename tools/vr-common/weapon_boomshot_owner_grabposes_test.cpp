#include "weapon_boomshot_owner_grabposes.h"
#include <cmath>
#include <cstdio>

int main(){
    namespace g=gears_boomshot_owner_grabposes;namespace v=gears_vr;
    const v::Pose grip{{20.,30.,-80.},{0.,.3826834323650898,0.,.9238795325112867}};
    const double scale=.75,units=100.,model_scale=1.;
    v::Pose gun{},roundtrip{};
    if(!g::main_gun_from_grip(grip,scale,units,model_scale,gun)||
       !g::main_grip(gun,scale,units,model_scale,roundtrip))return 1;
    const double dp=std::hypot(std::hypot(roundtrip.position.x-grip.position.x,
        roundtrip.position.y-grip.position.y),roundtrip.position.z-grip.position.z);
    const double dot=std::abs(roundtrip.orientation.x*grip.orientation.x+
        roundtrip.orientation.y*grip.orientation.y+roundtrip.orientation.z*grip.orientation.z+
        roundtrip.orientation.w*grip.orientation.w);
    const double da=2*std::acos(std::min(1.,dot));
    const v::Pose seated{{10.,20.,30.},{0.,0.,.25881904510252074,.9659258262890683}};
    const double ratio=1.,m0_scale=.75;
    g::AmmoCarrierRelation relation{};v::Pose carrier{};
    if(!g::ammo_wrist_relation(seated,ratio,units,relation)||
       !g::ammo_carrier_from_grip(grip,relation,m0_scale,units,model_scale,carrier))return 3;
    const auto p=relation.wrist_in_carrier.position;
    const double source_error=std::hypot(std::hypot(
        p.x-g::ammo_wrist_in_carrier_source.position.x,
        p.y-g::ammo_wrist_in_carrier_source.position.y),
        p.z-g::ammo_wrist_in_carrier_source.position.z);
    const auto q=g::ammo_wrist_in_carrier_source.orientation;
    const auto qr=relation.wrist_in_carrier.orientation;
    const double source_rotation=2*std::acos(std::min(1.,std::abs(q.x*qr.x+q.y*qr.y+q.z*qr.z+q.w*qr.w)));
    std::printf("Boomshot source relations: main_position_error=%.9g main_rotation_error_rad=%.9g ammo_source_error=%.9g ammo_rotation_error_rad=%.9g\n",
        dp,da,source_error,source_rotation);
    return dp<=1e-8&&da<=1e-6&&source_error<=1e-12&&source_rotation<=1e-12?0:2;
}
