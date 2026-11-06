// Private original-button consumer on the normal authenticated Native path.
// No bridge reread, pose or damage property writer. The explicit unified
// profile requires the separate same-shot muzzle result consumer before enable.
#include "weapon_unified_fire.h"
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
namespace lancerrefire {
bool certified_for(gears_weapon_hand::Frame const&) noexcept;
void begin_owned_press(gears_weapon_hand::Frame const&) noexcept;
void end_owned_press(bool) noexcept;
bool release_completed() noexcept;
bool release_required() noexcept;
bool has_initial_debt() noexcept;
void service(std::uintptr_t) noexcept;
void deliver_ready(gears_weapon_hand::Frame const&) noexcept;
}
#endif
namespace vrfire {
namespace lr=gears_lancer_r2;
namespace wh=gears_weapon_hand;
namespace meta=gripdamage_metadata;
enum : unsigned {off=0,running=1,stopping=2,stopped=3,fault=4};
std::atomic<unsigned> mode{off},last_status{};
lr::Input input;
Name r2{};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
gears_weapon::OriginalL2Aim aim;
Name l2{};
bool aim_owns() noexcept {return aim.owns_press();}
#else
bool aim_owns() noexcept {return false;}
#endif
unsigned logs{};int last_ammo{-1};unsigned previous_status{~0u},previous_source{~0u};
ULONGLONG next_ammo{};unsigned ammo_reads{},post_ammo_samples{2};
bool ammo_resume_pending{};
unsigned muzzle_logs{},trace_logs{};std::uint64_t last_muzzle_writes{},last_trace_count{};
unsigned trajectory_logs{};std::uint64_t trajectory_omitted{},last_trajectory_drop{};
unsigned muzzle_prepare_failure_logs{};
unsigned execute_failure_logs{};
lr::Actors actors(wh::Frame const& f) noexcept {return {f.world,f.controller,f.pawn,f.mesh,f.weapon};}
// A negative scheduling hint only. Actual fire still authenticates the live
// actor, Lancer class, held mount, source and result schemas on its normal path.
// Holding a pistol must not pay Lancer metadata cost every PlayerTick.
weaponhand::HoldBinding candidate_hold{};
bool candidate_class_known{},candidate_is_lancer{};
bool held_lancer_candidate() noexcept {
    auto const& h=weaponhand::held_binding;
    if(!h.known)return false;
    const bool same=candidate_class_known&&candidate_hold.generation==h.generation&&
        candidate_hold.native_cookie==h.native_cookie&&candidate_hold.native_started==h.native_started&&
        candidate_hold.world_generation==h.world_generation&&wh::same(candidate_hold.world,h.world)&&
        wh::same(candidate_hold.controller,h.controller)&&wh::same(candidate_hold.pawn,h.pawn)&&wh::same(candidate_hold.weapon,h.weapon);
    if(same)return candidate_is_lancer;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,h.weapon))return false;
    const bool lancer=lancermain_asset::held_asset_name(r,h.weapon.address,
        lancermain_asset::AssetScope::supported_instant_fire)!=lancermain_asset::HeldAsset::unknown;
    if(r.result.failure.reason!=meta::Reason::none)return false;
    candidate_hold=h;candidate_is_lancer=lancer;candidate_class_known=true;return lancer;
}
bool button_name(wchar_t first,Name& out) noexcept {
    Array names{},again{};if(!copy(&names,base+0x1c34800,sizeof names)||!names.data||
        names.count<1000||names.count>names.capacity||names.capacity>4000000)return false;
    const auto end=GetTickCount64()+2000;
    for(int i=0;i<names.count;++i){
        if(!(i%256)&&GetTickCount64()>=end)return false;
        uptr entry{};unsigned index{};wchar_t text[3]{};
        if(!copy(&entry,names.data+size_t(i)*8,8)||!entry||!copy(&index,entry,4)||index!=unsigned(i)||
            !copy(text,entry+20,sizeof text))continue;
        if(text[0]==first&&text[1]==L'2'&&!text[2]){
            if(!copy(&again,base+0x1c34800,sizeof again)||memcmp(&names,&again,sizeof names))return false;
            out={unsigned(i),0};return true;
        }
    }
    return false;
}
bool name_current(Name const& name,wchar_t first) noexcept {
    Array names{};uptr entry{};unsigned index{};wchar_t text[3]{};
    return !name.number&&copy(&names,base+0x1c34800,sizeof names)&&names.data&&names.count>1000&&
        names.count<=names.capacity&&names.capacity<=4000000&&name.index<unsigned(names.count)&&
        copy(&entry,names.data+size_t(name.index)*8,8)&&entry&&copy(&index,entry,4)&&index==name.index&&
        copy(text,entry+20,sizeof text)&&text[0]==first&&text[1]==L'2'&&!text[2];
}
bool read_game(lr::Actors const& expected,wh::Frame const* source,lr::Game& out,bool* source_current=nullptr,
    wh::GripGameState const* copied_game=nullptr) noexcept {
    out={};if(source_current)*source_current=false;
    if(!weaponhand::current_gt(nullptr)||!lr::valid(expected))return false;
    // This identity-only value is solely for fresh metadata observation. It
    // is never published, passed to Input, or used to renew tracking lifetime.
    wh::Frame identity{};identity.world=expected.world;identity.controller=expected.controller;
    identity.pawn=expected.pawn;identity.mesh=expected.mesh;identity.weapon=expected.weapon;
    if(source){if(!lr::same(expected,actors(*source)))return false;identity=*source;}
    wh::GripGameState before{},after{};
    if(source_current){
        // Only tick supplies the two outputs of its immediately preceding
        // successful copy_frame_on_gt. No engine/wait/republication occurs
        // between that copy and this use. armed() still catches worker-side
        // soft invalidation; a live registration alone is insufficient.
        // This is never a seed for execute/ammo/owned-release reads.
        if(!source||!copied_game||!copied_game->known||!weaponhand::client.armed()||
            !enabled.load(std::memory_order_acquire)||!nativeequipment::domain_ready()||
            !wh::frame_active(*source,weaponhand::clock_ns(nullptr)))return false;
        before=*copied_game;
        // Preserve source-vs-metadata failure classification: a later typed
        // R2/schema failure must not become a source-loss fallback/retry.
        if(source_current)*source_current=true;
    }else if(!weaponhand::observe(nullptr,identity,weaponhand::clock_ns(nullptr),before))return false;
    if(!before.known)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField held{},again{};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    weaponhand::BoolField target_key{},target_key_again{},toggle{},toggle_again{};
    weaponhand::AimObserveFields aim_fields;
#endif
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,expected.controller)||
        !weaponhand::object(r,expected.weapon))return false;
    // Historical Game::lancer means eligibility for this original R2 owner.
    // Only these explicit instant-fire profiles enter; reload remains Lancer-only.
    const bool lancer=lancermain_asset::held_asset_name(r,expected.weapon.address,
        lancermain_asset::AssetScope::supported_instant_fire)!=lancermain_asset::HeldAsset::unknown;
    if(r.result.failure.reason!=meta::Reason::none)return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    if(!aim_fields.begin(r,expected.controller.address)||!aim_fields.boolean(r,weaponhand::AimObserveFields::held_flag,held))return false;
