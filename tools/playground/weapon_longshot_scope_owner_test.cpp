// Production journal with injected stock calls, state reads and Native bridge.
#include "../vr-common/weapon_longshot_reload.h"
#include "../vr-common/weapon_longshot_scope_input.h"
#include "../vr-common/weapon_longshot_scope_view.h"
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <crtdbg.h>
using uptr=std::uintptr_t;using HMODULE=void*;using FARPROC=void(*)();
FILE* log_file{};
namespace hand=gears_weapon_hand;
namespace gears_weapon_hand {struct ExistingActionLifetime {};}
bool live=true,lifetime_live=true,bridges_live=true,view_ok=true,fence_ok=true,capture_ok=true;
bool state_ok=true,fail_cleanup=false,aim_owned=false,values_live=true,receivers_retired=false;unsigned fail_set{},sets{},enables{},view_calls{},fence_calls{};
bool fence_armed{},view_active{};double selected_fov{};
hand::Frame sample_frame{};std::uint64_t hold=1;
namespace weaponhand {
HMODULE native_module=reinterpret_cast<HMODULE>(16);
bool current_gt(void*){return true;}bool executable(FARPROC p,HMODULE){return p!=nullptr;}
std::int64_t clock_ns(void*){return sample_frame.controllers.sync_ns;}
struct Client {
    bool capture_existing_action_lifetime_on_gt(hand::FrameKey const&,hand::ExistingActionLifetime&){return capture_ok;}
    bool existing_action_lifetime_on_gt(hand::ExistingActionLifetime const&){return lifetime_live;}
}client;
}
namespace nativeequipment {bool current(void*,hand::Frame const&){return live;}}
namespace vrfire {bool aim_owns(){return aim_owned;}}
namespace longshot_fire {
gears_weapon::LongshotReloadIdentity reload_identity(hand::Frame const& f){
    return {f.world.address,f.pawn.address,f.weapon.address,1,2,3,4,5,hold};
}
}
namespace longshot_scope_state {
struct Component {hand::ObjectIdentity identity{};bool owner_no_see{};};
struct State {
    std::array<Component,4> components{};bool controller_target{},pawn_target{},target_key{},zoom{},ignore_controllers{true};float base_fov{85};
}actual;
bool read(hand::Frame const&,State& out){out=actual;return state_ok;}
bool neutral(State const& s){
    if(s.controller_target||s.pawn_target||s.target_key||s.zoom||!s.ignore_controllers)return false;
    for(auto& c:s.components)if(c.owner_no_see)return false;return true;
}
bool scoped(State const& s){
    if(!s.controller_target||!s.pawn_target||s.target_key||s.ignore_controllers)return false;
    for(auto& c:s.components)if(c.identity.address&&!c.owner_no_see)return false;return true;
}
}
namespace longshot_scope_original {
enum class Method {target,zoom,visibility,fov};struct Entry {Method method{};};
bool bind(hand::ObjectIdentity const&,Method method,Entry& out){out.method=method;return true;}
bool set(hand::Frame const&,Entry const&,bool,bool* =nullptr,hand::ExistingActionLifetime const* =nullptr);
bool fov(hand::Frame const&,Entry const&,float,float& out){out=longshot_scope_state::actual.zoom?10.f:40.f;return live;}
}
namespace lancerreload_runtime {bool weapon_retired(hand::ObjectIdentity const&){return receivers_retired;}}
namespace weapon_part_assets {
bool boolean(hand::ObjectIdentity const& object,const wchar_t* label,bool& out){
    if(!values_live)return false;
    const auto& s=longshot_scope_state::actual;
    if(!std::wcscmp(label,L"bIsTargeting")){
        if(object.address==sample_frame.controller.address){out=s.controller_target;return true;}
        if(object.address==sample_frame.pawn.address){out=s.pawn_target;return true;}
    }
    if(object.address==sample_frame.controller.address&&!std::wcscmp(label,L"bPressingTargetingKey")){out=s.target_key;return true;}
    if(object.address==sample_frame.weapon.address&&!std::wcscmp(label,L"bZoomed")){out=s.zoom;return true;}
    if(!std::wcscmp(label,L"bOwnerNoSee"))for(auto& c:s.components)if(hand::same(c.identity,object)){out=c.owner_no_see;return true;}
    if(hand::same(s.components[1].identity,object)&&!std::wcscmp(label,L"bIgnoreControllersWhenNotRendered")){out=s.ignore_controllers;return true;}
    return false;
}
}
namespace longshot_eyebox {
struct Observation {bool valid{true},near_eye{true},switch_requested{};}current;
bool matches(hand::Frame const&,std::int64_t){return current.valid&&live;}
bool view_request(hand::Frame const& f,double fov,gears_weapon_longshot_scope_view::Request& out){out.source=f;out.active=1;out.original_targeting_fov_degrees=fov;return true;}
}
std::uint32_t __stdcall input_bridge(gears_weapon_longshot_scope_input::Request const* q) noexcept {
    ++fence_calls;if(!fence_ok)return 0;fence_armed=q->armed!=0;return 1;
}
std::uint32_t __stdcall view_bridge(gears_weapon_longshot_scope_view::Request const* q) noexcept {
    ++view_calls;if(!view_ok)return 0;view_active=q->active!=0;selected_fov=q->original_targeting_fov_degrees;return 1;
}
FARPROC GetProcAddress(HMODULE,const char* name){
    if(!bridges_live)return nullptr;
    return !std::strcmp(name,gears_weapon_longshot_scope_input::export_name)?reinterpret_cast<FARPROC>(input_bridge):
        !std::strcmp(name,gears_weapon_longshot_scope_view::export_name)?reinterpret_cast<FARPROC>(view_bridge):nullptr;
}
#define near
#define GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL
#include "weapon_longshot_scope_owner.inl"
#undef GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL
#undef near
namespace owner=longshot_scope_owner;namespace state=longshot_scope_state;
namespace longshot_scope_original {
bool set(hand::Frame const&,Entry const& entry,bool value,bool*,hand::ExistingActionLifetime const* cleanup){
    assert(owner::owed()); // Journal must precede any uncertain side effect.
    if((cleanup&&(!lifetime_live||value||fail_cleanup))||(!cleanup&&!live))return false;
    ++sets;if(value)++enables;
    auto& s=state::actual;
    if(entry.method==Method::target)s.controller_target=s.pawn_target=value;
    if(entry.method==Method::zoom)s.zoom=value;
    if(entry.method==Method::zoom||entry.method==Method::visibility){
        for(auto& c:s.components)if(c.identity.address)c.owner_no_see=value;
        s.ignore_controllers=!value;
    }
    return !fail_set||sets!=fail_set; // Failure may occur AFTER a side effect.
}
}
void reset(){
    owner::journal={};owner::outstanding.store(false);owner::stop_requested.store(false);owner::blocked_until_exit=false;
    state::actual={};state::actual.components[0].identity={1024,0,16,1,1,0,1};
    state::actual.components[1].identity={1280,0,16,2,2,0,1};
    longshot_eyebox::current={};live=lifetime_live=bridges_live=view_ok=fence_ok=capture_ok=state_ok=true;
    fail_cleanup=aim_owned=receivers_retired=false;values_live=true;
    fail_set=sets=enables=view_calls=fence_calls=0;view_active=fence_armed=false;hold=1;
    sample_frame.world.address=16;sample_frame.pawn.address=32;sample_frame.weapon.address=48;
    sample_frame.reference_generation=1;sample_frame.controllers.sequence=1;sample_frame.controllers.sync_ns=1000000000;
    sample_frame.controller={64,0,16,4,4,0,1};
    sample_frame.key.game_start=11;sample_frame.key.native_started_qpc=12;sample_frame.key.registration_cookie=13;
    sample_frame.key.lifecycle=14;sample_frame.key.world_generation=15;sample_frame.key.game_pid=16;sample_frame.key.thread_id=17;
}
void tick(){++sample_frame.controllers.sequence;sample_frame.controllers.sync_ns+=20000000;owner::tick(sample_frame);}
int main(){
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    reset();bridges_live=false;tick();assert(!owner::owed()&&!sets&&!fence_calls);
    reset();state::actual.components[1].owner_no_see=true;tick();assert(!owner::owed()&&!sets); // Existing visibility owner preserved.
    reset();capture_ok=false;tick();assert(!owner::owed()&&!sets&&!fence_calls);
    reset();aim_owned=true;tick();assert(!owner::owed()&&!sets&&!fence_calls);
    reset();fence_ok=false;tick();assert(!owner::owed()&&!sets&&state::neutral(state::actual));
    reset();tick();assert(owner::ready(sample_frame)&&view_active&&fence_armed&&selected_fov==40);
    {auto foreign=sample_frame;++foreign.controller.registry_index;assert(!owner::ready(foreign)&&!owner::owns_target(foreign));
     foreign=sample_frame;++foreign.key.native_started_qpc;assert(!owner::ready(foreign)&&!owner::owns_target(foreign));
     foreign=sample_frame;++foreign.key.registration_cookie;assert(!owner::ready(foreign));
     foreign=sample_frame;++foreign.key.lifecycle;assert(!owner::ready(foreign));
     foreign=sample_frame;++foreign.weapon.registry_index;assert(!owner::ready(foreign));
     auto advancing=sample_frame;++advancing.key.frame;assert(owner::ready(advancing));}
    const auto entry_enables=enables;
    longshot_eyebox::current.switch_requested=true;tick();assert(state::actual.zoom&&selected_fov==10);
    const auto once=sets;owner::tick(sample_frame);assert(sets==once); // Same Frame cannot toggle twice.
    longshot_eyebox::current.switch_requested=false;tick();assert(state::actual.zoom&&enables>entry_enables);
    longshot_eyebox::current.switch_requested=true;tick();assert(!state::actual.zoom&&selected_fov==40&&state::scoped(state::actual));
    longshot_eyebox::current.near_eye=false;tick();assert(!owner::owed()&&state::neutral(state::actual)&&!view_active&&!fence_armed);
    const auto idle_view=view_calls,idle_fence=fence_calls;tick();assert(view_calls==idle_view&&fence_calls==idle_fence); // Other guns' idle path is neutral.
    for(unsigned failed:{1u,2u}){
        reset();fail_set=failed;tick();assert(!owner::owed()&&state::neutral(state::actual)&&owner::blocked_until_exit);
        const auto no_replay=sets;tick();assert(sets==no_replay);
        longshot_eyebox::current.near_eye=false;tick();longshot_eyebox::current.near_eye=true;fail_set=0;tick();assert(owner::owed());
    }
    reset();tick();fail_set=sets+1;longshot_eyebox::current.switch_requested=true;tick();
    assert(!owner::owed()&&state::neutral(state::actual));const auto no_toggle_retry=sets;tick();assert(sets==no_toggle_retry);
    reset();tick();view_ok=false;tick();assert(!owner::owed()&&state::neutral(state::actual));
    reset();tick();fail_cleanup=true;longshot_eyebox::current.near_eye=false;tick();assert(owner::owed()&&!owner::ready(sample_frame));
    assert(owner::owns_target(sample_frame));
    const auto no_new_enable=enables;tick();assert(enables==no_new_enable&&owner::owed());fail_cleanup=false;tick();assert(!owner::owed());
    reset();tick();live=false;lifetime_live=false;owner::unavailable(0);assert(owner::owed());
    lifetime_live=true;owner::unavailable(0);assert(!owner::owed()&&state::neutral(state::actual));
    reset();tick();live=lifetime_live=state_ok=values_live=false;
    const auto before_retirement=sets;owner::unavailable(0);assert(owner::owed()&&sets==before_retirement);
    receivers_retired=true;owner::unavailable(0);assert(!owner::owed()&&sets==before_retirement);
    reset();tick();live=lifetime_live=state_ok=false;
    state::actual.controller_target=state::actual.pawn_target=state::actual.zoom=false;
    state::actual.ignore_controllers=true;for(auto& c:state::actual.components)c.owner_no_see=false;
    const auto independently_restored=sets;owner::unavailable(0);assert(!owner::owed()&&sets==independently_restored);
    reset();tick();++hold;tick();assert(!owner::owed()&&state::neutral(state::actual)&&owner::blocked_until_exit);
    reset();tick();owner::stop_requested.store(true);tick();assert(!owner::owed()&&state::neutral(state::actual));tick();assert(!owner::owed());
    reset();tick();owner::stop_requested.store(true);owner::service();assert(!owner::owed()&&state::neutral(state::actual));
    owner::reset_after_drain();assert(!owner::stop_requested.load());
    reset();tick();assert(owner::yield(sample_frame)&&!owner::owed()&&state::neutral(state::actual));
    std::puts("Longshot production scope journal: neutral ownership, low/high FOV, once-only R3, uncertain setters, no replay, delayed OFF debt, source/lifetime loss, hold change, stop/yield and idle neutrality passed; stock/state/bridge boundaries injected");
}
