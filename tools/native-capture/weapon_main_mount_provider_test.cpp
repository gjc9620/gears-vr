#include "../vr-common/weapon_main_mount_provider.h"
#include <cstdio>
#include <limits>

namespace p=gears_weapon_main_mount_provider;
static_assert(sizeof(gears_weapon_hand::Frame)==1384);
static_assert(sizeof(gears_rig_tracking::Frame)==1824);
static_assert(sizeof(p::BodyGeometry)==112&&sizeof(p::Request)==1968&&sizeof(p::Response)==504);
static_assert(sizeof(p::Registration)==48&&alignof(p::Request)==8&&alignof(p::Response)==8);
static_assert(offsetof(p::Request,retained)==32&&offsetof(p::Request,body)==1856);
static_assert(offsetof(p::Response,key)==16&&offsetof(p::Response,world)==72);
static_assert(offsetof(p::Response,hold_generation)==352&&offsetof(p::Response,geometry)==392);
static_assert(offsetof(p::Response,grip_to_wrist_metres)==448&&offsetof(p::Response,grip_to_wrist_rotation)==472);
unsigned checks{},failures{};
#define CHECK(value) do{++checks;if(!(value)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#value);}}while(0)
std::uint32_t __cdecl prepare(void*,p::Request const*,p::Response*) noexcept {
    return static_cast<std::uint32_t>(p::Status::inactive);
}
int main(){
    p::Request request{};p::Response empty{};p::Registration registration{};
    CHECK(request.size==1968&&request.abi_version==1&&request.maximum_age_ns==50000000);
    CHECK(request.query_id==0&&request.hand_lifecycle==0&&request.retained.native.registration_cookie==0);
    CHECK(empty.size==504&&empty.abi_version==1&&empty.query_id==0&&empty.calibration_known==0);
    CHECK(empty.grip_to_wrist_rotation.x==0&&empty.grip_to_wrist_rotation.y==0&&empty.grip_to_wrist_rotation.z==0&&empty.grip_to_wrist_rotation.w==0);
    CHECK(!p::registration_shape_valid(registration));
    int context{};registration.hand_cookie=17;registration.native_started_qpc=29;registration.context=&context;registration.prepare=prepare;
    CHECK(p::registration_shape_valid(registration)&&p::same(registration,registration));
    auto changed=registration;--changed.request_bytes;CHECK(!p::registration_shape_valid(changed)&&!p::same(changed,registration));
    changed=registration;++changed.response_bytes;CHECK(!p::registration_shape_valid(changed)&&!p::same(changed,registration));
    changed=registration;++changed.abi_version;CHECK(!p::registration_shape_valid(changed)&&!p::same(changed,registration));
    changed=registration;changed.hand_cookie=0;CHECK(!p::registration_shape_valid(changed));
    changed=registration;changed.native_started_qpc=0;CHECK(!p::registration_shape_valid(changed));
    changed=registration;changed.context=nullptr;CHECK(!p::registration_shape_valid(changed));
    changed=registration;changed.prepare=nullptr;CHECK(!p::registration_shape_valid(changed));
    for(std::uint32_t status=0;status<=static_cast<std::uint32_t>(p::Status::source_mismatch);++status)CHECK(p::status_valid(status));
    CHECK(!p::status_valid(static_cast<std::uint32_t>(p::Status::source_mismatch)+1));
    CHECK(!p::status_valid((std::numeric_limits<std::uint32_t>::max)()));
    std::printf("MainMountProviderWire: %u checks %u failures; Request=%zu Response=%zu Registration=%zu; data shape only, no admission\n",checks,failures,sizeof(p::Request),sizeof(p::Response),sizeof(p::Registration));
    return failures?1:0;
}
