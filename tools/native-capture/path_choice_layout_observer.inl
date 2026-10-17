// Include after path_choice_position_trial_live.inl. CPU evidence only.
namespace path_choice_layout_observer {
#if defined(GEARS_PATH_CHOICE_LAYOUT_OBSERVER) && GEARS_PATH_CHOICE_LAYOUT_OBSERVER
#if !defined(GEARS_PATH_CHOICE_POSITION_TRIAL) || !GEARS_PATH_CHOICE_POSITION_TRIAL || !defined(GEARS_UI_SCENE_ROOT_OBSERVER) || !GEARS_UI_SCENE_ROOT_OBSERVER
#error Layout observation requires the position trial and scene root observer
#endif
namespace position=path_choice_position_live;
// Only RCX is consumed at entry. Preserve opaque RAX; no semantic return type is claimed.
using Exec=uintptr_t(__fastcall*)(void*);
Exec original{};void* target{};
inline constexpr uintptr_t code_rva=0x2983f8,child_rva=0x298694,vtable_rva=0x138a558,slot_offset=0x528,child_slot=0x2c8;
inline constexpr unsigned code_size=1101,child_size=240,row_limit=4,array_limit=64;
inline constexpr char code_sha256[]="9df4472db8e3ebaca59f6c14356c8de8c685a2aac1a1ff060ca0f7603d945d97";
inline constexpr char child_sha256[]="c455b118525aa5c957e5eea29cd8a46479aa2a95ef633693d4aa04da0e15ac28";
struct Span {uintptr_t rva;unsigned size;};
inline constexpr Span code_chunks[]={{code_rva,666},{0x8ead70,435}};
inline constexpr Span child_chunks[]={{child_rva,201},{0x8eaf24,39}};
// Historical artifact 1: section SHA256 911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6,
// base 0x140000000. Neither disk EXE identity nor current bytes follow from RVAs.
template<unsigned Size,std::size_t N,class Read,class Matches>
bool certify_chunks(Span const (&chunks)[N],char const* sha,Read read,Matches matches){
    std::array<unsigned char,Size> first{},second{};unsigned offset{};
    for(auto const& span:chunks){
        if(span.size>Size-offset||base>UINTPTR_MAX-span.rva-span.size||
           !read(first.data()+offset,reinterpret_cast<void const*>(base+span.rva),span.size)||
           !read(second.data()+offset,reinterpret_cast<void const*>(base+span.rva),span.size))return false;
        offset+=span.size;
    }
    return offset==Size&&first==second&&matches(first.data(),Size,sha);
}
template<class Read> bool slots_match(uintptr_t vt,Read read){
    uintptr_t entry{},entry_again{},child{},child_again{};
    return vt==base+vtable_rva&&read(&entry,reinterpret_cast<void const*>(vt+slot_offset),8)&&
        read(&entry_again,reinterpret_cast<void const*>(vt+slot_offset),8)&&entry==entry_again&&entry==base+code_rva&&
        read(&child,reinterpret_cast<void const*>(vt+child_slot),8)&&
        read(&child_again,reinterpret_cast<void const*>(vt+child_slot),8)&&child==child_again&&child==base+child_rva;
}
template<class Read,class Matches> bool certify_with(Read read,Matches matches){
    return base<=UINTPTR_MAX-vtable_rva-slot_offset-8&&
        certify_chunks<code_size>(code_chunks,code_sha256,read,matches)&&
        certify_chunks<child_size>(child_chunks,child_sha256,read,matches)&&slots_match(base+vtable_rva,read);
}
bool enable(FILE* log){
    const bool ok=certify_with(safe_copy,gears_native_metadata::matches);
    if(ok)target=reinterpret_cast<void*>(base+code_rva);
    fprintf(log,"path_layout_attested=%u module_base=%llu rva=0x2983f8 complete_bytes=1101 chunks=0x2983f8:666,0x8ead70:435 child_rva=0x298694 child_complete_bytes=240 child_chunks=0x298694:201,0x8eaf24:39 vslots=0x528,0x2c8 code_sha256=%s child_sha256=%s source_snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6 historical_module_base=0x140000000 observation_only=1\n",
        ok?1u:0u,static_cast<unsigned long long>(base),code_sha256,child_sha256);
    return ok;
}
enum class Bypass:unsigned {stereo,disabled,world,not_recording,foreign_thread,no_successful_nonce,wrong_object,identity,quota,count};
std::atomic<uint64_t> entries{};
std::array<std::atomic<uint64_t>,unsigned(Bypass::count)> bypass{};
void rejected(Bypass why){++bypass[unsigned(why)];}
struct Snapshot {
    std::array<std::uint32_t,4> position{};
    uintptr_t raw_1dc{},raw_4e8{};std::uint32_t units{},raw_4f8{},selected_bits{},zero_bits{};
    int num{},max{};unsigned char raw_4e4{};unsigned mask{};bool identity{},loss_ready{};
    std::uint64_t world{},applied{},ownership_lost{},loss_tick{};
};
struct Row {
    uintptr_t panel{},scene{},client{},caller{},result{};
    std::uint64_t nonce{},world{};DWORD thread{};unsigned depth{};
    long long enter{},exit{};bool normal{};Snapshot before{},after{};
};
std::array<Row,row_limit> rows{};
std::array<std::atomic<bool>,row_limit> ready{};
std::atomic<unsigned> slots{};thread_local unsigned depth{};
long long qpc(){LARGE_INTEGER v{};QueryPerformanceCounter(&v);return v.QuadPart;}
// State is owner-thread-only. Never enter this helper before the atomic thread gate.
template<class Read> bool identity(uintptr_t object,Read read){
    auto const& c=position::state.lease.config;
    return c.nonce&&position::successful_nonce==c.nonce&&object==c.panel&&c.base==base&&
        c.panel_vtable==base+vtable_rva&&position::trial::authenticate(c,position::context(c.client,c.scene),read)&&
        slots_match(c.panel_vtable,read);
}
template<class Read> Snapshot snapshot(Row const& row,Read read){
    Snapshot s{};s.world=committed_world_generation.load();s.applied=position::applied.load();
    s.ownership_lost=position::ownership_lost.load();s.loss_ready=position::loss_ready.load(std::memory_order_acquire);
    if(s.loss_ready)s.loss_tick=position::first_loss.tick;
    auto const& c=position::state.lease.config;
    s.identity=c.nonce==row.nonce&&c.scene==row.scene&&c.client==row.client&&c.world_generation==row.world&&identity(row.panel,read);
    if(!s.identity)return s;
    for(unsigned i=0;i<4;++i)if(read(&s.position[i],reinterpret_cast<void const*>(row.panel+0x60+4*i),4))s.mask|=1u<<i;
    if(read(&s.units,reinterpret_cast<void const*>(row.panel+0x70),4))s.mask|=16;
    if(read(&s.raw_1dc,reinterpret_cast<void const*>(row.panel+0x1dc),8))s.mask|=32;
    if(read(&s.raw_4e4,reinterpret_cast<void const*>(row.panel+0x4e4),1))s.mask|=64;
    struct RawArray {uintptr_t data;int num,max;} array{};
    static_assert(sizeof(RawArray)==16,"Expected x64 raw array layout");
    if(read(&array,reinterpret_cast<void const*>(row.panel+0x4e8),sizeof(array))){
        s.mask|=128;s.raw_4e8=array.data;s.num=array.num;s.max=array.max;
    }
    if(read(&s.raw_4f8,reinterpret_cast<void const*>(row.panel+0x4f8),4))s.mask|=256;
    // Offset-based raw observations: current reflected field names are not asserted.
    // Never trust the original's unchecked index or chase an unbounded array.
    if((s.mask&128)&&s.num>0&&s.max>=s.num&&s.max<=int(array_limit)&&s.raw_4e8&&
       s.raw_4e8<=UINTPTR_MAX-4u*static_cast<unsigned>(s.max)){
        if(read(&s.zero_bits,reinterpret_cast<void const*>(s.raw_4e8),4))s.mask|=512;
        if((s.mask&64)&&s.raw_4e4<static_cast<unsigned>(s.num)&&
           read(&s.selected_bits,reinterpret_cast<void const*>(s.raw_4e8+4u*s.raw_4e4),4))s.mask|=1024;
    }
    return s;
}
template<class Original,class Read> uintptr_t invoke_with(void* object,uintptr_t caller,Original call,Read read){
    // Mutually exclusive first-rejection counters. Foreign threads never inspect State.
    if(position::owner_thread.load(std::memory_order_acquire)!=GetCurrentThreadId()){
        rejected(Bypass::foreign_thread);return call(object);}
    if(slots.load()>=row_limit){rejected(Bypass::quota);return call(object);}
    auto const& c=position::state.lease.config;
    if(!position::applied.load()||!c.nonce||position::successful_nonce!=c.nonce){
        rejected(Bypass::no_successful_nonce);return call(object);}
    if(reinterpret_cast<uintptr_t>(object)!=c.panel){rejected(Bypass::wrong_object);return call(object);}
    if(c.world_generation!=committed_world_generation.load()){rejected(Bypass::world);return call(object);}
    if(!identity(reinterpret_cast<uintptr_t>(object),read)){rejected(Bypass::identity);return call(object);}
    const unsigned slot=slots.fetch_add(1);
    Row row{};row.panel=c.panel;row.scene=c.scene;row.client=c.client;row.nonce=c.nonce;row.world=c.world_generation;
    row.caller=caller;row.thread=GetCurrentThreadId();row.depth=++depth;row.enter=qpc();
    struct RestoreDepth{~RestoreDepth(){--depth;}} restore_depth;
    row.before=snapshot(row,read);
    auto finish=[&](bool normal){row.normal=normal;row.after=snapshot(row,read);row.exit=qpc();
        rows[slot]=row;ready[slot].store(true,std::memory_order_release);};
    uintptr_t result{};
    try{result=call(object);}catch(...){finish(false);throw;}
    row.result=result;finish(true);return result;
}
uintptr_t __fastcall hook(void* object){
    ++entries;
    if(stereo_publish_bypass){rejected(Bypass::stereo);return original(object);}
    Scope scope;
    if(!scope.world()){rejected(Bypass::world);return original(object);}
    if(!ui_observer_enabled){rejected(Bypass::disabled);return original(object);}
    if(!recording.load()){rejected(Bypass::not_recording);return original(object);}
    return invoke_with(object,reinterpret_cast<uintptr_t>(_ReturnAddress()),original,safe_copy);
}
void progress(FILE* f){
    fprintf(f,"path_layout_compiled=1 path_layout_kind=reflow2983f8 path_layout_entries=%llu path_layout_claimed=%u path_layout_limit=4 path_layout_observation_only=1 path_layout_bypass_tuple=stereo,disabled,world,not_recording,foreign_thread,no_successful_nonce,wrong_object,identity,quota path_layout_bypass=",entries.load(),slots.load());
    for(unsigned i=0;i<unsigned(Bypass::count);++i)fprintf(f,"%s%llu",i?",":"",bypass[i].load());
    fprintf(f," path_layout_row_tuple=panel,scene,client,caller,nonce,world,thread,depth,enter,exit,normal,opaque_rax path_layout_snapshot_tuple=identity,mask,left,top,right,bottom,units,raw1dc,raw4e4,raw4e8,num,max,raw4f8,selected_bits,zero_bits,world,applied,ownership_lost,loss_ready,loss_tick path_layout_mask_tuple=left:1,top:2,right:4,bottom:8,units:16,raw1dc:32,raw4e4:64,array:128,raw4f8:256,zero:512,selected:1024 ");
    for(unsigned i=0;i<row_limit;++i){
        if(!ready[i].load(std::memory_order_acquire))continue;
        auto const& r=rows[i];
        fprintf(f,"path_layout_%u=%llu,%llu,%llu,%llu,%llu,%llu,%lu,%u,%lld,%lld,%u,%llu ",i,
            static_cast<unsigned long long>(r.panel),static_cast<unsigned long long>(r.scene),static_cast<unsigned long long>(r.client),
            static_cast<unsigned long long>(r.caller),r.nonce,r.world,r.thread,r.depth,r.enter,r.exit,r.normal?1u:0u,static_cast<unsigned long long>(r.result));
        auto print=[&](char const* side,Snapshot const& s){
            fprintf(f,"path_layout_%u_%s=%u,%u,%u,%u,%u,%u,%u,%llu,%u,%llu,%d,%d,%u,%u,%u,%llu,%llu,%llu,%u,%llu ",i,side,
                s.identity?1u:0u,s.mask,s.position[0],s.position[1],s.position[2],s.position[3],s.units,
                static_cast<unsigned long long>(s.raw_1dc),unsigned(s.raw_4e4),static_cast<unsigned long long>(s.raw_4e8),s.num,s.max,s.raw_4f8,s.selected_bits,s.zero_bits,
                s.world,s.applied,s.ownership_lost,s.loss_ready?1u:0u,s.loss_tick);};
        print("before",r.before);print("after",r.after);
    }
}
#else
void progress(FILE* f){fprintf(f,"path_layout_compiled=0 ");}
#endif
}
