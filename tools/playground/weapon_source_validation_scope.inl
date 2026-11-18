// Included inside weaponhand, after the original complete observe_game.
// Caller-owned boolean liveness scopes may retain an exact fresh witness across callbacks.
// Lancer/known-unheld eligibility remains at existing callers; full payload readers never reuse it.
namespace source_validation {
struct NativeNames {
    struct Name {uptr entry{};U32 index{},length{};bool cold{};};
    std::array<Name,4096> names{};std::array<U32,8192> slots{};U32 count{};bool valid{};
    void begin() noexcept {slots.fill(0);count=0;valid=true;}
    void add(uptr entry,U32 index,U32 length,bool cold) noexcept {
        if(!valid)return;if(!entry||!length||length>=meta::max_name_units){valid=false;return;}
        auto hash=weapon_metadata_parse::bucket(entry^(std::uint64_t(index)*0x9e3779b97f4a7c15ull),8191);
        for(unsigned probe=0;probe<slots.size();++probe,hash=(hash+1)&8191){
            auto slot=slots[hash];if(slot){auto& name=names[slot-1];if(name.entry==entry&&name.index==index){
                if(name.length!=length)valid=false;else name.cold=name.cold||cold;return;}}
            else {if(count==names.size()){valid=false;return;}names[count]={entry,index,length,cold};slots[hash]=++count;return;}
        }valid=false;
    }
    bool current() const noexcept {
        if(!valid||!count)return false;
        for(U32 i=0;i<count;++i){auto const& name=names[i];const auto bound=weapon_metadata_parse::length_bound(name.entry,name.index);
            if(bound==name.length&&bound)continue;
            if(!bound&&name.cold)continue;return false;
        }
        return true;
    }
};
struct Domain {
    uptr module_base{};abi::FrameKey key{};
    abi::ObjectIdentity world{},controller{},pawn{},mesh{},weapon{};
    U32 size{},version{},flags{},reserved{};
    void set(abi::Frame const& f) noexcept {module_base=base;key=f.key;key.frame=0;world=f.world;controller=f.controller;pawn=f.pawn;mesh=f.mesh;weapon=f.weapon;
        size=f.size;version=f.abi_version;flags=f.flags;reserved=f.reserved;}
    bool matches(abi::Frame const& f) const noexcept {auto current=f.key;current.frame=0;
        return module_base==base&&abi::same(key,current)&&abi::same(world,f.world)&&abi::same(controller,f.controller)&&
            abi::same(pawn,f.pawn)&&abi::same(mesh,f.mesh)&&abi::same(weapon,f.weapon)&&size==f.size&&version==f.abi_version&&flags==f.flags&&reserved==f.reserved;}
};
struct PersistentEntry {
    gears_read_validation::SourceProof proof;NativeNames names;Domain domain;bool valid{};unsigned hits{},qualifications{},invalidations{};
    void invalidate() noexcept {if(valid)++invalidations;valid=false;}
};
inline thread_local PersistentEntry* existing_entry_view{};
inline std::unique_ptr<PersistentEntry>& owner() noexcept {static thread_local std::unique_ptr<PersistentEntry> storage;return storage;}
#if defined(GEARS_SOURCE_VALIDATION_PERSISTENT_TEST_RESET)
void reset_persistent_for_test() noexcept {if(existing_entry_view)existing_entry_view->invalidate();}
#endif
struct NameCapture {
    meta::SemanticNameObserver previous{meta::semantic_name_observer};void* context{meta::semantic_name_context};NativeNames* names_{};bool installed{};std::uint64_t start_event{},own_events{};
    static void observed(void* context,uptr entry,U32 index,U32 length,bool cold) noexcept {auto& capture=*static_cast<NameCapture*>(context);
        if(capture.own_events==UINT64_MAX)capture.names_->valid=false;else ++capture.own_events;
        capture.names_->add(entry,index,length,cold);}
    explicit NameCapture(NativeNames& names) noexcept {names.begin();if(previous){names.valid=false;return;}names_=&names;start_event=meta::semantic_name_events;meta::semantic_name_context=this;meta::semantic_name_observer=observed;installed=true;}
    ~NameCapture() noexcept {if(installed){if(meta::semantic_name_events-start_event!=own_events||meta::semantic_name_observer!=observed||meta::semantic_name_context!=this)names_->valid=false;meta::semantic_name_observer=previous;meta::semantic_name_context=context;}}
};
struct Scope;
thread_local Scope* active{};
struct Scope {
    abi::Frame const& frame;const uptr module_base{base};Scope* previous{};PersistentEntry* entry{};gears_read_validation::SourceProof* proof{};
    bool sealed{},disabled{};unsigned validations{},fallbacks{};
    bool cost_enabled{};unsigned qualifications{};
    std::int64_t capture_ns{},seal_ns{},validation_ns{},native_name_ns{};
    explicit Scope(abi::Frame const& f) noexcept:frame(f),previous(active){
        static const bool requested=[](){wchar_t value[4]{};return GetEnvironmentVariableW(L"GEARSVR_TEST_PERF_UNHELD",value,4)==1&&value[0]==L'1';}();cost_enabled=requested;
        if(previous){disabled=true;active=this;return;}
        auto& storage=owner();if(!storage)storage.reset(new(std::nothrow) PersistentEntry);
        entry=storage.get();existing_entry_view=entry;proof=entry?&entry->proof:nullptr;disabled=!proof;active=this;
        if(entry&&entry->valid&&!entry->domain.matches(f))entry->invalidate();
    }
    bool native_names_current() noexcept {const auto began=cost_enabled?clock_ns(nullptr):0;const auto same=entry->names.current();
        if(cost_enabled)native_name_ns+=clock_ns(nullptr)-began;return same;}
    ~Scope() noexcept {active=previous;}
    Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;
    bool matches(abi::Frame const& f) const noexcept {return module_base==base&&abi::same(frame.key,f.key)&&abi::same(frame.world,f.world)&&abi::same(frame.controller,f.controller)&&
        abi::same(frame.pawn,f.pawn)&&abi::same(frame.mesh,f.mesh)&&abi::same(frame.weapon,f.weapon);}
};
bool software_current() noexcept {
#if defined(GEARS_PLAYGROUND_RESTORE_STAGE_CORE)
    if(!weaponrestore::healthy())return false;
#endif
    if(!enabled.load(std::memory_order_acquire)||!current_gt(nullptr))return false;
#if defined(GEARS_PLAYGROUND_WEAPON_MESH_FOLLOW_TEST)
    if(vrequip::mesh_follow_active_visible.load(std::memory_order_acquire))return false;
#endif
#if defined(GEARS_PLAYGROUND_LANCER_MAGAZINE_FOLLOW_TEST)
    if(vrequip::lancer_magazine_active_visible.load(std::memory_order_acquire))return false;
#endif
    return true;
}
bool observe(abi::Frame const& f,abi::GripGameState& out) noexcept {
    auto* scope=active;
    static const bool compare=[](){wchar_t flag[4]{};return GetEnvironmentVariableW(L"GEARSVR_TEST_POSE_BATCH_COMPARE",flag,4)!=0;}();
    // Nested scopes never borrow or invalidate the outer persistent owner.
    if(scope&&scope->previous)return observe_game(f,out,false,false);
    if(meta::semantic_name_observer){if(existing_entry_view)existing_entry_view->invalidate();if(scope)scope->sealed=false;return observe_game(f,out,false,false);}
    if(!scope||scope->disabled||!scope->matches(f)||compare){if(existing_entry_view)existing_entry_view->invalidate();if(scope)scope->sealed=false;return observe_game(f,out,false,false);}
    auto& entry=*scope->entry;
    out={};if(!software_current()){entry.invalidate();return false;}
    bool cross=!scope->sealed&&entry.valid;
    if(cross&&(!entry.domain.matches(f)||!scope->native_names_current())){entry.invalidate();cross=false;}
    if(scope->sealed||cross){
        ++scope->validations;const auto key=f.key;
        const auto before=scope->cost_enabled?clock_ns(nullptr):0;
        const auto result=scope->proof->validate([](void* to,std::uintptr_t from,unsigned bytes){return copy(to,from,bytes);});
        if(scope->cost_enabled)scope->validation_ns+=clock_ns(nullptr)-before;
        if(result==gears_read_validation::Result::unreadable){entry.invalidate();scope->sealed=false;scope->disabled=true;return false;}
        if(cross&&(!software_current()||!scope->matches(f)||!abi::same(key,f.key)||!entry.domain.matches(f))){entry.invalidate();scope->sealed=false;return false;}
        if(result==gears_read_validation::Result::same){
            if(cross&&meta::semantic_name_observer){entry.invalidate();scope->sealed=false;return observe_game(f,out,false,false);}
            if(!cross||scope->native_names_current()){
                scope->sealed=true;if(cross)++entry.hits;out.known=true;return true;
            }
            entry.invalidate();scope->sealed=false;
        }else {entry.invalidate();scope->sealed=false;}
        ++scope->fallbacks;
    }
    entry.invalidate();bool okay{};Domain capture_domain;capture_domain.set(f);const auto capture_key=f.key;
    const auto capture_begin=scope->cost_enabled?clock_ns(nullptr):0;
    {NameCapture names(entry.names);gears_read_validation::Capture capture(*scope->proof);okay=observe_game(f,out,false,false);}
    if(scope->cost_enabled){scope->capture_ns+=clock_ns(nullptr)-capture_begin;++scope->qualifications;}
    ++entry.qualifications;
    if(okay){const auto seal_begin=scope->cost_enabled?clock_ns(nullptr):0;scope->sealed=scope->proof->seal();
        if(scope->cost_enabled)scope->seal_ns+=clock_ns(nullptr)-seal_begin;
        if(!scope->sealed)scope->disabled=true;
        else if(!capture_domain.matches(f)||!abi::same(capture_key,f.key)||!software_current()){scope->sealed=false;out={};return false;}
        else if(entry.names.valid&&entry.names.count){entry.domain=capture_domain;entry.valid=true;}
    }
    return okay;
}
}
