// Explicit experimental body-pose writer. Included after Scope/safe_copy.
// Retail metadata is discovered only along an authenticated hook receiver and
// the current controlled Pawn's class chains. No registry-wide object scan.
#include <new>
namespace cover_body_live {
enum class TargetSource {advance,live};
struct LiveTarget {
    gears_anchor::Identity pawn{},mesh{},world{};
    uint64_t world_generation{},frame{};long long timestamp_qpc{};unsigned thread{};float u{};
};
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
extern "C" void GearsCoverBlendTickHook();
extern "C" void GearsCoverChildPropagateHook();
extern "C" {void* GearsCoverBlendOriginal{};void* GearsCoverChildOriginal{};}
constexpr std::array<uintptr_t,2> rvas{{0x2a85a8,0xfa680}};
// First 32 bytes only; full historical/current method receipts stay in analysis.
constexpr std::array<std::array<unsigned char,32>,2> signatures{{
    {{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0xc5,0xf8,0x29,0x70,0xd8,0x45,0x33,0xff}},
    {{0x48,0x63,0xc2,0x48,0x6b,0xd0,0x44,0x48,0x8b,0x81,0xe4,0x00,0x00,0x00,0xc5,0xea,0x59,0x44,0x10,0x10,0xc5,0xfa,0x11,0x44,0x10,0x14,0x48,0x8b,0x81,0xe4,0x00,0x00}}
}};
std::array<void*,2> targets{},detours{{reinterpret_cast<void*>(GearsCoverBlendTickHook),reinterpret_cast<void*>(GearsCoverChildPropagateHook)}};
std::array<void**,2> originals{{&GearsCoverBlendOriginal,&GearsCoverChildOriginal}};
bool reserved{},enabled{};
enum class PoseProfile {top,side};
PoseProfile pose_profile=PoseProfile::top; // Immutable after configure.
// Private fixed-direction A/B only. This is NOT an authenticated native
// left/right selector; the native ancestor mirror baseline must stay fixed.
bool side_mirror_trial{}; // Immutable after configure; default is unchanged.
TargetSource target_source=TargetSource::advance;
bool live_source(){return target_source==TargetSource::live;}
const char* source_name(TargetSource source){return source==TargetSource::live?"live":"advance";}
constexpr unsigned endpoint_for(PoseProfile profile){return profile==PoseProfile::side?5u:4u;}
const char* profile_name(PoseProfile profile){return profile==PoseProfile::side?"side":"top";}
bool own_entries(); // Defined after the existing immutable hook-entry manifest.
HANDLE advance_event{},reset_event{};
gears_anchor::Config metadata{}; // Immutable startup certificate, not a mutable world pointer.
uint64_t initial_world{};
std::mutex mutex;
std::atomic<unsigned> pending{},pending_nodes{},game_thread{},step{};
// A temporarily native Weight is still logically owned. In addition, every
// wrapper (even an unadmitted one) retains code residency across its original.
std::atomic<unsigned> in_flight{},transactions{};
std::atomic<bool> stopping{},failed{},timed_out{};
std::atomic<ULONGLONG> drain_tick{};
constexpr unsigned node_limit=4,child_limit=32,row_limit=128;
constexpr std::array<float,7> steps{{0.f,.25f,.5f,.75f,1.f,.5f,0.f}};
ULONGLONG last_advance{},last_layout_attempt{};
unsigned next_step{},layout_attempts{};
bool injecting{},withdrawing{};
std::atomic<bool> layout_ready{};
std::atomic<bool> external_change{};
const wchar_t* layout_failure=L"not_attempted";
unsigned long long blend_calls{},child_calls{},rejected{},writes{},restores{},retired{},external{},rows_dropped{},prewrite_rebinds{};
unsigned long long child_posts{},native_posts{},pose_rearms{},abandoned{},reentries{};
struct LiveTime {long long qpc{},frequency{};};
LiveTime system_live_time(){LARGE_INTEGER qpc{},frequency{};if(!QueryPerformanceCounter(&qpc)||!QueryPerformanceFrequency(&frequency))return {};return {qpc.QuadPart,frequency.QuadPart};}
// Private clock seam for standalone tests; production keeps the system QPC.
LiveTime (*target_clock)()=system_live_time;
struct LiveState {LiveTarget target{};bool valid{};uint64_t frame_watermark{};long long time_watermark{};} live_state;
enum class TargetReject:unsigned {none,not_live,stopped,thread,order,range,time,context,exception};
std::atomic<unsigned long long> live_published{},live_rejected{},live_expired{},live_invalidated{};
std::atomic<uint64_t> live_last_frame{};std::atomic<float> live_last_u{};std::atomic<TargetReject> live_last_reject{TargetReject::none};
bool fresh_target(LiveTarget const& target,LiveTime clock){return target.timestamp_qpc>0&&clock.qpc>=target.timestamp_qpc&&clock.frequency>=10&&
    clock.qpc-target.timestamp_qpc<=clock.frequency/10;}

template<class T> bool read(T& out,uintptr_t address){return address&&address<=UINTPTR_MAX-sizeof(T)&&safe_copy(&out,reinterpret_cast<void const*>(address),sizeof(T));}
template<class T> bool field(T& out,uintptr_t address,unsigned offset){return address<=UINTPTR_MAX-offset&&read(out,address+offset);}
bool range(uintptr_t address,size_t size){return address&&size&&address<=UINTPTR_MAX-size;}
bool cohort(gears_anchor::Registry const& a,gears_anchor::Registry const& b){return a.data&&a.count>1000&&a.count<=a.capacity&&a.capacity<4000000&&
    a.data==b.data&&a.capacity==b.capacity&&b.count>=a.count&&b.count<=b.capacity&&range(a.data,size_t(a.capacity)*8);}
bool same(gears_anchor::Identity const& a,gears_anchor::Identity const& b){return gears_anchor::equal_identity(a,b);}
bool named(gears_anchor::Identity const& id,const wchar_t* name){return !id.number&&!wcscmp(id.name.data(),name);}
bool identity(uintptr_t address,gears_anchor::Identity& out,bool instance=false,bool allow_long_prefix=false){
    gears_anchor::Registry objects{},names{},after_objects{},after_names{};
    if(!address||(address&7)||!read(objects,metadata.objects_header)||!read(names,metadata.names_header)||!cohort(objects,objects)||!cohort(names,names))return false;
    auto capture=[&](gears_anchor::Identity& id){id.address=address;return field(id.index,address,metadata.object_internal_index)&&field(id.name_index,address,metadata.object_name)&&
        field(id.number,address,metadata.object_name+4)&&field(id.cls,address,metadata.object_class)&&field(id.outer,address,metadata.object_outer);};
    auto text=[&](uintptr_t entry,std::array<wchar_t,64>& name){
        name.fill(0);if(entry>UINTPTR_MAX-metadata.name_entry_text)return false;
        for(unsigned i=0;i<63;++i){wchar_t c{};if(!field(c,entry+metadata.name_entry_text,i*2))return false;
            name[i]=c;if(!c)return i!=0;}
        return allow_long_prefix;};
    gears_anchor::Identity id{},again{};uintptr_t slot{},entry{},again_entry{};unsigned name_index{},again_name_index{};
    if(!capture(id)||!id.cls||id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||
       !read(slot,objects.data+uintptr_t(id.index)*8)||slot!=address||!read(entry,names.data+uintptr_t(id.name_index)*8)||
       !field(name_index,entry,metadata.name_entry_index)||name_index!=id.name_index||!text(entry,id.name)||
       (instance&&!wcsncmp(id.name.data(),L"Default__",9)))return false;
    if(!capture(again)||!text(entry,again.name))return false;
    if(!same(id,again)||!read(slot,objects.data+uintptr_t(id.index)*8)||slot!=address||!read(again_entry,names.data+uintptr_t(id.name_index)*8)||again_entry!=entry||
       !field(again_name_index,entry,metadata.name_entry_index)||again_name_index!=id.name_index||
       !read(after_objects,metadata.objects_header)||!read(after_names,metadata.names_header)||!cohort(objects,after_objects)||!cohort(names,after_names))return false;
    out=id;return true;
}
struct Source {uintptr_t address{};unsigned size{};bool membership{};std::array<unsigned char,8> bytes{};};
struct Layout {
    std::array<Source,2048> sources{};unsigned source_count{};
    std::array<gears_anchor::Identity,64> ids{};unsigned id_count{};
    unsigned children{},anim{},weight{},total{},skel{},owner{},mesh{},cover{},special{},world{};
    unsigned mirror_word{};uint32_t mirror_mask{};
} layout;
bool remember(gears_anchor::Identity const& id){
    for(unsigned i=0;i<layout.id_count;++i)if(layout.ids[i].address==id.address)return same(layout.ids[i],id);
    if(layout.id_count==layout.ids.size())return false;layout.ids[layout.id_count++]=id;return true;
}
template<class T> bool source(T& out,uintptr_t address,bool membership=false){
    static_assert(sizeof(T)<=8);T second{};if(!read(out,address)||!read(second,address)||memcmp(&out,&second,sizeof(T)))return false;
    for(unsigned i=0;i<layout.source_count;++i)if(layout.sources[i].address==address&&layout.sources[i].size==sizeof(T))return !memcmp(layout.sources[i].bytes.data(),&out,sizeof(T));
    if(layout.source_count==layout.sources.size())return false;auto& s=layout.sources[layout.source_count++];s.address=address;s.size=sizeof(T);s.membership=membership;memcpy(s.bytes.data(),&out,sizeof(T));return true;
}
bool certified_id(uintptr_t address,gears_anchor::Identity& id){return identity(address,id)&&remember(id);}
bool kind(gears_anchor::Identity const& id,const wchar_t* expected){gears_anchor::Identity cls{};return certified_id(id.cls,cls)&&named(cls,expected);}
struct Property {gears_anchor::Identity id{};unsigned dimension{},width{},offset{};};
bool descriptor(uintptr_t address,uintptr_t owner,const wchar_t* name,const wchar_t* type,unsigned width,Property& out){
    Property p{};if(!certified_id(address,p.id)||p.id.number||p.id.outer!=owner||(name&&wcscmp(p.id.name.data(),name))||!kind(p.id,type)||
       !source(p.dimension,address+0x70)||!source(p.width,address+0x74)||!source(p.offset,address+0x94)||p.dimension!=1||p.width!=width||p.offset>0xffff-width)return false;
    out=p;return true;
}
// These metadata offsets are the supported retail Resolver layout already used
// by anchor/cover-call setup: Field.Next=68, Struct.Children=80, Property D0.
bool property(uintptr_t start,const wchar_t* name,const wchar_t* type,unsigned width,Property& out){
    layout_failure=name;std::array<uintptr_t,32> visited{};unsigned owner_count{};
    for(auto owner=start;owner&&owner_count<visited.size();){
        for(unsigned i=0;i<owner_count;++i)if(visited[i]==owner)return false;visited[owner_count++]=owner;
        gears_anchor::Identity owner_id{};if(!certified_id(owner,owner_id)||(!kind(owner_id,L"Class")&&!kind(owner_id,L"ScriptStruct")))return false;
        uintptr_t child{};if(!source(child,owner+0x80,true))return false;unsigned count{},matches{};Property found{};
        std::array<uintptr_t,4096> children{};
        while(child&&count<children.size()){
            for(unsigned i=0;i<count;++i)if(children[i]==child)return false;children[count++]=child;
            gears_anchor::Identity id{};if(!identity(child,id,false,true)||id.outer!=owner)return false;
            if(!wcscmp(id.name.data(),name)){if(id.number)return false;if(++matches!=1||!descriptor(child,owner,name,type,width,found))return false;}
            // Only the prefix reaching our selected field is a membership
            // certificate. Later unrelated fields are scanned for duplicates,
            // without enlarging the per-frame retained source set.
            if(matches){if(!read(child,child+0x68))return false;}
            else if(!source(child,child+0x68,true))return false;
        }
        if(child)return false;if(matches){out=found;return true;}
        if(kind(owner_id,L"ScriptStruct"))return false;
        if(!source(owner,owner+metadata.field_super))return false;
    }
    return false;
}
bool pointer_property(uintptr_t cls,const wchar_t* name,Property& out){
    // UE3 distinguishes ComponentProperty and ObjectProperty; both are certified
    // pointer fields, and no other property type is accepted.
    return property(cls,name,L"ObjectProperty",8,out)||property(cls,name,L"ComponentProperty",8,out);
}
bool mirror_descriptor(Property const& mirror,uint32_t mask,unsigned stride){
    // Typed reflection is required first. These equalities additionally bind
    // that property to the saved F7940 consumer, not an arbitrary bool field.
    return mirror.dimension==1&&mirror.width==4&&mirror.offset==0x3c&&
        !(mirror.offset&3)&&mirror.offset+mirror.width<=stride&&mask==1;
}
template<size_t N> bool mirror_code_source(uintptr_t rva,std::array<unsigned char,N> const& bytes){
    std::array<unsigned char,N> actual{};
    if(!read(actual,base+rva)||actual!=bytes)return false;
    for(size_t i=0;i<N;++i){unsigned char value{};if(!source(value,base+rva+i)||value!=bytes[i])return false;}
    return true;
}
bool mirror_consumer_sources(){
    // Read-only cache fields + both child-edge tests from the saved/current
    // byte-matched generic engine method. No extra hook or cache mutation.
    return mirror_code_source(0xf79a8,std::array<unsigned char,23>{{0x4c,0x8b,0x41,0x60,0x41,0x8b,0x80,0x38,0x03,0x00,0x00,0x39,0x81,0xbc,0x00,0x00,0x00,0x0f,0x84,0xf5,0x05,0x00,0x00}})&&
        mirror_code_source(0xf7a47,std::array<unsigned char,6>{{0x41,0xf6,0x44,0x11,0x3c,0x01}})&&
        mirror_code_source(0xf7bbe,std::array<unsigned char,6>{{0x42,0xf6,0x44,0x1a,0x3c,0x01}});
}
bool prepare_mirror_layout(uintptr_t structure,unsigned stride){
    Property mirror{};uint32_t mask{};
    if(pose_profile!=PoseProfile::side||!property(structure,L"bMirrorSkeleton",L"BoolProperty",4,mirror)||
       !source(mask,mirror.id.address+0xd0)||!mirror_descriptor(mirror,mask,stride)){layout_failure=L"mirror_reflection_mismatch";return false;}
    if(!mirror_consumer_sources()){layout_failure=L"mirror_consumer_bytes";return false;}
    layout.mirror_word=mirror.offset;layout.mirror_mask=mask;return true;
}
bool prepare_layout(gears_anchor::Identity const& node,gears_anchor::PawnBinding const& binding){
    layout={};Property children{},inner{},anim{},weight{},total{},skel{},owner{},mesh{},cover{},special{},world{};
    uintptr_t inner_address{},structure{};gears_anchor::Identity structure_id{};unsigned stride{};
    if(!property(node.cls,L"Children",L"ArrayProperty",16,children)||!source(inner_address,children.id.address+0xd0)||
       !descriptor(inner_address,children.id.address,nullptr,L"StructProperty",0x44,inner)||inner.offset||
       !source(structure,inner_address+0xd0)||!certified_id(structure,structure_id)||!kind(structure_id,L"ScriptStruct")||!named(structure_id,L"AnimBlendChild")||
       !source(stride,structure+0x88)||stride!=0x44||!property(structure,L"Anim",L"ObjectProperty",8,anim)||
       !property(structure,L"Weight",L"FloatProperty",4,weight)||!property(structure,L"TotalWeight",L"FloatProperty",4,total)||
       !pointer_property(node.cls,L"SkelComponent",skel)||!property(node.cls,L"WarPawnOwner",L"ObjectProperty",8,owner)||
       !pointer_property(binding.pawn.cls,L"Mesh",mesh)||!property(binding.pawn.cls,L"CoverType",L"ByteProperty",1,cover)||
       !property(binding.pawn.cls,L"SpecialMove",L"ByteProperty",1,special)||
       !property(binding.pawn.cls,L"WorldInfo",L"ObjectProperty",8,world))return false;
    if(children.offset!=0xe4||anim.offset!=8||weight.offset!=0x10||total.offset!=0x14||skel.offset!=0x60||owner.offset!=0x128||
       anim.offset+anim.width>stride||weight.offset+weight.width>stride||total.offset+total.width>stride){layout_failure=L"native_layout_mismatch";return false;}
    if(side_mirror_trial&&!prepare_mirror_layout(structure,stride))return false;
    layout.children=children.offset;layout.anim=anim.offset;layout.weight=weight.offset;layout.total=total.offset;layout.skel=skel.offset;
    layout.owner=owner.offset;layout.mesh=mesh.offset;layout.cover=cover.offset;layout.special=special.offset;layout.world=world.offset;layout_failure=L"none";return true;
}
bool metadata_current(bool membership=true){
    for(unsigned i=0;i<layout.id_count;++i){gears_anchor::Identity id{};if(!identity(layout.ids[i].address,id)||!same(id,layout.ids[i]))return false;}
    for(unsigned i=0;i<layout.source_count;++i){auto const& s=layout.sources[i];if(s.membership&&!membership)continue;std::array<unsigned char,8> bytes{};
        if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(s.address),s.size)||memcmp(bytes.data(),s.bytes.data(),s.size))return false;}
    return true;
}
bool live_context(LiveTarget const& target){
    if(!enabled||!live_source()||!layout_ready.load()||!recording.load()||stopping.load()||external_change.load()||
       target.thread!=GetCurrentThreadId()||game_thread.load()!=target.thread||!target.world_generation||
       target.world_generation!=initial_world||target.world_generation!=committed_world_generation.load()||!own_entries()||!metadata_current(false))return false;
    gears_anchor::PawnBinding binding{};gears_anchor::Identity mesh_id{},world_id{};uintptr_t component{},world{};unsigned char ct{},sm{};
    auto capture=[&](){return gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)&&!binding.unpossessed&&!binding.cinematic&&same(binding.pawn,target.pawn)&&
        field(component,binding.pawn.address,layout.mesh)&&component==target.mesh.address&&identity(component,mesh_id,true)&&same(mesh_id,target.mesh)&&
        field(world,binding.pawn.address,layout.world)&&world==target.world.address&&identity(world,world_id,true)&&same(world_id,target.world)&&world_id.outer==binding.level&&
        field(ct,binding.pawn.address,layout.cover)&&ct==2&&field(sm,binding.pawn.address,layout.special)&&!sm;};
    return capture()&&capture()&&target.world_generation==committed_world_generation.load()&&metadata_current(false)&&own_entries();
}
void withdraw_target(){if(live_state.valid)++live_invalidated;live_state.valid=false;injecting=false;withdrawing=true;}
bool target_current(LiveTarget const& target){return fresh_target(target,target_clock())&&live_context(target);}
bool reject_target(TargetReject reason){++live_rejected;live_last_reject=reason;return false;}
// Scene GT caller already owns its admitted Scope/current certified context.
// This API takes body::mutex itself; callers MUST NOT already hold that mutex.
// Publication only stages CPU metadata. An open propagation keeps its own u.
bool publish_target(LiveTarget const& target){
    try{std::lock_guard<std::mutex> lock(mutex);
        if(!enabled||!live_source())return reject_target(TargetReject::not_live);
        if(stopping.load()||failed.load()||external_change.load()||!recording.load())return reject_target(TargetReject::stopped);
        if(target.thread!=GetCurrentThreadId()||game_thread.load()!=target.thread)return reject_target(TargetReject::thread);
        if(!target.frame||target.frame<=live_state.frame_watermark||target.timestamp_qpc<=live_state.time_watermark)return reject_target(TargetReject::order);
        if(!std::isfinite(target.u)||target.u<0||target.u>1)return reject_target(TargetReject::range);
        if(!fresh_target(target,target_clock()))return reject_target(TargetReject::time);
        if(!live_context(target)){withdraw_target();return reject_target(TargetReject::context);}
        live_state.target=target;live_state.valid=true;live_state.frame_watermark=target.frame;live_state.time_watermark=target.timestamp_qpc;
        ++live_published;live_last_frame=target.frame;live_last_u=target.u;live_last_reject=TargetReject::none;return true;
    }catch(...){failed=true;stopping=true;return reject_target(TargetReject::exception);}
}
struct Children {uintptr_t data{};int count{},capacity{};};
static_assert(sizeof(Children)==16);
bool children_at(uintptr_t node,Children& children){return field(children,node,layout.children)&&children.data&&!(children.data&7)&&children.count>=5&&children.count<=int(child_limit)&&
    children.capacity>=children.count&&children.capacity<=64&&range(children.data,size_t(children.capacity)*0x44);}
