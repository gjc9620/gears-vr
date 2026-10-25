// Read-only per-Scene identity binding. Requires the frozen anchor_config,
// safe_copy, recording and committed_world_generation, not any body layout.
// Caller owns stereo_mutex and its admitted Scene Scope/current Receipt.
// No CoverLink, crosshair weapon cache, controller frame, broker or game call.
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <mutex>
#include <type_traits>

namespace weapon_hand_scene_binding {
struct Binding {
    gears_anchor::Identity controller{},pawn{},mesh{},world{},weapon{};
    uint64_t world_generation{},scene_invocation{};unsigned char special_move{};
    bool weapon_known=false,context_valid=false,shared_actor_world=false;
    bool actor_or_mesh_fields_written=false,atomic_snapshot_verified=false;
};
// Identity key for two successful samples, including typed known-empty Weapon.
// Invocation and SpecialMove are per-Scene/policy inputs, not identity keys.
// Equality never replaces the next sample's current Scene/metadata proof.
bool same_key(Binding const& a,Binding const& b){return a.context_valid&&b.context_valid&&a.weapon_known&&b.weapon_known&&
    a.world_generation&&a.world_generation==b.world_generation&&gears_anchor::equal_identity(a.controller,b.controller)&&
    gears_anchor::equal_identity(a.pawn,b.pawn)&&gears_anchor::equal_identity(a.mesh,b.mesh)&&
    gears_anchor::equal_identity(a.world,b.world)&&gears_anchor::equal_identity(a.weapon,b.weapon);}
enum class Reason:unsigned {ready,body_unavailable,no_scene,wrong_thread,context_guard,context_invalid,
    layout_wait,layout_rejected,layout_changed,world,mesh,weapon,instigator,changed,exception};
const char* reason_name(Reason r){static constexpr const char* names[]={"ready","body_unavailable","no_scene","wrong_thread",
    "context_guard","context_invalid","layout_wait","layout_rejected","layout_changed","world","mesh","weapon","instigator","changed","exception"};
    const auto n=unsigned(r);return n<sizeof(names)/sizeof(*names)?names[n]:"unknown";}
Reason last_reason=Reason::body_unavailable;
const char* stage="not_sampled";
const wchar_t* rejected_field=L"none";
bool reject(Reason r){last_reason=r;return false;}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
gears_anchor::Config metadata{};
std::array<unsigned char,sizeof(gears_anchor::Config)> metadata_bytes{};
bool metadata_ready{};
uint64_t metadata_generation{};
std::atomic<unsigned> game_thread{}; // First SUCCESSFUL admitted Scene latches GT.
std::mutex mutex;
template<class T>bool read(T& out,uintptr_t address){return address&&address<=UINTPTR_MAX-sizeof(T)&&safe_copy(&out,reinterpret_cast<void const*>(address),sizeof(T));}
template<class T>bool field(T& out,uintptr_t address,unsigned offset){return address<=UINTPTR_MAX-offset&&read(out,address+offset);}
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
    auto text=[&](uintptr_t entry,std::array<wchar_t,64>& name){name.fill(0);if(entry>UINTPTR_MAX-metadata.name_entry_text)return false;
        for(unsigned i=0;i<63;++i){wchar_t c{};if(!field(c,entry+metadata.name_entry_text,i*2))return false;name[i]=c;if(!c)return i!=0;}return allow_long_prefix;};
    gears_anchor::Identity id{},again{};uintptr_t slot{},entry{},again_entry{};unsigned name_index{},again_name_index{};
    if(!capture(id)||!id.cls||id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||
       !read(slot,objects.data+uintptr_t(id.index)*8)||slot!=address||!read(entry,names.data+uintptr_t(id.name_index)*8)||
       !field(name_index,entry,metadata.name_entry_index)||name_index!=id.name_index||!text(entry,id.name)||
       (instance&&!wcsncmp(id.name.data(),L"Default__",9)))return false;
    if(!capture(again)||!text(entry,again.name)||!same(id,again)||!read(slot,objects.data+uintptr_t(id.index)*8)||slot!=address||
       !read(again_entry,names.data+uintptr_t(id.name_index)*8)||again_entry!=entry||!field(again_name_index,entry,metadata.name_entry_index)||again_name_index!=id.name_index||
       !read(after_objects,metadata.objects_header)||!read(after_names,metadata.names_header)||!cohort(objects,after_objects)||!cohort(names,after_names))return false;
    out=id;return true;
}
struct Source {uintptr_t address{};unsigned size{};std::array<unsigned char,8> bytes{};};
struct Property {gears_anchor::Identity id{},owner{},reference{};unsigned offset{},width{},dimension{};};
struct Proof {
    std::array<Source,4096> sources{};unsigned source_count{};
    std::array<gears_anchor::Identity,128> identities{};unsigned identity_count{};
    std::array<Property,5> fields{};std::array<gears_anchor::Identity,2> keys{};
    unsigned attempts{};uint64_t last_attempt{};bool attempted{},ready{};
} base_proof,instigator_proof;
bool synchronize_metadata(){
    static_assert(std::is_trivially_copyable_v<gears_anchor::Config>);
    gears_anchor::Config current_config{};memcpy(&current_config,&anchor_config,sizeof current_config);
    const auto generation=committed_world_generation.load();
    if(!generation||!current_config.objects_header||!current_config.names_header||!current_config.identity_count||
       current_config.identity_count>current_config.identities.size()||current_config.role_controller>=current_config.identity_count||
       current_config.role_pawn>=current_config.identity_count||current_config.role_camera>=current_config.identity_count||
       current_config.role_local>=current_config.identity_count||current_config.role_level>=current_config.identity_count||
       current_config.object_internal_index>92||current_config.object_name>88||current_config.object_class>88||current_config.object_outer>88||
       current_config.field_super>0x1000||current_config.name_entry_index>0x1000||current_config.name_entry_text>0x1000)return false;
    if(!metadata_ready||metadata_generation!=generation||memcmp(metadata_bytes.data(),&current_config,sizeof current_config)){
        memcpy(&metadata,&current_config,sizeof metadata);memcpy(metadata_bytes.data(),&current_config,sizeof current_config);
        metadata_generation=generation;metadata_ready=true;base_proof={};instigator_proof={};
    }
    return !memcmp(metadata_bytes.data(),&anchor_config,sizeof anchor_config)&&generation==committed_world_generation.load();
}
bool metadata_current(){return metadata_ready&&metadata_generation&&metadata_generation==committed_world_generation.load()&&
    !memcmp(metadata_bytes.data(),&anchor_config,sizeof anchor_config);}
