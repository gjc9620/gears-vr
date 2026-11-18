// Shared hand arbiter, native reload lease and revisioned inventory writer.
// No independent runner, refill timer or second bone/component writer.
namespace snub_runtime {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace anim=hammerburst_animation;namespace visual=snub_visual;
using V=gears_vr::Vector;using P=gears_vr::Pose;
enum class Stage {idle,acquiring,hiding,old_magazine,waiting_ammo,new_magazine,seating,charging,completing,recovering};
Stage stage{};
std::atomic<std::uint64_t> configured_policy{gears_weapon::LancerReloadPolicy{}.packed()};
gears_weapon::LancerReloadPolicy opening_policy;
inv::ObservedInventory observer;inv::Transaction transaction;inv::InsertionWriter writer;
inv::Inventory opening;inv::Request completion;lancerreload_state::RewardReceipt reward;
gears_weapon::FeedInsertionStroke insertion;hand::Lease held{};
std::uint64_t serial{},animation_serial{},old_item{},new_item{},feed_sequence{},last_intent{};
std::uint64_t held_after_frame{};
inv::Identity last_identity{},proposal_identity{};P gun_in_right{};
bool proposal_known{},released{},extracted{},switch_requested{},standalone{},ui_pending{},native_exited{};
std::int64_t feed_ns{},closed_tick{},perfect_ms{};
unsigned sound_dispatched{},preparation_gate{~0u};
std::uint64_t reported_grip{},accepted_intent{};inv::Identity accepted_identity{};
struct ContactRead {std::uint64_t frame{},token{};unsigned gate{};double distance{};bool candidate{};};
ContactRead contact_read;unsigned contact_reports{},last_contact_signature{~0u};
void guide_tick(hand::Frame const&,hand::GripGameState const&) noexcept;
void report_contact(unsigned read_failure=0,unsigned difference=0) noexcept {
    if(!contact_read.token||contact_reports>=16)return;
    const auto signature=contact_read.gate|(unsigned(contact_read.candidate)<<8)|(read_failure<<9)|(difference<<17);
    if(signature==last_contact_signature)return;last_contact_signature=signature;++contact_reports;
    // Existing atomic/POD diagnostics only. These observations explain
    // an already rejected read; they never recover its Frame or part lease.
    if(log_file){const auto withdrawal=weaponhand::client.withdrawal_on_gt();
        fwprintf(log_file,L"snub_contact_admission frame=%llu token=%llu gate=%u candidate=%u distance=%.9g read_failure=%u game_difference=%u hand_disarm_reason=%u hand_phase=%u hand_armed=%u stage=%u withdrawal_reason=%u withdrawal_difference=%u withdrawal_frame=%llu withdrawal_sequence=%llu withdrawal_token=%llu\n",
        contact_read.frame,contact_read.token,contact_read.gate,unsigned(contact_read.candidate),contact_read.distance,read_failure,difference,
        weaponhand::client.last_disarm_reason(),unsigned(weaponhand::client.phase()),unsigned(weaponhand::client.armed()),unsigned(stage),
        unsigned(withdrawal.failure),withdrawal.game_difference,withdrawal.frame,withdrawal.sequence,withdrawal.press_token);fflush(log_file);}
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(stage!=Stage::idle||!d.new_grant||d.lease.kind!=hand::LeaseKind::magazine||
       !visual::granted(f,d)||!proposal_known||!(proposal_identity==inv::identity(f)))return;
    accepted_intent=f.press_token;accepted_identity=inv::identity(f);
}
struct Debt {inv::Identity identity{};bool active{};};
std::array<Debt,16> debts{};int debt_slot{-1};
bool manual_enabled() noexcept {return weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire);}
bool same_weapon(inv::Identity const& a,inv::Identity const& b) noexcept {
    return hand::same(a.world,b.world)&&hand::same(a.pawn,b.pawn)&&hand::same(a.weapon,b.weapon)&&a.game_start==b.game_start&&a.native_start==b.native_start;
}
int find_debt(inv::Identity const& id) noexcept {
    for(unsigned i=0;i<debts.size();++i)if(debts[i].active&&same_weapon(debts[i].identity,id))return int(i);
    return -1;
}
int reserve_debt(inv::Identity const& id) noexcept {
    const auto found=find_debt(id);if(found>=0)return found;
    for(unsigned i=0;i<debts.size();++i){auto& entry=debts[i];
        if(entry.active&&lancerreload_runtime::weapon_retired(entry.identity.weapon))entry={};
        if(!entry.active){entry={id,true};return int(i);}}
    return -1; // No unsafe eviction of an unresolved chamber debt.
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    if(stage!=Stage::idle&&hand::same(weapon,opening.identity.weapon))return true;
    for(auto const& debt:debts)if(debt.active&&hand::same(weapon,debt.identity.weapon))return true;
    return false;
}
void switch_dispatched(hand::Frame const& f) noexcept {
    if(stage!=Stage::idle&&same_weapon(inv::identity(f),opening.identity))switch_requested=true;
}
void sound(hand::Frame const& f,const wchar_t* name,unsigned bit) noexcept {
    if(sound_dispatched&bit)return;
    if(!nativeequipment::current(nullptr,f))return;
    playground::Args call(f.weapon.address,name);
    const auto owner=call.valid?get<uptr>(call.r.function+0x40):0;
    const auto package=owner?get<uptr>(owner+0x40):0;
    if(!call.valid||call.r.size||!registered(owner)||!registered(package)||
       !playground::named(owner,L"Weap_COGPistol")||!playground::named(package,L"WarfareGame")||get<uptr>(package+0x40))return;
    sound_dispatched|=bit; // An uncertain sound call is never replayed.
    const auto applied=call.run();
    if(log_file){fwprintf(log_file,L"snub_reload_sound bit=%u applied=%u\n",bit,unsigned(applied));fflush(log_file);}
}
bool local_left(hand::Frame const& f,V& position,double& units) noexcept {
    // The observer may not enter Native, including a read-only rig getter.
    // Use the same current raw hand pair and fixed held-gun mount as HB's
    // validated relative contact path. The common world origin cancels.
    units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return false;
    for(auto const& h:f.controllers.hands)
        if(!gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))return false;
    const auto convert=[&](P const& pose){const auto basis=gears_vr::xr_to_ue(gears_vr::rotation_matrix(pose.orientation));
        return gears_vr::Viewpoint{visual::mul(gears_vr::xr_to_ue(pose.position),units),gears_vr::ue_angles(basis)};};
    const auto left=convert(f.controllers.hands[0].grip),right=convert(f.controllers.hands[1].grip);
    gears_vr::Quaternion rotation{};
    if(!proposal_known||!(proposal_identity==inv::identity(f))||
       !gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(right.rotation),rotation))return false;
    const P gun{visual::add(right.position,gears_weapon::rotate(rotation,gun_in_right.position)),
        gears_vr::normalized(gears_vr::multiply(rotation,gun_in_right.orientation))};
    // Bounds already bake the original object scale exactly once.
    position=gears_weapon::rotate(gears_vr::inverse(gun.orientation),visual::sub(left.position,gun.position));
    return gears_vr::finite(position);
}
bool bind_proposal(hand::Frame const& f) noexcept {
    // Capture a rigid geometry binding on the normal GT, outside observe's
    // arbitration gate. Component translation is not necessarily the final
    // rendered LocalToWorld: measured Snub difference was about12.8cm.
    // Only this fixed relative mount survives; no old hand/world pose is used
    // as motion or as an input/visibility/inventory permission.
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    P gun{};double size{};
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)!=gears_equipment_rig::Status::matched||
       !(rig.world_pose_mask&gears_rig_tracking::right_grip_pose)||
       !visual::gun_pose(f,gun,size)||std::abs(size-visual::object_scale)>1e-6)return false;
    const auto& right=rig.world_grip[1];const auto inverse=gears_vr::inverse(right.orientation);
    const P relative{gears_weapon::rotate(inverse,visual::sub(gun.position,right.position)),
        gears_vr::normalized(gears_vr::multiply(inverse,gun.orientation))};
    if(!gears_vr::stereo_pose_valid(relative)||!nativeequipment::current(nullptr,f))return false;
    gun_in_right=relative;proposal_identity=inv::identity(f);proposal_known=true;return true;
}
void request_contact(hand::Frame const& f,hand::GripGameState& game,bool contact) noexcept {
    const bool pair_pending=contact&&visual::capture_observation_pending(f);
    game.request_magazine=contact&&!pair_pending;game.magazine_observation_pending=pair_pending;
    contact_read.candidate=game.request_magazine;if(pair_pending)contact_read.gate=8;
}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    if(f.left_grip_down&&playground::instance(f.weapon.address,L"Weap_COGPistol"))contact_read={f.key.frame,f.press_token,1,0,false};
    if(!manual_enabled()||!game.main_hand_held||game.busy||game.animation_owns_hands||!f.left_tracked||!f.left_grip_down||
       game.kind!=hand::WeaponKind::firearm||!playground::instance(f.weapon.address,L"Weap_COGPistol")||
       stage==Stage::recovering||stage==Stage::completing||stage==Stage::seating)return;
    if(held.id==f.press_token&&f.key.frame>held_after_frame&&hand::lease_valid(held,f.key.lifecycle)){
        for(auto& slot:game.object_incumbents){if(hand::same(slot,held))return;
            if(slot.kind==hand::LeaseKind::none){slot=held;return;}}return;
    }
    if(stage==Stage::waiting_ammo){
        const P body{f.stereo.head.position,f.reference.orientation};
        game.request_ammo=transaction.target()>0&&gears_weapon::body_slot(nativeequipment::calibration,body,
            f.controllers.hands[0].grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;return;
    }
    contact_read.gate=4;V left{};double units{};if(!local_left(f,left,units))return;
    if(stage==Stage::charging||((stage==Stage::idle||standalone)&&find_debt(inv::identity(f))>=0)){
        request_contact(f,game,proposal_known&&proposal_identity==inv::identity(f)&&snub_charging::contact_known&&
            snub_charging::contact_identity==inv::identity(f)&&visual::length(visual::sub(left,snub_charging::contact_local))<=.065*units);return;
    }
    // observe() already holds HandClient's arbitration gate. A recursive
    // visual::current -> nativeequipment::current -> source_game_on_gt here
    // disarmed that client on LGrip, hiding the held gun before capture.
    // This cached contact is a proposal only; capture/show keep full CURRENT
    // source, real wrist, Native lease and contact admission after the gate.
    contact_read.gate=6;
    if(!proposal_known||!(proposal_identity==inv::identity(f))||!visual::generation||
       !(visual::identity==inv::identity(f))||!(visual::gun_key==nativelancerfollow::bound_key)||!visual::same_binding(f))return;
    contact_read.gate=7;contact_read.distance=visual::length(visual::sub(left,visual::nearest(left)))/units;
    contact_read.candidate=contact_read.distance<=.065;request_contact(f,game,contact_read.candidate);
}
void reset() noexcept {
    stage=Stage::idle;observer={};transaction={};writer={};opening={};completion={};reward={};held={};
    serial=animation_serial=old_item=new_item=feed_sequence=accepted_intent=held_after_frame=0;accepted_identity={};feed_ns=closed_tick=perfect_ms=0;
    released=extracted=switch_requested=standalone=ui_pending=native_exited=proposal_known=false;
    debt_slot=-1;sound_dispatched=0;insertion.reset();snub_charging::reset();
    lancerreload_runtime::outstanding.store(false,std::memory_order_release);
}
bool cleanup(unsigned line) noexcept {
    if(stage!=Stage::recovering&&log_file){fwprintf(log_file,L"snub_reload_cleanup line=%u stage=%u serial=%llu pending=%u debt=%d\n",
        line,unsigned(stage),serial,unsigned(transaction.pending()),debt_slot);fflush(log_file);}
    stage=Stage::recovering;held={};insertion.reset();proposal_known=false;
    if(lancerreload_runtime::weapon_retired(opening.identity.weapon)){
        if(!snub_part_visibility::restore()||!visual::restore())return false;
        // Positive registry retirement only. No write into a recycled object.
        if(hand::same(lancerreload_state::lease.identity.weapon,opening.identity.weapon))lancerreload_state::lease={};
        if(hand::same(anim::ownership.inventory.weapon,opening.identity.weapon))anim::ownership={};
        if(debt_slot>=0)debts[unsigned(debt_slot)]={};reset();return true;
    }
    inv::Request pending_request{};
    if(transaction.dispatched_request(pending_request)&&!lancerreload_state::restore_partial_insert(pending_request,observer))return false;
    if(visual::held&&!visual::hide())return false;
    if(!snub_part_visibility::restore())return false;
    if(anim::ownership.active&&!anim::restore_closed_snub())return false;
    if(serial&&!lancerreload_state::recover_serial(serial))return false;
    if(anim::ownership.active&&!(anim::ownership.visual_only?snub_visual_clip::restore():anim::hand_back_after_native_exit()))return false;
    if(transaction.dispatched_request(pending_request)){
        inv::Inventory counts{};if(!observer.sample_owned(pending_request.identity,counts))return false;
        bool applied=false;
        if(pending_request.call==gears_weapon::ReloadTxnCall::insert){
            if(inv::counts_equal(counts,pending_request.expected))applied=true;
            else if(!inv::counts_equal(counts,pending_request.before))return false;
            if(!opening.loaded&&debt_slot>=0&&!applied)debts[unsigned(debt_slot)]={};
        }else{
            if(!inv::counts_equal(counts,pending_request.expected)||(pending_request.result==gears_weapon::ReloadTxnResult::perfect&&!reward.applied))return false;
            applied=true;
        }
        const auto result=pending_request.call==gears_weapon::ReloadTxnCall::insert||pending_request.result==gears_weapon::ReloadTxnResult::inspection?
            gears_weapon::ReloadTxnResult::none:pending_request.result;
        if(!transaction.acknowledge({pending_request.token,applied,counts,result}))return false;
    }
    if(!visual::restore())return false;
    reset();return true;
}
bool finish(hand::Frame const& f) noexcept {
    if(standalone){stage=Stage::completing;return true;}
    std::int64_t tick=closed_tick;
    if(!tick){tick=f.controllers.sync_ns/1000000;if(serial&&!lancerreload_state::closure_time(f,serial,tick))return false;closed_tick=tick;}
    if(!transaction.confirm_closed(tick,perfect_ms)||(serial&&!transaction.take_request(completion)))return false;
    stage=Stage::completing;held={};return true;
}
gears_weapon::FeedResult feed_step(hand::Frame const& f,std::uint64_t item,
    visual::FeedObservation const* feed_observation=nullptr) noexcept {
    if(f.controllers.sequence<=feed_sequence)return gears_weapon::FeedResult::rejected;
    gears_weapon::FeedPortMount port{};gears_weapon::FeedSample sample{};
    if(feed_observation){
        if(!visual::feed_observation_current(f,item,*feed_observation))return gears_weapon::FeedResult::rejected;
        port=feed_observation->port;sample=feed_observation->sample;
    }else if(!visual::feed_sample(f,item,port,sample))return gears_weapon::FeedResult::rejected;
    // A newer permission frame may see the same original C0 observation.
    // Ignore it without resetting the stroke or manufacturing a new sample.
    if(sample.sequence<=feed_sequence||(feed_ns&&sample.monotonic_ns<=feed_ns))return gears_weapon::FeedResult::rejected;
    port.maximum_sample_gap_ns=250000000; // observation continuity, never tracking permission
    if(!feed_ns||sample.monotonic_ns-feed_ns>port.maximum_sample_gap_ns)
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
    feed_sequence=sample.sequence;feed_ns=sample.monotonic_ns;return insertion.observe(sample,port);
}
bool configure_ui(hand::Frame const& f) noexcept {
    if(!serial)return true;
    if(ui_pending){
        perfect_ms=opening_policy.deadline(opening.loaded==0,lancerreload_state::lease.perfect_end_ms);
        const auto display=opening.loaded?opening_policy.tactical_ms:opening_policy.empty_ms;
        if(!lancerreload_state::set_ui_timing(f,serial,display,perfect_ms,opening_policy.mode!=gears_weapon::LancerPerfectMode::original)&&
           !(lancerreload_state::lease.ui_owned&&lancerreload_state::lease.attempts_owned&&lancerreload_state::ui_setup_gate>=6))return false;
        lancerreload_state::lease.perfect_end_ms=perfect_ms;ui_pending=false;return true;
    }
    return lancerreload_state::tick_ui_timing();
}
unsigned acquire_reported_gate=~0u,acquire_reported_curve=~0u,acquire_reported_observe=~0u,acquire_report_count{};
anim::Acquire acquire_animation(hand::Frame const& f) noexcept {
    if(!serial)return snub_visual_clip::start(f,animation_serial);
    if(snub_visual_clip::no_configured_gun_reload(f,serial)){
        const auto result=snub_visual_clip::start(f,serial,true);
        snub_animation::acquire_gate=result==anim::Acquire::acquired?0:100+snub_visual_clip::failure;return result;
    }
    return snub_animation::acquire(f,serial,snub_charging::curve);
}
void report_acquire(hand::Frame const& f,unsigned gate) noexcept {
    const auto curve=snub_animation::curve_failure,observe=snub_animation::observe_failure;
    if(!log_file||acquire_report_count>=16||(gate==acquire_reported_gate&&curve==acquire_reported_curve&&observe==acquire_reported_observe))return;
    acquire_reported_gate=gate;acquire_reported_curve=curve;acquire_reported_observe=observe;++acquire_report_count;
    fwprintf(log_file,L"snub_acquire_observation frame=%llu sequence=%llu token=%llu serial=%llu gate=%u curve_line=%u observe_line=%u active=%u stopped=%u left_known=%u left_down=%u left_tracked=%u\n",
        f.key.frame,f.controllers.sequence,f.press_token,serial,gate,curve,observe,unsigned(anim::ownership.active),unsigned(anim::ownership.stopped),
        f.left_grip_known,f.left_grip_down,f.left_tracked);fflush(log_file);
}
void retain_charging_grant(hand::Frame const& f,hand::Decision const& d,
    snub_charging::Stroke::Phase previous,snub_charging::Result result) noexcept {
    // A successful fresh slide capture owns this exact committed hand lease.
    // Report it through the existing incumbent path on the NEXT frame, just
    // like a held magazine. This neither allocates nor renews a hand grant.
    if(stage!=Stage::charging||previous!=snub_charging::Stroke::Phase::idle||
       snub_charging::stroke.phase()!=snub_charging::Stroke::Phase::pulling||
       result!=snub_charging::Result::waiting||!f.left_grip_known||!d.new_grant||!visual::granted(f,d))return;
    held=d.lease;held_after_frame=f.key.frame;
}
void step(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(stage==Stage::acquiring){
        if(!f.left_grip_known||!f.left_grip_down){cleanup(__LINE__);return;}
        if(!anim::ownership.active){
            const auto accepted=acquire_animation(f);
            report_acquire(f,snub_animation::acquire_gate);
            if(accepted==anim::Acquire::recovery_required)cleanup(__LINE__);return;
        }
        if(!anim::ownership.stopped){report_acquire(f,6);return;}
        if(!anim::restore_closed_snub()){report_acquire(f,7);return;}
        if(standalone){stage=Stage::charging;held={};snub_charging::release_seen=false;return;}
        if(!snub_part_visibility::begin(f,animation_serial)){report_acquire(f,8);return;}
        stage=Stage::hiding;return;
    }
    if(stage==Stage::hiding){
        if(!f.left_grip_known||!f.left_grip_down){cleanup(__LINE__);return;}
        if(!snub_part_visibility::hidden(f,animation_serial)||!visual::show(f,d,animation_serial))return;
        stage=Stage::old_magazine;feed_sequence=0;feed_ns=0;feed_step(f,old_item);return;
    }
    if(stage==Stage::seating){
        if(!snub_part_visibility::restore())return;
        if(transaction.inserted()&&!opening.loaded){stage=Stage::charging;held={};snub_charging::reset();snub_charging::release_seen=!f.left_grip_down;return;}
        if(!finish(f))cleanup(__LINE__);return;
    }
    if(stage==Stage::charging){
        const auto previous_stroke=snub_charging::stroke.phase();
        const auto result=snub_charging::tick(f,d,animation_serial);
        retain_charging_grant(f,d,previous_stroke,result);
        if(snub_charging::stroke.actual_full_pull())sound(f,L"CockOpen",4);
        if(snub_charging::stroke.phase()==snub_charging::Stroke::Phase::returning&&snub_charging::issued_full)sound(f,L"CockClose",8);
        if(result==snub_charging::Result::recovery_required){cleanup(__LINE__);return;}
        if(result==snub_charging::Result::complete){
            if(debt_slot>=0)debts[unsigned(debt_slot)]={};
            // Freeze finish before optional sound dispatch consumes the frame.
            if(!finish(f)){cleanup(__LINE__);return;}sound(f,L"CockCloseFinish",16);
        }else snub_grasp::slide(f,d,animation_serial);
        return;
    }
    if(!f.left_grip_known)return;
    if(!f.left_grip_down){
        released=true;held={};
        if(stage==Stage::old_magazine){
            if(!extracted){cleanup(__LINE__);return;}
            if(!transaction.stow_original(old_item)||!visual::hide()){cleanup(__LINE__);return;}
            stage=Stage::waiting_ammo;feed_sequence=0;feed_ns=0;
        }else if(stage==Stage::new_magazine){
            if(!transaction.release_item(new_item)||!visual::hide()){cleanup(__LINE__);return;}
            stage=Stage::waiting_ammo;new_item=0;feed_sequence=0;feed_ns=0;
        }
        return;
    }
    if(stage==Stage::waiting_ammo){
        if(!transaction.target()){cleanup(__LINE__);return;} // inspection: restore without refilling
        if(!released||!visual::granted(f,d)||d.lease.kind!=hand::LeaseKind::ammo||!d.new_grant)return;
        new_item=d.lease.id;
        if(!transaction.reserve_item(new_item)||!visual::reacquire(f,d)){cleanup(__LINE__);return;}
        held=d.lease;held_after_frame=f.key.frame;released=false;stage=Stage::new_magazine;feed_sequence=0;feed_ns=0;
        // Reclassification belongs to the next Native publication. Do not
        // reread this already committed shoulder proposal as a port proposal.
        return;
    }
    if(stage!=Stage::old_magazine&&stage!=Stage::new_magazine)return;
    const auto item=stage==Stage::old_magazine?old_item:new_item;
    if(!visual::granted(f,d)||!hand::same(held,d.lease))return;
    visual::FeedObservation feed_observation{};
    if(!visual::show_and_feed(f,d,animation_serial,item,feed_observation))return;
    const auto moved=feed_step(f,item,&feed_observation);
    if(moved==gears_weapon::FeedResult::approaching&&stage==Stage::old_magazine){extracted=true;sound(f,L"EjectClip",1);}
    // Optional skin work must not consume the physical observation's original
    // deadline before its authoritative feed/finish consumer has run.
    if(moved!=gears_weapon::FeedResult::inserted){snub_grasp::magazine(f,d,item);return;}
    if(stage==Stage::old_magazine){if(!extracted||!transaction.restore_original(old_item)){cleanup(__LINE__);return;}}
    else{
        if(!opening.loaded){debt_slot=reserve_debt(opening.identity);if(debt_slot<0){cleanup(__LINE__);return;}}
        inv::Request request{};inv::Receipt receipt{};
        if(!transaction.request_insertion(item)||!transaction.take_request(request)){cleanup(__LINE__);return;}
        const auto result=writer.commit(f,request,observer,lancerreload_state::owns,receipt);
        if(!receipt.token){receipt.token=request.token;observer.sample(f,receipt.observed);}
        if(result==inv::CommitResult::reconciliation_required||!transaction.acknowledge(receipt)||result!=inv::CommitResult::committed){cleanup(__LINE__);return;}
        if(opening.loaded&&!lancerreload_state::closure_time(f,serial,closed_tick)){cleanup(__LINE__);return;}
    }
    if(!visual::hide()){cleanup(__LINE__);return;}stage=Stage::seating;held={};sound(f,L"InsertClip",2);
}
bool tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(stage==Stage::idle&&!manual_enabled()&&!visual::generation)return false;
    report_contact();
    if(stage==Stage::recovering){cleanup(__LINE__);return true;}
    if(stage==Stage::completing){
        if(!native_exited){
            if(!snub_part_visibility::restore()||!anim::restore_closed_snub()||(serial&&!lancerreload_state::recover_serial(serial))||
               !(anim::ownership.visual_only?snub_visual_clip::restore():anim::hand_back_after_native_exit())){cleanup(__LINE__);return true;}
            native_exited=true;return true;
        }
        if(serial){
            inv::Inventory counts{};
            if(completion.result==gears_weapon::ReloadTxnResult::perfect&&!lancerreload_state::perfect_reward(completion,opening,observer,reward))return true;
            if(!observer.sample_owned(completion.identity,counts))return true;
            const auto result=completion.result==gears_weapon::ReloadTxnResult::inspection?gears_weapon::ReloadTxnResult::none:completion.result;
            if(!transaction.acknowledge({completion.token,true,counts,result})){cleanup(__LINE__);return true;}
        }
        if(!visual::restore())return true;
        if(log_file){fwprintf(log_file,L"snub_reload_complete serial=%llu standalone=%u result=%u physical_headset_pending=1\n",serial,unsigned(standalone),unsigned(completion.result));fflush(log_file);}
        reset();return true;
    }
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||switch_requested){
        if(stage!=Stage::idle){cleanup(__LINE__);return true;}if(visual::generation)visual::restore();return false;
    }
    if(stage==Stage::idle&&(lancerreload_state::lease.acquired||anim::ownership.active||lancerreload_visual::active||
       (lancerreload_runtime::stage!=lancerreload_runtime::Stage::idle&&lancerreload_runtime::stage!=lancerreload_runtime::Stage::parked)))return false;
    hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
    hand::FrameReadObservation read{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,d,&read)||f.controller.address!=pc){
        report_contact(unsigned(read.failure),read.game_difference);return stage!=Stage::idle;
    }
    if(stage!=Stage::idle&&!(opening.identity==inv::identity(f))){
        const auto& a=opening.identity;const auto b=inv::identity(f);
        if(log_file){fwprintf(log_file,L"snub_identity_boundary stage=%u life=%llu,%llu hold=%llu,%llu world=%llu,%llu session=%llu,%llu space=%llu,%llu input=%llu,%llu registration=%llu,%llu same_objects=%u\n",
            unsigned(stage),a.lifecycle,b.lifecycle,a.hold_generation,b.hold_generation,a.world_generation,b.world_generation,
            a.session,b.session,a.space,b.space,a.input,b.input,a.registration,b.registration,unsigned(same_weapon(a,b)));fflush(log_file);}
        cleanup(__LINE__);return true;
    }
    if(!visual::eligible(f)||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands){
        if(stage!=Stage::idle){cleanup(__LINE__);return true;}proposal_known=false;if(visual::generation)visual::restore();return false;
    }
    if(stage==Stage::idle){
        const auto debt=find_debt(inv::identity(f));
        if(debt>=0){double actual{};snub_charging::parts(f,actual);
            bind_proposal(f);}
        if(debt<0){
            // Warm global indices before a press, without repeatedly reading
            // five gun bones on every idle frame. No tracking bound changes.
            std::array<unsigned,3> names{};if(!visual::wrist_name(f,names))return true;
            const auto prepared=visual::prepare(f);
            const auto gate=unsigned(prepared)*100+visual::failure;
            if(gate!=preparation_gate&&log_file){
                fwprintf(log_file,L"snub_reload_preparation state=%u gate=%u asset_gate=%u physical_transaction_connected=1\n",unsigned(prepared),visual::failure,weapon_part_assets::failure);fflush(log_file);preparation_gate=gate;
            }
            if(prepared==visual::Preparation::recovery_required){visual::restore();return true;}
            if(prepared!=visual::Preparation::ready)return true;
            bind_proposal(f);
        }
        guide_tick(f,game);
        if(f.left_grip_down&&f.press_token!=reported_grip&&log_file){
            fwprintf(log_file,L"snub_reload_grip token=%llu new=%u status=%u lease=%u accepted=%llu current=%u\n",f.press_token,
                unsigned(d.new_grant),unsigned(d.status),unsigned(d.lease.kind),accepted_intent,unsigned(debt>=0||visual::current(f)));fflush(log_file);reported_grip=f.press_token;
        }
        const bool own_intent=accepted_intent==f.press_token&&accepted_identity==inv::identity(f);
        if(!f.left_tracked||!f.left_grip_down||!visual::granted(f,d)||(!d.new_grant&&!own_intent)||d.lease.kind!=hand::LeaseKind::magazine)return true;
        const auto identity=inv::identity(f);if(same_weapon(last_identity,identity)&&f.press_token<=last_intent)return true;
        inv::Inventory before{};
        if(!observer.sample(f,before)||!gears_weapon::LancerReloadPolicy::unpack(configured_policy.load(std::memory_order_acquire),opening_policy))return true;
        standalone=debt>=0;
        inv::Transaction proposed;
        if(!standalone&&(!proposed.bind(before)||!visual::capture(f,d))){
            if(log_file){fwprintf(log_file,L"snub_reload_capture_deferred token=%llu gate=%u\n",f.press_token,visual::failure);fflush(log_file);}return true;
        }
        if(!standalone)transaction=proposed;
        opening=before;held=d.lease;held_after_frame=f.key.frame;old_item=f.press_token;debt_slot=debt;animation_serial=f.press_token;serial=0;stage=Stage::acquiring;
        last_identity=identity;last_intent=f.press_token;lancerreload_runtime::outstanding.store(true,std::memory_order_release);
        if(standalone)return true;
        const auto deficit=before.capacity-before.loaded;
        const auto quantity=before.infinite_reserve?deficit:(std::min)(deficit,before.reserve);
        if(quantity){
            const auto accepted=lancerreload_state::begin(f,serial);animation_serial=serial;ui_pending=true;
            if(accepted!=lancerreload_state::BeginResult::acquired){cleanup(__LINE__);return true;}
        }
        const auto start=serial?lancerreload_state::lease.accepted_begin_ms:f.controllers.sync_ns/1000000;
        if(!transaction.begin(before,old_item,gears_weapon::ReloadIntentSource::magazine_pull,serial,gears_weapon::ReloadTxnMode::magazine,before.capacity,start,old_item)){cleanup(__LINE__);return true;}
        // Stop the original gun clip promptly, before its early magazine
        // notifies. UI/property work belongs to a later stopped-owner frame.
        if(serial&&acquire_animation(f)==anim::Acquire::recovery_required)cleanup(__LINE__);
        return true;
    }
    if(!f.left_tracked){cleanup(__LINE__);return true;}
    if(serial&&(stage!=Stage::acquiring||anim::ownership.stopped)){
        const bool setup=ui_pending;if(!configure_ui(f)){cleanup(__LINE__);return true;}if(setup)return true;
    }
    const auto previous=stage;step(f,d);
    guide_tick(f,game);
    if(stage!=previous&&log_file){fwprintf(log_file,L"snub_reload_transition from=%u to=%u serial=%llu extracted=%u loaded=%u reserve=%u debt=%d\n",
        unsigned(previous),unsigned(stage),serial,unsigned(extracted),transaction.inventory().loaded,transaction.inventory().reserve,debt_slot);fflush(log_file);}
    return true;
}
bool pending_work() noexcept {return stage!=Stage::idle;}
// Service an existing old owner's completion/restoration only. Its pending
// result must not select or block another weapon's current candidate.
void service_unselected(uptr pc,uptr selected) noexcept {
    if(stage!=Stage::idle&&opening.identity.weapon.address!=selected)tick(pc);
}
} // namespace snub_runtime
#include "weapon_snub_guide.inl"
