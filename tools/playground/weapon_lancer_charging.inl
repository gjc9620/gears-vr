// Owner-marked FRONT charging handle. B has no player interaction.
// Actual physical hand travel drives only the private original-vertex morph;
// observed render uploads, release and mechanical return gate completion.
#include "../vr-common/weapon_lancer_owner_grabposes.h"
namespace lancer_charging {
namespace hand=gears_weapon_hand;
namespace inv=lancerreload_inventory;
using P=gears_vr::Pose;using Q=gears_vr::Quaternion;using V=gears_vr::Vector;
V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
double length(V a) noexcept {return std::hypot(std::hypot(a.x,a.y),a.z);}
// Semantic ammunition state, not an old input grant. Insertion can outlive
// stow, recenter or tracking loss. None of those may erase an uncharged gun.
struct Debt {hand::ObjectIdentity world{},pawn{},weapon{};std::uint64_t game_start{};bool required{};};
std::array<Debt,32> debts{};
bool same(Debt const& d,hand::Frame const& f) noexcept {
    return d.required&&d.game_start==f.key.game_start&&hand::same(d.world,f.world)&&
        hand::same(d.pawn,f.pawn)&&hand::same(d.weapon,f.weapon);
}
bool required(hand::Frame const& f) noexcept {for(const auto& d:debts)if(same(d,f))return true;return false;}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    for(const auto& d:debts)if(d.required&&hand::same(d.weapon,weapon))return true;return false;
}
bool can_mark(hand::Frame const& f) noexcept {
    if(required(f))return true;
    for(const auto& d:debts)if(!d.required||d.game_start!=f.key.game_start||!hand::same(d.world,f.world))return true;
    return false;
}
bool mark(hand::Frame const& f) noexcept {
    if(required(f))return true;
    for(auto& d:debts)if(!d.required||d.game_start!=f.key.game_start||!hand::same(d.world,f.world)){
        d={f.world,f.pawn,f.weapon,f.key.game_start,true};return true;
    }return false;
}
void clear(hand::Frame const& f) noexcept {for(auto& d:debts)if(same(d,f))d={};}
void clear_uncommitted(inv::Identity const& id) noexcept {
    for(auto& d:debts)if(d.required&&d.game_start==id.game_start&&hand::same(d.world,id.world)&&
        hand::same(d.pawn,id.pawn)&&hand::same(d.weapon,id.weapon))d={};
}
struct Geometry {
    inv::Identity identity{};hand::ObjectIdentity component{},asset{};
    gears_weapon::SnubSlideGeometry slide{};bool known{};
};
Geometry geometry;
struct Ownership {inv::Identity identity{};bool active{},retiring{};};
Ownership ownership;
gears_weapon::lancer_front::Stroke stroke;
hand::Lease held{};bool release_seen{},armed{},enabled{};
inv::Identity pending_identity{};std::uint64_t pending_frame{};std::int64_t pending_until{};
hand::FrameKey contact_key{};std::uint64_t contact_token{};bool contact_proposed{};
std::uint64_t last_sample{},last_pose_frame{};unsigned failure{},pose_rows{};
double last_observed{};
bool current_geometry(hand::Frame const& f) noexcept;
struct ObservedRegionPart {
    inv::Identity identity{};double raw{};std::int64_t at_ns{};std::uint64_t upload{},owner_handle{};bool known{};
};
ObservedRegionPart region_part;
bool authored_contact(hand::Frame const& f,P const& local_hand,double raw_distance,std::int64_t observed_ns,
    std::uint64_t upload,std::int64_t now,double mesh_scale) noexcept {
    const auto region=gears_lancer_workbench_profile::charging();
    if(!region||!current_geometry(f)||!upload||observed_ns<=0||now<observed_ns||now-observed_ns>100000000||
       !std::isfinite(raw_distance)||raw_distance<0||raw_distance>gears_weapon::lancer_front::travel_raw+.001)return false;
    const double units=nativelancerfollow::bound_units;
    if(!gears_lancer_authored_volume::contains_moving_asset_point(*region,mul(local_hand.position,units),
        {{0,0,raw_distance*.01},{}}))return false;
    if(!gears_lancer_workbench_profile::charging_snap_limits_enabled)return true;
    if(!std::isfinite(mesh_scale)||mesh_scale<.1||mesh_scale>10)return false;
    // Same neutral physical left mount as existing Lancer support, expressed
    // in the current mesh frame. The cosmetic fitted wrist is never input.
    gears_hand_authored_mount::Mount mount{};
    if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
        gears_hand_authored_mount::Side::left,1,units,mount))return false;
    P physical{};
    if(!gears_lancer_authored_volume::physical_wrist_in_asset({mul(local_hand.position,units),local_hand.orientation},
        {mount.calibration.grip_to_wrist_metres,mount.calibration.grip_to_wrist_rotation},units,mesh_scale,physical))return false;
    P target=gears_lancer_owner_grabposes::loading_wrist_in_gun;target.position.y-=raw_distance;
    return gears_lancer_authored_volume::wrist_snap(*region,physical,target,units/mesh_scale);
}
unsigned contact_gate{},local_gate{};double contact_distance{-1};std::uint64_t contact_sequence{};
unsigned capture_gate{};uptr capture_node{};std::array<wchar_t,64> capture_name{};
hand::Decision last_decision{};
bool current_geometry(hand::Frame const& f) noexcept;
struct ContactObservation {std::uint64_t frame{},token{},held_id{};unsigned flags{},gate{};};
std::array<ContactObservation,24> contact_observations{};unsigned contact_observation_count{},contact_observation_total{};
ContactObservation last_contact_observation{};bool last_contact_known{};
void contact_observed(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    const unsigned flags=unsigned(enabled)|(unsigned(required(f))<<1)|(unsigned(current_geometry(f))<<2)|
        (unsigned(game.main_hand_held)<<3)|(unsigned(game.busy)<<4)|(unsigned(game.animation_owns_hands)<<5)|
        (unsigned(f.left_tracked)<<6)|(unsigned(f.left_grip_known)<<7)|(unsigned(f.left_grip_down)<<8)|
        (unsigned(pending_identity==inv::identity(f))<<9);
    if(last_contact_known){const auto& prior=last_contact_observation;
        if(prior.flags==flags&&prior.gate==contact_gate&&prior.token==f.press_token&&prior.held_id==held.id)return;}
    last_contact_observation={f.key.frame,f.press_token,held.id,flags,contact_gate};last_contact_known=true;
    if(contact_observation_count<contact_observations.size()&&contact_observation_total<96){
        contact_observations[contact_observation_count++]=last_contact_observation;++contact_observation_total;
    }
}
bool current_geometry(hand::Frame const& f) noexcept {
    return geometry.known&&geometry.identity==inv::identity(f)&&nativelancerfollow::bound_key_known&&
        nativelancerfollow::bound_is_lancer&&hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        geometry.identity.hold_generation==nativelancerfollow::bound_key.hold_generation;
}
bool discover(hand::Frame const& f) noexcept {
    failure=20;
    if(!required(f)||!nativelancerfollow::bound_is_lancer||!nativeequipment::current(nullptr,f)||
       !lancerhandle_visual::ready_for(f))return false;
    lancermain_asset::Evidence actual{};failure=21;
    if(lancermain_asset::read(f,actual)!=lancermain_asset::Status::ready)return false;
    Geometry g{};g.identity=inv::identity(f);g.component=actual.component;g.asset=actual.asset;
    failure=22;if(!gears_weapon::lancer_front::geometry(nativelancerfollow::bound_units,g.slide))return false;
    g.known=true;geometry=g;return current_geometry(f);
}
bool local_hand(hand::Frame const& f,P& left,std::uint64_t& sequence,std::uint64_t& ms,double& mesh_scale) noexcept {
    local_gate=1;
    // This also runs inside Native's proposal callback. Its Frame is
    // already authenticated there; recursively copying/observing that same
    // frame would invalidate the in-progress arbitration. The ordinary GT
    // step validates current again before consuming motion or dispatching.
    if(!current_geometry(f)||!f.left_tracked||
       !f.controllers.sequence||f.controllers.sync_ns<=0)return false;
    // Physical interaction follows this authenticated controller pair, as
    // in the existing Hammerburst path. A later rendered/IK hand endpoint
    // can be absent or constrained; it must not discard current tracking.
    // Both grips and the fixed gun mount share one basis, so the common
    // head/body transform cancels. No old hand sample or rendered pose is
    // reused. Actual render-upload observations certify travel/return below.
    local_gate=2;
    for(const auto& h:f.controllers.hands)if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;
    const double units=nativelancerfollow::bound_units;
    local_gate=4;if(!std::isfinite(units)||units<10||units>500)return false;
    const auto convert=[&](P const& p){return gears_vr::Viewpoint{mul(gears_vr::xr_to_ue(p.position),units),
        gears_vr::ue_angles(gears_vr::xr_to_ue(gears_vr::rotation_matrix(p.orientation)))};};
    const auto l=convert(f.controllers.hands[0].grip),r=convert(f.controllers.hands[1].grip);
    std::array<float,16> raw{};P gun{};double size{};
    local_gate=3;if(!nativelancerfollow::mount.model_matrix(r,raw)||!hammerburst_runtime::matrix(raw,gun,size))return false;
    const auto inverse=gears_vr::inverse(gun.orientation);
    Q lq{};if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(l.rotation),lq))return false;
    left={mul(gears_weapon::rotate(inverse,sub(l.position,gun.position)),1/(size*units)),
        gears_vr::normalized(gears_vr::multiply(inverse,lq))};
    sequence=f.controllers.sequence;ms=std::uint64_t(f.controllers.sync_ns/1000000);
    local_gate=5;if(!sequence||!ms||!gears_vr::stereo_pose_valid(left))return false;
    mesh_scale=size;local_gate=0;return true;
}
// Arbitration can precede the runtime consumer. Retain only this granted
// token, never a hand pose, so a new current publication can finish the same
// accepted grasp while it remains at the actual mechanism.
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    last_decision=d;
    if(!enabled||!required(f)||!current_geometry(f)||!release_seen||armed||
       stroke.phase()!=gears_weapon::lancer_front::Stroke::Phase::idle||!f.left_tracked||!f.left_grip_down||
       !contact_proposed||!hand::same(contact_key,f.key)||contact_token!=f.press_token||
       !d.new_grant||d.release_required||d.status!=hand::DecisionStatus::granted||
       d.lease.kind!=hand::LeaseKind::magazine||!hand::same(d.key,f.key)||
       !hand::lease_valid(d.lease,f.key.lifecycle)||d.lease.id!=f.press_token||
       f.controllers.sync_ns<=0||f.controllers.sync_ns>INT64_MAX-250000000)return;
    held=d.lease;pending_identity=inv::identity(f);pending_frame=f.key.frame;pending_until=f.controllers.sync_ns+250000000;
}
bool fresh_grant(hand::Frame const& f,hand::Decision const& d) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    const bool prepared=pending_frame&&f.key.frame>=pending_frame&&now>0&&now<=pending_until&&
        pending_identity==inv::identity(f)&&hand::same(held,d.lease);
    return (d.new_grant||prepared)&&!d.release_required&&hand::same(d.key,f.key)&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==hand::LeaseKind::magazine&&d.lease.id==f.press_token&&hand::lease_valid(d.lease,f.key.lifecycle);
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    struct Observe {hand::Frame const& f;hand::GripGameState const& game;~Observe(){contact_observed(f,game);}} observe{f,game};
    contact_gate=1;contact_distance=-1;contact_sequence=0;
    if(!enabled||!required(f)||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !f.left_tracked||!f.left_grip_known||!f.left_grip_down)return;
    // A current accepted hand grant is an incumbent even while the gun's
    // render geometry is being republished. Contact is required to ACQUIRE,
    // never to hide that same lease from Native's sole arbiter next frame.
    // This does not authorize motion: step still requires new actual poses.
    const auto now=weaponhand::clock_ns(nullptr);
    const bool pending_live=pending_frame&&now>0&&now<=pending_until;
    if(pending_frame==f.key.frame&&held.kind==hand::LeaseKind::magazine&&held.id==f.press_token&&
       hand::lease_valid(held,f.key.lifecycle)&&pending_identity==inv::identity(f)&&(armed||pending_live)){
        // Re-observation of this already granted frame keeps its original
        // candidate bit even after step arms the mechanism. The next frame
        // reports the incumbent; no second grant or cached hand pose is made.
        game.request_magazine=true;contact_gate=7;return;
    }
    // As with the magazine participant, publish the newly accepted lease
    // only AFTER its grant frame. Re-observing that very frame must still
    // return its original contact proposal, not mutate it into an incumbent.
    contact_gate=2;if(pending_frame&&f.key.frame>pending_frame&&
       held.kind==hand::LeaseKind::magazine&&held.id&&held.id==f.press_token&&
       hand::lease_valid(held,f.key.lifecycle)&&pending_identity==inv::identity(f)&&(armed||pending_live)){
        for(auto& item:game.object_incumbents){if(hand::same(item,held))return;
            if(item.kind==hand::LeaseKind::none){item=held;return;}}
        return;
    }
    contact_gate=6;if(!current_geometry(f))return;
    contact_gate=3;if(!release_seen||stroke.phase()!=gears_weapon::lancer_front::Stroke::Phase::idle)return;
    P left{};std::uint64_t seq{},ms{};double mesh_scale{};
    contact_gate=4;if(!local_hand(f,left,seq,ms,mesh_scale))return;
    contact_distance=length(sub(left.position,geometry.slide.closed_contact));contact_sequence=seq;
    if(gears_lancer_workbench_profile::charging()){
        const bool actual_current=region_part.known&&region_part.identity==geometry.identity&&region_part.upload&&
            lancerhandle_visual::ready_for(f)&&region_part.owner_handle==lancerhandle_visual::current_handle()&&
            region_part.at_ns>0&&now>=region_part.at_ns&&now-region_part.at_ns<=100000000;
        if(!actual_current){game.magazine_observation_pending=true;return;}
        game.request_magazine=authored_contact(f,left,region_part.raw,region_part.at_ns,region_part.upload,now,mesh_scale);
    }else game.request_magazine=contact_distance<=geometry.slide.contact_radius;
    contact_gate=game.request_magazine?0:5;
    if(game.request_magazine){contact_key=f.key;contact_token=f.press_token;contact_proposed=true;}
}
void release_gesture() noexcept {
    held={};armed=false;pending_identity={};pending_frame=0;pending_until=0;
    last_sample=last_pose_frame=0;
}
bool restore() noexcept {
    if(!ownership.active){stroke.reset();release_gesture();return true;}
    // No ammo/state mutation while this render owner retires. Native first
    // publishes a no-morph payload, observes it, fences the FIFO, then restores
    // the private instance table. The targets remain rooted until that receipt.
    ownership.retiring=true;
    if(!lancerhandle_visual::restore())return false;
    ownership={};geometry={};region_part={};stroke.reset();release_gesture();last_observed=0;return true;
}
bool actual_part(hand::Frame const& f,gears_lancer_handle_morph::Receipt const& r,double& distance) noexcept {
    const auto now=weaponhand::clock_ns(nullptr);
    if(!r.actual_known||!r.actual_upload_serial||r.actual_at_ns<=0||now<r.actual_at_ns||
       now-r.actual_at_ns>100000000||!std::isfinite(r.actual_distance_raw)||
       r.actual_distance_raw<0||r.actual_distance_raw>gears_weapon::lancer_front::travel_raw+.001||
       !current_geometry(f))return false;
    distance=r.actual_distance_raw/nativelancerfollow::bound_units;return std::isfinite(distance);
}
enum class Result {waiting,complete,recovery_required};
Result step(hand::Frame const& f,hand::Decision const& decision) noexcept {
    using Phase=gears_weapon::lancer_front::Stroke::Phase;
    failure=1;
    if(!enabled||!required(f)||!nativeequipment::current(nullptr,f))return Result::waiting;
    // Completion remains latched while its actual render owner drains. The
    // runtime captures the finish timestamp once, before this asynchronous work.
    if(ownership.active&&ownership.identity==inv::identity(f)&&stroke.phase()==Phase::complete)return Result::complete;
    if(ownership.retiring)return Result::waiting;
    if(!current_geometry(f)){failure=2;if(ownership.active)return Result::recovery_required;
        discover(f);return Result::waiting;}
    float game_time{};
    if(!lancerreload_state::time_field(f.world.address,L"TimeSeconds",game_time))return Result::waiting;
    P left{};std::uint64_t sequence{},ms{};double mesh_scale{};
    const bool tracked=local_hand(f,left,sequence,ms,mesh_scale);
    failure=3;
    if(!f.left_tracked||!f.left_grip_known)return Result::waiting;
    if(!f.left_grip_down){
        release_seen=true;
        if(stroke.phase()==Phase::pulling){if(!stroke.release(game_time))return Result::recovery_required;held={};}
    }
    if(stroke.phase()==Phase::pulling&&f.left_grip_down){
        if(decision.release_required||!hand::same(decision.lease,held)||held.id!=f.press_token){
            if(!stroke.cancel(game_time))return Result::recovery_required;
            release_gesture();release_seen=false;
        }else if(tracked&&sequence!=last_sample){
            if(!nativeequipment::current(nullptr,f))return Result::waiting;
            if(!stroke.move({left.position,sequence,ms,f.press_token,true,true})){
                if(log_file){fwprintf(log_file,L"lancer_charging_cancel frame=%llu rejection=%u sequence=%llu previous_sequence=%llu ms=%llu\n",
                    f.key.frame,unsigned(stroke.last_rejection()),sequence,last_sample,ms);fflush(log_file);}
                if(!stroke.cancel(game_time))return Result::recovery_required;
                release_gesture();release_seen=false;
            }
            last_sample=sequence;
        }
    }
    if(stroke.phase()==Phase::returning&&!stroke.advance_return(game_time))return Result::recovery_required;
    gears_lancer_handle_morph::Receipt receipt{};failure=4;double raw_distance{};
    if(!gears_weapon::lancer_front::raw_distance(nativelancerfollow::bound_units,stroke.wanted_distance(),raw_distance)||
       !lancerhandle_visual::set_distance(f,raw_distance,receipt))return Result::waiting;
    double actual{};failure=5;
    if(!actual_part(f,receipt,actual))return Result::waiting;
    // Preserve only this existing actual upload observation, with its original
    // timestamp and instance identity. Propose never polls Native recursively.
    if(gears_lancer_workbench_profile::charging())
        region_part={geometry.identity,receipt.actual_distance_raw,receipt.actual_at_ns,receipt.actual_upload_serial,receipt.handle,true};
    if(stroke.phase()==Phase::idle){
        if(ownership.active){ownership={};release_gesture();release_seen=!f.left_grip_down;}
        if(!f.left_grip_down||!release_seen||!tracked||!fresh_grant(f,decision)||
           !contact_proposed||contact_key.frame!=pending_frame||contact_token!=f.press_token||
           !(pending_identity==inv::identity(f)))return Result::waiting;
        failure=6;
        const gears_weapon::HammerburstLatchSample begin_sample{left.position,sequence,ms,f.press_token,true,true};
        if(gears_lancer_workbench_profile::charging()){
            if(!authored_contact(f,left,receipt.actual_distance_raw,receipt.actual_at_ns,receipt.actual_upload_serial,
                weaponhand::clock_ns(nullptr),mesh_scale)||!stroke.begin_in_region(geometry.slide,begin_sample,actual))return Result::waiting;
        }else if(!stroke.begin(geometry.slide,begin_sample,actual))return Result::waiting;
        ownership={inv::identity(f),true,false};held=decision.lease;armed=true;release_seen=false;
        last_sample=sequence;last_observed=actual;last_pose_frame=receipt.actual_upload_serial;
        if(log_file){fwprintf(log_file,L"lancer_charging_grasp frame=%llu sequence=%llu hand=%.9g,%.9g,%.9g front=1 travel_m=%.9g\n",
            f.key.frame,sequence,left.position.x,left.position.y,left.position.z,geometry.slide.travel);fflush(log_file);}
        return Result::waiting;
    }
    if(!ownership.active||!(ownership.identity==inv::identity(f)))return Result::recovery_required;
    if(receipt.actual_upload_serial!=last_pose_frame){
        if(!stroke.acknowledge_visible_part(actual,receipt.actual_upload_serial,game_time))return Result::recovery_required;
        last_pose_frame=receipt.actual_upload_serial;last_observed=actual;
        if(log_file&&pose_rows++<96){fwprintf(log_file,L"lancer_charging_pose frame=%llu phase=%u wanted=%.9g actual=%.9g upload=%llu front=1\n",
            f.key.frame,unsigned(stroke.phase()),stroke.wanted_distance(),actual,receipt.actual_upload_serial);fflush(log_file);}
    }
    failure=0;
    if(stroke.phase()==Phase::complete)return Result::complete;
    if(stroke.phase()==Phase::idle){ownership={};release_gesture();release_seen=!f.left_grip_down;}
    return Result::waiting;
}
// Retained typed animation/name helpers for the existing Snub adapter only.
// Lancer front charging never calls or seeks the B receiver animation.
bool script_call(playground::Args const& call,const wchar_t* label) noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(call.r.function,label))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    // The retail Lancer overrides this getter to read its own AnimTree child.
    // Play/Stop still belong to WarWeapon; accept only that exact getter
    // override, not arbitrary functions on derived weapon classes.
    const bool owner_ok=playground::named(owner,L"WarWeapon")||
        (!std::wcscmp(label,L"GetWeaponAnimNodeSeq")&&playground::named(owner,L"Weap_AssaultRifle"));
    return registered(owner)&&registered(package)&&owner_ok&&
        playground::named(package,L"WarfareGame")&&get<uptr>(package+0x40)==0;
}
gears_equipment_names::Lookup reload_name({L"AR_Reload",L"AR_Reload",L"AR_Reload"});
std::array<unsigned,3> reload_indices{};
bool prepare_reload_name(hand::Frame const& f) noexcept {
    std::int64_t name_started{};
    return reload_name.step(
        [](gears_equipment_names::Table& t){const auto a=get<Array>(base+0x1c34800);t={a.data,unsigned(a.count),unsigned(a.capacity)};return a.count>=0&&a.capacity>=0;},
        [](gears_equipment_names::Table t,unsigned i,std::array<wchar_t,32>& text){const auto e=get<uptr>(t.data+std::size_t(i)*8);unsigned index{};
            return e&&copy(&index,e,4)&&index==i&&copy(text.data(),e+20,sizeof(text));},
        [&](){return nativeequipment::current(nullptr,f);},[&](){const auto now=weaponhand::clock_ns(nullptr);
            if(!name_started)name_started=now;return now>=name_started&&now-name_started<3000000;},reload_indices,8192)==gears_equipment_names::State::ready;
}
} // namespace lancer_charging