bool equal(Children const& a,Children const& b){return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
enum class MirrorReason:unsigned {none,await_neutral,await_fresh_epoch,cache_unreadable,already_cached,epoch_changed,native_side_nonzero,
    baseline_changed,completed,ready,layout,conflict,write_failed,source_changed,child_changed,repeated_epoch};
struct CacheTags {uint32_t node{},mesh{};};
struct MirrorLease {
    bool captured{},owned{},native_visible{},conflict{},started{},completed{},neutral_pending{},neutral_verified{},round_trial{},cache_read_ok{},initial_cache_seen{};
    uint32_t offset{},mask{},original_word{},native_bits{},installed_bits{},neutral_epoch{},round_epoch{},activation_epoch{},pose_epoch{};
    uintptr_t address{},child_anim{},pawn{},mesh{},world{};uint64_t generation{};TargetSource source=TargetSource::advance;CacheTags initial_tags{},last_tags{};
    unsigned installs{},restores{},samples{},rejected{};MirrorReason reason=MirrorReason::none;
};
struct Journal {
    bool used{};gears_anchor::Identity id{};uintptr_t pawn{},mesh{},world{};uint64_t generation{};Children children{};
    std::array<uint32_t,child_limit> native{},installed{};std::array<bool,child_limit> owned{};
    std::array<bool,child_limit> native_visible{};
    // Do not service this journal from an NPC/nested entry or an asynchronous
    // stop while one parent is still propagating / testing its native result.
    bool round_open{},propagating{};
    unsigned injections{},write_count{},restore_count{},retire_count{};
    unsigned installed_step{},endpoint=4;
    float target_u{};TargetSource source=TargetSource::advance;LiveTarget target{};
    std::array<std::array<unsigned,7>,7> logged{};
    std::array<unsigned,7> step_injections{},step_writes{},step_restores{};
    int previous_child=-1;float previous_incoming{},previous_weight{};
    MirrorLease mirror{};
};
std::array<Journal,node_limit> journals{};
struct Row {long long qpc{};uintptr_t node{};uint64_t world{};unsigned thread{},kind{},step{};int child{};float u{},incoming{},weight{},total{},predicted{};TargetSource source{};uint64_t target_frame{};long long target_qpc{};};
std::array<Row,row_limit> rows{};unsigned row_count{};
std::array<unsigned,7> step_rows{}; // Reserve room for every later Advance step.
void row(unsigned kind_value,Journal& j,int child=-1,float incoming=0,float weight=0,float total=0){
    const unsigned sample_step=j.installed_step;
    const bool child_row=kind_value==0||kind_value>=4;
    if(child_row&&child!=0&&child!=int(j.endpoint))return;
    auto& logged=j.logged[sample_step][kind_value];const unsigned limit=child_row?2:1;
    if(logged>=limit)return;++logged;
    if(step_rows[sample_step]>=18||row_count==rows.size()){++rows_dropped;return;}++step_rows[sample_step];
    rows[row_count++]={now(),j.id.address,j.generation,GetCurrentThreadId(),kind_value,sample_step,child,j.target_u,incoming,weight,total,incoming*weight,j.source,j.target.frame,j.target.timestamp_qpc};
}
void recount(){unsigned fields{},nodes{};for(auto const& j:journals){unsigned n=j.mirror.owned?1u:0u;for(bool b:j.owned)if(b)++n;fields+=n;if(n)++nodes;}pending=fields;pending_nodes=nodes;}
// CAS touches exactly one aligned reflected Weight. An external value is never
// overwritten. No DesiredWeights, TotalWeight, Pawn/root or transition writes.
__declspec(noinline) bool exchange(uintptr_t address,uint32_t expected,uint32_t desired,uint32_t& observed){
    MEMORY_BASIC_INFORMATION mbi{};
    if((address&3)||!VirtualQuery(reinterpret_cast<void const*>(address),&mbi,sizeof(mbi))||mbi.State!=MEM_COMMIT||mbi.Type!=MEM_PRIVATE||
       (mbi.Protect!=PAGE_READWRITE&&mbi.Protect!=PAGE_WRITECOPY)||address<reinterpret_cast<uintptr_t>(mbi.BaseAddress)||
       address>reinterpret_cast<uintptr_t>(mbi.BaseAddress)+mbi.RegionSize-4)return false;
    __try{observed=static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),static_cast<LONG>(desired),static_cast<LONG>(expected)));return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
