// Private original-button consumer on the normal authenticated Native path.
// No bridge reread, pose write, aim override or damage write is installed.
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
lr::Actors actors(wh::Frame const& f) noexcept {return {f.world,f.controller,f.pawn,f.mesh,f.weapon};}
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
bool read_game(lr::Actors const& expected,wh::Frame const* source,lr::Game& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr)||!lr::valid(expected))return false;
    // This identity-only value is solely for fresh metadata observation. It
    // is never published, passed to Input, or used to renew tracking lifetime.
    wh::Frame identity{};identity.world=expected.world;identity.controller=expected.controller;
    identity.pawn=expected.pawn;identity.mesh=expected.mesh;identity.weapon=expected.weapon;
    if(source){if(!lr::same(expected,actors(*source)))return false;identity=*source;}
    wh::GripGameState before{},after{};
    if(!weaponhand::observe(nullptr,identity,weaponhand::clock_ns(nullptr),before)||!before.known)return false;
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
    out={expected,true,lancer,after.main_hand_held,after.animation_owns_hands,after.busy,true,bool(held.bits&held.mask)};
    return true;
}
bool execute(gears_weapon::OriginalR2Action action,std::uint64_t controller,wh::Frame const* source) noexcept {
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready()||
        !input.has_actors()||controller!=input.actors().controller.address||!name_current())return false;
    lr::Game game{};
    if(!read_game(input.actors(),source,game)||!game.lancer)return false;
    if(action==gears_weapon::OriginalR2Action::press){
        if(!source||!game.main_hand_held||game.busy||game.animation||
            !nativeequipment::current(nullptr,*source))return false;
        lancermain_asset::Evidence asset{};
        if(lancermain_asset::read(*source,asset)!=lancermain_asset::Status::ready||
            !nativeequipment::attached_frame(*source,asset.component.address)||
            !nativeequipment::current(nullptr,*source))return false;
    }else if(!game.r2_held)return true; // Already up, freshly certified; no redundant release.
    playground::Args call(controller,action==gears_weapon::OriginalR2Action::press?L"ButtonPress":L"ButtonRelease");
    const auto parameter=call.prop(L"ButtonName");
    if(!call.valid||call.r.size!=sizeof(Name)||!parameter||get<unsigned>(parameter+0x70)!=1||
        get<unsigned>(parameter+0x74)!=sizeof(Name)||!playground::named(get<uptr>(parameter+0x50),L"NameProperty")||
        (get<unsigned long long>(parameter+0x78)&0x580)!=0x80||!call.set(L"ButtonName",r2).valid)return false;
    return call.run();
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
    wh::Frame frame{};const bool have_source=state==running&&weaponhand::client.copy_frame_on_gt(frame);
    // Other PlayerControllers do not consume or invalidate this player's R2.
    // A freshly authenticated change of owner still reaches the policy below
    // so an owed release cannot be silently adopted by the new controller.
    if(!lr::tick_admitted(quiet,pc,have_source?frame.controller.address:0,
        input.has_actors()?input.actors().controller.address:0))return;
    const auto observation_started=weaponhand::clock_ns(nullptr);
    const bool current=have_source&&nativeequipment::current(nullptr,frame);
    lr::Game game{};
    if(current)read_game(actors(frame),&frame,game);
    else if(input.has_actors()&&input.actors().controller.address==pc)read_game(input.actors(),nullptr,game);
    const auto now=weaponhand::clock_ns(nullptr);
    const auto previous_sync=input.last_sync_ns(),previous_observed=input.last_observed_ns();
    const auto result=input.advance(current?&frame:nullptr,current,game,now,
        [&](gears_weapon::OriginalR2Action a,std::uint64_t c){return execute(a,c,current?&frame:nullptr);},state==stopping);
    const auto action_finished=weaponhand::clock_ns(nullptr);
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
        ready_after_ammo||(ammo_known&&count!=last_ammo))){
        fwprintf(log_file,L"native_lancer_r2 status=%u source_status=%u mode=%u owned=%u game_known=%u game_r2=%u before_action=1 frame=%llu sequence=%llu trigger_pressed=%u trigger_released=%u input_sync_ns=%lld observed_ns=%lld ammo_known=%u ammo=%d previous_sync_ns=%lld previous_observed_ns=%lld game_elapsed_ns=%lld action_elapsed_ns=%lld ammo_queried=%u diagnostic_elapsed_ns=%lld ready_after_ammo_read=%u\n",
            unsigned(result),unsigned(input.source_status()),state,unsigned(input.owns_press()),unsigned(game.known),unsigned(game.r2_held),
            current?frame.key.frame:0,current?frame.controllers.sequence:0,
            current?frame.controllers.hands[1].pressed[gears_vr::control_trigger]:0,
            current?frame.controllers.hands[1].released[gears_vr::control_trigger]:0,
            current?frame.controllers.sync_ns:0,now,unsigned(ammo_known),count,previous_sync,previous_observed,
            now-observation_started,action_finished-now,unsigned(ammo_queried),diagnostic_finished-action_finished,ready_after_ammo);
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
    wchar_t behind[4]{},muted[4]{};
    if(GetEnvironmentVariableW(L"GEARSVR_TEST_WINDOWS_BEHIND",behind,4)!=1||behind[0]!=L'1'||
        GetEnvironmentVariableW(L"GEARSVR_TEST_MUTE_GAME",muted,4)!=1||muted[0]!=L'1')return 2;
    if(!button_name()||!input.reset_drained())return 3;
    logs=0;previous_status=previous_source=~0u;last_ammo=-1;next_ammo=0;ammo_reads=0;post_ammo_samples=2;ammo_resume_pending=false;
    mode.store(running,std::memory_order_release);playground::message(L"native_lancer_r2_enabled_original_aim_only");return 0;
}
DWORD disable(){
    if(weaponhand::current_gt(nullptr))return 1;
    auto state=mode.load(std::memory_order_acquire);if(state==off)return 0;if(state==fault)return 4;
    if(state==running){mode.store(stopping,std::memory_order_release);state=stopping;}
    const auto end=GetTickCount64()+2000;
    while(state==stopping&&GetTickCount64()<end){Sleep(1);state=mode.load(std::memory_order_acquire);}
    if(state==stopping)return 2;if(state==fault)return 4;if(state!=stopped)return 3;
    mode.store(off,std::memory_order_release);playground::message(L"native_lancer_r2_disabled");return 0;
}
} // namespace vrfire
