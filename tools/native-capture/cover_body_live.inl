// Explicit experimental body-pose writer. Included after Scope/safe_copy.
// Retail metadata is discovered only along an authenticated hook receiver and
// the current controlled Pawn's class chains. No registry-wide object scan.
namespace cover_body_live {
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
bool own_entries(); // Defined after the existing immutable hook-entry manifest.
HANDLE advance_event{},reset_event{};
gears_anchor::Config metadata{}; // Immutable startup certificate, not a mutable world pointer.
uint64_t initial_world{};
std::mutex mutex;
std::atomic<unsigned> pending{},pending_nodes{},game_thread{},step{};
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
    layout.children=children.offset;layout.anim=anim.offset;layout.weight=weight.offset;layout.total=total.offset;layout.skel=skel.offset;
    layout.owner=owner.offset;layout.mesh=mesh.offset;layout.cover=cover.offset;layout.special=special.offset;layout.world=world.offset;layout_failure=L"none";return true;
}
bool metadata_current(bool membership=true){
    for(unsigned i=0;i<layout.id_count;++i){gears_anchor::Identity id{};if(!identity(layout.ids[i].address,id)||!same(id,layout.ids[i]))return false;}
    for(unsigned i=0;i<layout.source_count;++i){auto const& s=layout.sources[i];if(s.membership&&!membership)continue;std::array<unsigned char,8> bytes{};
        if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(s.address),s.size)||memcmp(bytes.data(),s.bytes.data(),s.size))return false;}
    return true;
}
struct Children {uintptr_t data{};int count{},capacity{};};
static_assert(sizeof(Children)==16);
bool children_at(uintptr_t node,Children& children){return field(children,node,layout.children)&&children.data&&!(children.data&7)&&children.count>=5&&children.count<=int(child_limit)&&
    children.capacity>=children.count&&children.capacity<=64&&range(children.data,size_t(children.capacity)*0x44);}
