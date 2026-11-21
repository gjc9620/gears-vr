#include "../vr-common/weapon_button_veto.h"
#include "../vr-common/weapon_longshot_scope_input.h"
#include "../vr-common/vr_gamepad_input.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include <mutex>
#include <cstdio>
#include <cstddef>
struct LegacyWire {
 std::uint32_t size{sizeof(LegacyWire)},version{1};gears_weapon_hand::Frame source{};std::uint32_t armed{},reserved{};
};
using Wire=gears_weapon_button_veto::Request;
static_assert(sizeof(Wire)==sizeof(LegacyWire)&&alignof(Wire)==alignof(LegacyWire));
static_assert(offsetof(Wire,source)==offsetof(LegacyWire,source)&&offsetof(Wire,armed)==offsetof(LegacyWire,armed));
static_assert(std::is_same_v<gears_weapon_longshot_scope_input::Request,Wire>);
bool admitted=true,copied=true;unsigned admissions{},copies{};
bool safe_copy(void* out,void const* in,std::size_t size){++copies;if(!copied)return false;std::memcpy(out,in,size);return true;}
namespace weapon_hand_runtime {
struct Broker {bool admit_weapon_render(gears_weapon_hand::Frame const&){++admissions;return admitted;}}broker;
Broker* current(){return &broker;}
}
namespace vr_gamepad_live {
std::mutex route_mutex;gears_vr_gamepad::Route route;std::atomic<bool> enabled{true},stopping{false};bool scope_fence{};std::int64_t scope_source_ns{};
}
#include "weapon_longshot_scope_input_live.inl"
#include "weapon_button_veto_live.inl"
int main(){
 namespace api=gears_weapon_button_veto;Wire r{};r.armed=1;r.source.controllers.sync_ns=1000000000;
 assert(weapon_button_veto_live::publish(&r)==unsigned(api::Result::applied));
 assert(admissions==1&&copies==1&&vr_gamepad_live::scope_fence);
 assert(vr_gamepad_live::scope_source_ns==r.source.controllers.sync_ns);
 // Sampling retains the original stamp and exact100ms expiry.
 assert(vr_gamepad_live::scope_input_mask(1100000000)==gears_vr_gamepad::right_thumb);
 assert(vr_gamepad_live::scope_source_ns==1000000000);
 assert(vr_gamepad_live::scope_input_mask(1100000001)==0&&!vr_gamepad_live::scope_fence);
 assert(weapon_button_veto_live::publish(&r));
 // Rejected owner/epoch admission cannot clear an incumbent's fence.
 admitted=false;r.armed=0;auto count=admissions;
 assert(!weapon_button_veto_live::publish(&r)&&vr_gamepad_live::scope_fence&&admissions==count+1);
 admitted=true;assert(longshot_scope_input_live::publish(&r)&&!vr_gamepad_live::scope_fence);
 r.armed=1;count=admissions;r.version=2;assert(!weapon_button_veto_live::publish(&r)&&admissions==count);r.version=1;
 r.reserved=1;assert(!weapon_button_veto_live::publish(&r)&&admissions==count);r.reserved=0;
 copied=false;assert(!weapon_button_veto_live::publish(&r)&&admissions==count);copied=true;
 vr_gamepad_live::stopping=true;assert(!weapon_button_veto_live::publish(&r));vr_gamepad_live::stopping=false;
 vr_gamepad_live::enabled=false;assert(!weapon_button_veto_live::publish(&r));vr_gamepad_live::enabled=true;
 assert(weapon_button_veto_live::publish(&r));
 assert(!vr_gamepad_live::scope_input_mask(999999999)&&!vr_gamepad_live::scope_fence);
 std::puts("Generic R3 veto alias: legacy wire, one copy/admission, old/new shared fence, rejected clear and unchanged100ms passed; mock Broker boundary only.");
}
