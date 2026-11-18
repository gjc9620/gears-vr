// A typed cosmetic publication, consumed by the existing sole C0 hand solve.
// No independent bone writer or gameplay/input/action authority is created.
#include "../vr-common/weapon_lancer_natural_hand.h"
#include "../vr-common/weapon_lancer_support_socket.h"
#include "../vr-common/hand_authored_grasp.h"
#include "../vr-common/weapon_lancer_part_hand.h"
namespace lancer_handle_morph_live {
bool grasp_owner_current(gears_weapon_hand::Frame const&) noexcept;
}
namespace lancer_grasp_live {
namespace api=gears_hand_object_grasp;namespace hand=gears_weapon_hand;
namespace socket=gears_lancer_support_socket;
bool socket_owner_current(socket::Descriptor const&,hand::Frame const&) noexcept;
bool starter_socket_binding_current(socket::Descriptor const&,hand::Frame const&) noexcept;
bool starter_socket_owner_current(socket::Descriptor const&,hand::Frame const&) noexcept;
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
api::Request requested{};api::Preset requested_preset{};bool available{};DWORD thread{};
socket::Descriptor socket_descriptor{};hand::Frame socket_current_frame{};
api::Kind socket_kind{api::Kind::support};
std::atomic<bool> socket_registered{};std::atomic<DWORD> socket_thread{};
std::atomic<std::uint64_t> socket_registrations{},socket_targets{},socket_admitted{};
std::atomic<std::uint64_t> starter_socket_registrations{},starter_socket_targets{},starter_socket_admitted{};
bool publish_socket(socket::Descriptor const* input) noexcept {
    socket::Descriptor d{};auto* broker=weapon_hand_runtime::current();
    if(!input||!safe_copy(&d,input,sizeof d)||!socket::structural(d)||!broker||
       !broker->admit_socket_visibility(d.binding))return false;
    socket_registered.store(false,std::memory_order_release);socket_descriptor=d;
    socket_kind=api::Kind::support;
    socket_thread.store(GetCurrentThreadId(),std::memory_order_release);
    available=false;socket_registered.store(true,std::memory_order_release);++socket_registrations;return true;
}
bool publish_starter_socket(socket::Descriptor const* input) noexcept {
    socket::Descriptor d{};auto* broker=weapon_hand_runtime::current();
    if(!input||!safe_copy(&d,input,sizeof d)||!socket::structural(d)||!broker||
       !broker->admit_socket_visibility(d.binding)||!starter_socket_binding_current(d,d.binding))return false;
    socket_registered.store(false,std::memory_order_release);socket_descriptor=d;
    socket_kind=api::Kind::starter;
    socket_thread.store(GetCurrentThreadId(),std::memory_order_release);
    available=false;socket_registered.store(true,std::memory_order_release);++starter_socket_registrations;return true;
}
struct SocketRejection {unsigned gate{};std::uint64_t frame{},sequence{};std::int64_t observed_ns{};};
SocketRejection first_socket_rejection{};std::atomic<bool> socket_rejection_known{};std::atomic<std::uint64_t> socket_rejected[8]{};
bool socket_target(gears_native_rig_tracking::CoreSnapshot const& source,
    gears_vr::ControllerInputFrame const& current,api::Target& out) noexcept {
    if(!socket_registered.load(std::memory_order_acquire)||GetCurrentThreadId()!=socket_thread.load(std::memory_order_acquire))return false;
    const bool starter=socket_kind==api::Kind::starter;
    if(starter)++starter_socket_targets;else ++socket_targets;
    const auto reject=[&](unsigned gate){
        if(!starter&&gate<8){++socket_rejected[gate];
            if(socket_admitted.load()&&!socket_rejection_known.load(std::memory_order_acquire)){
                first_socket_rejection={gate,socket_current_frame.key.frame,current.sequence,gears_vr::monotonic_ns()};
                socket_rejection_known.store(true,std::memory_order_release);}}
        return true;};
    auto* broker=weapon_hand_runtime::current();
    if(!broker||!broker->copy_socket_visibility_frame(socket_current_frame))return reject(1);
    const auto& f=socket_current_frame;const auto& d=socket_descriptor;const auto now=gears_vr::monotonic_ns();
    if(!socket::binding_matches(d,f))return reject(2);
    if(!(starter?starter_socket_owner_current(d,f):socket_owner_current(d,f)))return reject(3);
    if(now<=0||current.sync_ns<=0||now<current.sync_ns||now-current.sync_ns>100000000)return reject(4);
    if(
       source.native.game_start!=f.key.game_start||source.native.native_started_qpc!=f.key.native_started_qpc||
       source.world_generation!=f.key.world_generation||!hand::same(source.world,f.world)||!hand::same(source.pawn,f.pawn)||
       !hand::same(source.mesh,f.mesh)||!hand::same(source.weapon,f.weapon)||source.stereo.session!=f.stereo.session||
       source.stereo.space_epoch!=f.stereo.space_epoch||source.stereo.source!=f.stereo.source||
       source.reference_generation!=f.reference_generation||
       current.source!=f.controllers.source||current.session!=f.controllers.session||
       current.space_epoch!=f.controllers.space_epoch||
       current.sequence<f.controllers.sequence||current.input_epoch!=f.controllers.input_epoch||
       current.profile_generation!=f.controllers.profile_generation)return reject(5);
    if(current.hands[0].squeeze_active!=1||
       hand::control_held(current.hands[0],gears_vr::control_squeeze)||
       !gears_vr::controller_pose_usable(current.hands[0].grip,current.hands[0].grip_flags,current.hands[0].grip_active)||
       !gears_vr::controller_pose_usable(current.hands[1].grip,current.hands[1].grip_flags,current.hands[1].grip_active))return reject(6);
    if(!broker->admit_socket_visibility(f))return reject(7);
    out={socket_kind,d.in_main_grip,1.,true,starter?api::Preset::authored:api::Preset::support_surface};
    if(starter)std::copy_n(gears_lancer_part_hand::starter.begin(),15,out.authored_pose.begin());
    else std::copy_n(gears_lancer_natural_hand::support_closed,15,out.authored_pose.begin());
    out.has_authored_pose=true;out.passive_relative_socket=true;
    if(starter)++starter_socket_admitted;else ++socket_admitted;return true;
}
std::int64_t began{};
// Bounded provenance in the EXISTING Native status receipt. No per-frame log,
// extra game/rig read or permission is created. Only the first rejected ACTIVE
// support target during the SAME still-held physical press is retained;
// passive hover and an ordinary release cannot consume this witness.
std::atomic<std::uint64_t> support_requests{},support_targets{},support_admitted{};
struct SupportRejection {
    unsigned gate{};std::uint64_t frame{},request_sequence{},current_sequence{};
    std::int64_t observed_ns{},request_sync_ns{};
};
SupportRejection first_support_rejection{};std::atomic<bool> support_rejection_known{};
// Same bounded status receipt, separate neutral support provenance. Only a
// rejection AFTER a successful passive admission can claim this first slot.
// No engine reads, request lifetime or cosmetic/action admission is changed.
std::atomic<std::uint64_t> passive_support_requests{},passive_support_targets{},passive_support_admitted{};
SupportRejection first_passive_support_rejection{};std::atomic<bool> passive_support_rejection_known{};
void summary(FILE* output) noexcept {
    if(!output)return;
    fprintf(output,"body_support_socket_registrations=%llu body_support_socket_targets=%llu body_support_socket_admitted=%llu ",socket_registrations.load(),socket_targets.load(),socket_admitted.load());
    fprintf(output,"body_starter_socket_registrations=%llu body_starter_socket_targets=%llu body_starter_socket_admitted=%llu ",starter_socket_registrations.load(),starter_socket_targets.load(),starter_socket_admitted.load());
    const auto socket_first=socket_rejection_known.load(std::memory_order_acquire)?first_socket_rejection:SocketRejection{};
    fprintf(output,"body_support_socket_first_gate=%u body_support_socket_first_frame=%llu body_support_socket_first_sequence=%llu body_support_socket_first_ns=%lld ",socket_first.gate,socket_first.frame,socket_first.sequence,socket_first.observed_ns);
    for(unsigned g=1;g<8;++g)fprintf(output,"body_support_socket_rejected_%u=%llu ",g,socket_rejected[g].load());
    const bool known=support_rejection_known.load(std::memory_order_acquire);
    const auto first=known?first_support_rejection:SupportRejection{};
    fprintf(output,"body_support_requests=%llu body_support_targets=%llu body_support_admitted=%llu "
        "body_support_first_gate=%u body_support_first_frame=%llu body_support_first_request_sequence=%llu "
        "body_support_first_current_sequence=%llu body_support_first_observed_ns=%lld body_support_first_sync_ns=%lld ",
        support_requests.load(),support_targets.load(),support_admitted.load(),first.gate,first.frame,
        first.request_sequence,first.current_sequence,first.observed_ns,first.request_sync_ns);
    const bool passive_known=passive_support_rejection_known.load(std::memory_order_acquire);
    const auto passive=passive_known?first_passive_support_rejection:SupportRejection{};
    fprintf(output,"body_passive_support_requests=%llu body_passive_support_targets=%llu body_passive_support_admitted=%llu "
        "body_passive_support_first_gate=%u body_passive_support_first_frame=%llu "
        "body_passive_support_first_request_sequence=%llu body_passive_support_first_current_sequence=%llu "
        "body_passive_support_first_observed_ns=%lld body_passive_support_first_sync_ns=%lld ",
        passive_support_requests.load(),passive_support_targets.load(),passive_support_admitted.load(),passive.gate,passive.frame,
        passive.request_sequence,passive.current_sequence,passive.observed_ns,passive.request_sync_ns);
}
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
bool publish_value(api::Request const& q,api::Preset preset,bool extended) noexcept {
    auto* broker=weapon_hand_runtime::current();
    if(q.size!=sizeof q||q.version!=2||q.proximity>1||!api::valid_for(q.kind,preset)||
       q.kind==api::Kind::none||unsigned(q.kind)>unsigned(api::Kind::support)||
       !gears_vr::stereo_pose_valid(q.grip)||!broker)return false;
    if(q.proximity){
        if((q.kind!=api::Kind::support&&q.kind!=api::Kind::starter&&!(extended&&q.kind==api::Kind::magazine))||
           !broker->admit_socket_visibility(q.source))return false;
    }else if(q.kind==api::Kind::support){
        if(!gears_weapon_two_hand::geometry(q.grip)||!broker->admit_support_visibility(q.source))return false;
    }else if(!broker->admit_part_visibility(q.source))return false;
    // Each supported part supplies its own exact original-Lancer binding;
    // the fresh broker admission above remains mandatory for every kind.
    if(!owner_current(q.kind,q.source))return false;
    const bool same=available&&thread==GetCurrentThreadId()&&requested.kind==q.kind&&requested.proximity==q.proximity&&requested_preset==preset&&
        requested.source.press_token==q.source.press_token&&hand::same(requested.source.weapon,q.source.weapon)&&
        requested.source.key.registration_cookie==q.source.key.registration_cookie&&
        requested.source.key.lifecycle==q.source.key.lifecycle;
    socket_registered.store(false,std::memory_order_release);
    if(!same)began=gears_vr::monotonic_ns();requested=q;requested_preset=preset;thread=GetCurrentThreadId();available=true;
    if(q.kind==api::Kind::support&&!q.proximity)++support_requests;
    if(q.kind==api::Kind::support&&q.proximity)++passive_support_requests;
    return true;
}
bool publish(api::Request const* input) noexcept {
    api::Request q{};
    return input&&safe_copy(&q,input,sizeof q)&&publish_value(q,api::Preset::automatic,false);
}
bool publish_part(gears_hand_part_grasp::Request const* input) noexcept {
    gears_hand_part_grasp::Request q{};
    if(!input||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=3||q.reserved||
       !api::valid(q.preset))return false;
    const bool magazine=q.pose.kind==api::Kind::magazine&&
        (q.preset==api::Preset::precision||q.preset==api::Preset::cylinder);
    const bool support=q.pose.kind==api::Kind::support&&q.preset==api::Preset::support_surface;
    if(!magazine&&!support)return false;
    return publish_value(q.pose,q.preset,true);
}
void target(gears_native_rig_tracking::CoreSnapshot const& source,
    gears_vr::ControllerInputFrame const& current,gears_hand_object_grasp::Target& out) noexcept {
    out={};if(socket_target(source,current,out))return;
    if(!available||GetCurrentThreadId()!=thread)return;
    auto const& f=requested.source;const auto now=gears_vr::monotonic_ns();
    auto* broker=weapon_hand_runtime::current();
    const bool active_support=requested.kind==api::Kind::support&&!requested.proximity;
    const bool passive_support=requested.kind==api::Kind::support&&requested.proximity;
    if(active_support)++support_targets;
    if(passive_support)++passive_support_targets;
    const auto rejected=[&](unsigned gate) noexcept {
        if(passive_support&&passive_support_admitted.load(std::memory_order_relaxed)&&
           !passive_support_rejection_known.load(std::memory_order_relaxed)){
            first_passive_support_rejection={gate,f.key.frame,f.controllers.sequence,current.sequence,now,f.controllers.sync_ns};
            passive_support_rejection_known.store(true,std::memory_order_release);
        }
        const auto control=hand::left_control(f);
        if(active_support&&hand::control_held(current.hands[0],control)&&
            current.hands[0].pressed[control]==f.controllers.hands[0].pressed[control]&&
            current.hands[0].released[control]==f.controllers.hands[0].released[control]&&
            !support_rejection_known.load(std::memory_order_relaxed)){
            first_support_rejection={gate,f.key.frame,f.controllers.sequence,current.sequence,now,f.controllers.sync_ns};
            support_rejection_known.store(true,std::memory_order_release);
        }
    };
    if(now<=0||now<f.controllers.sync_ns||now-f.controllers.sync_ns>100000000){rejected(1);return;}
    if(source.native.game_start!=f.key.game_start||source.native.native_started_qpc!=f.key.native_started_qpc||
       source.world_generation!=f.key.world_generation||!hand::same(source.world,f.world)||!hand::same(source.pawn,f.pawn)||
       !hand::same(source.mesh,f.mesh)||!hand::same(source.weapon,f.weapon)||
       source.stereo.session!=f.stereo.session||source.stereo.space_epoch!=f.stereo.space_epoch||
       source.reference_generation!=f.reference_generation||current.sequence<f.controllers.sequence||
       current.input_epoch!=f.controllers.input_epoch||current.profile_generation!=f.controllers.profile_generation){rejected(2);return;}
    if(current.hands[0].pressed[hand::left_control(f)]!=f.controllers.hands[0].pressed[hand::left_control(f)]||
       current.hands[0].released[hand::left_control(f)]!=f.controllers.hands[0].released[hand::left_control(f)]||
       !broker||
       !owner_current(requested.kind,f)){rejected(3);return;}
    if(requested.proximity){
        if(current.hands[0].squeeze_active!=1||hand::control_held(current.hands[0],gears_vr::control_squeeze)||
           !broker->admit_socket_visibility(f)){rejected(7);return;}
    }else{
        if(!hand::control_held(current.hands[0],hand::left_control(f))){rejected(4);return;}
        if(!(requested.kind==api::Kind::support?broker->admit_support_visibility(f):broker->admit_part_visibility(f))){rejected(5);return;}
    }
    if(!gears_vr::controller_pose_usable(current.hands[0].grip,current.hands[0].grip_flags,current.hands[0].grip_active)){rejected(6);return;}
    if(active_support)++support_admitted;
    if(passive_support)++passive_support_admitted;
    out={requested.kind,requested.grip,1.,requested.proximity!=0,requested_preset}; // C0 owns the120ms transition.
    if(requested_preset==api::Preset::support_surface){
        std::copy_n(gears_lancer_natural_hand::support_closed,15,out.authored_pose.begin());
        out.has_authored_pose=true;
    }
}
#else
bool publish_socket(socket::Descriptor const*) noexcept{return false;}
bool publish_starter_socket(socket::Descriptor const*) noexcept{return false;}
bool publish(api::Request const*) noexcept{return false;}
bool publish_part(gears_hand_part_grasp::Request const*) noexcept{return false;}
void target(gears_native_rig_tracking::CoreSnapshot const&,gears_vr::ControllerInputFrame const&,api::Target& out) noexcept{out={};}
void summary(FILE*) noexcept{}
#endif
}