#else
    if(!weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_R2",held))return false;
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    if(!aim_fields.boolean(r,weaponhand::AimObserveFields::target_flag,target_key)||
       !aim_fields.boolean(r,weaponhand::AimObserveFields::toggle_flag,toggle))return false;
#endif
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(unsigned i=0;i<identities;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
            !meta::same_structure(schema.structures[i],fresh))return false;}
    meta::Table objects{},names{};
    if(
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
        !aim_fields.begin(r,expected.controller.address)||
        !aim_fields.boolean(r,weaponhand::AimObserveFields::held_flag,again)||
#else
        !weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_R2",again)||
#endif
        !meta::same_property(held.property,again.property)||held.mask!=again.mask||
        (held.bits&held.mask)!=(again.bits&again.mask)||
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
        !aim_fields.boolean(r,weaponhand::AimObserveFields::target_flag,target_key_again)||
        !aim_fields.boolean(r,weaponhand::AimObserveFields::toggle_flag,toggle_again)||
        !meta::same_property(target_key.property,target_key_again.property)||target_key.mask!=target_key_again.mask||
        (target_key.bits&target_key.mask)!=(target_key_again.bits&target_key_again.mask)||
        !meta::same_property(toggle.property,toggle_again.property)||toggle.mask!=toggle_again.mask||
        (toggle.bits&toggle.mask)!=(toggle_again.bits&toggle_again.mask)||
#endif
        !meta::tables(base,objects,names)||
        !meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
        !weaponhand::observe(nullptr,identity,weaponhand::clock_ns(nullptr),after)||!wh::same_game(before,after))return false;
    if(source_current&&!weaponhand::client.armed()){*source_current=false;return false;}
    out={expected,true,lancer,after.main_hand_held,after.animation_owns_hands,after.busy,true,bool(held.bits&held.mask)};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    out.l2_known=out.toggle_known=true;
    out.l2_held=bool(target_key.bits&target_key.mask);
    out.toggle_targeting=bool(toggle.bits&toggle.mask);
#endif
    return true;
}
gears_weapon::OriginalR2Execution execute(gears_weapon::OriginalR2Action action,std::uint64_t controller,wh::Frame const* source,
    lr::Game const* tick_game=nullptr) noexcept {
    auto reject=[&](wchar_t const* stage) noexcept {
        if(log_file&&execute_failure_logs<8){
            fwprintf(log_file,L"native_lancer_execute_rejected stage=%ls press=%u frame=%llu age_ns=%lld held=%u animation=%u busy=%u\n",
                stage,unsigned(action==gears_weapon::OriginalR2Action::press),source?source->key.frame:0,
                source?weaponhand::clock_ns(nullptr)-source->controllers.sync_ns:0,
                unsigned(tick_game&&tick_game->main_hand_held),unsigned(tick_game&&tick_game->animation),unsigned(tick_game&&tick_game->busy));
            fflush(log_file);++execute_failure_logs;
        }
        return gears_weapon::OriginalR2Execution::failed;
    };
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
        !input.has_actors()||controller!=input.actors().controller.address||!name_current(r2,L'R'))return reject(L"entry");
    lr::Game game{};
    // advance invokes its press callback synchronously on this GT, directly
    // after tick's authenticated read_game. Reuse exactly that observation for
    // a press; a second full metadata pass consumes the source's remaining
    // 100 ms before the original ButtonPress. Other call paths and releases
    // retain their independent fresh read.
    if(action==gears_weapon::OriginalR2Action::press&&source&&tick_game&&tick_game->known&&
       lr::same(tick_game->actors,actors(*source))&&lr::same(tick_game->actors,input.actors()))
        game=*tick_game;
    else if(!read_game(input.actors(),source,game))return reject(L"read_game");
    if(!game.lancer)return reject(L"class");
    if(action==gears_weapon::OriginalR2Action::press){
        if(!source||!game.main_hand_held||game.busy||game.animation||
            !nativeequipment::current(nullptr,*source))return reject(L"press_state");
        lancermain_asset::Evidence asset{};
        if(lancermain_asset::read(*source,asset,lancermain_asset::AssetScope::supported_instant_fire)!=lancermain_asset::Status::ready||
            !nativeequipment::attached_frame(*source,asset.component.address)||
            !nativeequipment::current(nullptr,*source))return reject(L"attachment");
    }else if(!game.r2_held){
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
        lancerrefire::release_completed(); // State cleanup may await the original next PlayerTick.
#endif
        return gears_weapon::OriginalR2Execution::executed; // Freshly certified up.
    }
    playground::Args call(controller,action==gears_weapon::OriginalR2Action::press?L"ButtonPress":L"ButtonRelease");
    const auto parameter=call.prop(L"ButtonName");
    if(!call.valid||call.r.size!=sizeof(Name)||!parameter||get<unsigned>(parameter+0x70)!=1||
        get<unsigned>(parameter+0x74)!=sizeof(Name)||!playground::named(get<uptr>(parameter+0x50),L"NameProperty")||
       (get<unsigned long long>(parameter+0x78)&0x580)!=0x80||!call.set(L"ButtonName",r2).valid)return reject(L"button_schema");
    // All attachment and ButtonName reflection precedes the last freshness
    // check. A late preparation has not invoked ButtonPress and can be
    // deferred by Input within the ORIGINAL press intent's fixed deadline.
    // Each attempt still needs its own <=100ms source and dispatch reserve.
    if(action==gears_weapon::OriginalR2Action::press&&unified_weapon_fire::manual_profile()&&
       !unified_weapon_fire::ready_for(*source,20000000))return gears_weapon::OriginalR2Execution::not_ready;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    const bool owned_press=action==gears_weapon::OriginalR2Action::press;
    if(owned_press)lancerrefire::begin_owned_press(*source);
    const bool invoked=call.run();
    if(owned_press)lancerrefire::end_owned_press(invoked);
    if(!invoked)return reject(L"button_call");
    if(!owned_press)lancerrefire::release_completed();
