// Private original-button consumer on the normal authenticated Native path.
// No bridge reread, pose or damage property writer. The explicit unified
// profile requires the separate same-shot muzzle result consumer before enable.
#include "weapon_unified_fire.h"
namespace vrfire {
namespace lr=gears_lancer_r2;
namespace wh=gears_weapon_hand;
namespace meta=gripdamage_metadata;
enum : unsigned {off=0,running=1,stopping=2,stopped=3,fault=4};
std::atomic<unsigned> mode{off},last_status{};
lr::Input input;
Name r2{};
unsigned logs{};int last_ammo{-1};unsigned previous_status{~0u},previous_source{~0u};
ULONGLONG next_ammo{};unsigned ammo_reads{},post_ammo_samples{2};
bool ammo_resume_pending{};
unsigned muzzle_logs{};std::uint64_t last_muzzle_writes{};
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
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};bool known{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,h.weapon))return false;
    const bool lancer=weaponhand::derives(r,h.weapon.address,L"Weap_AssaultRifle",L"WarfareGame",&known);
    if(!known)return false;
    candidate_hold=h;candidate_is_lancer=lancer;candidate_class_known=true;return lancer;
}
bool button_name() noexcept {
    Array names{},again{};if(!copy(&names,base+0x1c34800,sizeof names)||!names.data||
        names.count<1000||names.count>names.capacity||names.capacity>4000000)return false;
    const auto end=GetTickCount64()+2000;
    for(int i=0;i<names.count;++i){
        if(!(i%256)&&GetTickCount64()>=end)return false;
        uptr entry{};unsigned index{};wchar_t text[3]{};
        if(!copy(&entry,names.data+size_t(i)*8,8)||!entry||!copy(&index,entry,4)||index!=unsigned(i)||
            !copy(text,entry+20,sizeof text))continue;
        if(text[0]==L'R'&&text[1]==L'2'&&!text[2]){
            if(!copy(&again,base+0x1c34800,sizeof again)||memcmp(&names,&again,sizeof names))return false;
            r2={unsigned(i),0};return true;
        }
    }
    return false;
}
bool name_current() noexcept {
    Array names{};uptr entry{};unsigned index{};wchar_t text[3]{};
    return !r2.number&&copy(&names,base+0x1c34800,sizeof names)&&names.data&&names.count>1000&&
        names.count<=names.capacity&&names.capacity<=4000000&&r2.index<unsigned(names.count)&&
        copy(&entry,names.data+size_t(r2.index)*8,8)&&entry&&copy(&index,entry,4)&&index==r2.index&&
        copy(text,entry+20,sizeof text)&&text[0]==L'R'&&text[1]==L'2'&&!text[2];
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
    weaponhand::BoolField held{},again{};bool known{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,expected.controller)||
        !weaponhand::object(r,expected.weapon))return false;
    const bool lancer=weaponhand::derives(r,expected.weapon.address,L"Weap_AssaultRifle",L"WarfareGame",&known);
    if(!known||!weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_R2",held))return false;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(unsigned i=0;i<identities;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure fresh{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,fresh)||
            !meta::same_structure(schema.structures[i],fresh))return false;}
    meta::Table objects{},names{};
    if(!weaponhand::boolean(r,expected.controller.address,L"bHoldingButton_R2",again)||
        !meta::same_property(held.property,again.property)||held.mask!=again.mask||
        (held.bits&held.mask)!=(again.bits&again.mask)||!meta::tables(base,objects,names)||
        !meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
        !weaponhand::observe(nullptr,identity,weaponhand::clock_ns(nullptr),after)||!wh::same_game(before,after))return false;
    if(source_current&&!weaponhand::client.armed()){*source_current=false;return false;}
    out={expected,true,lancer,after.main_hand_held,after.animation_owns_hands,after.busy,true,bool(held.bits&held.mask)};
    return true;
}
bool execute(gears_weapon::OriginalR2Action action,std::uint64_t controller,wh::Frame const* source,
    lr::Game const* tick_game=nullptr) noexcept {
    auto reject=[&](wchar_t const* stage) noexcept {
        if(log_file&&execute_failure_logs<8){
            fwprintf(log_file,L"native_lancer_execute_rejected stage=%ls press=%u frame=%llu age_ns=%lld held=%u animation=%u busy=%u\n",
                stage,unsigned(action==gears_weapon::OriginalR2Action::press),source?source->key.frame:0,
                source?weaponhand::clock_ns(nullptr)-source->controllers.sync_ns:0,
                unsigned(tick_game&&tick_game->main_hand_held),unsigned(tick_game&&tick_game->animation),unsigned(tick_game&&tick_game->busy));
            fflush(log_file);++execute_failure_logs;
        }
        return false;
    };
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
        !input.has_actors()||controller!=input.actors().controller.address||!name_current())return reject(L"entry");
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
        if(unified_weapon_fire::manual_profile()&&!unified_weapon_fire::ready_for(*source))return reject(L"muzzle_source");
        lancermain_asset::Evidence asset{};
        if(lancermain_asset::read(*source,asset)!=lancermain_asset::Status::ready||
            !nativeequipment::attached_frame(*source,asset.component.address)||
            !nativeequipment::current(nullptr,*source))return reject(L"attachment");
    }else if(!game.r2_held)return true; // Already up, freshly certified; no redundant release.
    playground::Args call(controller,action==gears_weapon::OriginalR2Action::press?L"ButtonPress":L"ButtonRelease");
    const auto parameter=call.prop(L"ButtonName");
    if(!call.valid||call.r.size!=sizeof(Name)||!parameter||get<unsigned>(parameter+0x70)!=1||
        get<unsigned>(parameter+0x74)!=sizeof(Name)||!playground::named(get<uptr>(parameter+0x50),L"NameProperty")||
       (get<unsigned long long>(parameter+0x78)&0x580)!=0x80||!call.set(L"ButtonName",r2).valid)return reject(L"button_schema");
    if(!call.run())return reject(L"button_call");
    return true;
}
bool ammo(lr::Actors const& owner,int& count,bool& key_down) noexcept {
    lr::Game game{};if(!read_game(owner,nullptr,game)||!game.lancer)return false;
    playground::Args query(owner.weapon.address,L"GetTotalAmmoInGun");
    if(!vrequip::result_property(query,L"IntProperty",4)||!query.run())return false;
    const auto value=query.output<int>();lr::Game after{};
    if(value<0||value>1000000||!read_game(owner,nullptr,after)||!after.lancer)return false;
    count=value;key_down=after.r2_held;return true;
}
void tick(uptr pc) noexcept {
    const auto state=mode.load(std::memory_order_acquire);if(state!=running&&state!=stopping)return;
    const bool quiet=weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready();
    if(!lr::tick_admitted(quiet,pc,0,0))return;
    // Equipment must first commit a real held binding. Repeated fire metadata
    // reads while unheld starve its input baseline on the same GT. No source
    // or R2 state is fabricated here; an owed release and stopping still take
    // the complete existing path, even after equipment clears the binding.
    if(state==running&&unified_weapon_fire::manual_profile()&&
       !input.owns_press()&&!input.has_actors()&&!held_lancer_candidate()){
        unified_weapon_fire::clear_source();return;
    }
    const auto source_started=weaponhand::clock_ns(nullptr);
    wh::Frame frame{};wh::GripGameState copied_game{};
    const bool have_source=state==running&&weaponhand::client.copy_frame_on_gt(frame,copied_game);
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
    bool muzzle_ready{};
    if(unified_weapon_fire::manual_profile()){
        // trigger_active denotes a valid analog channel, not a pressed
        // trigger. Keep the input ledger advancing while the trigger is up,
        // but avoid the full socket/schema read on every idle PlayerTick.
        // The publisher's hysteresis keeps a held trigger down above .35.
        const bool trigger_in_use=have_source&&
            ((frame.controllers.hands[1].trigger_active==1&&
              (frame.controllers.hands[1].trigger>.35f||input.awaiting_release()))||
              input.owns_press());
        muzzle_ready=state==running&&trigger_in_use&&
            unified_weapon_fire::prepare_current(frame,copied_game);
        if(trigger_in_use&&!muzzle_ready&&log_file&&muzzle_prepare_failure_logs<8){
            fwprintf(log_file,L"native_lancer_muzzle_prepare_rejected stage=%u frame=%llu sync_age_ns=%lld elapsed_ns=%lld\n",
                unified_weapon_fire::prepare_stage,frame.key.frame,
                weaponhand::clock_ns(nullptr)-frame.controllers.sync_ns,
                weaponhand::clock_ns(nullptr)-observation_started);
            fflush(log_file);++muzzle_prepare_failure_logs;
        }
    }
    bool current{};lr::Game game{};
    if(have_source)read_game(actors(frame),&frame,game,&current,&copied_game);
    if(!current&&input.has_actors()&&input.actors().controller.address==pc)read_game(input.actors(),nullptr,game);
    if(unified_weapon_fire::manual_profile()){
        if(!muzzle_ready){
            unified_weapon_fire::clear_source();
            // Retain the authenticated actor/R2 state for an owed release, but
            // never keep an automatic burst down after losing its muzzle source.
            game.main_hand_held=false;
        }
    }
    const auto now=weaponhand::clock_ns(nullptr);
    const auto previous_sync=input.last_sync_ns(),previous_observed=input.last_observed_ns();
    const auto result=input.advance(current?&frame:nullptr,current,game,now,
        [&](gears_weapon::OriginalR2Action a,std::uint64_t c){return execute(a,c,current?&frame:nullptr,current?&game:nullptr);},state==stopping);
    const auto action_finished=weaponhand::clock_ns(nullptr);
    unified_weapon_fire::AppliedEvidence applied{};
    if(log_file&&muzzle_logs<8&&unified_weapon_fire::applied_evidence(applied)){
        const auto total=applied.writes[0]+applied.writes[1]+applied.writes[2]+applied.writes[3];
        if(total!=last_muzzle_writes){
            fwprintf(log_file,L"native_lancer_muzzle_applied shot=%llu aim_writes=%llu trace_writes=%llu pushed_writes=%llu physical_writes=%llu input_sequence=%llu input_sync_ns=%lld sampled_ns=%lld right_grip_m=%.9g,%.9g,%.9g aim=%d,%d,%d original_aim=%d,%d,%d origin=%.9g,%.9g,%.9g\n",
                applied.shot,applied.writes[0],applied.writes[1],applied.writes[2],applied.writes[3],
                applied.input_sequence,applied.input_sync_ns,applied.sampled_ns,
                double(applied.right_grip[0]),double(applied.right_grip[1]),double(applied.right_grip[2]),
                applied.aim[0],applied.aim[1],applied.aim[2],applied.original_aim[0],applied.original_aim[1],applied.original_aim[2],
                double(applied.origin[0]),double(applied.origin[1]),double(applied.origin[2]));
            fflush(log_file);last_muzzle_writes=total;++muzzle_logs;
        }
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
        fwprintf(log_file,L"native_lancer_r2 status=%u source_status=%u mode=%u owned=%u game_known=%u game_r2=%u before_action=1 frame=%llu sequence=%llu trigger_pressed=%u trigger_released=%u trigger_active=%u trigger_value=%.3f muzzle_ready=%u input_sync_ns=%lld observed_ns=%lld ammo_known=%u ammo=%d previous_sync_ns=%lld previous_observed_ns=%lld game_elapsed_ns=%lld action_elapsed_ns=%lld ammo_queried=%u diagnostic_elapsed_ns=%lld ready_after_ammo_read=%u copy_elapsed_ns=%lld input_elapsed_ns=%lld\n",
            unsigned(result),unsigned(input.source_status()),state,unsigned(input.owns_press()),unsigned(game.known),unsigned(game.r2_held),
            current?frame.key.frame:0,current?frame.controllers.sequence:0,
            current?frame.controllers.hands[1].pressed[gears_vr::control_trigger]:0,
            current?frame.controllers.hands[1].released[gears_vr::control_trigger]:0,
            current?frame.controllers.hands[1].trigger_active:0,
            current?double(frame.controllers.hands[1].trigger):0.,unsigned(muzzle_ready),
            current?frame.controllers.sync_ns:0,now,unsigned(ammo_known),count,previous_sync,previous_observed,
            now-observation_started,action_finished-now,unsigned(ammo_queried),diagnostic_finished-action_finished,ready_after_ammo,
            copy_finished-source_started,now-source_started);
        fflush(log_file);++logs;
    }
    previous_status=unsigned(result);previous_source=unsigned(input.source_status());if(ammo_known)last_ammo=count;
    if(result==lr::Status::abort_required){mode.store(fault,std::memory_order_release);return;}
    if(state==stopping){
        if(input.owns_press())mode.store(fault,std::memory_order_release);
        else mode.store(stopped,std::memory_order_release);
    }
}
bool yield_for_native_equipment(wh::Frame const& frame) noexcept {
    const auto state=mode.load(std::memory_order_acquire);if(state==off||state==stopped)return true;
    if(state!=running||!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,frame))return false;
    lr::Game game{};if(!read_game(actors(frame),&frame,game))return false;
    const auto result=input.yield(game,[&](gears_weapon::OriginalR2Action a,std::uint64_t c){return execute(a,c,nullptr);});
    if(result==lr::Status::abort_required){mode.store(fault,std::memory_order_release);return false;}
    lr::Game after{};return !input.owns_press()&&read_game(actors(frame),&frame,after)&&!after.r2_held;
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
    if(!button_name()||!input.reset_drained())return 3;
    candidate_hold={};candidate_class_known=candidate_is_lancer=false;
    muzzle_logs=0;last_muzzle_writes=0;muzzle_prepare_failure_logs=0;execute_failure_logs=0;
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
