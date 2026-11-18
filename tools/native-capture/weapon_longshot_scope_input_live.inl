// A veto on the original virtual pad only. This cannot publish tracking,
// grant a hand action, write a pose or create an original button press.
namespace vr_gamepad_live {
// Called with route_mutex already held by the original XInput sample path.
std::uint16_t scope_input_mask(std::int64_t now) noexcept {
    if(!scope_fence)return 0;
    if(scope_source_ns<=0||now<scope_source_ns||now-scope_source_ns>100000000){
        scope_fence=false;scope_source_ns=0;return 0;
    }
    route.consume_buttons(gears_vr_gamepad::right_thumb);
    return gears_vr_gamepad::right_thumb;
}
}
namespace longshot_scope_input_live {
namespace api=gears_weapon_longshot_scope_input;
std::uint32_t publish(api::Request const* input) noexcept try {
    api::Request request{};
    if(!input||!safe_copy(&request,input,sizeof request)||request.size!=sizeof request||
       request.version!=1||request.armed>1||request.reserved)return unsigned(api::Result::rejected);
    auto* broker=weapon_hand_runtime::current();
    if(!broker||!broker->admit_weapon_render(request.source))return unsigned(api::Result::rejected);
    std::lock_guard<std::mutex> lock(vr_gamepad_live::route_mutex);
    if(!vr_gamepad_live::enabled.load(std::memory_order_acquire)||
       vr_gamepad_live::stopping.load(std::memory_order_acquire))return unsigned(api::Result::rejected);
    vr_gamepad_live::scope_fence=request.armed!=0;
    vr_gamepad_live::scope_source_ns=request.armed?request.source.controllers.sync_ns:0;
    if(request.armed)vr_gamepad_live::route.consume_buttons(gears_vr_gamepad::right_thumb);
    return unsigned(api::Result::applied);
}catch(...){return unsigned(api::Result::rejected);}
}
