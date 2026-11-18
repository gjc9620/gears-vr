// Cosmetic publication only, consumed by the existing single C0 writer.
// Additional gun-specific owner checks are supplied by the existing owner.
#include "../vr-common/hand_authored_grasp.h"
#include "../vr-common/weapon_two_hand_hold.h"
#include "../vr-common/weapon_boltok_close_support.h"
#include <atomic>
#include <cstdio>
namespace authored_grasp_live {
namespace api=gears_hand_authored_grasp;namespace hand=gears_weapon_hand;
bool additional_owner_current(api::Request const&) noexcept;
bool boltok_close_owner_current(api::Request const&) noexcept;
struct Slot {api::Request request{};std::atomic<DWORD> thread{};std::atomic<bool> armed{};};
Slot slots[2]{},part_fingers{};
// Bounded POD counters in the existing final Native status, never an input or
// permission source. Buckets0..2 are exact press tokens;3 is overflow.
std::atomic<std::uint64_t> finger_observations[4][12]{};
std::atomic<std::uint64_t> finger_copy_rejections[4][12]{};
// Existing final-status diagnostics only. Kind*2+proximity distinguishes a
// neutral socket from its actual held part; never another engine observation.
std::atomic<std::uint64_t> dynamic_observations[8][12]{};
void observe_dynamic(api::Request const& q,unsigned reason) noexcept {
    const auto kind=unsigned(q.kind);
    if(q.side==api::Side::left&&q.mode==api::Mode::left_part&&kind>=1&&kind<=3&&q.proximity<=1&&reason<12)
        ++dynamic_observations[kind*2+q.proximity][reason];
}
void observe_fingers(api::Request const& q,unsigned reason) noexcept {
    if(reason<12)++finger_observations[std::min<std::uint64_t>(q.source.press_token,3)][reason];
}
void summary(FILE* out) noexcept {
    if(!out)return;
    for(unsigned bucket=2;bucket<8;++bucket){
        if(!dynamic_observations[bucket][0].load()&&!dynamic_observations[bucket][2].load())continue;
        std::fprintf(out,"authored_dynamic_observation kind=%u proximity=%u counters=",bucket/2,bucket%2);
        for(unsigned reason=0;reason<12;++reason)std::fprintf(out,"%s%llu",reason?",":"",dynamic_observations[bucket][reason].load());
        std::fprintf(out," diagnostic_only=1\n");
    }
    for(unsigned bucket=0;bucket<4;++bucket){
        std::fprintf(out,"authored_finger_observation token_bucket=%u publish_attempt=%llu publish_accept=%llu target=%llu copy_absent=%llu identity=%llu context=%llu order=%llu edges=%llu rotation=%llu admission=%llu profile=%llu applied=%llu diagnostic_only=1\n",bucket,
            finger_observations[bucket][0].load(),finger_observations[bucket][1].load(),finger_observations[bucket][2].load(),finger_observations[bucket][3].load(),
            finger_observations[bucket][4].load(),finger_observations[bucket][5].load(),finger_observations[bucket][6].load(),finger_observations[bucket][7].load(),
            finger_observations[bucket][8].load(),finger_observations[bucket][9].load(),finger_observations[bucket][10].load(),finger_observations[bucket][11].load());
        std::fprintf(out,"authored_finger_copy_rejections token_bucket=%u stages=",bucket);
        for(unsigned stage=0;stage<12;++stage)std::fprintf(out,"%s%llu",stage?",":"",finger_copy_rejections[bucket][stage].load());
        std::fprintf(out," diagnostic_only=1\n");
    }
}
void invalidate() noexcept {for(auto& s:slots)s.armed.store(false,std::memory_order_release);part_fingers.armed.store(false,std::memory_order_release);}
bool boltok_close_request(api::Request const& q) noexcept {
 return q.profile_id==gears_boltok_close_support::profile&&q.side==api::Side::left&&q.mode==api::Mode::left_part&&
 q.kind==api::object::Kind::support&&!q.proximity&&api::structural(q)&&gears_boltok_close_support::geometry(q.grip);
}
bool admitted(api::Request const& q) noexcept {
    auto* broker=weapon_hand_runtime::current();if(!broker)return false;
    bool okay=false;
    if(q.mode==api::Mode::right_main_fingers)okay=broker->admit_weapon_render(q.source);
    else if(q.proximity)okay=broker->admit_socket_visibility(q.source);
    else if(q.kind==api::object::Kind::support)
        okay=(q.profile_id==gears_boltok_close_support::profile?
            (boltok_close_request(q)&&boltok_close_owner_current(q)):
            gears_weapon_two_hand::geometry(q.grip))&&broker->admit_support_visibility(q.source);
    else okay=broker->admit_part_visibility(q.source);
    return okay&&additional_owner_current(q);
}
bool publish(api::Request const* input) noexcept {
    api::Request q{};
    if(!input||!safe_copy(&q,input,sizeof q)||!api::structural(q)){invalidate();return false;}
    observe_dynamic(q,0);
    auto& slot=slots[unsigned(q.side)];slot.armed.store(false,std::memory_order_release);
    if(!admitted(q))return false;
    slot.request=q;slot.thread.store(GetCurrentThreadId(),std::memory_order_release);slot.armed.store(true,std::memory_order_release);observe_dynamic(q,1);return true;
}
// An independently current finger preset. It never reuses a stale source,
// copies a historical world pose, supplies an action lease or moves the wrist.
bool publish_fingers(api::Request const* input) noexcept {
    // A rejected replacement is not a release of the existing fixed local15
    // descriptor. Its consumer still independently requires the current same
    // grasp/edges/model/owner; no old wrist or permission is retained here.
    api::Request q{};
    if(!input||!safe_copy(&q,input,sizeof q))return false;
    observe_fingers(q,0);
    if(!api::structural(q)||q.side!=api::Side::left||
       q.mode!=api::Mode::left_part||q.proximity||!api::identity(q.grip)||
       (q.kind!=api::object::Kind::magazine&&q.kind!=api::object::Kind::loading&&q.kind!=api::object::Kind::starter)||
       !admitted(q))return false;
    part_fingers.armed.store(false,std::memory_order_release);
    part_fingers.request=q;part_fingers.thread.store(GetCurrentThreadId(),std::memory_order_release);
    observe_fingers(q,1);
    part_fingers.armed.store(true,std::memory_order_release);return true;
}
void target(gears_native_rig_tracking::CoreSnapshot const& source,
    gears_vr::ControllerInputFrame const& current,api::Binding const (&bindings)[2],
    api::Output (&out)[2]) noexcept {
    out[0]={};out[1]={};const auto now=gears_vr::monotonic_ns();
    // Dynamic goals and current finger refresh are independently admitted.
    // The latter may override only rotations of the same active part profile.
    for(unsigned item=0;item<3;++item){
        const bool fingers_only=item==2;const unsigned side=fingers_only?0u:item;
        auto& slot=fingers_only?part_fingers:slots[side];if(!slot.armed.load(std::memory_order_acquire))continue;
        // Off-thread observation can disarm, never mutate GT-owned payload.
        if(GetCurrentThreadId()!=slot.thread.load(std::memory_order_acquire)){slot.armed.store(false,std::memory_order_release);continue;}
        auto q=slot.request;
        if(!fingers_only)observe_dynamic(q,2);
        if(fingers_only){
            observe_fingers(q,2);
            hand::Frame fresh{};auto* broker=weapon_hand_runtime::current();
            // Static local15 registration is not a cached pose/permission.
            // With NO current part grant output stays absent; keep only the
            // descriptor so a later REAL same-grasp current frame can use it.
            // Identity/epochs/control edges below still revoke on mismatch.
            unsigned copy_failure{};
            if(!broker||!broker->copy_part_visibility_frame(fresh,&copy_failure)){
                observe_fingers(q,3);
                ++finger_copy_rejections[std::min<std::uint64_t>(q.source.press_token,3)][std::min(copy_failure,11u)];continue;
            }
            auto const& old=q.source;auto const& a=old.key;auto const& b=fresh.key;
            bool same=a.game_pid==b.game_pid&&a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc&&
                a.registration_cookie==b.registration_cookie&&a.lifecycle==b.lifecycle&&a.world_generation==b.world_generation&&a.thread_id==b.thread_id&&
                hand::same(old.world,fresh.world)&&hand::same(old.controller,fresh.controller)&&hand::same(old.pawn,fresh.pawn)&&
                hand::same(old.mesh,fresh.mesh)&&hand::same(old.weapon,fresh.weapon)&&old.reference_generation==fresh.reference_generation&&
                old.stereo.session==fresh.stereo.session&&old.stereo.space_epoch==fresh.stereo.space_epoch&&
                old.controllers.source==fresh.controllers.source&&old.controllers.session==fresh.controllers.session&&
                old.controllers.space_epoch==fresh.controllers.space_epoch&&old.controllers.input_epoch==fresh.controllers.input_epoch&&
                old.controllers.profile_generation==fresh.controllers.profile_generation&&
                old.press_token==fresh.press_token&&old.interaction_control==fresh.interaction_control&&
                fresh.key.frame>=old.key.frame&&fresh.controllers.sequence>=old.controllers.sequence;
            for(unsigned control:{gears_vr::control_squeeze,gears_vr::control_trigger})
                same=same&&old.controllers.hands[0].pressed[control]==fresh.controllers.hands[0].pressed[control]&&
                    old.controllers.hands[0].released[control]==fresh.controllers.hands[0].released[control];
            if(!same){observe_fingers(q,4);slot.armed.store(false,std::memory_order_release);continue;}
            // Copy real current Native provenance, not old geometry or a renewed
            // permission. This profile contains ONLY15 local finger rotations.
            q.source=fresh;
        }
        auto const& f=q.source;
        const auto reject=[&]() noexcept {slot.armed.store(false,std::memory_order_release);};
        const bool bad_clock=now<=0||f.controllers.sync_ns<=0||now<f.controllers.sync_ns||now-f.controllers.sync_ns>100000000||
           current.sync_ns<=0||now<current.sync_ns||now-current.sync_ns>100000000;
        if(bad_clock||
           source.native.game_start!=f.key.game_start||source.native.native_started_qpc!=f.key.native_started_qpc||
           source.world_generation!=f.key.world_generation||!hand::same(source.world,f.world)||!hand::same(source.pawn,f.pawn)||
           !hand::same(source.mesh,f.mesh)||!hand::same(source.weapon,f.weapon)||source.stereo.session!=f.stereo.session||
           source.stereo.space_epoch!=f.stereo.space_epoch||source.reference_generation!=f.reference_generation||
           current.input_epoch!=f.controllers.input_epoch||
           current.profile_generation!=f.controllers.profile_generation){if(fingers_only)observe_fingers(q,5);else observe_dynamic(q,bad_clock?3u:4u);reject();continue;}
        if(current.sequence<f.controllers.sequence){
            if(fingers_only)observe_fingers(q,6);
            else observe_dynamic(q,5);
            // A current C0 observation can precede the separately committed
            // part frame. Do not publish that newer provenance into this C0.
            // It also is not a release of the fixed local15 descriptor: a
            // later observation must independently pass every existing gate.
            if(!fingers_only)reject();
            continue;
        }
        auto const& input=current.hands[side];auto const& original=f.controllers.hands[side];bool edges=true;
        for(unsigned control:{gears_vr::control_squeeze,gears_vr::control_trigger})
            edges=edges&&input.pressed[control]==original.pressed[control]&&input.released[control]==original.released[control];
        if(!edges||!gears_vr::controller_pose_usable(input.grip,input.grip_flags,input.grip_active)){if(fingers_only)observe_fingers(q,7);else observe_dynamic(q,6);reject();continue;}
        if(side==0){
            if(q.proximity){if(input.squeeze_active!=1||hand::control_held(input,gears_vr::control_squeeze)){if(!fingers_only)observe_dynamic(q,7);reject();continue;}}
            else if(!hand::control_held(input,hand::left_control(f))){if(!fingers_only)observe_dynamic(q,7);reject();continue;}
        }
        double trigger=0;
        if(q.has_trigger_endpoint){
            if(input.trigger_active!=1||!std::isfinite(input.trigger)||input.trigger<0||input.trigger>1){reject();continue;}
            trigger=input.trigger;
        }
        api::Joints local{};
        if(!api::rotations(q,bindings[side],trigger,local)){if(fingers_only)observe_fingers(q,8);else observe_dynamic(q,8);reject();continue;}
        if(!admitted(q)){
            if(fingers_only)observe_fingers(q,9);
            else observe_dynamic(q,9);
            // The independent second permission/owner check can fail after
            // copying a part frame (including expiry during that check).
            // Emit nothing. A fixed local15 descriptor is still authoring
            // data, not the failed frame's pose or permission. Revalidate it
            // against a genuinely current same-grasp frame next time.
            if(!fingers_only)reject();
            continue;
        }
        if(!slot.armed.load(std::memory_order_acquire))continue;
        auto& result=out[side];
        if(fingers_only){
            // Independently refresh only the SAME part profile. A current
            // dynamic wrist goal and its provenance are never relabelled.
            if(result.known&&(result.profile_id!=q.profile_id||result.left.kind!=q.kind)){observe_fingers(q,10);continue;}
            if(!result.known){result.known=true;result.mode=q.mode;result.source=f;result.profile_id=q.profile_id;}
            result.local=local;result.finger_override=true;result.finger_kind=q.kind;observe_fingers(q,11);continue;
        }
        result.known=true;result.mode=q.mode;result.source=f;result.profile_id=q.profile_id;result.local=local;
        observe_dynamic(q,10);
        if(side==0){
            result.left.kind=q.kind;result.left.grip=q.grip;result.left.weight=1.;result.left.passive_socket=q.proximity!=0;
            result.left.preset=api::object::Preset::authored;
            result.left.authored_pose=local;result.left.has_authored_pose=true;
        }
    }
}
}