enum class MaskExchange {changed,conflict,unavailable};
MaskExchange exchange_mask(uintptr_t address,uint32_t mask,uint32_t expected,uint32_t desired){
    if(!mask||(mask&(mask-1))||(expected&~mask)||(desired&~mask))return MaskExchange::unavailable;
    // Bounded retries only for unrelated bits. Never compare/write a saved
    // whole word over an external update to another packed bool.
    for(unsigned attempt=0;attempt<4;++attempt){uint32_t current{},observed{};
        if(!read(current,address))return MaskExchange::unavailable;
        if((current&mask)!=expected)return MaskExchange::conflict;
        if(!exchange(address,current,(current&~mask)|desired,observed))return MaskExchange::unavailable;
        if(observed==current)return MaskExchange::changed;
        if((observed&mask)!=expected)return MaskExchange::conflict;
    }
    return MaskExchange::unavailable;
}
bool mirror_reject(Journal& j,MirrorReason reason){j.mirror.reason=reason;++j.mirror.rejected;return false;}
bool cache_tags(Journal& j){
    // Equality is a completed-cache observation, not an evaluation lock. This
    // private trial does not certify concurrent/in-progress GetBoneAtoms.
    CacheTags first{},second{};auto& m=j.mirror;++m.samples;
    m.cache_read_ok=field(first.node,j.id.address,0xbc)&&field(first.mesh,j.mesh,0x338)&&
        field(second.node,j.id.address,0xbc)&&field(second.mesh,j.mesh,0x338)&&first.node==second.node&&first.mesh==second.mesh;
    m.last_tags=first;
    if(m.cache_read_ok&&!m.initial_cache_seen){m.initial_tags=first;m.initial_cache_seen=true;}
    return m.cache_read_ok||mirror_reject(j,MirrorReason::cache_unreadable);
}
bool mirror_zero_weight(Journal const& j){float value{};return field(value,j.children.data+uintptr_t(j.endpoint)*0x44,layout.weight)&&value==0.f;}
bool mirror_epoch_current(Journal& j){
    if(!side_mirror_trial)return true;
    if(!cache_tags(j))return false;
    if(j.mirror.last_tags.mesh!=j.mirror.round_epoch)return mirror_reject(j,MirrorReason::epoch_changed);
    if(j.mirror.last_tags.node==j.mirror.last_tags.mesh)return mirror_reject(j,MirrorReason::already_cached);
    return true;
}
bool mirror_begin_round(Journal& j,uintptr_t child_anim){
    if(!side_mirror_trial)return true;
    auto& m=j.mirror;m.round_trial=false;
    if(pose_profile!=PoseProfile::side||j.endpoint!=5||layout.mirror_word!=0x3c||layout.mirror_mask!=1)return mirror_reject(j,MirrorReason::layout);
    if(!mirror_zero_weight(j))return mirror_reject(j,MirrorReason::native_side_nonzero);
    uint32_t word{};const auto address=j.children.data+uintptr_t(j.endpoint)*0x44+layout.mirror_word;
    if(!read(word,address))return mirror_reject(j,MirrorReason::write_failed);
    if(!m.captured){m.captured=true;m.address=address;m.offset=layout.mirror_word;m.mask=layout.mirror_mask;m.original_word=word;
        m.native_bits=word&m.mask;m.installed_bits=m.native_bits^m.mask;m.child_anim=child_anim;m.source=j.source;
        m.pawn=j.pawn;m.mesh=j.mesh;m.world=j.world;m.generation=j.generation;
    }else if(m.address!=address||m.child_anim!=child_anim||m.pawn!=j.pawn||m.mesh!=j.mesh||m.world!=j.world||m.generation!=j.generation)
        return mirror_reject(j,MirrorReason::child_changed);
    if(m.source!=j.source)return mirror_reject(j,MirrorReason::source_changed);
    if((word&m.mask)!=m.native_bits)return mirror_reject(j,MirrorReason::baseline_changed);
    if(!cache_tags(j))return false;
    if(m.last_tags.node==m.last_tags.mesh)return mirror_reject(j,MirrorReason::already_cached);
    // A completed neutral Tick alone is insufficient. Observe that exact
    // node cache tag on a subsequent fresh mesh epoch before the first trial.
    if(m.neutral_pending&&m.last_tags.node==m.neutral_epoch&&m.last_tags.mesh!=m.neutral_epoch)m.neutral_verified=true;
    m.round_epoch=m.last_tags.mesh;
    if(j.target_u==0){if(m.started)m.completed=true;return true;}
    if(m.completed)return mirror_reject(j,MirrorReason::completed);
    if(!m.neutral_verified)return mirror_reject(j,m.neutral_pending?MirrorReason::await_fresh_epoch:MirrorReason::await_neutral);
    if(m.started&&m.pose_epoch==m.round_epoch)return mirror_reject(j,MirrorReason::repeated_epoch);
    m.round_trial=true;m.reason=MirrorReason::ready;return true;
}
bool mirror_install(Journal& j){
    auto& m=j.mirror;if(!side_mirror_trial||!m.round_trial)return true;
    if(!mirror_epoch_current(j)||!mirror_zero_weight(j)){if(m.reason==MirrorReason::ready)mirror_reject(j,MirrorReason::native_side_nonzero);return false;}
    const auto result=exchange_mask(m.address,m.mask,m.native_bits,m.installed_bits);
    if(result!=MaskExchange::changed){
        // An unknown write remains owed. A masked conflict after ownership is
        // acquired is sticky debt; the bit is never silently abandoned.
        if(result==MaskExchange::conflict&&m.owned){m.conflict=true;external_change=true;++external;}
        return mirror_reject(j,result==MaskExchange::conflict?MirrorReason::conflict:MirrorReason::write_failed);
    }
    m.owned=true;m.native_visible=false;if(!m.started)m.activation_epoch=m.round_epoch;m.started=true;++m.installs;
    ++writes;++j.write_count;++j.step_writes[j.installed_step];recount();return true;
}
bool mirror_restore(Journal& j,bool retain){
    auto& m=j.mirror;if(!m.owned)return true;
    if(m.conflict){stopping=true;return false;}
    if(!mirror_zero_weight(j)){stopping=true;return mirror_reject(j,MirrorReason::native_side_nonzero);}
    if(m.native_visible){uint32_t word{};
        if(!read(word,m.address)){failed=true;stopping=true;return mirror_reject(j,MirrorReason::write_failed);}
        if((word&m.mask)!=m.native_bits){m.conflict=true;external_change=true;++external;stopping=true;return mirror_reject(j,MirrorReason::conflict);}
    }else{
        const auto result=exchange_mask(m.address,m.mask,m.installed_bits,m.native_bits);
        if(result!=MaskExchange::changed){failed=true;stopping=true;if(result==MaskExchange::conflict){m.conflict=true;external_change=true;++external;}
            return mirror_reject(j,result==MaskExchange::conflict?MirrorReason::conflict:MirrorReason::write_failed);}
        m.native_visible=true;++m.restores;++restores;++j.restore_count;++j.step_restores[j.installed_step];
    }
    if(!retain)m.owned=false;return true;
}
bool any(Journal const& j){if(j.mirror.owned)return true;for(bool value:j.owned)if(value)return true;return false;}
void retire(Journal& j){for(auto& owned:j.owned)if(owned){owned=false;++retired;++j.retire_count;}
    if(j.mirror.owned){j.mirror.owned=false;++retired;++j.retire_count;}j.native_visible.fill(false);j.previous_child=-1;row(3,j);}