uint64_t system_ms(){return GetTickCount64();}
uint64_t (*clock_ms)()=system_ms; // Private offline test clock; production uses system_ms.
bool remember(Proof& p,gears_anchor::Identity const& id){
    for(unsigned i=0;i<p.identity_count;++i)if(p.identities[i].address==id.address)return same(p.identities[i],id);
    if(p.identity_count==p.identities.size())return false;p.identities[p.identity_count++]=id;return true;
}
bool certified(Proof& p,uintptr_t address,gears_anchor::Identity& id){return identity(address,id)&&remember(p,id);}
bool kind(Proof& p,gears_anchor::Identity const& id,const wchar_t* name){gears_anchor::Identity meta{};return certified(p,id.cls,meta)&&named(meta,name);}
template<class T>bool source(Proof& p,T& value,uintptr_t address){
    static_assert(sizeof(T)<=8);T again{};if(!read(value,address)||!read(again,address)||memcmp(&value,&again,sizeof value))return false;
    for(unsigned i=0;i<p.source_count;++i)if(p.sources[i].address==address&&p.sources[i].size==sizeof value)return !memcmp(p.sources[i].bytes.data(),&value,sizeof value);
    if(p.source_count==p.sources.size())return false;auto& s=p.sources[p.source_count++];s.address=address;s.size=sizeof value;memcpy(s.bytes.data(),&value,sizeof value);return true;
}
template<class T>bool source_field(Proof& p,T& value,uintptr_t address,unsigned offset){return address<=UINTPTR_MAX-offset&&source(p,value,address+offset);}
bool property(Proof& p,uintptr_t start,const wchar_t* name,const wchar_t* property_kind,unsigned width,const wchar_t* reference_name,Property& out){
    rejected_field=name;std::array<uintptr_t,32> owners{};unsigned owner_count{};
    for(auto owner=start;owner&&owner_count<owners.size();){
        for(unsigned i=0;i<owner_count;++i)if(owners[i]==owner)return false;owners[owner_count++]=owner;
        gears_anchor::Identity owner_id{};unsigned extent{};uintptr_t child{};
        if(!certified(p,owner,owner_id)||!kind(p,owner_id,L"Class")||!source_field(p,extent,owner,0x88)||!extent||extent>0x10000||!source_field(p,child,owner,0x80))return false;
        std::array<uintptr_t,4096> visited{};unsigned count{},matches{};Property selected{};
        while(child&&count<visited.size()){
            for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
            gears_anchor::Identity id{};if(!identity(child,id,false,true)||id.outer!=owner)return false;
            if(!wcscmp(id.name.data(),name)){
                uintptr_t reference{};
                const bool pointer_kind=!wcscmp(property_kind,L"MeshPointer")?
                    (kind(p,id,L"ComponentProperty")||kind(p,id,L"ObjectProperty")):kind(p,id,property_kind);
                if(id.number||++matches!=1||!certified(p,child,selected.id)||!pointer_kind||
                   !source_field(p,selected.dimension,child,0x70)||!source_field(p,selected.width,child,0x74)||!source_field(p,selected.offset,child,0x94)||
                   selected.dimension!=1||selected.width!=width||extent<width||selected.offset>extent-width)return false;
                if(reference_name&&(!source_field(p,reference,child,0xd0)||!certified(p,reference,selected.reference)||
                   !kind(p,selected.reference,L"Class")||!named(selected.reference,reference_name)))return false;
                selected.owner=owner_id;
            }
            if(matches){if(!field(child,child,0x68))return false;}
            else if(!source_field(p,child,child,0x68))return false;
        }
        if(child)return false;if(matches){out=selected;return true;}
        if(!source_field(p,owner,owner,metadata.field_super))return false;
    }return false;
}
bool current(Proof const& p){
    for(unsigned i=0;i<p.identity_count;++i){gears_anchor::Identity id{};if(!identity(p.identities[i].address,id)||!same(id,p.identities[i]))return false;}
    for(unsigned i=0;i<p.source_count;++i){auto const& s=p.sources[i];std::array<unsigned char,8> bytes{};
        if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(s.address),s.size)||memcmp(bytes.data(),s.bytes.data(),s.size))return false;}
    return true;
}
void keys(Proof& p,gears_anchor::Identity const& first,gears_anchor::Identity const& second={}){
    if(!same(p.keys[0],first)||!same(p.keys[1],second)){p={};p.keys={first,second};}
}
bool begin_prepare(Proof& p){
    if(p.ready){if(current(p))return true;p.ready=false;return reject(Reason::layout_changed);}
    const auto clock=clock_ms();if(p.attempts>=8)return reject(Reason::layout_rejected);
    if(p.attempted&&(clock<p.last_attempt||clock-p.last_attempt<1000))return reject(Reason::layout_wait);
    p.attempted=true;p.last_attempt=clock;++p.attempts;p.source_count=p.identity_count=0;p.fields={};return true;
}
bool prepare_base(){
    auto& p=base_proof;if(!begin_prepare(p))return false;if(p.ready)return true;
    if(!property(p,p.keys[0].address,L"WorldInfo",L"ObjectProperty",8,L"WorldInfo",p.fields[0])||
       !property(p,p.keys[1].address,L"WorldInfo",L"ObjectProperty",8,L"WorldInfo",p.fields[1])||
       !property(p,p.keys[0].address,L"Weapon",L"ObjectProperty",8,L"Weapon",p.fields[2])||
       !property(p,p.keys[0].address,L"Mesh",L"MeshPointer",8,L"SkeletalMeshComponent",p.fields[3])||
       !property(p,p.keys[0].address,L"SpecialMove",L"ByteProperty",1,nullptr,p.fields[4])||
       !same(p.fields[0].reference,p.fields[1].reference)||!current(p))return reject(Reason::layout_rejected);
    p.ready=true;rejected_field=L"none";return true;
}
bool prepare_instigator(gears_anchor::Identity const& cls){
    auto& p=instigator_proof;keys(p,cls);if(!begin_prepare(p))return false;if(p.ready)return true;
    if(!property(p,cls.address,L"Instigator",L"ObjectProperty",8,L"Pawn",p.fields[0])||!current(p))return reject(Reason::layout_rejected);
    p.ready=true;rejected_field=L"none";return true;
}
struct ClassPath {std::array<gears_anchor::Identity,32> ids{};unsigned count{};};
bool derives(uintptr_t cls,const wchar_t* expected,uintptr_t reference,ClassPath& path){
    bool name_found=false,reference_found=!reference;path={};
    while(cls&&path.count<path.ids.size()){
        for(unsigned i=0;i<path.count;++i)if(path.ids[i].address==cls)return false;
        gears_anchor::Identity id{},meta{},again{};uintptr_t next{},check{};
        if(!identity(cls,id)||!identity(id.cls,meta)||!named(meta,L"Class")||
           !field(next,cls,metadata.field_super)||!field(check,cls,metadata.field_super)||next!=check||
           !identity(cls,again)||!same(id,again))return false;
        name_found=name_found||named(id,expected);reference_found=reference_found||cls==reference;
        id.super=next;id.check_super=1;path.ids[path.count++]=id;cls=next;
    }return !cls&&name_found&&reference_found;
}
bool same_path(ClassPath const& a,ClassPath const& b){if(a.count!=b.count)return false;
    for(unsigned i=0;i<a.count;++i)if(!same(a.ids[i],b.ids[i]))return false;return true;}
