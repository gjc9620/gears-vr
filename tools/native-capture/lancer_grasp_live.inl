// A typed cosmetic publication, consumed by the existing sole C0 hand solve.
// No independent bone writer or gameplay/input/action authority is created.
namespace lancer_handle_morph_live {
bool grasp_owner_current(gears_weapon_hand::Frame const&) noexcept;
}
namespace lancer_grasp_live {
namespace api=gears_hand_object_grasp;namespace hand=gears_weapon_hand;
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
api::Request requested{};bool available{};DWORD thread{};
std::int64_t began{};
bool owner_current(api::Kind kind,hand::Frame const& f) noexcept {
    // The front charging handle has its own certified asset/render owner.
    // Requiring the unrelated C-starter lease here silently discarded the
    // charging hand fit after an original animation had returned C's bone.
    if(kind==api::Kind::loading)return lancer_handle_morph_live::grasp_owner_current(f);
    if(kind==api::Kind::support||kind==api::Kind::magazine)return true;
    // These kinds use their own exact support / magazine-ammo broker grant
    // below. Magazine fitting must not depend on an unrelated C/E render
    // lease; fresh object matrices and the current part grant still bind it.
    const bool starter=lancer_original_handle_live::retained.load(std::memory_order_acquire)&&
        !lancer_original_handle_live::bound.pose.restoring()&&
        hand::same(f.weapon,lancer_original_handle_live::bound.weapon)&&
        hand::same(f.world,lancer_original_handle_live::bound.world)&&
        hand::same(f.pawn,lancer_original_handle_live::bound.pawn)&&
        f.key.registration_cookie==lancer_original_handle_live::bound.key.registration_cookie&&
        f.key.lifecycle==lancer_original_handle_live::bound.key.lifecycle&&
        f.key.native_started_qpc==lancer_original_handle_live::bound.key.native_started_qpc;
    if(kind==api::Kind::starter)return starter;
    return false;
}
bool publish(api::Request const* input) noexcept {
    api::Request q{};auto* broker=weapon_hand_runtime::current();
    if(!input||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=1||
       q.kind==api::Kind::none||unsigned(q.kind)>unsigned(api::Kind::support)||
       !gears_vr::stereo_pose_valid(q.grip)||!broker)return false;
    if(q.kind==api::Kind::support){
        if(!gears_weapon_two_hand::geometry(q.grip)||!broker->admit_support_visibility(q.source))return false;
    }else if(!broker->admit_part_visibility(q.source))return false;
    // Each supported part supplies its own exact original-Lancer binding;
    // the fresh broker admission above remains mandatory for every kind.
    if(!owner_current(q.kind,q.source))return false;
    const bool same=available&&thread==GetCurrentThreadId()&&requested.kind==q.kind&&
        requested.source.press_token==q.source.press_token&&hand::same(requested.source.weapon,q.source.weapon)&&
        requested.source.key.registration_cookie==q.source.key.registration_cookie&&
        requested.source.key.lifecycle==q.source.key.lifecycle;
    if(!same)began=gears_vr::monotonic_ns();requested=q;thread=GetCurrentThreadId();available=true;return true;
}
void target(gears_native_rig_tracking::CoreSnapshot const& source,
    gears_vr::ControllerInputFrame const& current,gears_hand_object_grasp::Target& out) noexcept {
    out={};if(!available||GetCurrentThreadId()!=thread)return;
    auto const& f=requested.source;const auto now=gears_vr::monotonic_ns();
    auto* broker=weapon_hand_runtime::current();
    if(now<=0||now<f.controllers.sync_ns||now-f.controllers.sync_ns>100000000||
       source.native.game_start!=f.key.game_start||source.native.native_started_qpc!=f.key.native_started_qpc||
       source.world_generation!=f.key.world_generation||!hand::same(source.world,f.world)||!hand::same(source.pawn,f.pawn)||
       !hand::same(source.mesh,f.mesh)||!hand::same(source.weapon,f.weapon)||
       source.stereo.session!=f.stereo.session||source.stereo.space_epoch!=f.stereo.space_epoch||
       source.reference_generation!=f.reference_generation||current.sequence<f.controllers.sequence||
       current.input_epoch!=f.controllers.input_epoch||current.profile_generation!=f.controllers.profile_generation||
       current.hands[0].pressed[hand::left_control(f)]!=f.controllers.hands[0].pressed[hand::left_control(f)]||
       current.hands[0].released[hand::left_control(f)]!=f.controllers.hands[0].released[hand::left_control(f)]||
       !hand::control_held(current.hands[0],hand::left_control(f))||
       !broker||!(requested.kind==api::Kind::support?broker->admit_support_visibility(f):broker->admit_part_visibility(f))||
       !owner_current(requested.kind,f))return;
    if(!gears_vr::controller_pose_usable(current.hands[0].grip,current.hands[0].grip_flags,current.hands[0].grip_active))return;
    out={requested.kind,requested.grip,1.}; // The C0-owned skin blender owns the120ms transition.
}
#else
bool publish(api::Request const*) noexcept{return false;}
void target(gears_native_rig_tracking::CoreSnapshot const&,gears_vr::ControllerInputFrame const&,api::Target& out) noexcept{out={};}
#endif
}