void propagation(Journal& j){
    if(j.previous_child<0)return;Children current{};float total{};
    if(children_at(j.id.address,current)&&equal(current,j.children)&&field(total,j.children.data+uintptr_t(j.previous_child)*0x44,layout.total))
        row(4,j,j.previous_child,j.previous_incoming,j.previous_weight,total);
    j.previous_child=-1;
}
void phase_rows(Journal& j,unsigned kind_value){
    // Kinds 5/6 are phase samples, not another propagation receipt: no incoming
    // argument is assigned to them and their predicted value is intentionally 0.
    if(j.logged[j.installed_step][kind_value]>=2)return;
    for(unsigned i:{0u,j.endpoint}){float weight{},total{};
        if(field(weight,j.children.data+uintptr_t(i)*0x44,layout.weight)&&field(total,j.children.data+uintptr_t(i)*0x44,layout.total))row(kind_value,j,int(i),0,weight,total);}
}
bool journal_current(Journal& j){
        gears_anchor::Registry registry{};uintptr_t slot{},pawn{},owner{},mesh{},world{},table{};
        if(!read(registry,metadata.objects_header)||!cohort(registry,registry)||j.id.index>=unsigned(registry.count)||!read(slot,registry.data+uintptr_t(j.id.index)*8)){stopping=true;return false;}
        // Only a positive loss/change of this registered object identity can
        // retire ownership. A different world, possession, link or storage
        // header does not prove the old object/allocation has ceased to live.
        if(slot!=j.id.address){retire(j);return false;}
        gears_anchor::Identity id{};if(!identity(j.id.address,id)){stopping=true;return false;}if(!same(id,j.id)){retire(j);return false;}
        if(j.generation!=committed_world_generation.load()){stopping=true;return false;}
        if(!field(pawn,metadata.identities[metadata.role_controller].address,metadata.controller_pawn)||!field(owner,j.id.address,layout.owner)||
           !field(mesh,j.id.address,layout.skel)||!field(world,j.pawn,layout.world)||!read(table,j.id.address)){stopping=true;return false;}
        if(pawn!=j.pawn||owner!=j.pawn||mesh!=j.mesh||world!=j.world||table!=base+0x13d0080){stopping=true;return false;}
        if(j.source==TargetSource::live){gears_anchor::Identity world_id{},mesh_id{};
            if(!identity(world,world_id,true)||!same(world_id,j.target.world)||!identity(mesh,mesh_id,true)||!same(mesh_id,j.target.mesh)){stopping=true;return false;}}
        Children current{};if(!field(current,j.id.address,layout.children)||!equal(current,j.children)){stopping=true;return false;}
        if(!metadata_current(false)||!own_entries()){stopping=true;return false;}
        gears_anchor::PawnBinding binding{};if(!gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)||binding.unpossessed||binding.cinematic||binding.pawn.address!=j.pawn){stopping=true;return false;}
        if(j.source==TargetSource::live&&!same(binding.pawn,j.target.pawn)){stopping=true;return false;}
        return true;
}
// retain=true is the native-post window, not a completed restoration. Keep the
// exact field obligations even though their current expected bytes are native.
bool native_weights(Journal& j,bool retain){
    if(!journal_current(j))return false;
    propagation(j);
    for(unsigned i=0;i<unsigned(j.children.count);++i)if(j.owned[i]){
        uint32_t observed{};const auto address=j.children.data+uintptr_t(i)*0x44+layout.weight;
        if(j.native_visible[i]){
            if(!read(observed,address)){failed=true;stopping=true;continue;}
            if(observed!=j.native[i]){j.owned[i]=false;++external;external_change=true;stopping=true;}
            else if(!retain)j.owned[i]=false;
        }else{
            if(!exchange(address,j.installed[i],j.native[i],observed)){failed=true;stopping=true;continue;}
            if(observed==j.installed[i]){j.native_visible[i]=true;if(!retain)j.owned[i]=false;++restores;++j.restore_count;++j.step_restores[j.installed_step];}
            else{j.owned[i]=false;++external;external_change=true;stopping=true;}
        }
    }
    // W5 must reach zero before the local mirror bit can change. A failed
    // Weight restore therefore also retains the mirror field's obligation.
    const bool mirror_ok=mirror_restore(j,retain);
    row(2,j);recount();return mirror_ok;
}
void restore_journals(uintptr_t self,bool all){
    for(auto& j:journals){if(!j.used||!any(j)||j.round_open||(!all&&j.id.address!=self))continue;
        native_weights(j,false);
    }
    recount();
}
void restore_all(){restore_journals(0,true);}
void restore_matching(uintptr_t self){restore_journals(self,false);}
struct Tick {
    uintptr_t node{},pawn{},mesh{},world{};gears_anchor::Identity id{};Children children{};uint64_t generation{};bool injected{};
    Journal* journal{};std::array<uintptr_t,child_limit> animations{};int next_child=-1,last_child=-1;bool propagated{};
};
thread_local Tick tick{};
// POD exchanged with MASM: exactly these two register-only retail ABIs. The
// wrapper gives the original fresh shadow space; it does not forward stack
// parameters, the caller's RSP, return address, or home-space contents.
struct alignas(16) CallPacket {std::array<uint64_t,7> gpr{};uint64_t flags{};std::array<std::array<unsigned char,16>,6> xmm{};};
static_assert(sizeof(CallPacket)==0xa0&&offsetof(CallPacket,flags)==0x38&&offsetof(CallPacket,xmm)==0x40);
struct Call {
    bool live{},selected{},tracked_child{},poisoned{},finished{};unsigned kind{};uintptr_t self{};int child=-1;
    Tick saved_tick{};Call* previous_blend{};Call* parent{};
    alignas(Scope) std::array<unsigned char,sizeof(Scope)> scope_storage{};Scope* scope{};
};
constexpr unsigned call_limit=16;
thread_local std::array<Call,call_limit> calls{};
thread_local unsigned call_depth{};
thread_local Call* current_blend{};
void poison(){failed=true;stopping=true;if(current_blend)current_blend->poisoned=true;}
bool rounds_open(){for(auto const& j:journals)if(j.round_open)return true;return false;}
// Read-only; caller MUST hold body::mutex. Diagnostic buckets never certify
// neutral: an actual .05 target may share bucket 0 with the neutral endpoint.
bool neutral_pose_for(uintptr_t pawn,uintptr_t mesh){
    if(rounds_open()||transactions.load())return false;
    for(auto const& j:journals)if(j.used&&j.pawn==pawn&&j.mesh==mesh&&any(j)&&(j.target_u!=0||j.mirror.owned))return false;
    if(!enabled)return true;
    if(live_source())return !live_state.valid||live_state.target.pawn.address!=pawn||live_state.target.mesh.address!=mesh||live_state.target.u==0;
    const auto s=step.load();return !injecting||(s<steps.size()&&steps[s]==0);
}
void events(){
    const auto clock=GetTickCount64();
    if(reset_event&&WaitForSingleObject(reset_event,0)==WAIT_OBJECT_0){injecting=false;withdrawing=true;next_step=0;step=0;if(live_source())withdraw_target();}
    if(stopping.load()){injecting=false;return;}
    if(live_source()){
        if(live_state.valid&&!target_current(live_state.target)){if(!fresh_target(live_state.target,target_clock()))++live_expired;withdraw_target();}
        injecting=live_state.valid&&!withdrawing;return;
    }
    // Preserve a queued Advance until every old owned field has been released.
    if(!withdrawing&&advance_event&&WaitForSingleObject(advance_event,0)==WAIT_OBJECT_0){step=next_step;next_step=(next_step+1)%unsigned(steps.size());last_advance=clock;injecting=true;timed_out=false;}
    if(injecting&&clock-last_advance>=10000){injecting=false;withdrawing=true;timed_out=true;}
}
// Call only on the already-bound native tick thread. Normal entries release
// their own previous injection; only an explicit withdrawal may release peers.
void service_entry(uintptr_t self){
    events();
    if(side_mirror_trial)for(auto& j:journals)if(j.mirror.captured&&j.mirror.source!=target_source){
        j.mirror.reason=MirrorReason::source_changed;injecting=false;withdrawing=true;
    }
    if(stopping.load()||withdrawing){
        for(auto& j:journals){if(j.mirror.started)j.mirror.completed=true;j.mirror.neutral_pending=j.mirror.neutral_verified=false;}
        restore_all();tick={};
        if(!pending.load()&&!rounds_open())withdrawing=false;
    }else{
        restore_matching(self);
        if(tick.node==self)tick={};
    }
}
void blend_before(uintptr_t self,Scope const& scope){
    if(!enabled)return;
    ++blend_calls;
    const unsigned thread=GetCurrentThreadId();if(game_thread.load()&&game_thread.load()!=thread){++rejected;return;}
    const bool bound=game_thread.load()!=0;
    // Restoration precedes this receiver's qualification. An unrelated NPC
    // must neither restore another node's weights nor clear its TLS context.
    if(bound&&scope.world())service_entry(self);
    // Startup precedes campaign/world recovery. Before any actual write, a new
    // admitted anchor cohort may replace our unused certificate. Once written,
    // a generation change retains pending until actual identity loss/change and
    // permanently stops new injection. It never silently grants a new cohort.
    if(enabled&&scope.world()&&!writes&&!stopping.load()&&
       (!layout_ready.load()||initial_world!=committed_world_generation.load())){
        if(initial_world!=committed_world_generation.load()){
            layout_ready=false;layout={};layout_attempts=0;last_layout_attempt=0;journals={};row_count=0;step_rows={};++prewrite_rebinds;
        }
        metadata=anchor_config;initial_world=committed_world_generation.load();
    }
    if(!enabled||!scope.world()||!recording.load()||stopping.load()||external_change||!own_entries()||initial_world!=committed_world_generation.load())return;
    gears_anchor::PawnBinding binding{};gears_anchor::Identity id{},cls{};uintptr_t table{},owner{},mesh{};
    if(!gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)||binding.unpossessed||binding.cinematic||
       !identity(self,id,true)||!identity(id.cls,cls)||!named(cls,L"WarAnim_CoverBlend")||!read(table,self)||table!=base+0x13d0080||
       !field(owner,self,0x128)||owner!=binding.pawn.address||!field(mesh,self,0x60)||!mesh){++rejected;return;}
    if(!layout_ready){const auto clock=GetTickCount64();if(layout_attempts>=8||clock-last_layout_attempt<1000)return;last_layout_attempt=clock;++layout_attempts;
        if(!prepare_layout(id,binding))return;layout_ready=true;}
    uintptr_t pawn_mesh{},world{};unsigned char ct{},sm{};gears_anchor::Identity mesh_id{},world_id{};Children children{},after{};
    if(!metadata_current()||!field(pawn_mesh,binding.pawn.address,layout.mesh)||pawn_mesh!=mesh||!identity(mesh,mesh_id,true)||
       !field(ct,binding.pawn.address,layout.cover)||ct!=2||!field(sm,binding.pawn.address,layout.special)||sm!=0||
       !field(world,binding.pawn.address,layout.world)||!identity(world,world_id,true)||world_id.outer!=binding.level||
       !children_at(self,children)||children.count<=int(endpoint_for(pose_profile))||!children_at(self,after)||!equal(children,after)){++rejected;return;}
    if(!game_thread.load()){game_thread=thread;service_entry(self);}
    tick={self,binding.pawn.address,mesh,world,id,children,initial_world,false};
}
bool animations_current(){
    Children current{};if(!children_at(tick.node,current)||!equal(current,tick.children))return false;
    for(unsigned i=0;i<unsigned(current.count);++i){uintptr_t anim{};if(!field(anim,current.data+uintptr_t(i)*0x44,layout.anim)||anim!=tick.animations[i])return false;}
    return true;
}
void child_before(uintptr_t self,int child,float incoming,Scope const& scope,Call& call){
    if(!enabled||!current_blend||!current_blend->selected||tick.node!=self)return;
    ++child_calls;
    events();
    if(!enabled||game_thread.load()!=GetCurrentThreadId()||tick.node!=self||tick.generation!=committed_world_generation.load()||
       !scope.world()||!recording.load()||!std::isfinite(incoming)||incoming<=0||incoming>2){if(tick.injected)poison();return;}
    Journal* journal{};for(auto& j:journals)if(j.used&&same(j.id,tick.id)){journal=&j;break;}
    // Once a round has begun, a stop/Reset/expiry only prevents its rearm. It
    // must not mix W and native weights within the original propagation loop.
    if(!tick.injected){
        if(stopping.load()||withdrawing){restore_all();if(!pending.load()&&!rounds_open())withdrawing=false;return;}
        if(!injecting||external_change)return;
    }
    Children current{};uintptr_t owner{},mesh{},world{},pawn_mesh{};unsigned char ct{},sm{};gears_anchor::PawnBinding binding{};
    if(!own_entries()||(!tick.injected&&!metadata_current(false))||!gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)||binding.cinematic||binding.unpossessed||binding.pawn.address!=tick.pawn||
       !field(owner,self,layout.owner)||owner!=tick.pawn||!field(mesh,self,layout.skel)||mesh!=tick.mesh||!field(pawn_mesh,tick.pawn,layout.mesh)||pawn_mesh!=tick.mesh||
       !field(world,tick.pawn,layout.world)||world!=tick.world||!field(ct,tick.pawn,layout.cover)||ct!=2||!field(sm,tick.pawn,layout.special)||sm!=0||
       !children_at(self,current)||current.count<=int(endpoint_for(pose_profile))||!equal(current,tick.children)){++rejected;if(tick.injected)poison();return;}
    if(child<0||child>=current.count)return;
    if(!tick.injected){
        LiveTarget round_target{};float u{};const auto source_mode=target_source;unsigned bucket=step.load();
        if(live_source()){
            if(!live_state.valid||!target_current(live_state.target)){withdraw_target();return;}
            round_target=live_state.target;u=round_target.u;
            if(round_target.pawn.address!=tick.pawn||round_target.mesh.address!=tick.mesh||round_target.world.address!=tick.world||round_target.world_generation!=tick.generation){withdraw_target();return;}
            // Diagnostic buckets only; installed Weight uses the exact u.
            bucket=static_cast<unsigned>(std::lround(u*4));
        }else{if(bucket>=steps.size())return;u=steps[bucket];}
        unsigned first=child_limit;std::array<uint32_t,child_limit> native{};std::array<uintptr_t,child_limit> animations{};
        for(unsigned i=0;i<unsigned(current.count);++i){float weight{};
            if(!field(animations[i],current.data+uintptr_t(i)*0x44,layout.anim)||!field(weight,current.data+uintptr_t(i)*0x44,layout.weight)||!std::isfinite(weight)||weight<0||weight>1)return;
            if(animations[i]&&first==child_limit)first=i;memcpy(&native[i],&weight,4);}
        const unsigned endpoint=endpoint_for(pose_profile);
        if(first!=unsigned(child)||!animations[0]||!animations[endpoint])return;
        if(!journal){for(auto& j:journals)if(!j.used){journal=&j;j.used=true;j.id=tick.id;break;}}
        if(!journal||any(*journal)||journal->round_open){++rejected;return;}
        auto& j=*journal;j.pawn=tick.pawn;j.mesh=tick.mesh;j.world=tick.world;j.generation=tick.generation;j.children=current;j.native=native;j.installed_step=bucket;j.endpoint=endpoint;
        j.target_u=u;j.source=source_mode;j.target=round_target;
        j.native_visible.fill(false);
        for(unsigned i=0;i<unsigned(current.count);++i){float weight=i==0?1.f-j.target_u:(i==j.endpoint?j.target_u:0.f);memcpy(&j.installed[i],&weight,4);}
        // A final header/identity bracket before the first field CAS.
        gears_anchor::Identity again{};Children after{};
        if(!identity(self,again,true)||!same(again,j.id)||!children_at(self,after)||!equal(current,after))return;
        if(j.source==TargetSource::live&&!target_current(j.target)){withdraw_target();return;}
        if(!mirror_begin_round(j,animations[endpoint]))return;
        if(!mirror_install(j)){failed=true;stopping=true;native_weights(j,false);return;}
        for(unsigned i=0;i<unsigned(current.count);++i)if(j.native[i]!=j.installed[i]){
            uint32_t observed{};if(!exchange(current.data+uintptr_t(i)*0x44+layout.weight,j.native[i],j.installed[i],observed)){failed=true;stopping=true;break;}
            if(observed!=j.native[i]){external_change=true;++external;stopping=true;break;}j.owned[i]=true;++writes;++j.write_count;++j.step_writes[j.installed_step];
        }
        if(side_mirror_trial&&!mirror_epoch_current(j)){failed=true;stopping=true;}
        recount();
        if(stopping.load()){native_weights(j,false);return;}
        tick.injected=true;tick.journal=&j;tick.animations=animations;tick.next_child=child;
        for(unsigned i=0;i<unsigned(current.count);++i)if(animations[i])tick.last_child=int(i);
        j.round_open=j.propagating=true;++j.injections;++j.step_injections[j.installed_step];row(1,j);
    }
    if(!journal||tick.journal!=journal||child!=tick.next_child||!animations_current()){++rejected;poison();return;}
    call.tracked_child=true;call.child=child;
    if(journal){float weight{},total{};if(field(weight,current.data+uintptr_t(child)*0x44,layout.weight)&&field(total,current.data+uintptr_t(child)*0x44,layout.total)){
        row(0,*journal,child,incoming,weight,total);journal->previous_child=child;journal->previous_incoming=incoming;journal->previous_weight=weight;}}
}
void child_after(Call& call,bool returned){
    if(!call.tracked_child)return;
    ++child_posts;
    if(!returned||call.parent!=current_blend||!current_blend||tick.node!=call.self||!tick.journal||
       !tick.journal->round_open||call.child!=tick.next_child||!animations_current()){poison();return;}
    auto& j=*tick.journal;propagation(j);
    if(call.child==tick.last_child){
        // The sole native FA680 invocation for the last nonnull child has
        // completed. Native CoverBlend post-tests must now see its own weights.
        if(!native_weights(j,true)){poison();return;}
        for(unsigned i=0;i<unsigned(j.children.count);++i)if(j.owned[i]&&!j.native_visible[i]){poison();return;}
        if(j.mirror.owned&&!j.mirror.native_visible){poison();return;}
        j.propagating=false;tick.propagated=true;tick.next_child=-1;++native_posts;phase_rows(j,5);
    }else{
        tick.next_child=-1;for(int i=call.child+1;i<j.children.count;++i)if(tick.animations[unsigned(i)]){tick.next_child=i;break;}
        if(tick.next_child<0)poison();
    }
}
void rearm(Journal& j){
    if(j.source==TargetSource::live&&(!live_state.valid||!target_current(j.target)||!target_current(live_state.target))){withdraw_target();native_weights(j,false);return;}
    if(!journal_current(j))return;
    unsigned char ct{},sm{};
    if(!field(ct,j.pawn,layout.cover)||ct!=2||!field(sm,j.pawn,layout.special)||sm!=0){stopping=true;native_weights(j,false);return;}
    if(side_mirror_trial&&(!mirror_epoch_current(j)||!mirror_install(j))){poison();native_weights(j,false);return;}
    for(unsigned i=0;i<unsigned(j.children.count);++i)if(j.owned[i]){
        if(!j.native_visible[i]){poison();break;}
        uint32_t observed{};
        if(!exchange(j.children.data+uintptr_t(i)*0x44+layout.weight,j.native[i],j.installed[i],observed)){poison();break;}
        if(observed!=j.native[i]){j.owned[i]=false;++external;external_change=true;stopping=true;break;}
        j.native_visible[i]=false;++writes;++j.write_count;++j.step_writes[j.installed_step];
    }
    if(side_mirror_trial&&!mirror_epoch_current(j))poison();
    if(stopping.load()||external_change)native_weights(j,false);else{
        if(side_mirror_trial){auto& m=j.mirror;
            if(j.target_u==0){m.neutral_epoch=m.round_epoch;m.neutral_pending=true;m.neutral_verified=false;if(!m.started)m.reason=MirrorReason::await_fresh_epoch;}
            else m.pose_epoch=m.round_epoch;
        }
        ++pose_rearms;phase_rows(j,6);
    }
    recount();
}
Call* token_call(uintptr_t token){
    if(!token||!call_depth)return nullptr;
    auto* call=&calls[call_depth-1];return call->live&&reinterpret_cast<uintptr_t>(call)==token?call:nullptr;
}
// Called inside the protected wrapper. Publishing the token before construction
// lets its finally release a completed Scope even if later preparation raises.
void begin_call(CallPacket const& packet,unsigned kind,uintptr_t& token){
    const auto self=uintptr_t(packet.gpr[1]);
    if(!enabled)return;
    // Global FA680 hot path: no Scope, mutex, or selected-call count for peers.
    if(kind==1&&(!current_blend||!current_blend->selected||tick.node!=self))return;
    if(call_depth==call_limit){poison();return;}
    auto& c=calls[call_depth++];c=Call{};c.live=true;c.kind=kind;c.self=self;c.parent=current_blend;
    token=reinterpret_cast<uintptr_t>(&c);
    if(!kind){c.saved_tick=tick;c.previous_blend=current_blend;current_blend=&c;tick={};}
    c.scope=new(c.scope_storage.data()) Scope;
    std::lock_guard<std::mutex> lock(mutex);
    if(!kind){
        // A reentrant call of the same receiver may execute its original once,
        // but cannot restore/reinject its parent's journal or arm a pose rearm.
        for(auto* p=c.previous_blend;p;p=p->previous_blend)if(p->self==self){p->poisoned=true;++reentries;poison();return;}
        blend_before(self,*c.scope);c.selected=tick.node==self;
        if(c.selected)++transactions;
    }else{
        float incoming{};memcpy(&incoming,packet.xmm[2].data(),4);
        child_before(self,static_cast<int32_t>(packet.gpr[2]),incoming,*c.scope,c);
    }
}
void finish_call(uintptr_t token,bool returned){
    auto* c=token_call(token);if(!c)return;
    std::lock_guard<std::mutex> lock(mutex);
    if(c->kind){child_after(*c,returned);c->finished=true;return;}
    if(c!=current_blend){poison();return;}
    if(c->selected&&!returned){++abandoned;poison();}
    if(c->selected&&tick.journal&&tick.journal->round_open){
        auto& j=*tick.journal;events();
        if(!tick.propagated){if(returned)++abandoned;poison();}
        if(returned&&tick.propagated&&!c->poisoned&&!stopping.load()&&!withdrawing&&injecting&&!external_change)rearm(j);
        else {native_weights(j,false);if(j.mirror.started)j.mirror.completed=true;j.mirror.neutral_pending=j.mirror.neutral_verified=false;}
        j.propagating=j.round_open=false;recount();
    }
    c->finished=true;
}
void release_call(uintptr_t token){
    auto* c=token_call(token);if(!c){if(token)poison();return;}
    // No engine write is attempted here. An unexpected post failure leaves its
    // exact journal for a later bound-thread conditional restore / explicit fail.
    if(!c->finished){poison();if(!c->kind&&tick.journal)tick.journal->propagating=tick.journal->round_open=false;}
    if(!c->kind){if(c->selected)--transactions;tick=c->saved_tick;current_blend=c->previous_blend;}
    if(c->scope){c->scope->~Scope();c->scope=nullptr;}c->live=false;--call_depth;
}
// These C++ guards never enclose the original. Ordinary helper/lock exceptions
// unwind their RAII locks before fail-stop. Foreign-address SEH is contained
// only inside safe_copy/exchange; an unexpected SEH under a C++ lock is NOT
// swallowed by an outer __except, which would skip /EHsc lock destruction.
void begin_guarded(CallPacket const& packet,unsigned kind,uintptr_t& token){
    try{begin_call(packet,kind,token);}catch(...){failed=true;stopping=true;}
}
void finish_guarded(uintptr_t token,bool returned){
    try{finish_call(token,returned);}catch(...){failed=true;stopping=true;}
}
// /EHsc includes /EHc; explicitly permit exceptions across these C-linkage
// bridges instead of letting the compiler assume an engine call cannot throw.
extern "C" void GearsCoverRawInvoke(CallPacket*,void*) noexcept(false);
extern "C" __declspec(noinline) void GearsCoverInvokeProtected(CallPacket* packet,unsigned kind) noexcept(false){
    uintptr_t token{};bool returned{};++active;++in_flight;
    __try{
        begin_guarded(*packet,kind,token);
        GearsCoverRawInvoke(packet,kind?GearsCoverChildOriginal:GearsCoverBlendOriginal);
        returned=true;
    }__finally{
        // Compiler-generated UHandler metadata owns exceptional cleanup. Scope
        // and residency remain alive through both the original and normal post.
        __try{finish_guarded(token,returned&&!AbnormalTermination());}
        __finally{release_call(token);--in_flight;--active;}
    }
}
bool reserve(){
    if(reserved)return false;
    for(unsigned i=0;i<2;++i){targets[i]=reinterpret_cast<void*>(base+rvas[i]);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],before.size())||before!=signatures[i]||MH_CreateHook(targets[i],detours[i],originals[i])!=MH_OK||
           !safe_copy(after.data(),targets[i],after.size())||before!=after)return false;}
    reserved=true;return true;
}
bool reserved_target(void* target){return reserved&&(target==targets[0]||target==targets[1]);}
bool read_profile(const wchar_t* ini,PoseProfile& out){
    wchar_t text[16]{};const auto length=GetPrivateProfileStringW(L"CoverBodyPrototype",L"pose_profile",L"top",text,16,ini);
    if(!length||length>=15)return false;
    if(!wcscmp(text,L"top")){out=PoseProfile::top;return true;}
    if(!wcscmp(text,L"side")){out=PoseProfile::side;return true;}
    return false;
}
bool read_source(const wchar_t* ini,TargetSource& out){
    wchar_t text[16]{};const auto length=GetPrivateProfileStringW(L"CoverBodyPrototype",L"target_source",L"advance",text,16,ini);
    if(!length||length>=15)return false;
    if(!wcscmp(text,L"advance")){out=TargetSource::advance;return true;}
    if(!wcscmp(text,L"live")){out=TargetSource::live;return true;}return false;
}
bool read_mirror_trial(const wchar_t* ini,bool& out){
    wchar_t text[4]{};const auto length=GetPrivateProfileStringW(L"CoverBodyPrototype",L"side_mirror_trial",L"0",text,4,ini);
    if(length!=1||(text[0]!=L'0'&&text[0]!=L'1'))return false;out=text[0]==L'1';return true;
}
bool configure(const wchar_t* ini,FILE* file){
    PoseProfile profile{};if(!read_profile(ini,profile))return false;
    TargetSource source{};if(!read_source(ini,source))return false;
    bool mirror_trial{};if(!read_mirror_trial(ini,mirror_trial)||(mirror_trial&&profile!=PoseProfile::side))return false;
    const auto requested=GetPrivateProfileIntW(L"CoverBodyPrototype",L"enabled",0,ini);if(!requested)return true;
    if(requested!=1||enabled||!reserved||!continuous_session||!anchor_observer_enabled)return false;
    metadata=anchor_config;initial_world=committed_world_generation.load();
    if(metadata.object_internal_index!=8||metadata.object_name!=72||metadata.object_class!=80||metadata.object_outer!=64||metadata.field_super!=96)return false;
    wchar_t name[128]{};
    auto event=[&](HANDLE& handle,const wchar_t* action){swprintf_s(name,L"Local\\GearsVR.CoverBodyPrototype.%s.%lu",action,GetCurrentProcessId());SetLastError(0);
        handle=CreateEventW(nullptr,FALSE,FALSE,name);const auto error=GetLastError();return handle&&error!=ERROR_ALREADY_EXISTS;};
    if((source==TargetSource::advance&&!event(advance_event,L"Advance"))||!event(reset_event,L"Reset")){if(advance_event)CloseHandle(advance_event);if(reset_event)CloseHandle(reset_event);advance_event=reset_event=nullptr;return false;}
    pose_profile=profile;target_source=source;side_mirror_trial=mirror_trial;live_state={};enabled=true;
    // Preserve the legacy startup mirror_written=0 receipt: configuration has
    // made no game-memory write. Later node receipts report actual installs.
    fprintf(file,"cover_body_config enabled=1 experimental=1 pose_profile=%s endpoint_child=%u target_source=%s live_ttl_ms=100 reflected_lookup=actual_receiver bounded_nodes=4 cover_type=2 special_move=0 positive_incoming_only=1 advance_timeout_ms=10000 stop_drain_ms=5000 native_post_weight_separation=1 mirror_written=0 mirror_write_opt_in=%u side_mirror_trial=%u mirror_fixed_child5_toggle=1 mirror_single_activation=1 mirror_cache_gate=1 mirror_native_switch_certified=0 mirror_pose_concurrency_verified=0 side_direction_acceptance=0 two_dimensional_blend=0 firing_acceptance=0\n",profile_name(pose_profile),endpoint_for(pose_profile),source_name(target_source),side_mirror_trial?1:0,side_mirror_trial?1:0);return true;
}
void request_drain(){if(!enabled)return;std::lock_guard<std::mutex> lock(mutex);stopping=true;ULONGLONG empty{};drain_tick.compare_exchange_strong(empty,GetTickCount64());}
unsigned pending_count(){return pending.load()+in_flight.load()+transactions.load();}
bool draining(){return enabled&&pending_count()&&GetTickCount64()-drain_tick.load()<5000;}
bool drain_failed(){return enabled&&stopping.load()&&drain_tick.load()&&pending_count()&&!draining();}
bool proof_failed(){return failed.load()||external_change.load();}
void progress(FILE* file){fprintf(file,"cover_body_enabled=%u cover_body_pose_profile=%s cover_body_endpoint_child=%u cover_body_target_source=%s cover_body_live_published=%llu cover_body_live_rejected=%llu cover_body_live_expired=%llu cover_body_live_invalidated=%llu cover_body_live_frame=%llu cover_body_live_u=%.9g cover_body_live_reject=%u cover_body_layout_ready=%u cover_body_pending=%u cover_body_pending_fields=%u cover_body_pending_nodes=%u cover_body_in_flight=%u cover_body_transactions=%u cover_body_stopping=%u cover_body_failed=%u cover_body_step=%u ",enabled?1:0,profile_name(pose_profile),endpoint_for(pose_profile),source_name(target_source),live_published.load(),live_rejected.load(),live_expired.load(),live_invalidated.load(),static_cast<unsigned long long>(live_last_frame.load()),double(live_last_u.load()),unsigned(live_last_reject.load()),layout_ready.load()?1:0,pending_count(),pending.load(),pending_nodes.load(),in_flight.load(),transactions.load(),stopping.load()?1:0,failed.load()?1:0,step.load());}
void print(FILE* file,bool disabled){
    std::lock_guard<std::mutex> lock(mutex);progress(file);
    fprintf(file,"cover_body compiled=1 layout_ready=%u layout_attempts=%u layout_failure=%ls metadata_sources=%u metadata_identities=%u prewrite_rebinds=%llu blend_calls=%llu child_calls=%llu child_posts=%llu native_posts=%llu pose_rearms=%llu abandoned=%llu reentries=%llu rejected=%llu writes=%llu restores=%llu retired_fields=%llu external_changes=%llu timeout=%u rows=%u dropped=%llu native_post_weight_separation=1 firing_acceptance=0 gpu_or_exposure_acceptance=0\n",
        layout_ready?1:0,layout_attempts,layout_failure,layout.source_count,layout.id_count,prewrite_rebinds,blend_calls,child_calls,child_posts,native_posts,pose_rearms,abandoned,reentries,rejected,writes,restores,retired,external,timed_out.load()?1:0,row_count,rows_dropped);
    for(unsigned i=0;i<journals.size();++i){auto const& j=journals[i];if(j.used){unsigned native_fields{};for(unsigned n=0;n<child_limit;++n)if(j.owned[n]&&j.native_visible[n])++native_fields;
        fprintf(file,"cover_body_node index=%u node=%p pawn=%p mesh=%p world=%p generation=%llu endpoint_child=%u target_u=%.9g target_source=%s target_frame=%llu target_qpc=%lld injections=%u writes=%u restores=%u retired=%u pending=%u round_open=%u propagating=%u native_visible_fields=%u\n",i,reinterpret_cast<void*>(j.id.address),reinterpret_cast<void*>(j.pawn),reinterpret_cast<void*>(j.mesh),reinterpret_cast<void*>(j.world),j.generation,j.endpoint,double(j.target_u),source_name(j.source),static_cast<unsigned long long>(j.target.frame),j.target.timestamp_qpc,j.injections,j.write_count,j.restore_count,j.retire_count,any(j)?1:0,j.round_open?1:0,j.propagating?1:0,native_fields);
        if(side_mirror_trial){auto const& m=j.mirror;
            fprintf(file,"cover_body_mirror_trial node_index=%u experimental=1 native_switch_certified=0 captured=%u word_offset=0x%x mask=0x%08x original_word=0x%08x native_bits=0x%08x trial_bits=0x%08x initial_cache_seen=%u initial_node_cache_tag=%u initial_mesh_cache_tag=%u cache_read_ok=%u node_cache_tag=%u mesh_cache_tag=%u neutral_pending=%u neutral_verified=%u neutral_epoch=%u round_epoch=%u activation_epoch=%u pose_epoch=%u started=%u completed=%u pending=%u native_visible=%u sticky_conflict=%u reason=%u samples=%u rejected=%u installs=%u restores=%u\n",
                i,m.captured?1:0,m.offset,m.mask,m.original_word,m.native_bits,m.installed_bits,m.initial_cache_seen?1:0,m.initial_tags.node,m.initial_tags.mesh,m.cache_read_ok?1:0,m.last_tags.node,m.last_tags.mesh,m.neutral_pending?1:0,m.neutral_verified?1:0,m.neutral_epoch,m.round_epoch,m.activation_epoch,m.pose_epoch,m.started?1:0,m.completed?1:0,m.owned?1:0,m.native_visible?1:0,m.conflict?1:0,unsigned(m.reason),m.samples,m.rejected,m.installs,m.restores);
        }}}
    for(unsigned i=0;i<journals.size();++i){auto const& j=journals[i];if(j.used)for(unsigned s=0;s<steps.size();++s)
        fprintf(file,"cover_body_step node_index=%u step=%u u=%.9g u_is_advance_only=1 diagnostic_bucket=%u injections=%u writes=%u restores=%u\n",i,s,steps[s],j.source==TargetSource::live?1:0,j.step_injections[s],j.step_writes[s],j.step_restores[s]);}
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];fprintf(file,"cover_body_row index=%u qpc=%lld node=%p world=%llu thread=%u kind=%u step=%u u=%.9g child=%d incoming=%.9g weight=%.9g child_total=%.9g predicted=%.9g target_source=%s target_frame=%llu target_qpc=%lld total_sample_is_after_previous_native_call=%u native_post=%u pose_rearm=%u\n",i,r.qpc,reinterpret_cast<void*>(r.node),r.world,r.thread,r.kind,r.step,r.u,r.child,r.incoming,r.weight,r.total,r.predicted,source_name(r.source),static_cast<unsigned long long>(r.target_frame),r.target_qpc,r.kind==4?1:0,r.kind==5?1:0,r.kind==6?1:0);}
    if(disabled&&!pending_count()){if(advance_event){CloseHandle(advance_event);advance_event=nullptr;}if(reset_event){CloseHandle(reset_event);reset_event=nullptr;}}
}
#else
constexpr bool enabled=false;
bool live_source(){return false;}
bool neutral_pose_for(uintptr_t,uintptr_t){return true;}
bool publish_target(LiveTarget const&){return false;}
bool reserve(){return true;}
bool reserved_target(void*){return false;}
bool configure(const wchar_t* ini,FILE*){wchar_t profile[2]{};return GetPrivateProfileIntW(L"CoverBodyPrototype",L"enabled",0,ini)==0&&
    GetPrivateProfileStringW(L"CoverBodyPrototype",L"pose_profile",L"",profile,2,ini)==0&&
    GetPrivateProfileStringW(L"CoverBodyPrototype",L"target_source",L"",profile,2,ini)==0&&
    GetPrivateProfileStringW(L"CoverBodyPrototype",L"side_mirror_trial",L"",profile,2,ini)==0;}
void request_drain(){}
bool draining(){return false;}
bool drain_failed(){return false;}
bool proof_failed(){return false;}
unsigned pending_count(){return 0;}
void progress(FILE*){}
void print(FILE*,bool){}
#endif
}
