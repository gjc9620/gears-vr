#ifndef BOOMSHOT_RUNTIME_SEATING_BOUNDARY_FIXTURE
#include "weapon_boomshot_seating.inl"
#endif
#ifndef BOOMSHOT_RUNTIME_SKIN_BOUNDARY_FIXTURE
#include "weapon_boomshot_skin.inl"
#endif
#ifndef BOOMSHOT_RUNTIME_ENDPOINT_BOUNDARY_FIXTURE
#include "weapon_boomshot_endpoint_sound.inl"
#include "weapon_boomshot_endpoint.inl"
#endif
#ifndef BOOMSHOT_RUNTIME_GUIDE_BOUNDARY_FIXTURE
#include "weapon_boomshot_guide.inl"
#endif
#ifndef BOOMSHOT_RUNTIME_FEEDBACK_BOUNDARY_FIXTURE
#include "weapon_boomshot_feedback.inl"
#endif
// Native observation/commit are data-only. The one normal-GT entry below owns
// all physical reload calls through the existing unique reload owner.
namespace boomshot_runtime {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;namespace core=gears_weapon;
using Contact=core::BoomshotContact;using Stage=boomshot_reload_owner::Owner::Stage;
core::BoomshotContactRoute route{};uptr controller_owner{};
void demo_port(hand::Frame const& f)noexcept{
    // A bounded observation for the demo driver, never an input or action.
    static std::uint64_t hold{};static gears_vr::Pose last_right{};static unsigned rows{},settle{};
    const auto& current_right=f.controllers.hands[1].grip;
    if(hold!=vrequip::model.hold_generation()){hold=vrequip::model.hold_generation();rows=0;}
    if(f.left_grip_down){settle=3;return;}
    if(!log_file||rows>=128||(rows&&!settle&&gears_vr::stereo_same_pose(last_right,current_right)))return;
    boomshot_skin_source::Source source{};core::boomshot_insertion::Geometry geometry{};
    if(!boomshot_skin_source::read(f,source))return;
    const bool geometry_known=boomshot_seating::read(f,geometry);
    const auto p=geometry.seated_in_root_m.position;
    const auto offset=gears_weapon::rotate(source.root.orientation,{p.x*source.root_scale*source.units,p.y*source.root_scale*source.units,p.z*source.root_scale*source.units});
    const auto port=gears_vr::Vector{source.root.position.x+offset.x,source.root.position.y+offset.y,source.root.position.z+offset.z};
    const auto& raw=f.controllers.hands[1].grip;const auto& world=source.rig.world_grip[1];
    if(geometry_known)fwprintf(log_file,L"boomshot_demo_port sequence=%llu units=%.9g px=%.9g py=%.9g pz=%.9g rx=%.9g ry=%.9g rz=%.9g rqx=%.9g rqy=%.9g rqz=%.9g rqw=%.9g wx=%.9g wy=%.9g wz=%.9g wqx=%.9g wqy=%.9g wqz=%.9g wqw=%.9g\n",
        f.controllers.sequence,source.units,port.x,port.y,port.z,raw.position.x,raw.position.y,raw.position.z,raw.orientation.x,raw.orientation.y,raw.orientation.z,raw.orientation.w,
        world.position.x,world.position.y,world.position.z,world.orientation.x,world.orientation.y,world.orientation.z,world.orientation.w);
    gears_vr::Pose support{};
    if(boomshot_skin_source::support_world(source,support))
        fwprintf(log_file,L"boomshot_demo_support sequence=%llu units=%.9g px=%.9g py=%.9g pz=%.9g pqx=%.9g pqy=%.9g pqz=%.9g pqw=%.9g rx=%.9g ry=%.9g rz=%.9g rqx=%.9g rqy=%.9g rqz=%.9g rqw=%.9g wx=%.9g wy=%.9g wz=%.9g wqx=%.9g wqy=%.9g wqz=%.9g wqw=%.9g\n",
            f.controllers.sequence,source.units,support.position.x,support.position.y,support.position.z,support.orientation.x,support.orientation.y,support.orientation.z,support.orientation.w,
            raw.position.x,raw.position.y,raw.position.z,raw.orientation.x,raw.orientation.y,raw.orientation.z,raw.orientation.w,
            world.position.x,world.position.y,world.position.z,world.orientation.x,world.orientation.y,world.orientation.z,world.orientation.w);
    fflush(log_file);last_right=current_right;++rows;if(settle)--settle;
}
void demo_support_state(hand::Frame const& f,hand::Decision const& d)noexcept{
    static unsigned rows{};static bool previous{};
    const bool active=f.left_grip_down&&d.lease.kind==hand::LeaseKind::support;
    if(!log_file||(rows>=64&&active==previous)||(!active&&!previous))return;
    hammerburst_animation::Parts parts{};gears_vr::Pose root{};double scale{};
    if(!boomshot_parts::read(f,parts)||!hammerburst_runtime::matrix(parts.world[0],root,scale))return;
    fwprintf(log_file,L"boomshot_demo_support_state sequence=%llu active=%u lease=%llu frame=%llu gx=%.9g gy=%.9g gz=%.9g qx=%.9g qy=%.9g qz=%.9g qw=%.9g\n",
        f.controllers.sequence,unsigned(active),d.lease.id,f.key.frame,root.position.x,root.position.y,root.position.z,
        root.orientation.x,root.orientation.y,root.orientation.z,root.orientation.w);fflush(log_file);previous=active;++rows;
}
struct Candidate {
    hand::Frame frame{};inv::Identity identity{};Contact purpose{};
    hand::GripGameState before_game{};
    std::array<hand::Lease,2> before{},after{};
    bool magazine{},ammo{},pending{},before_magazine{},before_ammo{},before_pending{},known{},frozen{},poisoned{};
};
Candidate candidate{};
// Data-only same-frame observation. Never used by arbitration or admission.
struct GameChange {std::uint64_t frame{},sequence{};unsigned fields{};hand::GripGameState before{},after{};};
std::array<GameChange,16> game_changes{};unsigned game_change_count{},game_change_reported{};
unsigned changed_fields(hand::GripGameState const& a,hand::GripGameState const& b) noexcept {
    const auto vector=[](gears_vr::Vector x,gears_vr::Vector y){return x.x==y.x&&x.y==y.y&&x.z==y.z;};
    return unsigned(a.known!=b.known)|(unsigned(a.main_hand_held!=b.main_hand_held)<<1)|
        (unsigned(a.animation_owns_hands!=b.animation_owns_hands)<<2)|(unsigned(a.busy!=b.busy)<<3)|
        (unsigned(a.mount_known!=b.mount_known)<<4)|(unsigned(a.kind!=b.kind)<<5)|
        (unsigned(!hand::same(a.mount_asset,b.mount_asset))<<6)|(unsigned(a.mount_revision!=b.mount_revision)<<7)|
        (unsigned(!gears_vr::stereo_same_pose(a.mount.right_grip_in_weapon,b.mount.right_grip_in_weapon))<<8)|
        (unsigned(!vector(a.mount.support_grip_in_weapon,b.mount.support_grip_in_weapon))<<9)|
        (unsigned(!vector(a.mount.muzzle_in_weapon,b.mount.muzzle_in_weapon))<<10)|
        (unsigned(!vector(a.mount.bore_forward_in_weapon,b.mount.bore_forward_in_weapon))<<11)|
        (unsigned(a.mount.support_length_tolerance_metres!=b.mount.support_length_tolerance_metres)<<12)|
        (unsigned(a.mount.maximum_support_correction_radians!=b.mount.maximum_support_correction_radians)<<13)|
        (unsigned(a.acquire_metres!=b.acquire_metres)<<14)|(unsigned(a.release_metres!=b.release_metres)<<15);
}
void capture_game_change(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    if(game_change_count==game_changes.size()||!candidate.known||!candidate.frozen||
       !hand::same_frame(candidate.frame,f))return;
    const auto fields=changed_fields(candidate.before_game,game);if(!fields)return;
    game_changes[game_change_count++]={f.key.frame,f.controllers.sequence,fields,candidate.before_game,game};
}
void report_game_changes() noexcept {
    while(game_change_reported<game_change_count){
        const auto& c=game_changes[game_change_reported++];
        if(!log_file)continue;
        const auto flags=[](hand::GripGameState const& g){return unsigned(g.known)|(unsigned(g.main_hand_held)<<1)|
            (unsigned(g.animation_owns_hands)<<2)|(unsigned(g.busy)<<3)|(unsigned(g.mount_known)<<4);};
        fwprintf(log_file,L"boomshot_game_change frame=%llu sequence=%llu fields=%x before_flags=%x after_flags=%x before_kind=%u after_kind=%u before_asset=%llx after_asset=%llx before_revision=%llu after_revision=%llu observation_only=1\n",
            c.frame,c.sequence,c.fields,flags(c.before),flags(c.after),unsigned(c.before.kind),unsigned(c.after.kind),
            static_cast<unsigned long long>(c.before.mount_asset.address),static_cast<unsigned long long>(c.after.mount_asset.address),
            c.before.mount_revision,c.after.mount_revision);fflush(log_file);
    }
}
bool support_geometry(hand::Frame const& f,gears_vr::Pose& out)noexcept{return boomshot_skin::support_geometry(f,out);}
bool fill_support_mount(hand::Frame const& f,hand::GripGameState& game)noexcept{return boomshot_skin::fill_support_mount(f,game);}
bool selected_hint(hand::Frame const& f) noexcept {
    // No nativeequipment/current or client reentry inside Native observation.
    return unified_weapon_fire::manual_profile()&&nativelancerfollow::bound_key_known&&nativelancerfollow::mount.bound()&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
bool same_slots(std::array<hand::Lease,2> const& a,std::array<hand::Lease,2> const& b) noexcept {
    return hand::same(a[0],b[0])&&hand::same(a[1],b[1]);
}
bool empty_slots(std::array<hand::Lease,2> const& slots) noexcept {
    return slots[0].kind==hand::LeaseKind::none&&slots[1].kind==hand::LeaseKind::none;
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    const bool on_gt=weaponhand::current_gt(nullptr)!=0;
    if(on_gt)capture_game_change(f,game);
    if(!on_gt||lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||!selected_hint(f)||
       !game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::firearm||game.busy||game.animation_owns_hands)return;
    const auto current=inv::identity(f);const auto observed_now=weaponhand::clock_ns(nullptr);
    if(candidate.known&&hand::same(candidate.frame.key,f.key)&&candidate.frozen){
        // Freeze ONLY this producer's exact committed contribution. Never mask
        // current actors/game flags/mount/input or overwrite a foreign holder.
        // Expiry rejects only this publication. Keep the accepted item receipt for a later fresh same-cohort frame.
        if(!hand::frame_active(f,observed_now))return;
        if(candidate.poisoned||!hand::same_frame(candidate.frame,f)||!(candidate.identity==current)||
           !same_slots(candidate.before,game.object_incumbents)||game.request_magazine!=candidate.before_magazine||
           game.request_ammo!=candidate.before_ammo||game.magazine_observation_pending!=candidate.before_pending){candidate.poisoned=true;route.reset();return;}
        game.object_incumbents=candidate.after;game.request_magazine=candidate.magazine;game.request_ammo=candidate.ammo;
        game.magazine_observation_pending=candidate.pending;return;
    }
    candidate={};
    if(!hand::frame_active(f,observed_now)||f.recentered||f.heading_deferred)return;
    Candidate proposed{};proposed.frame=f;proposed.identity=current;proposed.before=proposed.after=game.object_incumbents;proposed.known=true;
    proposed.before_game=game;
    proposed.before_magazine=proposed.magazine=game.request_magazine;proposed.before_ammo=proposed.ammo=game.request_ammo;
    proposed.before_pending=proposed.pending=game.magazine_observation_pending;
    if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down){candidate=proposed;return;}
    if(!boomshot_reload_owner::owner.pending()){
        int used{};
        if(!inv::integer(f.weapon.address,L"AmmoUsedCount",used)||used!=1){candidate=proposed;return;}
    }
    const auto purpose=boomshot_reload_owner::owner.pending()?boomshot_reload_owner::owner.contact(f):Contact::opening;
    if(purpose==Contact::none){candidate=proposed;return;}
    auto slots=game.object_incumbents;
    if(route.grant().append_incumbent(f,purpose,observed_now,slots)){
        proposed.purpose=purpose;proposed.after=slots;game.object_incumbents=slots;candidate=proposed;return;
    }
    if(!empty_slots(slots)||game.request_magazine||game.request_ammo||game.magazine_observation_pending){candidate=proposed;return;} // Never replace another producer.
    Contact wanted{Contact::none};
    if(purpose==Contact::take_carrier){
        core::boomshot_insertion::Geometry geometry{};
        if(!boomshot_seating::read(f,geometry))proposed.pending=true;
        else if(boomshot_reload_owner::Owner::shoulder(f))wanted=purpose;
    }else{
        core::boomshot_insertion::Geometry geometry{};double port_distance{};
        // Idle contact may start a zero-target gun-only inspection. The GT
        // owner reads original counts and still requires certified insertion
        // geometry before admitting an actual empty reload.
        const bool geometry_ready=boomshot_seating::read(f,geometry)&&boomshot_seating::grip_distance(f,geometry,port_distance);
        if(!geometry_ready)proposed.pending=true;
        else if(!boomshot_reload_owner::owner.pending()){
            int used{};
            // Typed reflected scalar only: no original inventory getter or
            // policy assumption. The owner later rechecks all actual counts.
            if(!inv::integer(f.weapon.address,L"AmmoUsedCount",used)||used<0||used>1)proposed.pending=true;
            // The direct drum exchange starts from the closed port. Barrel
            // travel is not part of the action; only actual port proximity
            // proposes the contact. The owner still requires a real drum item.
            else if(used==1){
                if(!boomshot_seating::read(f,geometry))proposed.pending=true;
                else{
                    if(port_distance<=.10)wanted=purpose;
                }
            }
        }else if(port_distance<=.10)wanted=purpose;
    }
    proposed.purpose=route.propose(f,wanted,observed_now);
    proposed.magazine=proposed.purpose==Contact::opening||proposed.purpose==Contact::closing;
    proposed.ammo=proposed.purpose==Contact::take_carrier;
    proposed.pending=proposed.pending&&!proposed.magazine&&!proposed.ammo;
    proposed.after=slots;game.request_magazine=proposed.magazine;game.request_ammo=proposed.ammo;
    game.magazine_observation_pending=proposed.pending;candidate=proposed;
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(!weaponhand::current_gt(nullptr)||!candidate.known||candidate.poisoned||
       !hand::same_frame(candidate.frame,f)||!(candidate.identity==inv::identity(f)))return;
    const auto observed_now=weaponhand::clock_ns(nullptr);
    if(d.size!=sizeof d||d.abi_version!=hand::version||d.reserved||d.new_grant>1||d.release_required>1||!d.proposal_nonce||
       !hand::same(d.key,f.key)||d.press_token!=f.press_token||d.now_ns<f.now_ns||d.now_ns>observed_now||
       observed_now-d.now_ns>100000000||!hand::lease_valid(d.lease,f.key.lifecycle))return;
    if(!candidate.pending&&candidate.purpose!=Contact::none&&d.new_grant)route.remember(f,d,candidate.purpose,observed_now);
    // Include no-part/pending decisions: a later receipt-only seat/finish must
    // not change the contribution of the frame that Native already committed.
    candidate.frozen=true; // No engine call, action ownership or input restamp.
}
bool pending_work() noexcept {
    return boomshot_reload_owner::owner.lifecycle_pending()||boomshot_carrier_original::owner.active()||
        boomshot_clip::serial||boomshot_inspection_clip::owner.active();
}
bool drain() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    boomshot_feedback::lost();
    boomshot_endpoint::lost();
    if(!boomshot_reload_owner::owner.drain())return false;
    return boomshot_inspection_clip::owner.drain()&&!pending_work();
}
void service_unselected(uptr pc,std::uint64_t selected) noexcept {
    if(!weaponhand::current_gt(nullptr)||(controller_owner&&pc!=controller_owner))return;
    if(!nativelancerfollow::bound_key_known||selected!=nativelancerfollow::bound_key.weapon.address){boomshot_feedback::lost();boomshot_endpoint::lost();}
    boomshot_reload_owner::owner.retire_parked();
    boomshot_reload_owner::owner.service_seating();
    if(pending_work()&&(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||
       !boomshot_reload_owner::owner.selected(selected)))drain();
    if(!boomshot_reload_owner::owner.pending())controller_owner=0;
}
bool inspection_pending_work() noexcept {return boomshot_inspection_clip::owner.active();}
bool drain_inspection() noexcept {
    return weaponhand::current_gt(nullptr)&&boomshot_inspection_clip::owner.drain();
}
bool blocks_fire(gears_weapon_hand::Frame const& f) noexcept {
    return boomshot_carrier_original::owner.matches(f)||boomshot_reload_owner::owner.blocks_fire(f)||boomshot_sessions::blocks_fire(f)||
        (boomshot_clip::serial&&gears_weapon_hand::same(boomshot_clip::identity.weapon,f.weapon))||
        (boomshot_inspection_clip::owner.active()&&
            gears_weapon_hand::same(boomshot_inspection_clip::backend.identity.weapon,f.weapon));
}
bool tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||(controller_owner&&pc!=controller_owner))return pending_work();
    report_game_changes(); // Formatting only on the existing normal-GT entry.
    boomshot_reload_owner::owner.retire_parked();
    boomshot_reload_owner::owner.service_seating();
    boomshot_contact::clear();
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||boomshot_reload_owner::owner.stage()==Stage::recovery){drain();return pending_work();}
    if(boomshot_reload_owner::owner.stage()==Stage::completing){boomshot_feedback::lost();boomshot_endpoint::lost();boomshot_reload_owner::owner.complete_owned();return pending_work();}
    gears_weapon_hand::Frame source{};gears_weapon_hand::GripGameState game{};gears_weapon_hand::Decision decision{};
    gears_weapon_hand::FrameReadObservation source_read{};
    const bool copied=weaponhand::client.copy_frame_on_gt(source,game,decision,&source_read);
    boomshot_core_source::GameScope current_game(source,game,copied);
    // Observe this existing read only. It is not the R2 consumer's own read
    // and never retries/renews a publication or changes its admission.
    static unsigned read_rows{};static bool read_seen{},previous_copy{};
    static gears_weapon_hand::FrameReadFailure previous_failure{};
    static std::uint32_t previous_difference{};
    if(log_file&&read_rows<32&&(!read_seen||copied!=previous_copy||
       source_read.failure!=previous_failure||source_read.game_difference!=previous_difference)){
        // Existing diagnostic state only; these snapshots cannot recover or
        // authorize the rejected Frame. No observe/source read is repeated.
        const auto withdrawal=weaponhand::client.withdrawal_on_gt();
        fwprintf(log_file,L"boomshot_source_read phase=reload_tick copied=%u failure=%u game_difference=%u frame=%llu sequence=%llu token=%llu input_age_ns=%lld client_armed=%u current_gt=%u disarm_reason=%u withdrawal_failure=%u withdrawal_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu withdrawal_input_age_ns=%lld\n",
            unsigned(copied),unsigned(source_read.failure),source_read.game_difference,source_read.frame,
            source_read.sequence,source_read.press_token,source_read.input_age_ns,
            unsigned(weaponhand::client.armed()),weaponhand::current_gt(nullptr),weaponhand::client.last_disarm_reason(),
            unsigned(withdrawal.failure),withdrawal.game_difference,withdrawal.frame,withdrawal.sequence,withdrawal.input_age_ns);
        fflush(log_file);++read_rows;
    }
    read_seen=true;previous_copy=copied;previous_failure=source_read.failure;previous_difference=source_read.game_difference;
    if(!copied||source.controller.address!=pc||
       !game.known||!game.main_hand_held||game.kind!=gears_weapon_hand::WeaponKind::firearm||
       game.busy||game.animation_owns_hands||!boomshot_carrier::selected(source)){boomshot_feedback::lost();boomshot_endpoint::lost();return pending_work();}
    // One original-getter comparison per stable reference binding; callbacks
    // then read CURRENT SpaceBases/component data without any engine call.
    // Read-only original getters on neutral normal GT, never in Native's
    // candidate callback or on a newly pressed left control. No action begins.
    if(source.left_grip_known&&!source.left_grip_down&&!boomshot_reload_owner::owner.lifecycle_pending()&&
       boomshot_inspection_clip::fire_neutral(source))boomshot_seating::certify(source);
    core::boomshot_insertion::Geometry geometry{};boomshot_seating::read(source,geometry);
    demo_port(source);
    demo_support_state(source,decision);
    const auto before=boomshot_reload_owner::owner.stage();const bool pending_before=boomshot_reload_owner::owner.pending();
    if(!pending_before)controller_owner=pc; // Persist before any accepted original action.
    const auto result=boomshot_reload_owner::owner.advance(source,decision,route.grant(),geometry);
    if(!boomshot_reload_owner::owner.pending())controller_owner=0;
    // Existing driver/recorder may observe transitions without extra engine
    // reads, clocks, attempts or per-frame logging. Original stamps retained.
    static unsigned rows{};static core::BoomshotEvent last_result{};static Stage last_stage{};
    const auto after=boomshot_reload_owner::owner.stage();
    if(log_file&&rows<96&&(pending_before||boomshot_reload_owner::owner.pending())&&
       (before!=after||after!=last_stage||result!=last_result)){
        fwprintf(log_file,L"boomshot_gt_transition before=%u after=%u result=%u serial=%llu weapon=%llx frame=%llu sequence=%llu sync_ns=%lld token=%llu\n",
            unsigned(before),unsigned(after),unsigned(result),boomshot_reload_owner::owner.original_serial(),
            static_cast<unsigned long long>(source.weapon.address),source.key.frame,source.controllers.sequence,source.controllers.sync_ns,source.press_token);
        fflush(log_file);++rows;
    }
    last_result=result;last_stage=after;
    // Finger-only: the existing right attachment/wrist/weapon aim is untouched.
    // An older Native without V4 simply declines this optional publication.
    boomshot_grasp::publish_marcus115(source,gears_boomshot_grasp_pose::Part::main_grip,{},false);
    boomshot_skin::publish_left(source,decision,route.grant());
    boomshot_feedback::sample(source,game,decision);
    boomshot_endpoint::sample(source);
    // Optional transient cues run after all mechanics and skin publication.
    // Direct drum exchange has no barrel-opening guide or charging action.
    return pending_work();
}
}
