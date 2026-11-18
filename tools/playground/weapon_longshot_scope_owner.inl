// One Longshot stock-state journal. No input, camera, skeleton or rig writer.
// The Native bridges only consume R3 / select an existing stereo projection.
namespace longshot_scope_owner {
namespace hand=gears_weapon_hand;namespace original=longshot_scope_original;
namespace state=longshot_scope_state;
std::atomic<bool> outstanding{},stop_requested{};
struct Journal {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    gears_weapon::LongshotReloadIdentity identity{};std::uint64_t reference{},switch_sequence{};
    original::Entry target{},zoom{},visibility{},fov{};state::State before{};
    bool prepared{};int reported_zoom{-1};
};
Journal journal{};bool blocked_until_exit{};
template<class Update>Update bridge(const char* name) noexcept {
    const auto scope_module=weaponhand::native_module;
    const auto address=scope_module?GetProcAddress(scope_module,name):nullptr;
    return scope_module&&weaponhand::executable(address,scope_module)?reinterpret_cast<Update>(address):nullptr;
}
bool available() noexcept {
    return bridge<gears_weapon_longshot_scope_input::Update>(gears_weapon_longshot_scope_input::export_name)&&
        bridge<gears_weapon_longshot_scope_view::Update>(gears_weapon_longshot_scope_view::export_name);
}
bool fence(hand::Frame const& f,bool armed) noexcept {
    auto call=bridge<gears_weapon_longshot_scope_input::Update>(gears_weapon_longshot_scope_input::export_name);
    gears_weapon_longshot_scope_input::Request request{};request.source=f;request.armed=unsigned(armed);
    return call&&call(&request)==unsigned(gears_weapon_longshot_scope_input::Result::applied);
}
bool display(hand::Frame const& f,bool active,float fov=0) noexcept {
    auto call=bridge<gears_weapon_longshot_scope_view::Update>(gears_weapon_longshot_scope_view::export_name);
    gears_weapon_longshot_scope_view::Request request{};
    if(active){if(!longshot_eyebox::view_request(f,fov,request))return false;}
    else request.source=f;
    return call&&call(&request)==unsigned(gears_weapon_longshot_scope_view::Result::applied);
}
bool owed() noexcept {return outstanding.load(std::memory_order_acquire);}
bool same_hold(hand::Frame const& f) noexcept {
    const auto& a=journal.source.key;const auto& b=f.key;
    return a.game_start==b.game_start&&a.native_started_qpc==b.native_started_qpc&&
        a.registration_cookie==b.registration_cookie&&a.lifecycle==b.lifecycle&&a.world_generation==b.world_generation&&
        a.game_pid==b.game_pid&&a.thread_id==b.thread_id&&
        hand::same(journal.source.world,f.world)&&hand::same(journal.source.controller,f.controller)&&
        hand::same(journal.source.pawn,f.pawn)&&hand::same(journal.source.weapon,f.weapon)&&
        journal.identity==longshot_fire::reload_identity(f)&&journal.reference==f.reference_generation;
}
bool ready(hand::Frame const& f) noexcept {
    return owed()&&journal.prepared&&!stop_requested.load(std::memory_order_acquire)&&same_hold(f);
}
bool owns_target(hand::Frame const& f) noexcept {return owed()&&same_hold(f);}
bool restored(state::State const& s) noexcept {
    if(!state::neutral(s))return false;
    for(unsigned i=0;i<s.components.size();++i)
        if(!hand::same(s.components[i].identity,journal.before.components[i].identity))return false;
    return true;
}
bool components_match(state::State const& s) noexcept {
    for(unsigned i=0;i<s.components.size();++i)
        if(!hand::same(s.components[i].identity,journal.before.components[i].identity))return false;
    return true;
}
bool terminal_flag(hand::ObjectIdentity const& object,const wchar_t* label,bool expected) noexcept {
    // This existing registry service proves removal/replacement by two stable
    // reads. Unreadable/unknown is NOT retirement and grants no action.
    if(lancerreload_runtime::weapon_retired(object))return true;
    bool first{},second{};
    return weapon_part_assets::boolean(object,label,first)&&first==expected&&
        weapon_part_assets::boolean(object,label,second)&&second==first;
}
bool terminal_objects_restored() noexcept {
    // After travel/destruction the old actor LINK tuple can no longer exist.
    // Observe each exact owned receiver's terminal value or proven retirement;
    // never renew the old lifetime or write through a new pawn/weapon.
    if(!terminal_flag(journal.source.controller,L"bIsTargeting",false)||
       !terminal_flag(journal.source.controller,L"bPressingTargetingKey",false)||
       !terminal_flag(journal.source.pawn,L"bIsTargeting",false)||
       !terminal_flag(journal.source.weapon,L"bZoomed",false))return false;
    for(const auto& c:journal.before.components)if(c.identity.address&&
        !terminal_flag(c.identity,L"bOwnerNoSee",c.owner_no_see))return false;
    const auto& body=journal.before.components[1].identity;
    return body.address&&terminal_flag(body,L"bIgnoreControllersWhenNotRendered",journal.before.ignore_controllers);
}
bool stop(hand::Frame const* current=nullptr) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!owed())return true;
    if(current&&nativeequipment::current(nullptr,*current)){
        (void)display(*current,false);(void)fence(*current,false);
    }
    journal.prepared=false;
    state::State actual{};
    bool done=state::read(journal.source,actual)&&restored(actual);
    if(!done)done=terminal_objects_restored();
    if(!done){
        // Each call is independently reauthenticated; an old Frame cannot
        // enable any state. Failure retains debt instead of inventing neutral.
        (void)original::set(journal.source,journal.zoom,false,nullptr,&journal.lifetime);
        (void)original::set(journal.source,journal.target,false,nullptr,&journal.lifetime);
        (void)original::set(journal.source,journal.visibility,false,nullptr,&journal.lifetime);
        done=state::read(journal.source,actual)&&restored(actual);
        if(!done)done=terminal_objects_restored();
    }
    if(!done)return false;
    journal={};outstanding.store(false,std::memory_order_release);return true;
}
void fail(hand::Frame const& f) noexcept {blocked_until_exit=true;(void)stop(&f);}
bool begin(hand::Frame const& f) noexcept {
    if(owed()||!available()||!nativeequipment::current(nullptr,f))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
    if(vrfire::aim_owns())return false;
#endif
    Journal next{};next.source=f;next.identity=longshot_fire::reload_identity(f);next.reference=f.reference_generation;
    if(!state::read(f,next.before)||!state::neutral(next.before)||
       !original::bind(f.controller,original::Method::target,next.target)||
       !original::bind(f.weapon,original::Method::zoom,next.zoom)||
       !original::bind(f.weapon,original::Method::visibility,next.visibility)||
       !original::bind(f.weapon,original::Method::fov,next.fov)||
       !weaponhand::client.capture_existing_action_lifetime_on_gt(f.key,next.lifetime)||
       !nativeequipment::current(nullptr,f))return false;
    // Durable BEFORE the first virtual-pad or original-state side effect.
    journal=next;outstanding.store(true,std::memory_order_release);
    if(!fence(f,true)||!original::set(f,journal.target,true)||!original::set(f,journal.visibility,true)){
        fail(f);return false;
    }
    state::State actual{};
    if(!state::read(f,actual)||!state::scoped(actual)||!components_match(actual)||actual.zoom){fail(f);return false;}
    journal.prepared=true;return true;
}
void tick(hand::Frame const& f) noexcept {
    const auto& eye=longshot_eyebox::current;
    const bool eye_near=longshot_eyebox::matches(f,weaponhand::clock_ns(nullptr))&&eye.near_eye;
    if(!eye_near){blocked_until_exit=false;(void)stop(&f);return;}
    if(stop_requested.load(std::memory_order_acquire)){(void)stop(&f);return;}
    if(owed()&&(!same_hold(f)||!journal.prepared)){fail(f);return;}
    if(!owed()){
        if(blocked_until_exit||!available())return;
        if(!begin(f)){blocked_until_exit=true;return;}
    }
    state::State actual{};
    if(!ready(f)||!state::read(f,actual)||!state::scoped(actual)||!components_match(actual)||!fence(f,true)){fail(f);return;}
    if(eye.switch_requested&&f.controllers.sequence>journal.switch_sequence){
        journal.switch_sequence=f.controllers.sequence;
        const bool desired=!actual.zoom;
        // The eye controller has already consumed this exact new R3 edge.
        // A failed/uncertain setter is recovered, never replayed next tick.
        if(!original::set(f,journal.zoom,desired)||!original::set(f,journal.visibility,true)||
           !state::read(f,actual)||!state::scoped(actual)||!components_match(actual)||actual.zoom!=desired){fail(f);return;}
    }
    float fov{};
    if(!original::fov(f,journal.fov,actual.base_fov,fov)||!display(f,true,fov)){fail(f);return;}
    // Only values already observed/used above. A successful request is not a
    // rendered-image receipt; real game and XR images remain required.
    static unsigned report_rows{};
    if(log_file&&report_rows<32&&journal.reported_zoom!=int(actual.zoom)){
        unsigned hidden{};for(unsigned i=0;i<actual.components.size();++i)
            if(actual.components[i].identity.address&&actual.components[i].owner_no_see)hidden|=1u<<i;
        fwprintf(log_file,L"longshot_scope_state weapon=%llx frame=%llu source_sync_ns=%lld zoom=%u original_fov=%.9g pc_target=%u pawn_target=%u target_key=%u ignore_controllers=%u hidden_mask=%u display_request_accepted=1 image_verified=0\n",
            static_cast<unsigned long long>(f.weapon.address),f.key.frame,f.controllers.sync_ns,unsigned(actual.zoom),double(fov),
            unsigned(actual.controller_target),unsigned(actual.pawn_target),unsigned(actual.target_key),unsigned(actual.ignore_controllers),hidden);fflush(log_file);
        journal.reported_zoom=int(actual.zoom);++report_rows;
    }
}
bool yield(hand::Frame const& f) noexcept {return stop(&f);}
void unavailable(uptr) noexcept {if(owed())(void)stop();}
void service() noexcept {
    if(owed()&&stop_requested.load(std::memory_order_acquire))(void)stop();
}
void reset_after_drain() noexcept {
    if(owed())return;
    journal={};blocked_until_exit=false;stop_requested.store(false,std::memory_order_release);
}
} // namespace longshot_scope_owner
