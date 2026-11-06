// Included inside nativeequipment, after its identity/attachment readers.
// One normal-player adapter over the EXISTING WeaponStowLease and original
// retail holster helper. No arena actor, raw field write or new pose writer.
namespace stow_native {
using Attachment=gears_weapon::StowAttachment;
struct Key {
    // world,level,controller,pawn,inventory,pawn mesh,weapon,weapon mesh,asset
    std::array<abi::ObjectIdentity,9> objects{};
    std::array<uptr,4> slots{};
    std::array<unsigned char,52> original{};
    uint64_t epoch{};
    bool operator==(Key const& b) const noexcept {
        if(epoch!=b.epoch||slots!=b.slots||original!=b.original)return false;
        for(unsigned i=0;i<objects.size();++i)if(!abi::same(objects[i],b.objects[i]))return false;
        return true;
    }
};
gears_weapon::WeaponStowLease<Key> lease;
// The original equip animation can reattach the gun after the normal holster
// helper. Keep only its visibility during that unheld transition; never skip
// the original state/animation or treat a hidden component as a held weapon.
gears_weapon::WeaponStowLease<Key> equip_visibility;
std::atomic<bool> stop_requested{},outstanding{};
bool mutating{};uint64_t epoch{};
unsigned report_count{};
struct FirstRevoke {
    uint64_t epoch{};uptr object{},function{},entry{},caller{},caller_object{};
    unsigned script_offset{},code_count{};std::array<unsigned char,24> code{};
};
FirstRevoke first_revoke{};uint64_t explained_epoch{};unsigned read_stage{};
uint64_t empty_touch_skipped{},empty_touch_reported{};
uint64_t decal_attach_skipped{},decal_attach_reported{};
struct IdleKey {
    abi::ObjectIdentity world{},pawn{},weapon{};
    uint64_t native_started{},cookie{},generation{};
    bool operator==(IdleKey const& b) const noexcept {
        return abi::same(world,b.world)&&abi::same(pawn,b.pawn)&&abi::same(weapon,b.weapon)&&
            native_started==b.native_started&&cookie==b.cookie&&generation==b.generation;
    }
};
gears_weapon::StowIdleCadence<IdleKey> idle_cadence;
bool profile() noexcept {
#if defined(GEARS_PLAYGROUND_UNIFIED_MANUAL_PROFILE) && GEARS_PLAYGROUND_UNIFIED_MANUAL_PROFILE
    return true;
#else
    return false;
#endif
}
void publish() noexcept {outstanding.store(lease.dirty()||equip_visibility.dirty(),std::memory_order_release);}
using InputFenceFn=DWORD(WINAPI*)(void*);
InputFenceFn input_function(char const* name) noexcept {
    const auto host_module=weaponhand::native_module;
    const auto function=host_module?GetProcAddress(host_module,name):nullptr;
    return function&&weaponhand::executable(function,host_module)?reinterpret_cast<InputFenceFn>(function):nullptr;
}
bool original_action_pending() noexcept {
    const auto read=input_function("GearsCaptureVrGamepadOriginalActionPending");
    return !read||read(nullptr)!=0;
}
bool input_fence(bool arm) noexcept {
    const auto set=input_function("GearsCaptureVrGamepadStowFence");
    return set&&set(arm?reinterpret_cast<void*>(uintptr_t{1}):nullptr)==ERROR_SUCCESS;
}
bool sync_input_fence() noexcept {return input_fence(lease.dirty()||equip_visibility.dirty());}
bool certified_empty_pickup_touch(uptr function) noexcept {
    // NoAuth classification only: no engine calls or journal change. Trust
    // neither a name nor an offline decompilation without these live bytes.
    if(!registered(function)||!playground::named(get<uptr>(function+0x50),L"Function"))return false;
    const auto owner_state=get<uptr>(function+0x40);
    if(!registered(owner_state)||!playground::named(owner_state,L"Pickup")||
       !playground::named(get<uptr>(owner_state+0x50),L"State"))return false;
    const auto owner_class=get<uptr>(owner_state+0x40);
    if(!registered(owner_class)||!playground::named(owner_class,L"WarDroppedPickup")||
       !playground::named(get<uptr>(owner_class+0x50),L"Class"))return false;
    const auto owner_package=get<uptr>(owner_class+0x40);
    if(!registered(owner_package)||!playground::named(owner_package,L"WarfareGame")||get<uptr>(owner_package+0x40))return false;
    Array script{},after{};std::array<unsigned char,3> body{},again{};
    const auto flags=get<unsigned>(function+0xf4);const auto result=get<unsigned short>(function+0x108);
    const auto entry=get<uptr>(function+0x114);
    if(!copy(&script,function+0x8c,sizeof(script))||!script.data||script.count!=3||script.capacity<3||script.capacity>64||
       !copy(body.data(),script.data,body.size())||
       !gears_weapon::empty_pickup_touch_body(body,unsigned(script.count),flags,result,entry==base+0x1d6ca0)||
       !copy(&after,function+0x8c,sizeof(after))||std::memcmp(&script,&after,sizeof(script))||
       !copy(again.data(),script.data,again.size())||again!=body||get<unsigned>(function+0xf4)!=flags||
       get<unsigned short>(function+0x108)!=result||get<uptr>(function+0x114)!=entry)return false;
    return registered(function)&&get<uptr>(function+0x40)==owner_state&&get<uptr>(owner_state+0x40)==owner_class&&
        get<uptr>(owner_class+0x40)==owner_package;
}
bool identities(Key const& k) noexcept {
    if(GetCurrentThreadId()!=game_thread.load(std::memory_order_acquire))return false;
    for(auto const& wanted:k.objects){abi::ObjectIdentity actual{};
        if(!identify(nullptr,wanted.address,actual)||!abi::same(wanted,actual))return false;}
    const auto& o=k.objects;
    if(get<uptr>(o[0].address+0x40)!=o[1].address||get<uptr>(o[2].address+0x40)!=o[1].address||
       get<uptr>(o[3].address+0x40)!=o[1].address||
       playground::pointer(o[2].address,L"WorldInfo")!=o[0].address||
       playground::pointer(o[2].address,L"Pawn")!=o[3].address||
       playground::pointer(o[3].address,L"Controller")!=o[2].address||
       playground::pointer(o[3].address,L"InvManager")!=o[4].address||
       playground::pointer(o[3].address,L"Mesh")!=o[5].address||
       playground::pointer(o[3].address,L"Weapon")!=o[6].address||
       playground::pointer(o[6].address,L"Instigator")!=o[3].address||
       playground::pointer(o[6].address,L"Mesh")!=o[7].address||
       playground::pointer(o[7].address,L"SkeletalMesh")!=o[8].address)return false;
    for(std::uint8_t i=0;i<4;++i){uptr item{};
        if(!vrequip::read_slot(o[4].address,i,item)||item!=k.slots[i])return false;}
    return true;
}
bool read(Key const& k,gears_weapon::StowState<Key>& out) noexcept {
    out={};read_stage=1;if(!identities(k))return false;
    abi::Frame metadata{};metadata.world=k.objects[0];metadata.controller=k.objects[2];
    metadata.pawn=k.objects[3];metadata.mesh=k.objects[5];metadata.weapon=k.objects[6];
    std::array<unsigned char,52> actual{};bool detached{};
    read_stage=2;if(!read_attached_frame(metadata,k.objects[7].address,true,&actual,&detached,false))return false;
    read_stage=3;if(!identities(k))return false;
    out.key=k;out.ready=true;
    out.attachment=detached?Attachment::detached:actual==k.original?Attachment::original_hand:Attachment::other;
    read_stage=10+unsigned(out.attachment);
    return true;
}
gears_weapon::StowPhase phase(uptr weapon) noexcept {
    // The original helper must not cut through a latent firing/reload state.
    // Same exact Core.GetStateName declaration used by the existing stow
    // implementation; no trial arena state or inferred animation timer.
    playground::Args call(weapon,L"GetStateName");const auto result=call.prop(L"ReturnValue");
    if(!call.valid||call.r.size!=8||get<unsigned short>(call.r.function+0x108)!=0||!result||
       get<unsigned>(result+0x70)!=1||get<unsigned>(result+0x74)!=8||get<unsigned>(result+0x94)!=0||
       !playground::named(get<uptr>(result+0x50),L"NameProperty")||
       (get<unsigned long long>(result+0x78)&0x580)!=0x580||!call.run())return gears_weapon::StowPhase::unknown;
    const auto weapon_state=call.output<Name>();if(weapon_state.number)return gears_weapon::StowPhase::unknown;
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||weapon_state.index>=unsigned(names.count))return gears_weapon::StowPhase::unknown;
    const auto entry=get<uptr>(names.data+size_t(weapon_state.index)*8);wchar_t text[7]{};
    if(!entry||get<unsigned>(entry)!=weapon_state.index||!copy(text,entry+20,sizeof text))return gears_weapon::StowPhase::unknown;
    if(!std::wmemcmp(text,L"Active",7))return gears_weapon::StowPhase::active;
    wchar_t equipping[16]{};
    if(copy(equipping,entry+20,sizeof equipping)&&!std::wmemcmp(equipping,L"WeaponEquipping",16))return gears_weapon::StowPhase::equipping;
    return gears_weapon::StowPhase::unknown;
}
bool clear_equip_transition(uptr pawn) noexcept {
    // The original helper also has melee/turret side effects. The narrow early
    // visual path is permitted only outside both, using typed current fields.
    const auto special=playground::property(pawn,L"SpecialMove"),vehicle=playground::property(pawn,L"DrivenVehicle");
    if(!special||!vehicle||get<unsigned>(special+0x70)!=1||get<unsigned>(special+0x74)!=1||
       get<unsigned>(vehicle+0x70)!=1||get<unsigned>(vehicle+0x74)!=8||
       !playground::named(get<uptr>(special+0x50),L"ByteProperty")||
       !playground::named(get<uptr>(vehicle+0x50),L"ObjectProperty"))return false;
    const auto special_offset=get<unsigned>(special+0x94),vehicle_offset=get<unsigned>(vehicle+0x94);
    unsigned char move{};uptr driven{};
    return special_offset<65536&&vehicle_offset<65536&&copy(&move,pawn+special_offset,sizeof move)&&
        copy(&driven,pawn+vehicle_offset,sizeof driven)&&move==0&&!driven;
}
struct Backend {
    Key expected{};abi::Frame const* request{};
    bool early_equip{};
    bool read(gears_weapon::StowState<Key>& out) noexcept {return stow_native::read(expected,out);}
    bool set_holstered(bool value) noexcept {
        if(mutating||!identities(expected)||!(value?domain_ready():restoration_quiet()))return false;
        if(value&&(!request||stop_requested.load(std::memory_order_acquire)||!current(nullptr,*request)||
           !clear_equip_transition(request->pawn.address)))return false;
        if(value&&early_equip&&(phase(request->weapon.address)!=gears_weapon::StowPhase::equipping||
           !clear_equip_transition(request->pawn.address)||!current(nullptr,*request)))return false;
        playground::Args call(expected.objects[3].address,L"SetWeaponTemporarilyHolstered");
        const auto parameter=call.prop(L"bHolstered");
        if(!call.valid||call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0xffff||
           !parameter||get<unsigned>(parameter+0x70)!=1||get<unsigned>(parameter+0x74)!=4||
           get<unsigned>(parameter+0x94)!=0||
           !playground::named(get<uptr>(parameter+0x50),L"BoolProperty")||
           (get<unsigned long long>(parameter+0x78)&0x580)!=0x80)return false;
        const auto mask=get<unsigned>(parameter+0xd0);if(!mask||(mask&(mask-1)))return false;
        if(!identities(expected)||(value&&!current(nullptr,*request)))return false;
        struct Mutation {Mutation(){mutating=true;}~Mutation(){mutating=false;}} mutation;
        call.flag(L"bHolstered",value);return call.run();
    }
};
struct VisibilityBackend {
    Key expected{};abi::Frame const* request{};
    bool active_idle{};
    unsigned reads{};bool first_hidden{},unhide_attempted{},unhide_called{};
    bool read(gears_weapon::StowState<Key>& out) noexcept {
        out={};if(!identities(expected))return false;
        weaponhand::meta::Snapshot schema{};schema.module_base=base;
        weaponhand::meta::Reader reader{schema,weaponhand::meta::Stage::final_check};weaponhand::BoolField hidden{};
        if(!weaponhand::meta::tables(base,schema.objects,schema.names)||
           !weaponhand::boolean(reader,expected.objects[7].address,L"HiddenGame",hidden)||!identities(expected))return false;
        out.key=expected;out.ready=true;
        out.attachment=(hidden.bits&hidden.mask)?Attachment::detached:Attachment::original_hand;
        if(!reads++)first_hidden=out.attachment==Attachment::detached;
        return true;
    }
    bool set_holstered(bool value) noexcept {
        if(mutating||!identities(expected)||!(value?domain_ready():restoration_quiet()))return false;
        if(value&&(!request||stop_requested.load()||!current(nullptr,*request)||
           phase(request->weapon.address)!=(active_idle?gears_weapon::StowPhase::active:gears_weapon::StowPhase::equipping)||
           !clear_equip_transition(request->pawn.address)))return false;
        playground::Args call(expected.objects[7].address,L"SetHidden");const auto parameter=call.prop(L"NewHidden");
        if(!call.valid||call.r.size!=4||get<unsigned short>(call.r.function+0x108)!=0xffff||
           !(get<unsigned>(call.r.function+0xf4)&0x400)||
           !playground::named(get<uptr>(call.r.function+0x40),L"PrimitiveComponent")||
           !playground::named(get<uptr>(get<uptr>(call.r.function+0x40)+0x40),L"Engine")||
           !parameter||get<unsigned>(parameter+0x70)!=1||get<unsigned>(parameter+0x74)!=4||
           get<unsigned>(parameter+0x94)!=0||!playground::named(get<uptr>(parameter+0x50),L"BoolProperty")||
           (get<unsigned long long>(parameter+0x78)&0x580)!=0x80)return false;
        const auto mask=get<unsigned>(parameter+0xd0);if(!mask||(mask&(mask-1))||
           !identities(expected)||(value&&!current(nullptr,*request)))return false;
        struct Mutation {Mutation(){mutating=true;}~Mutation(){mutating=false;}} mutation;
        call.flag(L"NewHidden",value);
        if(!value)unhide_attempted=true;
        const bool called=call.run();if(!value)unhide_called=called;return called;
    }
};
bool restore_visibility(unsigned reason=0,uptr object=0,uptr function=0) noexcept {
    if(!equip_visibility.dirty())return true;
    VisibilityBackend b{equip_visibility.key(),nullptr};const bool clean=equip_visibility.restore(b);publish();
    if(log_file&&report_count<32){
        ++report_count;wchar_t label[96]{};
        if(registered(function))name(function,label,96);
        // Existing bounded restore row: distinguish our explicit restoration
        // from an original-engine visibility change already seen on entry.
        // These witnesses add no reads, writes or authority in CF notices.
        fwprintf(log_file,L"native_equipment_visibility restore=%u outstanding=%u revoked=%u weapon=%llx reason=%u first_read=%u first_hidden=%u unhide_attempted=%u unhide_called=%u object=%llx function=%ls\n",
            unsigned(clean),unsigned(equip_visibility.dirty()),unsigned(equip_visibility.revoked()),equip_visibility.key().objects[6].address,
            reason,unsigned(b.reads>0),unsigned(b.first_hidden),unsigned(b.unhide_attempted),unsigned(b.unhide_called),object,label);fflush(log_file);
    }
    const bool fenced=sync_input_fence();return clean&&!equip_visibility.dirty()&&fenced;
}
bool hide_equip_transition(abi::Frame const& f,Snapshot const& game,bool active_idle=false) noexcept {
    if(equip_visibility.dirty()||equip_visibility.faulted()||epoch==UINT64_MAX||
       !profile()||stop_requested.load()||!current(nullptr,f)||!game.can_switch||
       game.current_weapon!=f.weapon.address||game.world!=f.world.address||game.pawn!=f.pawn.address||
       phase(f.weapon.address)!=(active_idle?gears_weapon::StowPhase::active:gears_weapon::StowPhase::equipping)||
       (active_idle&&game.animation_busy)||!clear_equip_transition(f.pawn.address))return false;
    lancermain_asset::Evidence asset{};
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready&&
       lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)!=lancermain_asset::Status::ready)return false;
    Key k{};k.epoch=++epoch;k.slots=game.weapon_in_slot;
    const uptr addresses[]={f.world.address,get<uptr>(f.world.address+0x40),f.controller.address,f.pawn.address,
        playground::pointer(f.pawn.address,L"InvManager"),f.mesh.address,f.weapon.address,asset.component.address,asset.asset.address};
    for(unsigned i=0;i<k.objects.size();++i)if(!identify(nullptr,addresses[i],k.objects[i]))return false;
    if(!identities(k)||!current(nullptr,f))return false;
    if(!input_fence(true))return false;
    VisibilityBackend b{k,&f,active_idle};const bool accepted=equip_visibility.acquire(b);publish();(void)sync_input_fence();
    if(log_file&&report_count<32){++report_count;fwprintf(log_file,L"native_equipment_visibility acquired=%u outstanding=%u revoked=%u weapon=%llx\n",
        unsigned(accepted),unsigned(equip_visibility.dirty()),unsigned(equip_visibility.revoked()),f.weapon.address);fflush(log_file);}
    return accepted;
}
bool restore_with_reason(unsigned reason) noexcept {
    if(!restore_visibility(reason))return false;
    if(!lease.dirty())return sync_input_fence();
    Backend b{lease.key(),nullptr};const bool clean=lease.restore(b);publish();
    if(!clean&&log_file&&explained_epoch!=lease.key().epoch){
        explained_epoch=lease.key().epoch;wchar_t label[96]{};
        if(first_revoke.epoch==explained_epoch&&registered(first_revoke.function))name(first_revoke.function,label,96);
        fwprintf(log_file,L"native_equipment_stow_first_failure epoch=%llu read_stage=%u revoked=%u first_object=%llx first_function=%llx name=%ls weapon=%llx mesh=%llx\n",
            explained_epoch,read_stage,unsigned(lease.revoked()),first_revoke.object,first_revoke.function,label,
            lease.key().objects[6].address,lease.key().objects[7].address);fflush(log_file);
        wchar_t caller_name[96]{},caller_owner[96]{};
        if(registered(first_revoke.caller)){
            name(first_revoke.caller,caller_name,96);name(get<uptr>(first_revoke.caller+0x40),caller_owner,96);}
        fwprintf(log_file,L"native_equipment_stow_first_call entry_rva=%llx caller=%ls owner=%ls caller_object=%llx script_offset=%u argument_prefix=",
            first_revoke.entry>=base?first_revoke.entry-base:0,caller_name,caller_owner,first_revoke.caller_object,first_revoke.script_offset);
        for(unsigned i=0;i<first_revoke.code_count;++i)fwprintf(log_file,L"%02x",unsigned(first_revoke.code[i]));
        fwprintf(log_file,L" observed_only=1\n");fflush(log_file);
    }
    if(log_file&&report_count<32){++report_count;fwprintf(log_file,L"native_equipment_stow restore=%u outstanding=%u revoked=%u epoch=%llu\n",
        unsigned(clean),unsigned(lease.dirty()),unsigned(lease.revoked()),lease.key().epoch);fflush(log_file);}
    const bool fenced=sync_input_fence();return clean&&!lease.dirty()&&fenced;
}
bool restore() noexcept {return restore_with_reason(0);}
bool acquire(abi::Frame const& f,Snapshot const& game,bool idle_unheld=false) noexcept {
    if(!profile()||stop_requested.load(std::memory_order_acquire)||lease.dirty()||lease.faulted()||
       !game.can_switch||game.world!=f.world.address||game.pawn!=f.pawn.address||
       game.current_weapon!=f.weapon.address||!current(nullptr,f)||epoch==UINT64_MAX)return false;
    if(!playground::instance(f.controller.address,L"WarPC")||!playground::instance(f.pawn.address,L"WarPawn"))return false;
    const auto weapon_phase=phase(f.weapon.address);const bool early=weapon_phase==gears_weapon::StowPhase::equipping;
    if(!gears_weapon::stow_phase_allowed(weapon_phase,game.can_switch,game.animation_busy,idle_unheld,
        clear_equip_transition(f.pawn.address))||!current(nullptr,f))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // The sole gun writer must close its component journal BEFORE the engine
    // detaches the weapon. Never restore its old component fields afterward.
    if(nativelancerfollow::active()){
        nativelancerfollow::pre_original_tick(L"before_native_stow");
        if(nativelancerfollow::active()||!current(nullptr,f))return false;
    }
#endif
    lancermain_asset::Evidence asset{};
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready&&
       lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)!=lancermain_asset::Status::ready)return false;
    Key k{};k.epoch=++epoch;k.slots=game.weapon_in_slot;
    const auto inv=playground::pointer(f.pawn.address,L"InvManager"),level=get<uptr>(f.world.address+0x40);
    const uptr addresses[]={f.world.address,level,f.controller.address,f.pawn.address,inv,
        f.mesh.address,f.weapon.address,asset.component.address,asset.asset.address};
    for(unsigned i=0;i<k.objects.size();++i)if(!identify(nullptr,addresses[i],k.objects[i]))return false;
    bool detached{};
    if(!read_attached_frame(f,asset.component.address,false,&k.original,&detached,true)||detached||
       !identities(k)||!current(nullptr,f))return false;
    if(!input_fence(true))return false;
    Backend b{k,&f,early};const bool accepted=lease.acquire(b);publish();(void)sync_input_fence();
    if(log_file&&report_count<32){++report_count;fwprintf(log_file,L"native_equipment_stow acquired=%u outstanding=%u revoked=%u epoch=%llu weapon=%llx early_equipping=%u\n",
        unsigned(accepted),unsigned(lease.dirty()),unsigned(lease.revoked()),k.epoch,f.weapon.address,unsigned(early));fflush(log_file);}
    return accepted;
}
bool stow(void*,abi::Frame const& f,Snapshot const& game) noexcept {
    if(!profile()||!current(nullptr,f))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(lancerreload_runtime::needs_priority()&&!lancerreload_runtime::stow_ready(f))return false;
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(!fragpreparation::stop()||!current(nullptr,f))return false;
#endif
    if(!vrfire::yield_for_native_equipment(f)||!current(nullptr,f))return false;
#endif
    // A new explicit press may start a new journal only after the old one is
    // demonstrably clean. Idle automatic hiding never resets a revoked fault.
    if(!lease.dirty()&&lease.faulted()&&!lease.reset())return false;
    return acquire(f,game);
}
bool unholster(void*,abi::Frame const& f,Snapshot const&,bool& changed) noexcept {
    changed=false;if(!profile()||stop_requested.load()||!current(nullptr,f))return false;
    if(equip_visibility.dirty()){
        if(!abi::same(equip_visibility.key().objects[6],f.weapon))return false;
        changed=true;if(!restore_visibility(3))return false;
    }
    if(!lease.dirty())return true;
    const auto& k=lease.key();
    if(!abi::same(k.objects[0],f.world)||!abi::same(k.objects[2],f.controller)||
       !abi::same(k.objects[3],f.pawn)||!abi::same(k.objects[5],f.mesh)||!abi::same(k.objects[6],f.weapon))return false;
    changed=true;return restore();
}
void service() noexcept {
    if(stop_requested.load(std::memory_order_acquire)&&(lease.dirty()||equip_visibility.dirty()))(void)restore_with_reason(1);
}
void pre_original_tick(uptr pc) noexcept {
    // This is the existing real PlayerTick entry, before original input can
    // change inventory. The Native veto has not consumed the original edge.
    // Restore only an already-owned journal, with its exact identities; this
    // grants no acquisition/action authority and never runs in a CF notice.
    if(GetCurrentThreadId()!=game_thread.load()||!profile()||!restoration_quiet())return;
    if(empty_touch_skipped!=empty_touch_reported&&empty_touch_reported<4&&log_file){
        empty_touch_reported=empty_touch_skipped;fwprintf(log_file,L"native_equipment_empty_touch attested_skips=%llu body=040b53 original_actions_unchanged=1\n",empty_touch_skipped);fflush(log_file);
    }
    if(decal_attach_skipped!=decal_attach_reported&&decal_attach_reported<4&&log_file){
        decal_attach_reported=decal_attach_skipped;
        fwprintf(log_file,L"native_equipment_decal_attach attested_skips=%llu original_component_only=1 journal_write=0\n",decal_attach_skipped);fflush(log_file);
    }
    const Key* key=lease.dirty()?&lease.key():equip_visibility.dirty()?&equip_visibility.key():nullptr;
    if(key&&key->objects[2].address==pc&&original_action_pending())(void)restore_with_reason(2);
}
void idle(abi::Frame const& f,Snapshot const& game,bool may_hide) noexcept {
    service();
    if(!may_hide||!profile()||lease.dirty()||lease.faulted()||stop_requested.load()||!current(nullptr,f)||original_action_pending())return;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(lancerreload_runtime::needs_priority())return;
#endif
    const IdleKey key{f.world,f.pawn,f.weapon,f.key.native_started_qpc,f.key.registration_cookie,f.key.world_generation};
    const auto weapon_phase=phase(f.weapon.address);
    if(equip_visibility.dirty()){
        if(!abi::same(equip_visibility.key().objects[6],f.weapon))return;
        // manual31404: empty hands approaching the ground rack repeatedly
        // restored/reacquired the SAME HiddenGame bit with no grab/face input.
        // New-acquisition animation gates are not a lifetime for an already
        // hidden, still-unheld mesh. Preserve only that existing interval,
        // after re-reading its exact actor/inventory/component/property key.
        // Deliberate original input, unholster, foreign setters and shutdown
        // keep their existing restoration/revocation paths. World pickups
        // are separate components and are never hidden by this journal.
        VisibilityBackend retained{equip_visibility.key(),nullptr};
        if(equip_visibility.retains_detached(retained))return;
        if(!restore_visibility(4)||!current(nullptr,f))return;
        // A completed clean visibility interval may serve a later equip.
        if(!equip_visibility.faulted())equip_visibility.reset();
    }
    if(weapon_phase==gears_weapon::StowPhase::equipping){(void)hide_equip_transition(f,game);return;}
    const bool eligible=gears_weapon::stow_phase_allowed(weapon_phase,game.can_switch,game.animation_busy,true,
        clear_equip_transition(f.pawn.address));
    if(!idle_cadence.attempt(key,clock_ns(nullptr),eligible))return;
    // Idle presentation needs only HiddenGame, never a mechanical holster.
    //50724: stock TryToEvade/DoSpecialMove revoked the automatic attachment
    // journal before X pickup and correctly left its input fence closed.
    // Real shoulder stow still uses acquire(). This existing visual lease
    // retains actor/inventory identity, foreign-setter and restore checks.
    (void)hide_equip_transition(f,game,true);
}
void idle_unheld(void*,abi::Frame const& f,Snapshot const& game) noexcept {idle(f,game,true);}
bool relevant(uptr object,uptr function) noexcept {
    bool related=false;for(auto const& id:lease.key().objects)related=related||id.address==object;
    wchar_t label[96]{};if(!registered(function)||!name(function,label,96))return related;
    // A pickup's GiveTo runs on a DIFFERENT actor before it changes our
    // inventory. Close the old journal at that boundary, while its complete
    // original inventory is still authentic. Do not discover it only after
    // SetCurrentWeapon has already replaced the old key. The CF notice is
    // still revoke-only; no property query or engine call is added there.
    if(!related){
        const bool pickup=!wcscmp(label,L"GiveTo")||!wcscmp(label,L"Pickup")||!wcscmp(label,L"Touch");
        if(!pickup||!registered(object)||get<uptr>(object+0x40)!=lease.key().objects[1].address)return false;
        if(!wcscmp(label,L"Touch")&&certified_empty_pickup_touch(function)){++empty_touch_skipped;return false;}
        return true;
    }
    constexpr const wchar_t* actions[]={L"SetWeaponTemporarilyHolstered",L"HolsterWeaponTemporarily",
        L"SetWeaponFromSlot",L"SetCurrentWeapon",L"ChangedWeapon",L"ClientWeaponSet",
        L"AttachWeaponToHand",L"DetachWeapon",L"AttachWeaponToSlot",L"DetachWeaponFromSlot",
        L"AttachComponent",L"AttachComponentToSocket",L"DetachComponent",L"DetachComponentFromSocket",
        L"ServerUse",L"GrabPickup",L"DoSpecialMove",L"StartSpecialMove",L"EndSpecialMove",
        L"StartMeleeAttack",L"PlayBodyStance",L"SetBodyStance",L"BeginState",L"EndState",
        L"ForceReload",L"Died",L"Destroyed"};
    for(auto const* action:actions)if(!wcscmp(action,label))return true;
    return false;
}
bool visibility_relevant(uptr object,uptr function) noexcept {
    if(!equip_visibility.dirty()||!registered(function))return false;
    wchar_t label[96]{};if(!name(function,label,96))return true;
    const auto& k=equip_visibility.key();
    // Attachment/animation events do not mutate HiddenGame. A foreign setter
    // does, even when its requested bit happens to match our value.
    if(object==k.objects[7].address)return !wcscmp(label,L"SetHidden")||
        !wcscmp(label,L"SetSkeletalMesh")||!wcscmp(label,L"Destroyed");
    if(object==k.objects[6].address&&!wcscmp(label,L"Destroyed"))return true;
    //2572: local WarPawn.Touch -> Actor.Touch repeatedly entered here with
    // an owned hidden bit; our own SetHidden(false) caused the flash. Touch
    // is a contact notification, not a change to the held item's identity
    // or visibility. The original handler still runs; any nested inventory
    // change, exact-component setter, original action input or destruction
    // crosses the existing boundaries. Do not reveal a gun merely for a
    // pawn/world overlap. Mechanical attachment relevant() stays unchanged.
    constexpr const wchar_t* actions[]={L"GiveTo",L"Pickup",L"SetWeaponFromSlot",L"SetCurrentWeapon",
        L"ChangedWeapon",L"ClientWeaponSet",L"Died",L"Destroyed"};
    bool related=false;for(auto const& id:k.objects)related=related||id.address==object;
    // A nearby WORLD pickup/volume is not the local hidden component's
    // mutation owner. Restoring for every same-level Touch/GiveTo/Pickup
    // briefly showed the stock gun while VR remained empty (manual31404,
    // and581 read-only samples in36648). Actual local inventory/weapon
    // changes still cross the named owner boundaries below; SetHidden,
    // mesh replacement and destruction on the exact component remain
    // foreign setters above. The mechanical detach lease keeps its stricter
    // unrelated-pickup handling in relevant(); this changes visibility only.
    if(!related)return false;
    for(auto const* action:actions)if(!wcscmp(label,action))return true;
    return false;
}
bool certified_unrelated_decal_attach(uptr object,uptr function,uptr caller_frame) noexcept {
    //51224 first failure: Engine.Actor.AddDecal -> AttachComponent(local
    // DecalComponent), not a weapon reattachment. Classify only this exact
    // native entry and direct typed local. No VM evaluation/advance, engine
    // invocation, rollback-right reset, or action permission occurs here.
    const auto& k=lease.key();
    if(object!=k.objects[3].address||!caller_frame||(caller_frame&7)||caller_frame>UINTPTR_MAX-0x38||
       !registered(function)||!playground::named(function,L"AttachComponent")||
       !playground::named(get<uptr>(function+0x50),L"Function")||
       !(get<unsigned>(function+0xf4)&0x400)||get<uptr>(function+0x114)!=base+0x20900c)return false;
    const auto actor_class=get<uptr>(function+0x40),package=get<uptr>(actor_class+0x40);
    if(!registered(actor_class)||!playground::named(actor_class,L"Actor")||
       !playground::named(get<uptr>(actor_class+0x50),L"Class")||!registered(package)||
       !playground::named(package,L"Engine")||get<uptr>(package+0x40))return false;
    std::array<unsigned char,207> native{};uint64_t hash=14695981039346656037ull;
    if(!copy(native.data(),base+0x20900c,native.size()))return false;
    for(auto value:native)hash=(hash^value)*1099511628211ull;
    if(hash!=0xd8d6e954bb39dc16ull)return false;
    uptr node{},self{},code{},locals{};Array script{};
    if(!copy(&node,caller_frame+0x18,8)||!copy(&self,caller_frame+0x20,8)||
       !copy(&code,caller_frame+0x28,8)||!copy(&locals,caller_frame+0x30,8)||
       self!=object||!locals||(locals&7)||!registered(node)||
       !playground::named(node,L"AddDecal")||get<uptr>(node+0x40)!=actor_class||
       !playground::named(get<uptr>(node+0x50),L"Function")||
       (get<unsigned>(node+0xf4)&0x400)||get<uptr>(node+0x114)!=base+0x1d6ca0||
       !copy(&script,node+0x8c,sizeof script)||!script.data||script.count<10||
       script.count>script.capacity||script.capacity>131072||
       script.data>UINTPTR_MAX-unsigned(script.count)||code<script.data||
       code>script.data+unsigned(script.count)-10)return false;
    std::array<unsigned char,10> argument{},again{};
    if(!copy(argument.data(),code,argument.size()))return false;
    const auto property=gears_weapon::stow_local_component_argument(argument);
    if(!registered(property)||get<uptr>(property+0x40)!=node||
       !(playground::named(get<uptr>(property+0x50),L"ObjectProperty")||
         playground::named(get<uptr>(property+0x50),L"ComponentProperty"))||
       get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=8)return false;
    const auto extent=get<unsigned>(node+0x88),offset=get<unsigned>(property+0x94);
    uptr component{};
    if(extent<8||extent>65536||offset>extent-8||locals>UINTPTR_MAX-extent||
       !copy(&component,locals+offset,8)||!registered(component)||
       !playground::instance(component,L"DecalComponent"))return false;
    for(auto const& id:k.objects)if(component==id.address)return false;
    const auto cls=get<uptr>(component+0x50);const auto index=get<unsigned>(component+8);
    uptr same_component{},same_code{};
    return copy(&same_component,locals+offset,8)&&same_component==component&&
        registered(component)&&get<uptr>(component+0x50)==cls&&get<unsigned>(component+8)==index&&
        copy(&same_code,caller_frame+0x28,8)&&same_code==code&&
        copy(again.data(),code,again.size())&&again==argument;
}
// Native CF notices are NoAuth. Only revoke old rollback rights here; never
// call the engine, drain a journal, query inventory or format a log. While our
// one synchronous original helper runs, its body owns the intended mutation;
// that flag permits NO additional callback write or engine call.
void observe_call_no_engine(uptr object,uptr function,uptr caller_frame=0) noexcept {
    if(!mutating&&GetCurrentThreadId()==game_thread.load()&&visibility_relevant(object,function)){
        equip_visibility.revoke();publish();
    }
    if(!mutating&&lease.dirty()&&GetCurrentThreadId()==game_thread.load()&&relevant(object,function)){
        if(certified_unrelated_decal_attach(object,function,caller_frame)){
            ++decal_attach_skipped;return;
        }
        if(first_revoke.epoch!=lease.key().epoch){
            first_revoke={};first_revoke.epoch=lease.key().epoch;first_revoke.object=object;
            first_revoke.function=function;first_revoke.entry=get<uptr>(function+0x114);
            // Bounded read-only context for the EXISTING first-failure row.
            // FFrame's Code is unconsumed bytecode, never callee parameters or
            // action permission. Do not evaluate it, call the engine, or log
            // inside this NoAuth notice.
            uptr code{},node{},self{};Array script{};
            if(caller_frame&&!(caller_frame&7)&&copy(&node,caller_frame+0x18,8)&&
               copy(&self,caller_frame+0x20,8)&&copy(&code,caller_frame+0x28,8)&&
               registered(node)&&playground::named(get<uptr>(node+0x50),L"Function")&&
               copy(&script,node+0x8c,sizeof script)&&script.data&&script.count>0&&
               script.count<=script.capacity&&script.capacity<=131072&&
               script.data<=UINTPTR_MAX-unsigned(script.count)&&code>=script.data&&code<script.data+unsigned(script.count)){
                first_revoke.caller=node;first_revoke.caller_object=self;
                first_revoke.script_offset=unsigned(code-script.data);
                const auto count=(std::min)(unsigned(first_revoke.code.size()),unsigned(script.count)-first_revoke.script_offset);
                if(copy(first_revoke.code.data(),code,count))first_revoke.code_count=count;
            }
        }
        lease.revoke();publish();
    }
}
void before_action(uptr object,uptr function) noexcept {
    if(mutating||GetCurrentThreadId()!=game_thread.load()||!registered(function))return;
    if(visibility_relevant(object,function)){
        if(!restore_visibility(5,object,function))equip_visibility.revoke();publish();
    }
    if(!lease.dirty())return;
    if(relevant(object,function)){
        if(!restore())lease.revoke();publish();return;
    }
}
void reset_after_drain() noexcept {
    if(lease.dirty()||equip_visibility.dirty())return;
    lease.reset();equip_visibility.reset();stop_requested.store(false);idle_cadence.reset();publish();
}
}