bool context_ready(){const auto gt=game_thread.load();return recording.load()&&(!gt||gt==GetCurrentThreadId())&&metadata_current();}
struct Context {
    gears_anchor::PawnBinding possession{};Binding binding{};gears_anchor::Registry objects{},names{};
    std::array<ClassPath,4> paths{};uintptr_t instigator{};
};
bool root(gears_anchor::Receipt const& scene,Context& c){
    stage="scene_possession";c={};c.binding.world_generation=committed_world_generation.load();c.binding.scene_invocation=scene.invocation;
    if(!c.binding.world_generation||c.binding.world_generation!=metadata_generation||
       !read(c.objects,metadata.objects_header)||!read(c.names,metadata.names_header)||
       !cohort(c.objects,c.objects)||!cohort(c.names,c.names)||
       !gears_anchor::capture_pawn_binding(metadata,safe_copy,c.possession)||c.possession.cinematic||c.possession.unpossessed||
       scene.anchor.cinematic||scene.anchor.unpossessed||!gears_anchor::scene_binding_matches(c.possession,scene.anchor)||
       !identity(c.possession.controller,c.binding.controller,true)||!identity(c.possession.pawn.address,c.binding.pawn,true)||
       !same(c.binding.pawn,c.possession.pawn)||!derives(c.binding.pawn.cls,L"WarPawn",0,c.paths[0]))return reject(Reason::context_invalid);
    return true;
}
bool values(Context& c){
    auto& result=c.binding;auto const& p=base_proof;uintptr_t pawn_world{},pc_world{},mesh{},weapon{};
    stage="current_world";
    if(!field(pawn_world,result.pawn.address,p.fields[0].offset)||!field(pc_world,result.controller.address,p.fields[1].offset)||
       !pawn_world||pawn_world!=pc_world||!identity(pawn_world,result.world,true)||
       !derives(result.world.cls,L"WorldInfo",p.fields[0].reference.address,c.paths[1]))return reject(Reason::world);
    stage="current_mesh";
    if(!field(mesh,result.pawn.address,p.fields[3].offset)||!identity(mesh,result.mesh,true)||
       !derives(result.mesh.cls,L"SkeletalMeshComponent",p.fields[3].reference.address,c.paths[2]))return reject(Reason::mesh);
    stage="current_weapon";
    if(!field(weapon,result.pawn.address,p.fields[2].offset)||!field(result.special_move,result.pawn.address,p.fields[4].offset))return reject(Reason::weapon);
    if(weapon){
        if(!identity(weapon,result.weapon,true)||!derives(result.weapon.cls,L"WarWeapon",p.fields[2].reference.address,c.paths[3]))return reject(Reason::weapon);
        gears_anchor::Identity cls{};if(!identity(result.weapon.cls,cls))return reject(Reason::weapon);if(!prepare_instigator(cls))return false;
        stage="current_instigator";ClassPath pawn_type{};
        if(!derives(result.pawn.cls,L"WarPawn",instigator_proof.fields[0].reference.address,pawn_type)||
           !field(c.instigator,weapon,instigator_proof.fields[0].offset)||c.instigator!=result.pawn.address)return reject(Reason::instigator);
    }
    result.weapon_known=result.context_valid=true;
    result.shared_actor_world=same(p.fields[0].id,p.fields[1].id)&&same(p.fields[0].owner,p.fields[1].owner)&&named(p.fields[0].owner,L"Actor");
    return true;
}
bool same_context(Context const& a,Context const& b){auto const& x=a.binding;auto const& y=b.binding;
    if(!gears_anchor::equal_binding(a.possession,b.possession)||!same(x.controller,y.controller)||!same(x.pawn,y.pawn)||
       !same(x.mesh,y.mesh)||!same(x.world,y.world)||!same(x.weapon,y.weapon)||
       x.world_generation!=y.world_generation||x.scene_invocation!=y.scene_invocation||x.special_move!=y.special_move||a.instigator!=b.instigator||
       !cohort(a.objects,b.objects)||!cohort(a.names,b.names))return false;
    for(unsigned i=0;i<a.paths.size();++i)if(!same_path(a.paths[i],b.paths[i]))return false;return true;
}
bool proofs_current(bool weapon){return current(base_proof)&&(!weapon||current(instigator_proof));}
// Existing certified fields only; no fabricated Receipt and no engine call.
bool current_actor(Binding const& expected){
    try{
        std::unique_lock<std::mutex> lock(mutex,std::try_to_lock);
        if(!lock.owns_lock()||!context_ready()||!expected.context_valid||!expected.weapon_known||
           expected.world_generation!=committed_world_generation.load()||!proofs_current(expected.weapon.address!=0))return false;
        auto check=[&](gears_anchor::PawnBinding& possession){
            gears_anchor::Identity pc{},pawn{},world{},mesh{},weapon{};uintptr_t pw{},cw{},m{},w{},instigator{};unsigned char sm{};
            if(!gears_anchor::capture_pawn_binding(metadata,safe_copy,possession)||possession.cinematic||possession.unpossessed||
               possession.controller!=expected.controller.address||!same(possession.pawn,expected.pawn)||
               !identity(possession.controller,pc,true)||!same(pc,expected.controller)||
               !identity(possession.pawn.address,pawn,true)||!same(pawn,expected.pawn))return false;
            auto const& p=base_proof;
            return field(pw,pawn.address,p.fields[0].offset)&&field(cw,pc.address,p.fields[1].offset)&&pw==cw&&pw==expected.world.address&&
                identity(pw,world,true)&&same(world,expected.world)&&field(m,pawn.address,p.fields[3].offset)&&m==expected.mesh.address&&
                identity(m,mesh,true)&&same(mesh,expected.mesh)&&field(w,pawn.address,p.fields[2].offset)&&w==expected.weapon.address&&
                (!w||(identity(w,weapon,true)&&same(weapon,expected.weapon)&&field(instigator,w,instigator_proof.fields[0].offset)&&instigator==pawn.address))&&
                field(sm,pawn.address,p.fields[4].offset)&&sm==expected.special_move;
        };
        gears_anchor::PawnBinding a{},b{};
        return check(a)&&check(b)&&gears_anchor::equal_binding(a,b)&&context_ready()&&proofs_current(expected.weapon.address!=0)&&
            expected.world_generation==committed_world_generation.load();
    }catch(...){return false;}
}
bool sample(gears_anchor::Receipt const* scene,Binding& out){
    try{
        std::lock_guard<std::mutex> lock(mutex);stage="scene_admission";
        if(!scene||!scene->invocation||scene->scene_qpc<=0||scene->family_qpc<scene->scene_qpc||scene->renderer_qpc<scene->family_qpc)return reject(Reason::no_scene);
        if(scene->thread!=GetCurrentThreadId()||(game_thread.load()&&game_thread.load()!=scene->thread))return reject(Reason::wrong_thread);
        const auto scene_time=scene->scene_qpc,family_time=scene->family_qpc,renderer_time=scene->renderer_qpc;
        if(!synchronize_metadata()||!context_ready())return reject(Reason::context_guard);
        Context first{},second{},last{};if(!root(*scene,first))return false;
        gears_anchor::Identity pawn_class{},controller_class{};
        if(!identity(first.binding.pawn.cls,pawn_class)||!identity(first.binding.controller.cls,controller_class))return reject(Reason::context_invalid);
        keys(base_proof,pawn_class,controller_class);stage="base_metadata";if(!prepare_base()||!values(first))return false;
        const auto nonempty=first.binding.weapon.address!=0;
        stage="closing_context";
        if(!context_ready()||!proofs_current(nonempty)||!root(*scene,second)||!values(second)||!same_context(first,second))return reject(Reason::changed);
        if(!context_ready()||!proofs_current(nonempty)||!root(*scene,last)||!values(last)||!same_context(first,last)||
           !proofs_current(nonempty)||!context_ready()||first.binding.world_generation!=committed_world_generation.load()||
           scene->invocation!=first.binding.scene_invocation||scene->thread!=GetCurrentThreadId()||
           scene->scene_qpc!=scene_time||scene->family_qpc!=family_time||scene->renderer_qpc!=renderer_time)return reject(Reason::changed);
        unsigned unbound{};if(!game_thread.compare_exchange_strong(unbound,GetCurrentThreadId())&&unbound!=GetCurrentThreadId())return reject(Reason::wrong_thread);
        out=first.binding;stage="ready";last_reason=Reason::ready;return true;
    }catch(...){return reject(Reason::exception);}
}
#else
bool sample(gears_anchor::Receipt const*,Binding&){return reject(Reason::body_unavailable);}
#endif
} // namespace weapon_hand_scene_binding
