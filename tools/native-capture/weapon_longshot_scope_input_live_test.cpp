#include "../vr-common/weapon_longshot_scope_input.h"
#include "../vr-common/vr_gamepad_input.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include <mutex>
#include <cstdio>
// Real veto implementation and Route, with only Broker/source and POD copy
// boundaries injected. No hook installation or original engine input occurs.
bool admitted=true,copied=true;unsigned admissions{};
bool safe_copy(void* out,void const* in,std::size_t size){if(!copied)return false;std::memcpy(out,in,size);return true;}
namespace weapon_hand_runtime {
struct Broker {bool admit_weapon_render(gears_weapon_hand::Frame const&){++admissions;return admitted;}}broker;
Broker* current(){return &broker;}
}
namespace vr_gamepad_live {std::mutex route_mutex;gears_vr_gamepad::Route route;std::atomic<bool> enabled{true},stopping{false};bool scope_fence{};std::int64_t scope_source_ns{};}
#include "weapon_longshot_scope_input_live.inl"
int main(){
    namespace api=gears_weapon_longshot_scope_input;namespace pad=gears_vr_gamepad;
    using namespace gears_vr;
    api::Request request{};request.source.controllers.sync_ns=1000000000;
    assert(longshot_scope_input_live::publish(&request)==unsigned(api::Result::applied));
    request.armed=1;admitted=false;assert(!longshot_scope_input_live::publish(&request)&&!vr_gamepad_live::scope_fence);
    admitted=true;assert(longshot_scope_input_live::publish(&request)&&vr_gamepad_live::scope_fence);
    request.armed=0;admitted=false;assert(!longshot_scope_input_live::publish(&request)&&vr_gamepad_live::scope_fence);
    admitted=true;request.version=2;assert(!longshot_scope_input_live::publish(&request));request.version=1;
    request.reserved=1;assert(!longshot_scope_input_live::publish(&request));request.reserved=0;
    copied=false;assert(!longshot_scope_input_live::publish(&request));copied=true;
    vr_gamepad_live::stopping=true;assert(!longshot_scope_input_live::publish(&request));vr_gamepad_live::stopping=false;
    vr_gamepad_live::enabled=false;assert(!longshot_scope_input_live::publish(&request));vr_gamepad_live::enabled=true;
    ControllerInputFrame f{};f.session=f.space_epoch=f.profile_generation=f.input_epoch=1;f.sequence=1;f.predicted_time=1;
    f.sync_ns=f.received_ns=1000000000;f.focused=1;
    for(auto& h:f.hands){h.grip_active=h.squeeze_active=h.trigger_active=h.stick_active=1;h.grip_flags=15;}
    f.hands[1].buttons_active=controller_stick_click;
    gears_hand_continuity::TrackedInput tracking{};tracking.stream_epoch=tracking.tracking_epoch[0]=tracking.tracking_epoch[1]=1;tracking.input=f;
    gears_hand_continuity::Publisher publisher{11,22};
    auto observe=[&](){tracking.input=f;assert(vr_gamepad_live::route.observe(f,tracking,publisher,f.sync_ns,true,false));};
    auto advance=[&](){++f.sequence;f.sync_ns+=10000000;f.received_ns=f.sync_ns;};
    observe();advance();f.hands[1].buttons=controller_stick_click;f.hands[1].pressed[control_stick]=1;observe();
    std::uint32_t packet{};{
        std::lock_guard<std::mutex> lock(vr_gamepad_live::route_mutex);
        const auto mask=vr_gamepad_live::scope_input_mask(f.sync_ns);assert(mask==pad::right_thumb);
        assert(vr_gamepad_live::route.sample(f.sync_ns,true,packet,mask).buttons==0);
    }
    assert(longshot_scope_input_live::publish(&request)&&!vr_gamepad_live::scope_fence);
    advance();observe();assert(vr_gamepad_live::route.sample(f.sync_ns,true,packet).buttons==0);
    advance();f.hands[1].buttons=0;f.hands[1].released[control_stick]=1;observe();
    advance();f.hands[1].buttons=controller_stick_click;f.hands[1].pressed[control_stick]=2;observe();
    assert(vr_gamepad_live::route.sample(f.sync_ns,true,packet).buttons==pad::right_thumb);
    request.armed=1;request.source.controllers.sync_ns=f.sync_ns;
    assert(longshot_scope_input_live::publish(&request));
    assert(vr_gamepad_live::scope_input_mask(f.sync_ns+100000000)==pad::right_thumb);
    assert(!vr_gamepad_live::scope_input_mask(f.sync_ns+100000001)&&!vr_gamepad_live::scope_fence);
    // Expiry restores routing eligibility, but must not replay a held R3.
    advance();observe();assert(vr_gamepad_live::route.sample(f.sync_ns,true,packet).buttons==0);
    request.source.controllers.sync_ns=f.sync_ns;assert(longshot_scope_input_live::publish(&request));
    assert(!vr_gamepad_live::scope_input_mask(f.sync_ns-1)&&!vr_gamepad_live::scope_fence);
    std::puts("Longshot Native scope input veto: current Broker admission, exact request shape, stop/enable gates, R3 consume and no held replay passed");
}
