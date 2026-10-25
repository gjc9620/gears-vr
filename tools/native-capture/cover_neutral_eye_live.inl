// Read-only publisher, included after cover_body_live. The caller owns its
// current one-use scene Receipt, stereo_mutex, Scope and scene game thread.
// This module neither selects a facing nor changes any camera / engine field.
#include "cover_bone_getters.h"
#include "../vr-common/pose_math.h"

namespace cover_neutral_eye_live {
enum class Reason {ready,body_unavailable,no_scene,wrong_thread,body_guard,binding,layout_wait,layout_rejected,
    layout_changed,link,not_low_cover,neutral_wait,attempt_limit,query_wait,head_name,query_rejected,
    changed,invalid_matrix,range,exception};
const char* reason_name(Reason r){static constexpr const char* names[]={"ready","body_unavailable","no_scene","wrong_thread","body_guard","binding","layout_wait","layout_rejected",
    "layout_changed","link","not_low_cover","neutral_wait","attempt_limit","query_wait","head_name","query_rejected","changed","invalid_matrix","range","exception"};
    const auto i=unsigned(r);return i<std::size(names)?names[i]:"unknown";}
struct EyeSample {
    gears_anchor::Identity pawn{},mesh{},link{};uintptr_t current_world{};uint64_t world_generation{};
    int slot=-1;unsigned char native_ct{},native_ca{},special_move{},last_cover_action{};
    uint32_t transition_word{},transition_mask{};bool transition{};
    gears_vr::Vector pawn_location{},neutral_eye_world{},neutral_offset{};
    uint64_t head_name{};int head_index=-1;uint64_t calibrated_ms{},scene_invocation{};
    // This receipt is a neutral position only, never view/body/write authority.
    bool mutation_authorized=false,view_activation_authorized=false;
};
Reason last_reason=Reason::body_unavailable;
const wchar_t* rejected_field=L"none";
const char* context_stage="not_sampled";
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace body=cover_body_live;
struct ProofSource {uintptr_t address{};unsigned size{};bool membership{};std::array<unsigned char,8> bytes{};};
struct Property {gears_anchor::Identity id{};unsigned dimension{},width{},offset{};};
enum WorldRole:unsigned {pawn_world,link_world,controller_world,world_role_count};
enum Field:unsigned {current_link,current_slot,cover_action,last_action,transition,field_count};
struct Spec {const wchar_t* name;const wchar_t* kind;unsigned width;};
constexpr std::array<Spec,field_count> specs{{{L"CurrentLink",L"ObjectProperty",8},{L"CurrentSlotIdx",L"IntProperty",4},
    {L"CoverAction",L"ByteProperty",1},{L"LastCoverActionAnim",L"ByteProperty",1},{L"bDoingCoverActionAnimTransition",L"BoolProperty",4}}};
struct Proof {
    std::array<ProofSource,4096> sources{};unsigned source_count{};
    std::array<gears_anchor::Identity,96> identities{};unsigned identity_count{};
    std::array<Property,field_count> fields{};gears_anchor::Identity pawn_class{},link_class{},controller_class{},world_class{};
    std::array<Property,world_role_count> world_fields{};std::array<gears_anchor::Identity,world_role_count> world_owners{};
    bool shared_actor_world{};
    unsigned mask{};bool ready{};unsigned attempts{};uint64_t last_attempt{};bool attempted{};
} proof;
bool remember(gears_anchor::Identity const& id){
    for(unsigned i=0;i<proof.identity_count;++i)if(proof.identities[i].address==id.address)return body::same(id,proof.identities[i]);
    if(proof.identity_count==proof.identities.size())return false;proof.identities[proof.identity_count++]=id;return true;
}
bool certified_id(uintptr_t address,gears_anchor::Identity& id){return body::identity(address,id)&&remember(id);}
bool kind(gears_anchor::Identity const& id,const wchar_t* type){gears_anchor::Identity cls{};return certified_id(id.cls,cls)&&body::named(cls,type);}
template<class T> bool source(T& value,uintptr_t address,bool membership=false){
    static_assert(sizeof(T)<=8);T again{};
    if(!body::read(value,address)||!body::read(again,address)||memcmp(&value,&again,sizeof(T)))return false;
    for(unsigned i=0;i<proof.source_count;++i)if(proof.sources[i].address==address&&proof.sources[i].size==sizeof(T))return !memcmp(proof.sources[i].bytes.data(),&value,sizeof(T));
    if(proof.source_count==proof.sources.size())return false;auto& s=proof.sources[proof.source_count++];
    s.address=address;s.size=sizeof(T);s.membership=membership;memcpy(s.bytes.data(),&value,sizeof(T));return true;
}
template<class T> bool source_field(T& value,uintptr_t address,unsigned offset,bool membership=false){
    return address<=UINTPTR_MAX-offset&&source(value,address+offset,membership);
}
bool typed_property(uintptr_t address,uintptr_t owner,unsigned owner_size,Spec const& spec,Property& out){
    Property p{};rejected_field=spec.name;
    if(!certified_id(address,p.id)||p.id.number||p.id.outer!=owner||wcscmp(p.id.name.data(),spec.name)||!kind(p.id,spec.kind)||
       !source_field(p.dimension,address,0x70)||!source_field(p.width,address,0x74)||!source_field(p.offset,address,0x94)||p.dimension!=1||p.width!=spec.width||
       owner_size>0x10000||p.width>owner_size||p.offset>owner_size-p.width)return false;
    out=p;return true;
}
bool property(uintptr_t address,uintptr_t owner,unsigned owner_size,unsigned n,Property& out){return typed_property(address,owner,owner_size,specs[n],out);}
// Each role certifies its own inheritance path and selected ObjectProperty.
// The retained sources/identities belong to this neutral Proof, never body Layout.
bool world_property(gears_anchor::Identity const& start,unsigned role){
    static constexpr Spec spec{L"WorldInfo",L"ObjectProperty",8};
    rejected_field=spec.name;std::array<uintptr_t,32> owners{};unsigned owner_count{};
    for(auto owner=start.address;owner&&owner_count<owners.size();){
        for(unsigned i=0;i<owner_count;++i)if(owners[i]==owner)return false;owners[owner_count++]=owner;
        gears_anchor::Identity owner_id{};unsigned owner_size{};
        if(!certified_id(owner,owner_id)||!kind(owner_id,L"Class")||!source_field(owner_size,owner,0x88)||!owner_size||owner_size>0x10000)return false;
        uintptr_t child{};if(!source_field(child,owner,0x80,true))return false;
        std::array<uintptr_t,4096> visited{};unsigned count{},matches{};Property selected{};
        while(child&&count<visited.size()){
            for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
            gears_anchor::Identity id{};if(!body::identity(child,id,false,true)||id.outer!=owner)return false;
            if(!wcscmp(id.name.data(),spec.name)){
                if(id.number||++matches!=1||!typed_property(child,owner,owner_size,spec,selected))return false;
            }
            if(matches){if(!body::field(child,child,0x68))return false;}
            else if(!source_field(child,child,0x68,true))return false;
        }
        if(child)return false;
        if(matches){
            uintptr_t reference{},world_super{};gears_anchor::Identity world_class{};
            if(!source_field(reference,selected.id.address,0xd0)||!certified_id(reference,world_class)||
               !kind(world_class,L"Class")||!body::named(world_class,L"WorldInfo")||!source_field(world_super,reference,body::metadata.field_super))return false;
            if(proof.world_class.address&&!body::same(proof.world_class,world_class))return false;
            proof.world_class=world_class;proof.world_fields[role]=selected;proof.world_owners[role]=owner_id;return true;
        }
        if(!source_field(owner,owner,body::metadata.field_super))return false;
    }return false;
}
// One bounded owner traversal discovers all five fields. Retain only the Next
// prefix needed to reach selected properties, as in body; later unrelated
// numbered/long names do not enlarge the hot proof or become selected fields.
bool prepare(gears_anchor::Identity const& pawn_class,gears_anchor::Identity const& controller_class){
    proof.ready=false;proof.source_count=proof.identity_count=0;proof.fields={};proof.world_fields={};proof.world_owners={};proof.world_class={};proof.shared_actor_world=false;proof.mask=0;proof.pawn_class=pawn_class;proof.controller_class=controller_class;
    unsigned found{};std::array<uintptr_t,32> owners{};unsigned owner_count{};
    for(auto owner=pawn_class.address;owner&&found<field_count&&owner_count<owners.size();){
        for(unsigned i=0;i<owner_count;++i)if(owners[i]==owner)return false;owners[owner_count++]=owner;
        gears_anchor::Identity owner_id{};unsigned owner_size{};
        if(!certified_id(owner,owner_id)||!kind(owner_id,L"Class")||!source_field(owner_size,owner,0x88)||!owner_size||owner_size>0x10000)return false;
        uintptr_t child{};if(!source_field(child,owner,0x80,true))return false;
        std::array<uintptr_t,4096> visited{};unsigned count{};std::array<unsigned,field_count> matches{};
        while(child&&count<visited.size()){
            for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
            gears_anchor::Identity id{};if(!body::identity(child,id,false,true)||id.outer!=owner)return false;
            for(unsigned n=0;n<field_count;++n)if(!wcscmp(id.name.data(),specs[n].name)){
                if(id.number||++matches[n]!=1||proof.fields[n].id.address||!property(child,owner,owner_size,n,proof.fields[n]))return false;
                ++found;
            }
            if(found<field_count){if(!source_field(child,child,0x68,true))return false;}
            else if(!body::field(child,child,0x68))return false;
        }
        if(child)return false;if(found==field_count)break;
        if(!source_field(owner,owner,body::metadata.field_super))return false;
    }
    if(found!=field_count){for(unsigned n=0;n<field_count;++n)if(!proof.fields[n].id.address){rejected_field=specs[n].name;break;}return false;}
    uintptr_t link_class{};rejected_field=specs[current_link].name;
    if(!source_field(link_class,proof.fields[current_link].id.address,0xd0)||!certified_id(link_class,proof.link_class)||
       !kind(proof.link_class,L"Class")||!body::named(proof.link_class,L"CoverLink"))return false;
    rejected_field=specs[transition].name;
    if(!source_field(proof.mask,proof.fields[transition].id.address,0xd0)||!proof.mask||(proof.mask&(proof.mask-1)))return false;
    if(!world_property(pawn_class,pawn_world)||!world_property(proof.link_class,link_world)||!world_property(controller_class,controller_world)||
       proof.world_fields[pawn_world].offset!=body::layout.world)return false;
    proof.shared_actor_world=body::named(proof.world_owners[pawn_world],L"Actor")&&
        body::same(proof.world_owners[pawn_world],proof.world_owners[link_world])&&body::same(proof.world_owners[pawn_world],proof.world_owners[controller_world])&&
        body::same(proof.world_fields[pawn_world].id,proof.world_fields[link_world].id)&&body::same(proof.world_fields[pawn_world].id,proof.world_fields[controller_world].id);
    rejected_field=L"none";proof.ready=true;return true;
}
bool proof_current(bool membership=true){
    for(unsigned i=0;i<proof.identity_count;++i){gears_anchor::Identity id{};
        if(!body::identity(proof.identities[i].address,id)||!body::same(id,proof.identities[i]))return false;}
    for(unsigned i=0;i<proof.source_count;++i){auto const& s=proof.sources[i];if(s.membership&&!membership)continue;
        std::array<unsigned char,8> value{};if(!safe_copy(value.data(),reinterpret_cast<void const*>(s.address),s.size)||memcmp(value.data(),s.bytes.data(),s.size))return false;}
    return true;
}
bool derives(uintptr_t cls,uintptr_t expected){
    std::array<uintptr_t,32> visited{};unsigned count{};
    while(cls&&count<visited.size()){
        for(unsigned i=0;i<count;++i)if(visited[i]==cls)return false;visited[count++]=cls;
        gears_anchor::Identity id{},meta{};if(!body::identity(cls,id)||!body::identity(id.cls,meta)||!body::named(meta,L"Class"))return false;
        if(cls==expected)return true;if(!body::field(cls,cls,body::metadata.field_super))return false;
    }return false;
}
bool skeletal_mesh(uintptr_t cls){
    for(unsigned count=0;cls&&count<32;++count){gears_anchor::Identity id{},meta{};
        if(!body::identity(cls,id)||!body::identity(id.cls,meta)||!body::named(meta,L"Class"))return false;
        if(body::named(id,L"SkeletalMeshComponent"))return true;if(!body::field(cls,cls,body::metadata.field_super))return false;
    }return false;
}
struct Context {
    gears_anchor::PawnBinding binding{};gears_anchor::Identity mesh{},link{},world{},world_type{};
    gears_anchor::Registry objects{},names{};uint64_t generation{};int slot=-1;unsigned char ct{},ca{},sm{},last{};
    uint32_t transition_word{};gears_vr::Vector location{};
};
bool same_key(Context const& a,Context const& b){return a.generation==b.generation&&gears_anchor::equal_binding(a.binding,b.binding)&&
    body::same(a.mesh,b.mesh)&&body::same(a.link,b.link)&&body::same(a.world,b.world)&&body::same(a.world_type,b.world_type)&&a.slot==b.slot;}
bool same_values(Context const& a,Context const& b){return same_key(a,b)&&a.ct==b.ct&&a.ca==b.ca&&a.sm==b.sm&&a.last==b.last&&
    ((a.transition_word^b.transition_word)&proof.mask)==0&&a.location.x==b.location.x&&a.location.y==b.location.y&&a.location.z==b.location.z&&
    body::cohort(a.objects,b.objects)&&body::cohort(a.names,b.names);}
struct Calibration {
    Context key{};bool have_key{},ready{},stable{},queried{};uint64_t neutral_since{},last_query{},calibrated_ms{};
    unsigned attempts{};gears_vr::Vector offset{};uint64_t head_name{};uintptr_t name_entry{};int head_index=-1;
} calibration;
bool reject(Reason reason,bool unstable=true){last_reason=reason;if(unstable)calibration.stable=false;return false;}
void clear_binding(){calibration.have_key=calibration.ready=calibration.stable=calibration.queried=false;calibration.attempts=0;}
bool body_ready(){return body::layout_ready.load()&&body::game_thread.load()==GetCurrentThreadId()&&
    recording.load()&&body::own_entries()&&!body::failed.load()&&!body::external_change.load()&&body::metadata_current(false);}
bool context(gears_anchor::Receipt const& scene,Context& out){
    Context c{};c.generation=committed_world_generation.load();uintptr_t mesh{},link{},world{},link_world_value{},controller_world_value{};std::array<float,3> location{};
    context_stage="world_generation";if(!c.generation||c.generation!=body::initial_world)return false;
    context_stage="registry";if(!body::read(c.objects,body::metadata.objects_header)||!body::read(c.names,body::metadata.names_header)||
       !body::cohort(c.objects,c.objects)||!body::cohort(c.names,c.names))return false;
    context_stage="scene_pawn_binding";if(!gears_anchor::capture_pawn_binding(body::metadata,safe_copy,c.binding)||
       c.binding.cinematic||c.binding.unpossessed||!gears_anchor::scene_binding_matches(c.binding,scene.anchor))return false;
    context_stage="mesh";if(!body::field(mesh,c.binding.pawn.address,body::layout.mesh)||!body::identity(mesh,c.mesh,true)||!skeletal_mesh(c.mesh.cls))return false;
    gears_anchor::Identity controller_id{},controller_class{};
    context_stage="controller_class";if(!body::identity(c.binding.controller,controller_id,true)||!body::identity(controller_id.cls,controller_class)||!body::same(controller_class,proof.controller_class))return false;
    context_stage="world_info";if(proof.world_fields[pawn_world].offset!=body::layout.world||!body::field(world,c.binding.pawn.address,proof.world_fields[pawn_world].offset)||
       !body::identity(world,c.world,true)||!body::identity(c.world.cls,c.world_type)||!derives(c.world.cls,proof.world_class.address))return false;
    context_stage="current_link";if(!body::field(link,c.binding.pawn.address,proof.fields[current_link].offset)||!body::identity(link,c.link,true)||
       !derives(c.link.cls,proof.link_class.address)||(!proof.shared_actor_world&&c.link.cls!=proof.link_class.address))return false;
    // Streaming CoverLinks and WorldInfo may belong to different Levels from
    // this Pawn. Their certified current world relation is the authority here.
    context_stage="link_world_info";if(!body::field(link_world_value,link,proof.world_fields[link_world].offset)||link_world_value!=world)return false;
    context_stage="controller_world_info";if(!body::field(controller_world_value,c.binding.controller,proof.world_fields[controller_world].offset)||controller_world_value!=world)return false;
    context_stage="current_slot";if(!body::field(c.slot,c.binding.pawn.address,proof.fields[current_slot].offset)||c.slot<0)return false;
    context_stage="typed_fields";if(!body::field(c.ct,c.binding.pawn.address,body::layout.cover)||!body::field(c.sm,c.binding.pawn.address,body::layout.special)||
       !body::field(c.ca,c.binding.pawn.address,proof.fields[cover_action].offset)||!body::field(c.last,c.binding.pawn.address,proof.fields[last_action].offset)||
       !body::field(c.transition_word,c.binding.pawn.address,proof.fields[transition].offset))return false;
    context_stage="pawn_location";if(!body::metadata.pawn_location||body::metadata.pawn_location>0x10000-sizeof(location)||!body::field(location,c.binding.pawn.address,body::metadata.pawn_location))return false;
    for(float v:location)if(!std::isfinite(v))return false;c.location={location[0],location[1],location[2]};
    gears_anchor::Registry objects{},names{};
    context_stage="closing_cohort";if(c.generation!=committed_world_generation.load()||!body::read(objects,body::metadata.objects_header)||!body::read(names,body::metadata.names_header)||
       !body::cohort(c.objects,objects)||!body::cohort(c.names,names))return false;
    context_stage="ready";out=c;return true;
}
bool neutral(Context const& c){
    if(c.ct!=2||c.sm||c.ca||c.last||(c.transition_word&proof.mask)||body::stopping.load()||body::rounds_open()||body::transactions.load())return false;
    if(!body::neutral_pose_for(c.binding.pawn.address,c.mesh.address))return false;
    const unsigned s=body::step.load();if(body::injecting&&(s>=body::steps.size()||body::steps[s]!=0))return false;
    // A released global input flag cannot make an old owned driven pose neutral.
    for(auto const& j:body::journals)if(j.used&&j.pawn==c.binding.pawn.address&&j.mesh==c.mesh.address&&body::any(j)){
        if(j.installed_step>=body::steps.size())return false;
        for(unsigned i=0;i<body::child_limit;++i)if(j.owned[i]&&!j.native_visible[i]&&body::steps[j.installed_step]!=0)return false;
    }return true;
}
bool name_current(Context const& c,uint64_t name,uintptr_t entry){
    if(!name||name>>32||unsigned(name)>=unsigned(c.names.count))return false;uintptr_t current{};unsigned index{};
    std::array<wchar_t,10> text{};
    return body::read(current,c.names.data+uintptr_t(unsigned(name))*8)&&current==entry&&
        body::field(index,entry,body::metadata.name_entry_index)&&index==unsigned(name)&&
        body::field(text,entry,body::metadata.name_entry_text)&&!memcmp(text.data(),L"b_MF_Head",sizeof(text));
}
bool find_head(Context const& c,uint64_t& name,uintptr_t& entry){
    // Bounded registry traversal, selected name only. Unrelated entries need no
    // full-name read or UObject scan. The dynamic Match resolves the bone index.
    if(c.names.count>262144)return false;
    for(unsigned n=1;n<unsigned(c.names.count);++n){uintptr_t candidate{};std::array<wchar_t,10> text{};
        if(!body::read(candidate,c.names.data+uintptr_t(n)*8))return false;
        if(!candidate||!body::field(text,candidate,body::metadata.name_entry_text)||memcmp(text.data(),L"b_MF_Head",sizeof(text)))continue;
        if(!name_current(c,n,candidate))return false;name=n;entry=candidate;return true;
    }return false;
}
struct NativeQuery {
    template<class Read,class Guard> bool operator()(uintptr_t module,uintptr_t mesh,uint64_t name,unsigned thread,Read read,Guard guard,gears_cover_bones::Result& out)const{
        return gears_cover_bones::query(module,mesh,name,thread,read,guard,out);}
};
template<class Clock,class Query> bool sample_with(gears_anchor::Receipt const* scene,EyeSample& out,Clock clock,Query query){
    try{
        std::lock_guard<std::mutex> lock(body::mutex);
        if(!scene||!scene->invocation||scene->scene_qpc<=0||scene->family_qpc<scene->scene_qpc||scene->renderer_qpc<scene->family_qpc)return reject(Reason::no_scene);
        if(scene->thread!=GetCurrentThreadId()||body::game_thread.load()!=scene->thread)return reject(Reason::wrong_thread);
        if(!body_ready())return reject(Reason::body_guard);
        gears_anchor::PawnBinding binding{};gears_anchor::Identity cls{},controller{},controller_class{};
        if(!gears_anchor::capture_pawn_binding(body::metadata,safe_copy,binding)||!gears_anchor::scene_binding_matches(binding,scene->anchor)||!body::identity(binding.pawn.cls,cls)||
           !body::identity(binding.controller,controller,true)||!body::identity(controller.cls,controller_class))return reject(Reason::binding);
        const uint64_t time=clock();
        if(!proof.pawn_class.address||!body::same(proof.pawn_class,cls)||!body::same(proof.controller_class,controller_class)){proof.ready=false;proof.attempts=0;proof.attempted=false;clear_binding();proof.pawn_class=cls;proof.controller_class=controller_class;}
        if(proof.ready&&!proof_current()){proof.ready=false;calibration.ready=false;return reject(Reason::layout_changed);}
        if(!proof.ready){
            if(proof.attempts>=8)return reject(Reason::layout_rejected);
            if(proof.attempted&&(time<proof.last_attempt||time-proof.last_attempt<1000))return reject(Reason::layout_wait);
            proof.last_attempt=time;proof.attempted=true;++proof.attempts;
            if(!prepare(cls,controller_class)||!proof_current(true))return reject(Reason::layout_rejected);
        }
        Context first{};if(!context(*scene,first)){calibration.ready=false;return reject(Reason::link);}
        if(!calibration.have_key||!same_key(calibration.key,first)){clear_binding();calibration.have_key=true;calibration.key=first;}
        if(first.ct!=2){calibration.ready=false;return reject(Reason::not_low_cover);}
        auto guard=[&](){Context current{};return body_ready()&&proof_current()&&context(*scene,current)&&same_values(first,current);};
        if(!guard())return reject(Reason::changed);
        if(!std::isfinite(stereo_units_per_metre)||stereo_units_per_metre<10||stereo_units_per_metre>500)return reject(Reason::range);
        if(!calibration.ready){
            if(!neutral(first))return reject(Reason::neutral_wait);
            if(!calibration.stable){calibration.stable=true;calibration.neutral_since=time;return reject(Reason::neutral_wait,false);}
            if(time<calibration.neutral_since||time-calibration.neutral_since<100)return reject(Reason::neutral_wait,false);
            if(calibration.attempts>=8)return reject(Reason::attempt_limit,false);
            if(calibration.queried&&(time<calibration.last_query||time-calibration.last_query<1000))return reject(Reason::query_wait,false);
            calibration.queried=true;calibration.last_query=time;++calibration.attempts;
            uint64_t name{};uintptr_t entry{};
            if(!find_head(first,name,entry))return reject(Reason::head_name,false);
            auto query_guard=[&](){return guard()&&neutral(first)&&name_current(first,name,entry);};
            gears_cover_bones::Result result{};
            if(!query(base,first.mesh.address,name,scene->thread,safe_copy,query_guard,result)||!query_guard())return reject(Reason::query_rejected);
            if(result.index<0||!gears_cover_bones::valid(result.world))return reject(Reason::invalid_matrix);
            gears_vr::Vector offset{result.world.values[12]-first.location.x,result.world.values[13]-first.location.y,result.world.values[14]-first.location.z};
            if(std::hypot(std::hypot(offset.x,offset.y),offset.z)>2*stereo_units_per_metre)return reject(Reason::range);
            calibration.offset=offset;calibration.head_name=name;calibration.name_entry=entry;calibration.head_index=result.index;calibration.calibrated_ms=time;calibration.ready=true;
        }
        if(!guard())return reject(Reason::changed);
        if(std::hypot(std::hypot(calibration.offset.x,calibration.offset.y),calibration.offset.z)>2*stereo_units_per_metre)return reject(Reason::range);
        EyeSample candidate{};candidate.pawn=first.binding.pawn;candidate.mesh=first.mesh;candidate.link=first.link;candidate.current_world=first.world.address;candidate.world_generation=first.generation;
        candidate.slot=first.slot;candidate.native_ct=first.ct;candidate.native_ca=first.ca;candidate.special_move=first.sm;candidate.last_cover_action=first.last;
        candidate.transition_word=first.transition_word;candidate.transition_mask=proof.mask;candidate.transition=(first.transition_word&proof.mask)!=0;
        candidate.pawn_location=first.location;candidate.neutral_offset=calibration.offset;candidate.neutral_eye_world={first.location.x+calibration.offset.x,first.location.y+calibration.offset.y,first.location.z+calibration.offset.z};
        candidate.head_name=calibration.head_name;candidate.head_index=calibration.head_index;candidate.calibrated_ms=calibration.calibrated_ms;candidate.scene_invocation=scene->invocation;
        out=candidate;last_reason=Reason::ready;return true;
    }catch(...){return reject(Reason::exception);}
}
bool sample(gears_anchor::Receipt const* scene,EyeSample& out){return sample_with(scene,out,[]{return GetTickCount64();},NativeQuery{});}
#else
bool sample(gears_anchor::Receipt const*,EyeSample&){last_reason=Reason::body_unavailable;return false;}
#endif
} // namespace cover_neutral_eye_live