bool equal(Children const& a,Children const& b){return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
struct Journal {
    bool used{};gears_anchor::Identity id{};uintptr_t pawn{},mesh{},world{};uint64_t generation{};Children children{};
    std::array<uint32_t,child_limit> native{},installed{};std::array<bool,child_limit> owned{};
    unsigned injections{},write_count{},restore_count{},retire_count{};
    unsigned installed_step{};
    std::array<std::array<unsigned,5>,7> logged{};
    std::array<unsigned,7> step_injections{},step_writes{},step_restores{};
    int previous_child=-1;float previous_incoming{},previous_weight{};
};
std::array<Journal,node_limit> journals{};
struct Row {long long qpc{};uintptr_t node{};uint64_t world{};unsigned thread{},kind{},step{};int child{};float u{},incoming{},weight{},total{},predicted{};};
std::array<Row,row_limit> rows{};unsigned row_count{};
std::array<unsigned,7> step_rows{}; // Reserve room for every later Advance step.
void row(unsigned kind_value,Journal& j,int child=-1,float incoming=0,float weight=0,float total=0){
    const unsigned sample_step=j.installed_step;
    if((kind_value==0||kind_value==4)&&child!=0&&child!=4)return;
    auto& logged=j.logged[sample_step][kind_value];const unsigned limit=(kind_value==0||kind_value==4)?2:1;
    if(logged>=limit)return;++logged;
    if(step_rows[sample_step]>=18||row_count==rows.size()){++rows_dropped;return;}++step_rows[sample_step];
    rows[row_count++]={now(),j.id.address,j.generation,GetCurrentThreadId(),kind_value,sample_step,child,steps[sample_step],incoming,weight,total,incoming*weight};
}
void recount(){unsigned fields{},nodes{};for(auto const& j:journals){unsigned n{};for(bool b:j.owned)if(b)++n;fields+=n;if(n)++nodes;}pending=fields;pending_nodes=nodes;}
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
bool any(Journal const& j){for(bool value:j.owned)if(value)return true;return false;}
void retire(Journal& j){for(auto& owned:j.owned)if(owned){owned=false;++retired;++j.retire_count;}j.previous_child=-1;row(3,j);}
void propagation(Journal& j){
    if(j.previous_child<0)return;Children current{};float total{};
    if(children_at(j.id.address,current)&&equal(current,j.children)&&field(total,j.children.data+uintptr_t(j.previous_child)*0x44,layout.total))
        row(4,j,j.previous_child,j.previous_incoming,j.previous_weight,total);
    j.previous_child=-1;
}
void restore_journals(uintptr_t self,bool all){
    for(auto& j:journals){if(!j.used||!any(j)||(!all&&j.id.address!=self))continue;
        gears_anchor::Registry registry{};uintptr_t slot{},pawn{},owner{},mesh{},world{},table{};
        if(!read(registry,metadata.objects_header)||!cohort(registry,registry)||j.id.index>=unsigned(registry.count)||!read(slot,registry.data+uintptr_t(j.id.index)*8)){stopping=true;continue;}
        // Only a positive loss/change of this registered object identity can
        // retire ownership. A different world, possession, link or storage
        // header does not prove the old object/allocation has ceased to live.
        if(slot!=j.id.address){retire(j);continue;}
        gears_anchor::Identity id{};if(!identity(j.id.address,id)){stopping=true;continue;}if(!same(id,j.id)){retire(j);continue;}
        if(j.generation!=committed_world_generation.load()){stopping=true;continue;}
        if(!field(pawn,metadata.identities[metadata.role_controller].address,metadata.controller_pawn)||!field(owner,j.id.address,layout.owner)||
           !field(mesh,j.id.address,layout.skel)||!field(world,j.pawn,layout.world)||!read(table,j.id.address)){stopping=true;continue;}
        if(pawn!=j.pawn||owner!=j.pawn||mesh!=j.mesh||world!=j.world||table!=base+0x13d0080){stopping=true;continue;}
        Children current{};if(!field(current,j.id.address,layout.children)||!equal(current,j.children)){stopping=true;continue;}
        if(!metadata_current(false)||!own_entries()){stopping=true;continue;}
        gears_anchor::PawnBinding binding{};if(!gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)||binding.unpossessed||binding.cinematic||binding.pawn.address!=j.pawn){stopping=true;continue;}
        propagation(j);
        for(unsigned i=0;i<unsigned(j.children.count);++i)if(j.owned[i]){
            uint32_t observed{};const auto address=j.children.data+uintptr_t(i)*0x44+layout.weight;
            if(!exchange(address,j.installed[i],j.native[i],observed)){failed=true;stopping=true;continue;}
            j.owned[i]=false;if(observed==j.installed[i]){++restores;++j.restore_count;++j.step_restores[j.installed_step];}else{++external;external_change=true;stopping=true;}
        }
        row(2,j);
    }
    recount();
}
void restore_all(){restore_journals(0,true);}
void restore_matching(uintptr_t self){restore_journals(self,false);}
struct Tick {uintptr_t node{},pawn{},mesh{},world{};gears_anchor::Identity id{};Children children{};uint64_t generation{};bool injected{};};
thread_local Tick tick{};
void events(){
    const auto clock=GetTickCount64();
    if(reset_event&&WaitForSingleObject(reset_event,0)==WAIT_OBJECT_0){injecting=false;withdrawing=true;next_step=0;step=0;}
    if(stopping.load()){injecting=false;return;}
    // Preserve a queued Advance until every old owned field has been released.
    if(!withdrawing&&advance_event&&WaitForSingleObject(advance_event,0)==WAIT_OBJECT_0){step=next_step;next_step=(next_step+1)%unsigned(steps.size());last_advance=clock;injecting=true;timed_out=false;}
    if(injecting&&clock-last_advance>=10000){injecting=false;withdrawing=true;timed_out=true;}
}
// Call only on the already-bound native tick thread. Normal entries release
// their own previous injection; only an explicit withdrawal may release peers.
void service_entry(uintptr_t self){
    events();
    if(stopping.load()||withdrawing){
        restore_all();tick={};
        if(!pending.load())withdrawing=false;
    }else{
        restore_matching(self);
        if(tick.node==self)tick={};
    }
}
void blend_before(uintptr_t self){
    if(!enabled)return;
    Scope scope;std::lock_guard<std::mutex> lock(mutex);++blend_calls;
    const unsigned thread=GetCurrentThreadId();if(game_thread.load()&&game_thread.load()!=thread){++rejected;return;}
    const bool bound=game_thread.load()!=0;
    // Restoration precedes this receiver's qualification. An unrelated NPC
    // must neither restore another node's weights nor clear its TLS context.
    if(bound)service_entry(self);
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
       !children_at(self,children)||!children_at(self,after)||!equal(children,after)){++rejected;return;}
    if(!game_thread.load()){game_thread=thread;service_entry(self);}
    tick={self,binding.pawn.address,mesh,world,id,children,initial_world,false};
}
void child_before(uintptr_t self,int child,float incoming){
    if(!enabled||tick.node!=self)return;
    Scope scope;std::lock_guard<std::mutex> lock(mutex);++child_calls;
    if(!enabled||game_thread.load()!=GetCurrentThreadId()||tick.node!=self||tick.generation!=committed_world_generation.load()||
       !scope.world()||!recording.load()||!std::isfinite(incoming)||incoming<=0||incoming>2)return;
    Journal* journal{};for(auto& j:journals)if(j.used&&same(j.id,tick.id)){journal=&j;break;}
    events();
    if(stopping.load()||withdrawing){restore_all();tick={};if(!pending.load())withdrawing=false;return;}
    if(!injecting||external_change)return;
    Children current{};uintptr_t owner{},mesh{},world{},pawn_mesh{};unsigned char ct{},sm{};gears_anchor::PawnBinding binding{};
    if(!own_entries()||(!tick.injected&&!metadata_current(false))||!gears_anchor::capture_pawn_binding(metadata,safe_copy,binding)||binding.cinematic||binding.unpossessed||binding.pawn.address!=tick.pawn||
       !field(owner,self,layout.owner)||owner!=tick.pawn||!field(mesh,self,layout.skel)||mesh!=tick.mesh||!field(pawn_mesh,tick.pawn,layout.mesh)||pawn_mesh!=tick.mesh||
       !field(world,tick.pawn,layout.world)||world!=tick.world||!field(ct,tick.pawn,layout.cover)||ct!=2||!field(sm,tick.pawn,layout.special)||sm!=0||
       !children_at(self,current)||!equal(current,tick.children)){++rejected;return;}
    if(child<0||child>=current.count)return;
    if(journal)propagation(*journal);
    if(!tick.injected){
        unsigned first=child_limit;std::array<uint32_t,child_limit> native{};std::array<uintptr_t,child_limit> animations{};
        for(unsigned i=0;i<unsigned(current.count);++i){float weight{};
            if(!field(animations[i],current.data+uintptr_t(i)*0x44,layout.anim)||!field(weight,current.data+uintptr_t(i)*0x44,layout.weight)||!std::isfinite(weight)||weight<0||weight>1)return;
            if(animations[i]&&first==child_limit)first=i;memcpy(&native[i],&weight,4);}
        if(first!=unsigned(child)||!animations[0]||!animations[4])return;
        if(!journal){for(auto& j:journals)if(!j.used){journal=&j;j.used=true;j.id=tick.id;break;}}
        if(!journal||any(*journal)){++rejected;return;}
        auto& j=*journal;j.pawn=tick.pawn;j.mesh=tick.mesh;j.world=tick.world;j.generation=tick.generation;j.children=current;j.native=native;j.installed_step=step.load();
        for(unsigned i=0;i<unsigned(current.count);++i){float weight=i==0?1.f-steps[step.load()]:(i==4?steps[step.load()]:0.f);memcpy(&j.installed[i],&weight,4);}
        // A final header/identity bracket before the first field CAS.
        gears_anchor::Identity again{};Children after{};
        if(!identity(self,again,true)||!same(again,j.id)||!children_at(self,after)||!equal(current,after))return;
        for(unsigned i=0;i<unsigned(current.count);++i)if(j.native[i]!=j.installed[i]){
            uint32_t observed{};if(!exchange(current.data+uintptr_t(i)*0x44+layout.weight,j.native[i],j.installed[i],observed)){failed=true;stopping=true;break;}
            if(observed!=j.native[i]){external_change=true;++external;stopping=true;break;}j.owned[i]=true;++writes;++j.write_count;++j.step_writes[j.installed_step];
        }
        tick.injected=true;++j.injections;++j.step_injections[j.installed_step];recount();row(1,j);
        if(stopping.load()){restore_all();return;}
    }
    if(journal){float weight{},total{};if(field(weight,current.data+uintptr_t(child)*0x44,layout.weight)&&field(total,current.data+uintptr_t(child)*0x44,layout.total)){
        row(0,*journal,child,incoming,weight,total);journal->previous_child=child;journal->previous_incoming=incoming;journal->previous_weight=weight;}}
}
extern "C" void GearsCoverBlendBefore(uintptr_t self,float,float){try{blend_before(self);}catch(...){failed=true;stopping=true;}}
extern "C" void GearsCoverChildBefore(uintptr_t self,int child,float incoming){
    if(!enabled||tick.node!=self)return;
    try{child_before(self,child,incoming);}catch(...){failed=true;stopping=true;}
}
bool reserve(){
    if(reserved)return false;
    for(unsigned i=0;i<2;++i){targets[i]=reinterpret_cast<void*>(base+rvas[i]);std::array<unsigned char,32> before{},after{};
        if(!safe_copy(before.data(),targets[i],before.size())||before!=signatures[i]||MH_CreateHook(targets[i],detours[i],originals[i])!=MH_OK||
           !safe_copy(after.data(),targets[i],after.size())||before!=after)return false;}
    reserved=true;return true;
}
bool reserved_target(void* target){return reserved&&(target==targets[0]||target==targets[1]);}
bool configure(const wchar_t* ini,FILE* file){
    const auto requested=GetPrivateProfileIntW(L"CoverBodyPrototype",L"enabled",0,ini);if(!requested)return true;
    if(requested!=1||enabled||!reserved||!continuous_session||!anchor_observer_enabled)return false;
    metadata=anchor_config;initial_world=committed_world_generation.load();
    if(metadata.object_internal_index!=8||metadata.object_name!=72||metadata.object_class!=80||metadata.object_outer!=64||metadata.field_super!=96)return false;
    wchar_t name[128]{};
    auto event=[&](HANDLE& handle,const wchar_t* action){swprintf_s(name,L"Local\\GearsVR.CoverBodyPrototype.%s.%lu",action,GetCurrentProcessId());SetLastError(0);
        handle=CreateEventW(nullptr,FALSE,FALSE,name);const auto error=GetLastError();return handle&&error!=ERROR_ALREADY_EXISTS;};
    if(!event(advance_event,L"Advance")||!event(reset_event,L"Reset")){if(advance_event)CloseHandle(advance_event);if(reset_event)CloseHandle(reset_event);advance_event=reset_event=nullptr;return false;}
    enabled=true;fprintf(file,"cover_body_config enabled=1 experimental=1 reflected_lookup=actual_receiver bounded_nodes=4 cover_type=2 special_move=0 positive_incoming_only=1 advance_timeout_ms=10000 stop_drain_ms=5000 half_weights_may_delay_transition_and_firing=1\n");return true;
}
void request_drain(){if(!enabled)return;std::lock_guard<std::mutex> lock(mutex);stopping=true;ULONGLONG empty{};drain_tick.compare_exchange_strong(empty,GetTickCount64());}
bool draining(){return enabled&&pending.load()&&GetTickCount64()-drain_tick.load()<5000;}
bool drain_failed(){return enabled&&stopping.load()&&drain_tick.load()&&pending.load()&&!draining();}
bool proof_failed(){return failed.load()||external_change.load();}
unsigned pending_count(){return pending.load();}
void progress(FILE* file){fprintf(file,"cover_body_enabled=%u cover_body_layout_ready=%u cover_body_pending=%u cover_body_pending_nodes=%u cover_body_stopping=%u cover_body_failed=%u cover_body_step=%u ",enabled?1:0,layout_ready.load()?1:0,pending.load(),pending_nodes.load(),stopping.load()?1:0,failed.load()?1:0,step.load());}
void print(FILE* file,bool disabled){
    std::lock_guard<std::mutex> lock(mutex);progress(file);
    fprintf(file,"cover_body compiled=1 layout_ready=%u layout_attempts=%u layout_failure=%ls metadata_sources=%u metadata_identities=%u prewrite_rebinds=%llu blend_calls=%llu child_calls=%llu rejected=%llu writes=%llu restores=%llu retired_fields=%llu external_changes=%llu timeout=%u rows=%u dropped=%llu half_weights_may_delay_transition_and_firing=1 gpu_or_exposure_acceptance=0\n",
        layout_ready?1:0,layout_attempts,layout_failure,layout.source_count,layout.id_count,prewrite_rebinds,blend_calls,child_calls,rejected,writes,restores,retired,external,timed_out.load()?1:0,row_count,rows_dropped);
    for(unsigned i=0;i<journals.size();++i){auto const& j=journals[i];if(j.used)fprintf(file,"cover_body_node index=%u node=%p pawn=%p mesh=%p world=%p generation=%llu injections=%u writes=%u restores=%u retired=%u pending=%u\n",i,reinterpret_cast<void*>(j.id.address),reinterpret_cast<void*>(j.pawn),reinterpret_cast<void*>(j.mesh),reinterpret_cast<void*>(j.world),j.generation,j.injections,j.write_count,j.restore_count,j.retire_count,any(j)?1:0);}
    for(unsigned i=0;i<journals.size();++i){auto const& j=journals[i];if(j.used)for(unsigned s=0;s<steps.size();++s)
        fprintf(file,"cover_body_step node_index=%u step=%u u=%.9g injections=%u writes=%u restores=%u\n",i,s,steps[s],j.step_injections[s],j.step_writes[s],j.step_restores[s]);}
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];fprintf(file,"cover_body_row index=%u qpc=%lld node=%p world=%llu thread=%u kind=%u step=%u u=%.9g child=%d incoming=%.9g weight=%.9g child_total=%.9g predicted=%.9g total_sample_is_after_previous_native_call=%u\n",i,r.qpc,reinterpret_cast<void*>(r.node),r.world,r.thread,r.kind,r.step,r.u,r.child,r.incoming,r.weight,r.total,r.predicted,r.kind==4?1:0);}
    if(disabled&&!pending.load()){if(advance_event){CloseHandle(advance_event);advance_event=nullptr;}if(reset_event){CloseHandle(reset_event);reset_event=nullptr;}}
}
#else
constexpr bool enabled=false;
bool reserve(){return true;}
bool reserved_target(void*){return false;}
bool configure(const wchar_t* ini,FILE*){return GetPrivateProfileIntW(L"CoverBodyPrototype",L"enabled",0,ini)==0;}
void request_drain(){}
bool draining(){return false;}
bool drain_failed(){return false;}
bool proof_failed(){return false;}
unsigned pending_count(){return 0;}
void progress(FILE*){}
void print(FILE*,bool){}
#endif
}
