// Include after path_choice_position_trial_live.inl. CPU evidence only.
namespace path_choice_layout_observer {
#if defined(GEARS_PATH_CHOICE_LAYOUT_OBSERVER) && GEARS_PATH_CHOICE_LAYOUT_OBSERVER
#if !defined(GEARS_PATH_CHOICE_POSITION_TRIAL) || !GEARS_PATH_CHOICE_POSITION_TRIAL || !defined(GEARS_UI_SCENE_ROOT_OBSERVER) || !GEARS_UI_SCENE_ROOT_OBSERVER
#error Layout observation requires the position trial and scene root observer
#endif
namespace position=path_choice_position_live;
using Exec=void(__fastcall*)(void*,void const*);
Exec original{};void* target{};
inline constexpr uintptr_t code_rva=0x298bdc,vtable_rva=0x138a558,slot_offset=0x488;
inline constexpr unsigned code_size=161,row_limit=4;
inline constexpr char code_sha256[]="b5dae9f89c8182b26eef27e87ca2ae57804899ed313cb9acdc630280a03b6fb5";
// artifact 1: historical code-section SHA256 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6,
// base 0x140000000. Current full body and actual panel slot were matched in 80d.
template<class Read,class Matches> bool certify_with(Read read,Matches matches){
    if(base>UINTPTR_MAX-vtable_rva-slot_offset-8)return false;
    std::array<unsigned char,code_size> first{},second{};uintptr_t slot{},again{};
    return read(first.data(),reinterpret_cast<void const*>(base+code_rva),first.size())&&
        read(second.data(),reinterpret_cast<void const*>(base+code_rva),second.size())&&first==second&&
        matches(first.data(),code_size,code_sha256)&&
        read(&slot,reinterpret_cast<void const*>(base+vtable_rva+slot_offset),8)&&
        read(&again,reinterpret_cast<void const*>(base+vtable_rva+slot_offset),8)&&slot==again&&slot==base+code_rva;
}
bool enable(FILE* log){
    const bool ok=certify_with(safe_copy,gears_native_metadata::matches);
    if(ok)target=reinterpret_cast<void*>(base+code_rva);
    fprintf(log,"path_layout_attested=%u module_base=%llu rva=0x298bdc complete_bytes=161 global_vslot=0x138a9e0 code_sha256=%s source_snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 historical_module_base=0x140000000 observation_only=1\n",
        ok?1u:0u,static_cast<unsigned long long>(base),code_sha256);
    return ok;
}
struct Snapshot {
    std::array<std::uint32_t,3> stored{}; // object+3e8..3f3
    std::array<std::uint32_t,2> position{};
    std::uint32_t units{};unsigned mask{};bool identity{},loss_ready{};
    std::uint64_t world{},applied{},ownership_lost{},loss_tick{};
};
struct Row {
    uintptr_t panel{},scene{},client{},caller{},input_address{};
    std::uint64_t nonce{},world{};DWORD thread{};unsigned depth{};
    long long enter{},exit{};std::array<std::uint32_t,3> input{};bool input_readable{},normal{};
    Snapshot before{},after{};
};
std::array<Row,row_limit> rows{};
std::array<std::atomic<bool>,row_limit> ready{};
std::atomic<unsigned> slots{};thread_local unsigned depth{};
long long qpc(){LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
// State is owner-thread-only. Never enter this helper before the atomic thread gate.
template<class Read> bool identity(uintptr_t object,Read read){
    auto const& c=position::state.lease.config;
    if(!position::applied.load()||!c.nonce||position::successful_nonce!=c.nonce||object!=c.panel||c.base!=base||c.panel_vtable!=base+vtable_rva||
       !position::trial::authenticate(c,position::context(c.client,c.scene),read))return false;
    uintptr_t slot{};
    return read(&slot,reinterpret_cast<void const*>(c.panel_vtable+slot_offset),8)&&slot==base+code_rva;
}
template<class Read> Snapshot snapshot(Row const& row,Read read){
    Snapshot s{};s.world=committed_world_generation.load();s.applied=position::applied.load();
    s.ownership_lost=position::ownership_lost.load();s.loss_ready=position::loss_ready.load(std::memory_order_acquire);
    if(s.loss_ready)s.loss_tick=position::first_loss.tick;
    auto const& c=position::state.lease.config;
    s.identity=c.nonce==row.nonce&&c.scene==row.scene&&c.client==row.client&&c.world_generation==row.world&&identity(row.panel,read);
    // Failed identity prohibits dereferencing a stale panel. Missing reads are
    // explicit mask bits, never indistinguishable zero-valued observations.
    if(!s.identity)return s;
    if(read(s.stored.data(),reinterpret_cast<void const*>(row.panel+0x3e8),12))s.mask|=1;
    if(read(&s.position[0],reinterpret_cast<void const*>(row.panel+0x64),4))s.mask|=2;
    if(read(&s.position[1],reinterpret_cast<void const*>(row.panel+0x6c),4))s.mask|=4;
    if(read(&s.units,reinterpret_cast<void const*>(row.panel+0x70),4))s.mask|=8;
    return s;
}
template<class Original,class Read> void invoke_with(void* object,void const* input,uintptr_t caller,Original call,Read read){
    // Do not claim a thread or inspect position State on a foreign callback.
    if(position::owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId()||slots.load()>=row_limit||
       !identity(reinterpret_cast<uintptr_t>(object),read)){call(object,input);return;}
    const unsigned slot=slots.fetch_add(1); // Single fixed owner, including nesting.
    auto const& c=position::state.lease.config;
    Row row{};row.panel=c.panel;row.scene=c.scene;row.client=c.client;row.nonce=c.nonce;row.world=c.world_generation;
    row.caller=caller;row.input_address=reinterpret_cast<uintptr_t>(input);row.thread=GetCurrentThreadId();row.depth=++depth;row.enter=qpc();
    struct RestoreDepth{~RestoreDepth(){--depth;}} restore_depth;
    row.input_readable=read(row.input.data(),input,12);row.before=snapshot(row,read);
    auto finish=[&](bool normal){row.normal=normal;row.after=snapshot(row,read);row.exit=qpc();
        rows[slot]=row;ready[slot].store(true,std::memory_order_release);};
    try{call(object,input);}catch(...){finish(false);throw;}
    finish(true);
}
void __fastcall hook(void* object,void const* input){
    if(stereo_publish_bypass){original(object,input);return;}
    Scope scope;
    if(!scope.world()||!ui_observer_enabled||!recording.load()){original(object,input);return;}
    invoke_with(object,input,reinterpret_cast<uintptr_t>(_ReturnAddress()),original,safe_copy);
}
void progress(FILE* f){
    fprintf(f,"path_layout_compiled=1 path_layout_claimed=%u path_layout_limit=4 path_layout_observation_only=1 path_layout_row_tuple=panel,scene,client,caller,input_address,nonce,world,thread,depth,enter,exit,normal,input_readable,input0,input1,input2 path_layout_snapshot_tuple=identity,mask,stored0,stored1,stored2,top,bottom,units,world,applied,ownership_lost,loss_ready,loss_tick path_layout_mask_tuple=stored12:1,top:2,bottom:4,units:8 ",slots.load());
    for(unsigned i=0;i<row_limit;++i){
        if(!ready[i].load(std::memory_order_acquire))continue; // Never read an in-flight row.
        auto const& r=rows[i];
        fprintf(f,"path_layout_%u=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%lu,%u,%lld,%lld,%u,%u,%u,%u,%u ",i,
            static_cast<unsigned long long>(r.panel),static_cast<unsigned long long>(r.scene),static_cast<unsigned long long>(r.client),
            static_cast<unsigned long long>(r.caller),static_cast<unsigned long long>(r.input_address),
            r.nonce,r.world,r.thread,r.depth,r.enter,r.exit,r.normal?1u:0u,r.input_readable?1u:0u,r.input[0],r.input[1],r.input[2]);
        auto print=[&](char const* side,Snapshot const& s){
            fprintf(f,"path_layout_%u_%s=%u,%u,%u,%u,%u,%u,%u,%u,%llu,%llu,%llu,%u,%llu ",i,side,
                s.identity?1u:0u,s.mask,s.stored[0],s.stored[1],s.stored[2],s.position[0],s.position[1],s.units,
                s.world,s.applied,s.ownership_lost,s.loss_ready?1u:0u,s.loss_tick);};
        print("before",r.before);print("after",r.after);
    }
}
#else
void progress(FILE* f){fprintf(f,"path_layout_compiled=0 ");}
#endif
}