#else
    if(!call.run())return reject(L"button_call");
#endif
    return gears_weapon::OriginalR2Execution::executed;
}
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
gears_weapon::OriginalL2Context aim_context(lr::Game const& game,bool allow) noexcept {
    const bool ordinary=game.known&&game.lancer&&game.l2_known&&game.toggle_known&&lr::valid(game.actors);
    return {game.actors.world.address,game.actors.controller.address,game.actors.weapon.address,
        ordinary,game.l2_held,game.toggle_targeting,ordinary&&allow};
}
bool execute_aim(gears_weapon::OriginalL2Action action,std::uint64_t controller,
    wh::Frame const* source=nullptr,lr::Game const* tick_game=nullptr) noexcept {
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
       !input.has_actors()||controller!=input.actors().controller.address||!name_current(l2,L'L'))return false;
    lr::Game game{};
    if(action==gears_weapon::OriginalL2Action::press&&source&&tick_game&&tick_game->known&&
       lr::same(tick_game->actors,actors(*source))&&lr::same(tick_game->actors,input.actors()))
        game=*tick_game;
    else if(!read_game(input.actors(),source,game))return false;
    if(!game.lancer||!game.l2_known||!game.toggle_known||game.toggle_targeting)return false;
    if(action==gears_weapon::OriginalL2Action::press){
        if(!source||!game.main_hand_held||game.busy||game.animation||game.l2_held||
           !nativeequipment::current(nullptr,*source))return false;
    }else if(!game.l2_held)return true;
    playground::Args call(controller,action==gears_weapon::OriginalL2Action::press?
        L"ButtonPress":L"ButtonRelease");
    const auto parameter=call.prop(L"ButtonName");
    if(!call.valid||call.r.size!=sizeof(Name)||!parameter||get<unsigned>(parameter+0x70)!=1||
       get<unsigned>(parameter+0x74)!=sizeof(Name)||!playground::named(get<uptr>(parameter+0x50),L"NameProperty")||
       (get<unsigned long long>(parameter+0x78)&0x580)!=0x80||!call.set(L"ButtonName",l2).valid)return false;
    return call.run();
}
#endif
bool ammo(lr::Actors const& owner,int& count,bool& key_down) noexcept {
    lr::Game game{};if(!read_game(owner,nullptr,game)||!game.lancer)return false;
    playground::Args query(owner.weapon.address,L"GetTotalAmmoInGun");
    if(!vrequip::result_property(query,L"IntProperty",4)||!query.run())return false;
    const auto value=query.output<int>();lr::Game after{};
    if(value<0||value>1000000||!read_game(owner,nullptr,after)||!after.lancer)return false;
    count=value;key_down=after.r2_held;return true;
}
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
void pre_original_held_shot(uptr pc) noexcept {
    // Both HUD queries and automatic shots execute inside original PlayerTick.
    // Refresh an already-held Lancer even with R2 up, using the current Native
    // publication. A new equip/press still belongs to the post-equipment path.
    // Preparing a muzzle neither synthesizes input nor extends source age.
    if(mode.load(std::memory_order_acquire)!=running||!unified_weapon_fire::manual_profile()||
       !held_lancer_candidate()||pc!=weaponhand::held_binding.controller.address||
       !weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return;
    wh::Frame frame{};wh::GripGameState game{};
    if(!weaponhand::client.copy_frame_on_gt(frame,game)||frame.controller.address!=pc||
       !game.known||!game.main_hand_held||game.animation_owns_hands||game.busy){
        unified_weapon_fire::clear_source();return;
    }
    if(unified_weapon_fire::ready_for(frame,20000000))return;
    nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
    if(!unified_weapon_fire::prepare_current(frame,game))unified_weapon_fire::clear_source();
}
#endif
void tick(uptr pc) noexcept {
    const auto state=mode.load(std::memory_order_acquire);if(state!=running&&state!=stopping)return;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    // Frag acquired the original targeting pair only after both firearm
    // ledgers were neutral. Do not classify that owned key as foreign input
    // or start another L2 owner while the grenade preparation is active.
    if(fragpreparation::outstanding.load(std::memory_order_acquire))return;
#endif
    const bool quiet=weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready();
    if(!lr::tick_admitted(quiet,pc,0,0))return;
    // Equipment must first commit a real held binding. Repeated fire metadata
    // reads while unheld starve its input baseline on the same GT. No source
    // or R2 state is fabricated here; an owed release and stopping still take
    // the complete existing path, even after equipment clears the binding.
    if(state==running&&unified_weapon_fire::manual_profile()&&
       !input.owns_press()&&!aim_owns()&&!input.has_actors()&&!held_lancer_candidate()){
        unified_weapon_fire::clear_source();return;
    }
    const auto source_started=weaponhand::clock_ns(nullptr);
    wh::Frame frame{};wh::GripGameState copied_game{};wh::Decision copied_decision{};
    const bool have_source=state==running&&weaponhand::client.copy_frame_on_gt(frame,copied_game,copied_decision);
    lr::Continuity continuity{};
    gears_current_tracking::Failure continuity_failure{};
    const auto native_selected_before=nativeequipment::input_continuity_native_selected;
    const bool continuity_matched=have_source&&nativeequipment::read_input_continuity(frame,continuity.tracking,continuity.publisher,nullptr,&continuity_failure);
    const bool continuity_native_selected=nativeequipment::input_continuity_native_selected!=native_selected_before;
    const auto copy_finished=weaponhand::clock_ns(nullptr);
    // Other PlayerControllers do not consume or invalidate this player's R2.
    // A freshly authenticated change of owner still reaches the policy below
    // so an owed release cannot be silently adopted by the new controller.
    if(!lr::tick_admitted(quiet,pc,have_source?frame.controller.address:0,
        input.has_actors()?input.actors().controller.address:0))return;
    const auto observation_started=weaponhand::clock_ns(nullptr);
    // Sample the expensive socket before the second complete game/schema read.
    // That read can consume the remaining freshness window of an otherwise
    // current XR frame. prepare_current independently rechecks the frozen
    // source, held binding, live asset and game state on this same GT.
    bool muzzle_ready{},trigger_in_use{},preparation_budget{};
    if(unified_weapon_fire::manual_profile()){
        // trigger_active denotes a valid analog channel, not a pressed
        // trigger. Keep the input ledger advancing while the trigger is up,
        // but avoid the full socket/schema read on every idle PlayerTick.
        // The publisher's hysteresis keeps a held trigger down above .35.
        trigger_in_use=have_source&&
            ((frame.controllers.hands[1].trigger_active==1&&
              (frame.controllers.hands[1].trigger>.35f||input.awaiting_release()))||
              input.owns_press());
        const auto preparation_now=weaponhand::clock_ns(nullptr);
        //7180: a valid copied source with ~87ms already elapsed was sent
        // through the full socket path, which reached100.8ms and invalidated
        // the input ledger as a side effect. It could never meet the existing
        //20ms dispatch reserve. Leave that work for a newer publication so
        // the existing certified pre-ammo wait can retain a still-current
        // held press. Actual stale input still follows the unchanged release
        // path below; no source stamp, deadline or input edge is renewed.
        preparation_budget=have_source&&preparation_now<=INT64_MAX-20000000&&
            wh::frame_active(frame,preparation_now+20000000);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        // Reuse only an already admitted ray for THIS exact Native Frame and
        // its original clock, with dispatch reserve. Otherwise prepare once.
        const bool cached_ray=state==running&&trigger_in_use&&unified_weapon_fire::ready_for(frame,20000000);
        if(state==running&&trigger_in_use&&!cached_ray&&preparation_budget)nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
#endif
        muzzle_ready=state==running&&trigger_in_use&&unified_weapon_fire::ready_for(frame,20000000);
        if(state==running&&trigger_in_use&&!muzzle_ready&&preparation_budget)
            muzzle_ready=unified_weapon_fire::prepare_current(frame,copied_game)&&unified_weapon_fire::ready_for(frame,20000000);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        static std::uint64_t refresh_reported{};static unsigned refresh_rows{};
        if(refresh_reported!=nativelancerfollow::muzzle_reprepared){
            refresh_reported=nativelancerfollow::muzzle_reprepared;
            if(log_file&&refresh_rows++<8){fwprintf(log_file,L"native_lancer_current_muzzle_reprepared count=%llu tracking_plan=%llu frame=%llu ready=%u\n",
                refresh_reported,nativelancerfollow::muzzle_reprepared_tracking,frame.key.frame,unsigned(muzzle_ready));fflush(log_file);}
        }
#endif
        if(trigger_in_use&&!muzzle_ready&&log_file&&muzzle_prepare_failure_logs<8){
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
            fwprintf(log_file,L"native_lancer_muzzle_prepare_rejected stage=%u follow_stage=%u plan_frame=%llu frame=%llu sync_age_ns=%lld elapsed_ns=%lld\n",
                unified_weapon_fire::prepare_stage,nativelancerfollow::muzzle_stage,nativelancerfollow::plan.source.key.frame,frame.key.frame,
#else
            fwprintf(log_file,L"native_lancer_muzzle_prepare_rejected stage=%u frame=%llu sync_age_ns=%lld elapsed_ns=%lld\n",
                unified_weapon_fire::prepare_stage,frame.key.frame,
#endif
                weaponhand::clock_ns(nullptr)-frame.controllers.sync_ns,
                weaponhand::clock_ns(nullptr)-observation_started);
            fflush(log_file);++muzzle_prepare_failure_logs;
        }
    }
    bool current{};lr::Game game{};
    if(have_source)read_game(actors(frame),&frame,game,&current,&copied_game);
    if(!current&&input.has_actors()&&input.actors().controller.address==pc)read_game(input.actors(),nullptr,game);
    if(have_source&&lancerreload_runtime::blocks_fire(frame.weapon)){
        game.main_hand_held=false;muzzle_ready=false;
    }
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    const auto aim_game=game;
#endif
    if(unified_weapon_fire::manual_profile()){
        game.press_ready=muzzle_ready&&unified_weapon_fire::ready_for(frame,20000000);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
        game.retain_owned_for_muzzle=current&&input.owns_press()&&lancerrefire::certified_for(frame);
        game.defer_new_press_for_muzzle=current&&nativelancerfollow::bound_is_lancer&&nativelancerfollow::bound_key_known&&
            wh::same(nativelancerfollow::bound_key.weapon,frame.weapon)&&
            nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
        if(lancerrefire::release_required()){
            game.press_ready=false;game.retain_owned_for_muzzle=false;game.defer_new_press_for_muzzle=false;
        }
#endif
        if(!game.press_ready){
            unified_weapon_fire::clear_source();
            // Retain the authenticated actor/R2 state for an owed release, but
            // never keep an automatic burst down after losing its muzzle source.
            // An observed UP baseline is still allowed without a muzzle;
            // equipment ownership itself was independently read above.
        }
    }
    const auto now=weaponhand::clock_ns(nullptr);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    // The Native arbiter can grant the magazine on this very press, before
    // PG's reload consumer makes game.busy true. Let that existing owner win
    // now, rather than sending one stock aim press and cancelling next tick.
    const bool part_input_reserved=current&&wh::part_reserves_secondary_trigger(frame,copied_decision);
    const bool aim_allowed=state==running&&current&&!part_input_reserved&&aim_game.main_hand_held&&
        !aim_game.animation&&!aim_game.busy&&frame.controllers.hands[0].trigger_active==1&&
        nativeequipment::current(nullptr,frame);
    const auto aim_result=aim.advance(current?frame.controllers:gears_vr::ControllerInputFrame{},
        current?frame.stereo:gears_vr::StereoSample{},aim_context(aim_game,aim_allowed),now,
        [&](gears_weapon::OriginalL2Action a,std::uint64_t c){
            return execute_aim(a,c,current?&frame:nullptr,current?&aim_game:nullptr);
        },current&&frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);
    if(aim_result==gears_weapon::OriginalL2Status::abort_required){mode.store(fault,std::memory_order_release);return;}
#endif
    const auto previous_sync=input.last_sync_ns(),previous_observed=input.last_observed_ns();
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    const bool initial_owed_before=lancerrefire::has_initial_debt();
#endif
    const auto result=input.advance(current?&frame:nullptr,current,game,weaponhand::clock_ns(nullptr),
        [&](gears_weapon::OriginalR2Action a,std::uint64_t c){return execute(a,c,current?&frame:nullptr,current?&game:nullptr);},
        state==stopping,continuity_matched?&continuity:nullptr);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    // ButtonPress may synchronously capture the real first BeginState AFTER
    // the outer PlayerTick's service pass. Validate that newly captured event
    // now instead of adding a whole game-frame delay to a short trigger press.
    // service retains its full actor/state/original-loop checks; deliver_ready
    // still requires the same fresh Frame, cohort and20ms dispatch reserve.
    if(current&&!initial_owed_before&&lancerrefire::has_initial_debt())lancerrefire::service(pc);
    if(current)lancerrefire::deliver_ready(frame);
#endif
    const auto action_finished=weaponhand::clock_ns(nullptr);
    // Held HUD queries prepare their muzzle in pre_original_held_shot before
    // the original PlayerTick consumes it. Preparing another idle sample here
    // cannot serve that completed query and delays its scene publication.
    // Triggered shots retain their own fresh preparation above; query consumers
    // still enforce the original source age and held-binding checks.
    unified_weapon_fire::AppliedEvidence applied{};
#if defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE
    static thread_local bool hud_arguments_logged{};
    unified_weapon_fire::HudArgumentEvidence hud_arguments{};
    if(log_file&&!hud_arguments_logged&&unified_weapon_fire::hud_argument_evidence(hud_arguments)){
        fwprintf(log_file,L"native_lancer_hud_arguments offset=%llu start_property=%llx end_property=%llx callee_name=%u bytes=",
            hud_arguments.offset,hud_arguments.start_property,hud_arguments.end_property,hud_arguments.callee_name);
        for(auto byte:hud_arguments.bytes)fwprintf(log_file,L"%02x",unsigned(byte));
        fwprintf(log_file,L"\n");fflush(log_file);hud_arguments_logged=true;
    }
#endif
    static thread_local std::array<std::uint64_t,8> last_hud_admission{};
    static thread_local ULONGLONG next_hud_admission{};
    static thread_local unsigned hud_admission_logs{};
    std::array<std::uint64_t,8> hud_admission{};
    if(log_file&&hud_admission_logs<12&&GetTickCount64()>=next_hud_admission&&
       unified_weapon_fire::hud_admission_evidence(hud_admission)&&hud_admission!=last_hud_admission){
        fwprintf(log_file,L"native_lancer_hud_admission live=%llu controller=%llu functions=%llu fields=%llu arguments=%llu capture=%llu applied=%llu accepted=%llu\n",
            hud_admission[0],hud_admission[1],hud_admission[2],hud_admission[3],hud_admission[4],hud_admission[5],hud_admission[6],hud_admission[7]);
        fflush(log_file);last_hud_admission=hud_admission;next_hud_admission=GetTickCount64()+2000;++hud_admission_logs;
    }
    // Bounded, change-only admission summary on the normal tick. No callback I/O.
    static thread_local std::array<std::uint64_t,11> last_admission{};
    static thread_local unsigned admission_logs{};
    static thread_local unsigned extra_rejection_logs{};
    std::array<std::uint64_t,11> admission{};
    if(log_file&&unified_weapon_fire::admission_evidence(admission)&&admission!=last_admission&&
       (admission_logs<24||(admission[6]!=last_admission[6]&&extra_rejection_logs<8))){
        if(admission_logs>=24)++extra_rejection_logs;
        fwprintf(log_file,L"native_lancer_admission unknown=%llu candidates=%llu bind_reject=%llu receiver_reject=%llu caller_reject=%llu bound=%llu stale_sample=%llu identity_reject=%llu source_ready=%llu packet_reject=%llu packet_ready=%llu\n",
            admission[0],admission[1],admission[2],admission[3],admission[4],admission[5],
            admission[6],admission[7],admission[8],admission[9],admission[10]);
        fflush(log_file);last_admission=admission;++admission_logs;
        std::array<std::int64_t,10> rejected{};
        if(unified_weapon_fire::rejected_source_evidence(rejected)){
            fwprintf(log_file,L"native_lancer_rejected_source valid=%lld sync_age_ns=%lld sample_age_ns=%lld held_current=%lld stereo_age_ns=%lld render_age_ns=%lld frame_age_ns=%lld frame_active=%lld tracking_position_difference_ue_x1e6=%lld tracking_quaternion_difference_x1e9=%lld\n",
                rejected[0],rejected[1],rejected[2],rejected[3],rejected[4],rejected[5],rejected[6],rejected[7],rejected[8],rejected[9]);fflush(log_file);
        }
    }
    unified_weapon_fire::FireTraceEvidence trace{};
    if(log_file&&trace_logs<8&&unified_weapon_fire::fire_trace_evidence(trace)&&
       trace.count!=last_trace_count){
        fwprintf(log_file,L"native_lancer_fire_trace count=%llu shot=%llu frame=%llu input_sequence=%llu input_sync_ns=%lld observed_ns=%lld start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g pushed=%.9g,%.9g,%.9g muzzle_origin=%.9g,%.9g,%.9g muzzle_aim=%d,%d,%d\n",
            trace.count,trace.shot,trace.frame,trace.input_sequence,trace.input_sync_ns,trace.observed_ns,
            double(trace.start[0]),double(trace.start[1]),double(trace.start[2]),
            double(trace.end[0]),double(trace.end[1]),double(trace.end[2]),
            double(trace.pushed[0]),double(trace.pushed[1]),double(trace.pushed[2]),
            double(trace.muzzle_origin[0]),double(trace.muzzle_origin[1]),double(trace.muzzle_origin[2]),
            trace.muzzle_aim[0],trace.muzzle_aim[1],trace.muzzle_aim[2]);
        fflush(log_file);last_trace_count=trace.count;++trace_logs;
    }
    if(log_file&&muzzle_logs<8&&unified_weapon_fire::applied_evidence(applied)){
        const auto total=applied.writes[0]+applied.writes[1]+applied.writes[2]+applied.writes[3];
        if(total!=last_muzzle_writes){
            fwprintf(log_file,L"native_lancer_muzzle_applied shot=%llu aim_writes=%llu trace_writes=%llu pushed_writes=%llu physical_writes=%llu frame=%llu input_sequence=%llu input_sync_ns=%lld sampled_ns=%lld right_grip_m=%.9g,%.9g,%.9g follow_target_known=%u grip_world=%.9g,%.9g,%.9g target_mesh=%.9g,%.9g,%.9g aim=%d,%d,%d original_aim=%d,%d,%d origin=%.9g,%.9g,%.9g socket_origin=%.9g,%.9g,%.9g socket_rot=%d,%d,%d\n",
                applied.shot,applied.writes[0],applied.writes[1],applied.writes[2],applied.writes[3],
                applied.frame,applied.input_sequence,applied.input_sync_ns,applied.sampled_ns,
                double(applied.right_grip[0]),double(applied.right_grip[1]),double(applied.right_grip[2]),
                unsigned(applied.follow_target_known),
                applied.world_grip[0],applied.world_grip[1],applied.world_grip[2],
                applied.target_mesh[0],applied.target_mesh[1],applied.target_mesh[2],
                applied.aim[0],applied.aim[1],applied.aim[2],applied.original_aim[0],applied.original_aim[1],applied.original_aim[2],
                double(applied.origin[0]),double(applied.origin[1]),double(applied.origin[2]),
                double(applied.original_socket_origin[0]),double(applied.original_socket_origin[1]),double(applied.original_socket_origin[2]),
                applied.socket_rotation[0],applied.socket_rotation[1],applied.socket_rotation[2]);
            fflush(log_file);last_muzzle_writes=total;++muzzle_logs;
        }
    }
    // Export records only on this normal GT tick. Callback code just copies
    // bounded POD data; it never formats text or touches the file.
    unified_weapon_fire::ShotTrajectoryEvidence trajectory{};
    while(unified_weapon_fire::next_shot_trajectory(trajectory)){
        if(!log_file)continue;
        if(trajectory_logs>=128){
            ++trajectory_omitted;
            if(trajectory_omitted==1){
                fwprintf(log_file,L"native_lancer_trajectory_truncated first_omitted_shot=%llu limit=128\n",trajectory.shot);
                fflush(log_file);
            }
            continue;
        }
        auto const& t=trajectory.trace;
        fwprintf(log_file,L"native_lancer_shot_trajectory shot=%llu epoch=%llu revision=%llu trace_valid=%u write_source_consistent=%u trace_source_consistent=%u trace_count=%llu trace_frame=%llu trace_sequence=%llu trace_sync_ns=%lld trace_observed_ns=%lld write_frame=%llu write_sequence=%llu write_sync_ns=%lld aim_writes=%llu trace_writes=%llu pushed_writes=%llu physical_writes=%llu aim_before_trace=%llu origin_before_trace=%llu pushed_before_trace=%llu start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g pushed=%.9g,%.9g,%.9g muzzle_origin=%.9g,%.9g,%.9g muzzle_aim=%d,%d,%d\n",
            trajectory.shot,trajectory.epoch,trajectory.revision,unsigned(trajectory.trace_valid),
            unsigned(trajectory.write_source_consistent),unsigned(trajectory.trace_source_consistent),t.count,
            t.frame,t.input_sequence,t.input_sync_ns,t.observed_ns,
            trajectory.write_frame,trajectory.write_input_sequence,trajectory.write_input_sync_ns,
            trajectory.writes[0],trajectory.writes[1],trajectory.writes[2],trajectory.writes[3],
            trajectory.writes_at_trace[0],trajectory.writes_at_trace[1],trajectory.writes_at_trace[2],
            double(t.start[0]),double(t.start[1]),double(t.start[2]),
            double(t.end[0]),double(t.end[1]),double(t.end[2]),
            double(t.pushed[0]),double(t.pushed[1]),double(t.pushed[2]),
            double(t.muzzle_origin[0]),double(t.muzzle_origin[1]),double(t.muzzle_origin[2]),
            t.muzzle_aim[0],t.muzzle_aim[1],t.muzzle_aim[2]);
        if(trajectory.trace_valid&&t.sight_zero_known)fwprintf(log_file,
            L"native_lancer_sight_zero shot=%llu epoch=%llu frame=%llu sequence=%llu profile=lancer-original-rings-vr-zero-25m-v1 range_m=25 units=%.9g model_scale=%.9g rear=%.9g,%.9g,%.9g front=%.9g,%.9g,%.9g target=%.9g,%.9g,%.9g\n",
            trajectory.shot,trajectory.epoch,t.frame,t.input_sequence,t.sight_units,t.sight_model_scale,
            t.sight_rear[0],t.sight_rear[1],t.sight_rear[2],t.sight_front[0],t.sight_front[1],t.sight_front[2],
            t.sight_zero[0],t.sight_zero[1],t.sight_zero[2]);
        fwprintf(log_file,L"native_lancer_impact_stage shot=%llu stage=%u\n",trajectory.shot,trajectory.impact_stage);
        if(trajectory.impact_valid)fwprintf(log_file,
            L"native_lancer_shot_impact shot=%llu epoch=%llu revision=%llu location=%.9g,%.9g,%.9g\n",
            trajectory.shot,trajectory.epoch,trajectory.revision,
            double(trajectory.impact_location[0]),double(trajectory.impact_location[1]),double(trajectory.impact_location[2]));
        fflush(log_file);++trajectory_logs;
    }
    const auto dropped=unified_weapon_fire::shot_trajectory_dropped_updates();
    if(log_file&&dropped!=last_trajectory_drop){
        fwprintf(log_file,L"native_lancer_trajectory_overflow dropped_updates=%llu\n",dropped);
        fflush(log_file);last_trajectory_drop=dropped;
    }
    last_status.store(unsigned(result),std::memory_order_release);
    if(result==lr::Status::released||result==lr::Status::recovered||result==lr::Status::tapped){
        next_ammo=GetTickCount64()+500;post_ammo_samples=0;}
    int count{-1};bool key_down{};bool ammo_known{};
    bool ammo_queried{};unsigned ready_after_ammo{};
    // Report a genuinely advancing, released input AFTER the diagnostic query.
    // This is a test observation only: it never re-arms or changes the ledger.
    if(ammo_resume_pending&&result==lr::Status::accepted&&input.source_status()==lr::SourceStatus::ready&&
        current&&game.known&&!game.r2_held&&!input.owns_press()&&
        frame.controllers.hands[1].trigger_active==1&&frame.controllers.hands[1].trigger<=.35f){
        ready_after_ammo=ammo_reads;ammo_resume_pending=false;
    }
    // Diagnostic getters run only while up, once initially and twice after a
    // release. Do not add repeated VM/getter work to the held-trigger path.
    if(state==running&&input.has_actors()&&!input.owns_press()&&ammo_reads<8&&GetTickCount64()>=next_ammo&&
        ((last_ammo<0&&ammo_reads<3)||post_ammo_samples<2)){
        ammo_known=ammo(input.actors(),count,key_down);++ammo_reads;next_ammo=GetTickCount64()+250;
        ammo_queried=true;ammo_resume_pending=true;
        if(post_ammo_samples<2)++post_ammo_samples;
        if(log_file){fwprintf(log_file,L"native_lancer_r2_ammo known=%u ammo=%d key_down=%u after_release_sample=%u reads=%u\n",
            unsigned(ammo_known),count,unsigned(key_down),post_ammo_samples,ammo_reads);fflush(log_file);}
    }
    const auto diagnostic_finished=weaponhand::clock_ns(nullptr);
    if(log_file&&logs<64&&(previous_status!=unsigned(result)||previous_source!=unsigned(input.source_status())||
        ready_after_ammo||ammo_queried||(ammo_known&&count!=last_ammo))){
        fwprintf(log_file,L"native_lancer_r2 status=%u source_status=%u mode=%u owned=%u game_known=%u game_r2=%u before_action=1 frame=%llu sequence=%llu trigger_pressed=%u trigger_released=%u trigger_active=%u trigger_value=%.3f muzzle_ready=%u input_sync_ns=%lld observed_ns=%lld ammo_known=%u ammo=%d previous_sync_ns=%lld previous_observed_ns=%lld game_elapsed_ns=%lld action_elapsed_ns=%lld ammo_queried=%u diagnostic_elapsed_ns=%lld ready_after_ammo_read=%u copy_elapsed_ns=%lld input_elapsed_ns=%lld continuity_matched=%u continuous_poll_gap=%u copied_source=%u copied_frame=%llu copied_input_age_ns=%lld preparation_budget=%u continuity_failure=%u continuity_native_selected=%u\n",
            unsigned(result),unsigned(input.source_status()),state,unsigned(input.owns_press()),unsigned(game.known),unsigned(game.r2_held),
            current?frame.key.frame:0,current?frame.controllers.sequence:0,
            current?frame.controllers.hands[1].pressed[gears_vr::control_trigger]:0,
            current?frame.controllers.hands[1].released[gears_vr::control_trigger]:0,
            current?frame.controllers.hands[1].trigger_active:0,
            current?double(frame.controllers.hands[1].trigger):0.,unsigned(muzzle_ready),
            current?frame.controllers.sync_ns:0,now,unsigned(ammo_known),count,previous_sync,previous_observed,
            now-observation_started,action_finished-now,unsigned(ammo_queried),diagnostic_finished-action_finished,ready_after_ammo,
            copy_finished-source_started,now-source_started,unsigned(continuity_matched),unsigned(input.continuous_poll_gap()),
            unsigned(have_source),frame.key.frame,have_source?now-frame.controllers.sync_ns:-1,unsigned(preparation_budget),unsigned(continuity_failure),unsigned(continuity_native_selected));
        fflush(log_file);++logs;
    }
    previous_status=unsigned(result);previous_source=unsigned(input.source_status());if(ammo_known)last_ammo=count;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    static unsigned previous_aim{~0u},aim_logs{};static bool previous_part_reserved{};
    if(log_file&&aim_logs<24&&(previous_aim!=unsigned(aim_result)||previous_part_reserved!=part_input_reserved)){
        fwprintf(log_file,L"native_lancer_l2 status=%u mode=%u owned=%u game_known=%u game_l2=%u toggle=%u allow=%u part_reserved=%u frame=%llu sequence=%llu left_trigger=%.3f\n",
            unsigned(aim_result),state,unsigned(aim_owns()),unsigned(aim_game.known),unsigned(aim_game.l2_held),
            unsigned(aim_game.toggle_targeting),unsigned(aim_allowed),unsigned(part_input_reserved),current?frame.key.frame:0,
            current?frame.controllers.sequence:0,current?double(frame.controllers.hands[0].trigger):0.);
        fflush(log_file);++aim_logs;
    }
    previous_aim=unsigned(aim_result);previous_part_reserved=part_input_reserved;
#endif
    if(result==lr::Status::abort_required){mode.store(fault,std::memory_order_release);return;}
    if(state==stopping){
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
        if(lancerrefire::has_initial_debt())return; // Keep normal-GT cleanup owner alive.
#endif
        if(input.owns_press()||aim_owns())mode.store(fault,std::memory_order_release);
        else mode.store(stopped,std::memory_order_release);
    }
}
bool yield_for_native_equipment(wh::Frame const& frame) noexcept {
    const auto state=mode.load(std::memory_order_acquire);if(state==off||state==stopped)return true;
    if(state!=running||!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,frame))return false;
    lr::Game game{};if(!read_game(actors(frame),&frame,game))return false;
    // Switching back from a pistol is not a Lancer aiming operation. The
    // Lancer gate was yielded before switching away; feeding a non-Lancer
    // context to its L2 yield would fault the gate even with no owned press.
    // Admit no release/reset here: require both ownership ledgers drained
    // and freshly observed original keys up on the current live actor.
    if(!game.lancer){
        if(input.owns_press()||aim_owns()||!game.r2_known||game.r2_held)return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
        if(!game.l2_known||game.l2_held)return false;
#endif
        return nativeequipment::current(nullptr,frame);
    }
    const auto result=input.yield(game,[&](gears_weapon::OriginalR2Action a,std::uint64_t c){return execute(a,c,nullptr);});
    if(result==lr::Status::abort_required){mode.store(fault,std::memory_order_release);return false;}
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE)
    if(!lancerrefire::release_completed())return false; // Do not switch past an undelivered initial-state debt.
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    const auto aim_result=aim.yield(aim_context(game,false),
        [&](gears_weapon::OriginalL2Action a,std::uint64_t c){return execute_aim(a,c);});
    if(aim_result==gears_weapon::OriginalL2Status::abort_required){mode.store(fault,std::memory_order_release);return false;}
#endif
    lr::Game after{};
    if(input.owns_press()||aim_owns()||!read_game(actors(frame),&frame,after)||after.r2_held)return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    return after.l2_known&&!after.l2_held;
#else
    return true;
#endif
}
DWORD enable(){
    if(weaponhand::current_gt(nullptr)||!enabled.load(std::memory_order_acquire)||!playground::automatic||
        mode.load(std::memory_order_acquire)!=off||!weaponcall::ready())return 1;
    if(!unified_weapon_fire::manual_profile()){
        wchar_t behind[4]{},muted[4]{};
        if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
            GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1')return 2;
    }
    if(unified_weapon_fire::manual_profile()&&!unified_weapon_fire::consumer_compiled())return 4;
    if(!button_name(L'R',r2)||!input.reset_drained())return 3;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    if(aim_owns()||!button_name(L'L',l2))return 3;
    aim.reset();
#endif
    candidate_hold={};candidate_class_known=candidate_is_lancer=false;
    muzzle_logs=0;trace_logs=0;last_muzzle_writes=0;last_trace_count=0;
    muzzle_prepare_failure_logs=0;execute_failure_logs=0;
    logs=0;previous_status=previous_source=~0u;last_ammo=-1;next_ammo=0;ammo_reads=0;post_ammo_samples=2;ammo_resume_pending=false;
    if(unified_weapon_fire::manual_profile()&&!unified_weapon_fire::set_running(true))return 4;
    mode.store(running,std::memory_order_release);
    playground::message(unified_weapon_fire::manual_profile()?L"native_lancer_r2_enabled_unified_muzzle":L"native_lancer_r2_enabled_original_aim_only");return 0;
}
DWORD disable(){
    if(weaponhand::current_gt(nullptr))return 1;
    if(unified_weapon_fire::manual_profile())unified_weapon_fire::set_running(false);
    auto state=mode.load(std::memory_order_acquire);if(state==off)return 0;if(state==fault)return 4;
    if(state==running){mode.store(stopping,std::memory_order_release);state=stopping;}
    const auto end=GetTickCount64()+2000;
    while(state==stopping&&GetTickCount64()<end){Sleep(1);state=mode.load(std::memory_order_acquire);}
    if(state==stopping)return 2;if(state==fault)return 4;if(state!=stopped)return 3;
    mode.store(off,std::memory_order_release);playground::message(L"native_lancer_r2_disabled");return 0;
}
} // namespace vrfire
