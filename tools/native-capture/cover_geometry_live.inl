// Read-only candidate geometry, included after cover_neutral_eye_live.inl.
// Caller owns stereo_mutex and the current admitted Scene GT Scope/Receipt.
// sample_with remains a CPU candidate transform. The separate explicit
// sample_native_with calls only the two certified RO slot getters. Include
// cover_surface_getters.h at GLOBAL scope before including this .inl.
// Neither path writes game memory, queries collision or admits a grab.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <type_traits>

namespace cover_geometry_live {
namespace neutral=cover_neutral_eye_live;
using EyeSample=neutral::EyeSample;
using Vector=std::array<float,3>;
using Rotator=std::array<int32_t,3>;
enum class Reason:unsigned {ready,body_unavailable,no_scene,nested_scene,wrong_thread,body_guard,neutral_proof,
    eye_binding,layout_wait,layout_rejected,layout_changed,read,world,slots,nonfinite,changed,exception,native_layout,native_getter};
const char* reason_name(Reason reason){static constexpr const char* names[]={"ready","body_unavailable","no_scene","nested_scene",
    "wrong_thread","body_guard","neutral_proof","eye_binding","layout_wait","layout_rejected","layout_changed","read","world","slots","nonfinite","changed","exception","native_layout","native_getter"};
    const auto n=unsigned(reason);return n<sizeof(names)/sizeof(*names)?names[n]:"unknown";}
Reason last_reason=Reason::body_unavailable;
const wchar_t* rejected_field=L"none";
const char* stage="not_sampled";
struct Array {uintptr_t data{};int count{},capacity{};};
static_assert(sizeof(Array)==16);
struct Boolean {uint32_t word{},mask{};bool value{};};
struct Slot {
    int index=-1;std::array<Boolean,4> permissions{}; // Enabled, LeanLeft, LeanRight, CanPopUp.
    unsigned char cover_type{};Vector location_offset{};Rotator rotation_offset{};
};
struct Candidates {
    gears_vr::Vector slot_world{},forward{},right{},up{};
    double mid_top_z{},stand_top_z{};
    // Top candidates add the raw configured height to candidate slot world Z.
    // Neither the height convention nor this transform has native/collision proof.
    bool candidate_only=true,collision_verified=false,native_getter_verified=false;
};
struct Geometry {
    EyeSample binding{};Vector link_location{},pawn_location{},standing_lean_offset{},crouch_lean_offset{};
    Rotator link_rotation{},pawn_rotation{};float mid_height{},stand_height{};
    Array slots{};unsigned slot_stride{};Slot selected{};Candidates candidates{};
    gears_vr::Vector native_slot_world{};Rotator native_slot_rotation{};
    unsigned metadata_sources{},metadata_identities{};
    bool candidate_only=true,collision_verified=false,native_getter_verified=false,atomic_snapshot_verified=false;
};
class SceneScope;
thread_local SceneScope* current_scene{};
class SceneScope {
    SceneScope* previous_{};gears_anchor::Receipt const* receipt_{};bool poisoned_{};
public:
    explicit SceneScope(gears_anchor::Receipt const* receipt):previous_(current_scene),receipt_(receipt){
        if(previous_){previous_->poisoned_=true;poisoned_=true;}current_scene=this;
    }
    ~SceneScope(){current_scene=previous_;}
    SceneScope(SceneScope const&)=delete;SceneScope& operator=(SceneScope const&)=delete;
    bool poisoned()const{return poisoned_;}
    gears_anchor::Receipt const* receipt()const{return poisoned_?nullptr:receipt_;}
};
bool reject(Reason reason){last_reason=reason;return false;}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
namespace body=cover_body_live;
struct Property {gears_anchor::Identity id{};unsigned dimension{},width{},offset{};};
struct Source {uintptr_t address{};unsigned size{};std::array<unsigned char,8> bytes{};};
struct Spec {const wchar_t* name;const wchar_t* kind;unsigned width;};
constexpr std::array<Spec,8> link_specs{{{L"Location",L"StructProperty",12},{L"Rotation",L"StructProperty",12},
    {L"MidHeight",L"FloatProperty",4},{L"StandHeight",L"FloatProperty",4},{L"StandingLeanOffset",L"StructProperty",12},
    {L"CrouchLeanOffset",L"StructProperty",12},{L"Slots",L"ArrayProperty",16},{L"WorldInfo",L"ObjectProperty",8}}};
constexpr std::array<Spec,3> pawn_specs{{{L"Location",L"StructProperty",12},{L"Rotation",L"StructProperty",12},{L"WorldInfo",L"ObjectProperty",8}}};
constexpr std::array<Spec,1> world_specs{{{L"WorldInfo",L"ObjectProperty",8}}};
constexpr std::array<Spec,7> slot_specs{{{L"bEnabled",L"BoolProperty",4},{L"bLeanLeft",L"BoolProperty",4},
    {L"bLeanRight",L"BoolProperty",4},{L"bCanPopUp",L"BoolProperty",4},{L"CoverType",L"ByteProperty",1},
    {L"LocationOffset",L"StructProperty",12},{L"RotationOffset",L"StructProperty",12}}};
constexpr std::array<Spec,3> vector_specs{{{L"X",L"FloatProperty",4},{L"Y",L"FloatProperty",4},{L"Z",L"FloatProperty",4}}};
constexpr std::array<Spec,3> rotation_specs{{{L"Pitch",L"IntProperty",4},{L"Yaw",L"IntProperty",4},{L"Roll",L"IntProperty",4}}};
struct Proof {
    std::array<Source,4096> sources{};unsigned source_count{};
    std::array<gears_anchor::Identity,192> identities{};unsigned identity_count{};
    std::array<Property,8> link{};std::array<Property,3> pawn{},vector_fields{},rotation_fields{};
    std::array<Property,1> controller_world{};std::array<Property,7> slot{};Property inner{};
    std::array<uint32_t,4> masks{};gears_anchor::Identity vector_type{},rotation_type{},slot_type{};
    gears_anchor::Identity pawn_class{},link_class{},controller_class{};
    unsigned stride{},attempts{};uint64_t last_attempt{};bool attempted{},ready{};
} proof;
// This module owns its complete metadata proof. It never swaps or mutates
// body::layout, and never calls body's property/source/remember helpers.
bool remember(gears_anchor::Identity const& id){
    for(unsigned i=0;i<proof.identity_count;++i)if(proof.identities[i].address==id.address)return body::same(id,proof.identities[i]);
    if(proof.identity_count==proof.identities.size())return false;proof.identities[proof.identity_count++]=id;return true;
}
bool certified_id(uintptr_t address,gears_anchor::Identity& id){return body::identity(address,id)&&remember(id);}
bool kind(gears_anchor::Identity const& id,const wchar_t* name){gears_anchor::Identity cls{};return certified_id(id.cls,cls)&&body::named(cls,name);}
template<class T> bool source(T& value,uintptr_t address){
    static_assert(sizeof(T)<=8);T second{};
    if(!body::read(value,address)||!body::read(second,address)||memcmp(&value,&second,sizeof(T)))return false;
    for(unsigned i=0;i<proof.source_count;++i)if(proof.sources[i].address==address&&proof.sources[i].size==sizeof(T))return !memcmp(proof.sources[i].bytes.data(),&value,sizeof(T));
    if(proof.source_count==proof.sources.size())return false;auto& s=proof.sources[proof.source_count++];s.address=address;s.size=sizeof(T);memcpy(s.bytes.data(),&value,sizeof(T));return true;
}
template<class T> bool source_field(T& value,uintptr_t address,unsigned offset){return address<=UINTPTR_MAX-offset&&source(value,address+offset);}
bool descriptor(uintptr_t address,uintptr_t owner,unsigned owner_size,Spec const& spec,Property& out){
    rejected_field=spec.name;Property p{};
    if(!certified_id(address,p.id)||p.id.number||p.id.outer!=owner||wcscmp(p.id.name.data(),spec.name)||!kind(p.id,spec.kind)||
       !source_field(p.dimension,address,0x70)||!source_field(p.width,address,0x74)||!source_field(p.offset,address,0x94)||
       p.dimension!=1||p.width!=spec.width||!owner_size||owner_size>0x10000||p.width>owner_size||p.offset>owner_size-p.width)return false;
    out=p;return true;
}
template<size_t N> bool fields(uintptr_t start,std::array<Spec,N> const& specs,std::array<Property,N>& out){
    unsigned found{};out={};std::array<uintptr_t,32> owners{};unsigned owner_count{};
    for(auto owner=start;owner&&found<N&&owner_count<owners.size();){
        for(unsigned i=0;i<owner_count;++i)if(owners[i]==owner)return false;owners[owner_count++]=owner;
        gears_anchor::Identity owner_id{};unsigned owner_size{};
        if(!certified_id(owner,owner_id))return false;const bool structure=kind(owner_id,L"ScriptStruct");
        if((!structure&&!kind(owner_id,L"Class"))||!source_field(owner_size,owner,0x88)||!owner_size||owner_size>0x10000)return false;
        uintptr_t child{};if(!source_field(child,owner,0x80))return false;
        std::array<uintptr_t,4096> visited{};unsigned count{};std::array<unsigned,N> matches{};
        while(child&&count<visited.size()){
            for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
            gears_anchor::Identity id{};if(!body::identity(child,id,false,true)||id.outer!=owner)return false;
            for(unsigned n=0;n<N;++n)if(!wcscmp(id.name.data(),specs[n].name)){
                if(id.number||++matches[n]!=1||out[n].id.address||!descriptor(child,owner,owner_size,specs[n],out[n]))return false;++found;
            }
            // Retain every link needed to reach a selected property; scan the
            // remainder for duplicate names without keeping unrelated tails.
            if(found<N){if(!source_field(child,child,0x68))return false;}
            else if(!body::field(child,child,0x68))return false;
        }
        if(child)return false;if(found==N)return true;
        if(structure||!source_field(owner,owner,body::metadata.field_super))return false;
    }
    for(unsigned n=0;n<N;++n)if(!out[n].id.address){rejected_field=specs[n].name;break;}return false;
}
bool structure(Property const& field,const wchar_t* name,unsigned size,gears_anchor::Identity& out){
    uintptr_t address{};unsigned width{};
    return source_field(address,field.id.address,0xd0)&&certified_id(address,out)&&kind(out,L"ScriptStruct")&&body::named(out,name)&&
        source_field(width,address,0x88)&&width==size;
}
bool shape(Property const& field,const wchar_t* name,gears_anchor::Identity const& expected){
    gears_anchor::Identity actual{};return structure(field,name,12,actual)&&body::same(actual,expected);
}
bool prepare(){
    proof.ready=false;proof.source_count=proof.identity_count=0;proof.masks={};
    if(!fields(proof.link_class.address,link_specs,proof.link)||!fields(proof.pawn_class.address,pawn_specs,proof.pawn)||
       !fields(proof.controller_class.address,world_specs,proof.controller_world)||
       !structure(proof.link[0],L"Vector",12,proof.vector_type)||!structure(proof.link[1],L"Rotator",12,proof.rotation_type)||
       !fields(proof.vector_type.address,vector_specs,proof.vector_fields)||!fields(proof.rotation_type.address,rotation_specs,proof.rotation_fields))return false;
    for(unsigned i=0;i<3;++i)if(proof.vector_fields[i].offset!=i*4||proof.rotation_fields[i].offset!=i*4)return false;
    for(auto const* p:{&proof.link[4],&proof.link[5],&proof.pawn[0]})if(!shape(*p,L"Vector",proof.vector_type))return false;
    if(!shape(proof.pawn[1],L"Rotator",proof.rotation_type))return false;
    uintptr_t inner{};
    if(!source_field(inner,proof.link[6].id.address,0xd0)||!certified_id(inner,proof.inner.id)||proof.inner.id.number||
       proof.inner.id.outer!=proof.link[6].id.address||!kind(proof.inner.id,L"StructProperty")||
       !source_field(proof.inner.dimension,inner,0x70)||!source_field(proof.inner.width,inner,0x74)||!source_field(proof.inner.offset,inner,0x94)||
       proof.inner.dimension!=1||proof.inner.width!=212||proof.inner.offset||!structure(proof.inner,L"CoverSlot",212,proof.slot_type)||
       !fields(proof.slot_type.address,slot_specs,proof.slot)||!shape(proof.slot[5],L"Vector",proof.vector_type)||
       !shape(proof.slot[6],L"Rotator",proof.rotation_type))return false;
    proof.stride=proof.inner.width;
    for(unsigned n=0;n<4;++n)if(!source_field(proof.masks[n],proof.slot[n].id.address,0xd0)||!proof.masks[n]||(proof.masks[n]&(proof.masks[n]-1)))return false;
    if(proof.pawn[0].offset!=body::metadata.pawn_location||proof.pawn[1].offset!=body::metadata.pawn_rotation||
       proof.pawn[2].offset!=body::layout.world)return false;
    proof.ready=true;rejected_field=L"none";return true;
}
bool proof_current(){
    for(unsigned i=0;i<proof.identity_count;++i){gears_anchor::Identity id{};if(!body::identity(proof.identities[i].address,id)||!body::same(id,proof.identities[i]))return false;}
    for(unsigned i=0;i<proof.source_count;++i){auto const& s=proof.sources[i];std::array<unsigned char,8> value{};
        if(!safe_copy(value.data(),reinterpret_cast<void const*>(s.address),s.size)||memcmp(value.data(),s.bytes.data(),s.size))return false;}
    return true;
}
template<class T> bool bytes_equal(T const& a,T const& b){static_assert(std::is_trivially_copyable_v<T>);return !memcmp(&a,&b,sizeof(T));}
bool equal_vector(gears_vr::Vector const& a,gears_vr::Vector const& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
bool matches(neutral::Context const& c,EyeSample const& eye){return body::same(c.binding.pawn,eye.pawn)&&body::same(c.mesh,eye.mesh)&&body::same(c.link,eye.link)&&
    c.world.address==eye.current_world&&c.generation==eye.world_generation&&c.slot==eye.slot&&c.ct==eye.native_ct&&c.ca==eye.native_ca&&c.sm==eye.special_move&&c.last==eye.last_cover_action&&
    eye.transition_mask==neutral::proof.mask&&((c.transition_word^eye.transition_word)&eye.transition_mask)==0&&eye.transition==((c.transition_word&eye.transition_mask)!=0)&&
    equal_vector(c.location,eye.pawn_location)&&!eye.mutation_authorized&&!eye.view_activation_authorized;}
bool calibration_matches(neutral::Context const& c,EyeSample const& eye){auto const& k=neutral::calibration;return k.ready&&k.have_key&&neutral::same_key(k.key,c)&&
    eye.calibrated_ms==k.calibrated_ms&&eye.head_name==k.head_name&&eye.head_index==k.head_index&&equal_vector(eye.neutral_offset,k.offset)&&
    equal_vector(eye.neutral_eye_world,{c.location.x+k.offset.x,c.location.y+k.offset.y,c.location.z+k.offset.z});}
struct Values {Vector location{},pawn_location{},standing{},crouch{},offset{};Rotator rotation{},pawn_rotation{},rotation_offset{};
    float mid{},stand{};Array slots{};std::array<uint32_t,4> words{};unsigned char cover_type{};};
bool values(neutral::Context const& c,Values& v){
    const auto link=c.link.address,pawn=c.binding.pawn.address;uintptr_t world{},pawn_world{},pc_world{};
    stage="world_references";
    if(!body::field(world,link,proof.link[7].offset)||!body::field(pawn_world,pawn,proof.pawn[2].offset)||
       !body::field(pc_world,c.binding.controller,proof.controller_world[0].offset)||world!=c.world.address||pawn_world!=world||pc_world!=world)return reject(Reason::world);
    stage="raw_transform_parameters";
    if(!body::field(v.location,link,proof.link[0].offset)||!body::field(v.rotation,link,proof.link[1].offset)||
       !body::field(v.mid,link,proof.link[2].offset)||!body::field(v.stand,link,proof.link[3].offset)||
       !body::field(v.standing,link,proof.link[4].offset)||!body::field(v.crouch,link,proof.link[5].offset)||
       !body::field(v.pawn_location,pawn,proof.pawn[0].offset)||!body::field(v.pawn_rotation,pawn,proof.pawn[1].offset))return reject(Reason::read);
    stage="slots_header";
    if(!body::field(v.slots,link,proof.link[6].offset)||!v.slots.data||(v.slots.data&3)||v.slots.count<=0||v.slots.count>256||
       v.slots.capacity<v.slots.count||v.slots.capacity>256||c.slot<0||c.slot>=v.slots.count||
       !body::range(v.slots.data,size_t(v.slots.capacity)*proof.stride))return reject(Reason::slots);
    const auto selected=v.slots.data+uintptr_t(c.slot)*proof.stride;stage="selected_slot_fields";
    for(unsigned n=0;n<4;++n)if(!body::field(v.words[n],selected,proof.slot[n].offset))return reject(Reason::read);
    if(!body::field(v.cover_type,selected,proof.slot[4].offset)||!body::field(v.offset,selected,proof.slot[5].offset)||
       !body::field(v.rotation_offset,selected,proof.slot[6].offset))return reject(Reason::read);
    for(auto const* a:{&v.location,&v.pawn_location,&v.standing,&v.crouch,&v.offset})for(float x:*a)if(!std::isfinite(x))return reject(Reason::nonfinite);
    if(!std::isfinite(v.mid)||!std::isfinite(v.stand))return reject(Reason::nonfinite);
    return true;
}
bool same_values(Values const& a,Values const& b){return bytes_equal(a.location,b.location)&&bytes_equal(a.pawn_location,b.pawn_location)&&
    bytes_equal(a.standing,b.standing)&&bytes_equal(a.crouch,b.crouch)&&bytes_equal(a.offset,b.offset)&&bytes_equal(a.rotation,b.rotation)&&
    bytes_equal(a.pawn_rotation,b.pawn_rotation)&&bytes_equal(a.rotation_offset,b.rotation_offset)&&bytes_equal(a.mid,b.mid)&&bytes_equal(a.stand,b.stand)&&
    a.slots.data==b.slots.data&&a.slots.count==b.slots.count&&a.slots.capacity==b.slots.capacity&&a.words==b.words&&a.cover_type==b.cover_type;}
using Axes=std::array<gears_vr::Vector,3>;
Axes axes(Rotator const& rotation){constexpr double tau=6.283185307179586476925286766559;const double p=rotation[0]*tau/65536.,y=rotation[1]*tau/65536.,r=rotation[2]*tau/65536.;
    const double cp=cos(p),sp=sin(p),cy=cos(y),sy=sin(y),cr=cos(r),sr=sin(r);
    return {{{cp*cy,cp*sy,sp},{sr*sp*cy-cr*sy,sr*sp*sy+cr*cy,-sr*cp},{-cr*sp*cy-sr*sy,sr*cy-cr*sp*sy,cr*cp}}};}
gears_vr::Vector apply(Axes const& a,gears_vr::Vector const& v){return {a[0].x*v.x+a[1].x*v.y+a[2].x*v.z,
    a[0].y*v.x+a[1].y*v.y+a[2].y*v.z,a[0].z*v.x+a[1].z*v.y+a[2].z*v.z};}
Candidates candidates(Values const& v){const auto a=axes(v.rotation),b=axes(v.rotation_offset);const auto offset=apply(a,{v.offset[0],v.offset[1],v.offset[2]});Candidates result{};
    result.slot_world={v.location[0]+offset.x,v.location[1]+offset.y,v.location[2]+offset.z};result.forward=apply(a,b[0]);result.right=apply(a,b[1]);result.up=apply(a,b[2]);
    result.mid_top_z=result.slot_world.z+v.mid;result.stand_top_z=result.slot_world.z+v.stand;return result;}
template<class Finalize>
bool sample_with_impl(gears_anchor::Receipt const* scene,EyeSample const& eye,Geometry& out,Finalize finalize){
    try{
        std::lock_guard<std::mutex> lock(body::mutex);stage="scene_binding";
        if(!scene||!scene->invocation||scene->scene_qpc<=0||scene->family_qpc<scene->scene_qpc||scene->renderer_qpc<scene->family_qpc||eye.scene_invocation!=scene->invocation)return reject(Reason::no_scene);
        if(scene->thread!=GetCurrentThreadId()||body::game_thread.load()!=scene->thread)return reject(Reason::wrong_thread);
        if(!neutral::body_ready())return reject(Reason::body_guard);
        if(!neutral::proof.ready||!neutral::proof_current())return reject(Reason::neutral_proof);
        const EyeSample key=eye;neutral::Context before{},after{},last{};
        if(!neutral::context(*scene,before)||!matches(before,key)||!calibration_matches(before,key))return reject(Reason::eye_binding);
        gears_anchor::Identity selected_pawn_class{},selected_link_class{},selected_controller{},selected_controller_class{};
        if(!body::identity(before.binding.pawn.cls,selected_pawn_class)||!body::identity(before.link.cls,selected_link_class)||
           !body::identity(before.binding.controller,selected_controller)||!body::identity(selected_controller.cls,selected_controller_class))return reject(Reason::eye_binding);
        if(!body::same(proof.pawn_class,selected_pawn_class)||!body::same(proof.link_class,selected_link_class)||!body::same(proof.controller_class,selected_controller_class)){
            proof={};proof.pawn_class=selected_pawn_class;proof.link_class=selected_link_class;proof.controller_class=selected_controller_class;}
        stage="geometry_metadata";
        if(proof.ready&&!proof_current()){proof.ready=false;return reject(Reason::layout_changed);}
        if(!proof.ready){const auto now=GetTickCount64();if(proof.attempts>=8)return reject(Reason::layout_rejected);
            if(proof.attempted&&(now<proof.last_attempt||now-proof.last_attempt<1000))return reject(Reason::layout_wait);
            proof.attempted=true;proof.last_attempt=now;++proof.attempts;if(!prepare()||!proof_current())return reject(Reason::layout_rejected);}
        Values first{},second{};if(!values(before,first))return false;
        stage="closing_context";
        if(!neutral::body_ready()||!neutral::proof_current()||!proof_current()||!neutral::context(*scene,after)||
           !neutral::same_values(before,after)||!matches(after,key)||!calibration_matches(after,key))return reject(Reason::changed);
        if(!values(after,second))return false;
        stage="closing_values_metadata";
        if(!same_values(first,second)||!neutral::body_ready()||!neutral::proof_current()||!proof_current()||!neutral::context(*scene,last)||
           !neutral::same_values(before,last)||!matches(last,key)||!matches(last,eye)||!calibration_matches(last,eye)||
           eye.scene_invocation!=key.scene_invocation)return reject(Reason::changed);
        Geometry candidate{};candidate.binding=key;candidate.link_location=first.location;candidate.link_rotation=first.rotation;
        candidate.pawn_location=first.pawn_location;candidate.pawn_rotation=first.pawn_rotation;candidate.mid_height=first.mid;candidate.stand_height=first.stand;
        candidate.standing_lean_offset=first.standing;candidate.crouch_lean_offset=first.crouch;candidate.slots=first.slots;candidate.slot_stride=proof.stride;
        candidate.selected.index=key.slot;candidate.selected.location_offset=first.offset;candidate.selected.rotation_offset=first.rotation_offset;candidate.selected.cover_type=first.cover_type;
        for(unsigned n=0;n<4;++n)candidate.selected.permissions[n]={first.words[n],proof.masks[n],(first.words[n]&proof.masks[n])!=0};
        candidate.candidates=candidates(first);candidate.metadata_sources=proof.source_count;candidate.metadata_identities=proof.identity_count;
        if(!finalize(before,first,key,candidate))return false;
        out=candidate;stage="ready";last_reason=Reason::ready;return true;
    }catch(...){return reject(Reason::exception);}
}
bool sample_with(gears_anchor::Receipt const* scene,EyeSample const& eye,Geometry& out){
    return sample_with_impl(scene,eye,out,[](neutral::Context const&,Values const&,EyeSample const&,Geometry&){return true;});
}
bool native_layout_matches(){
    // Saved implementation dataflow, additionally required to agree with this
    // current reflected proof. No guessed layout or legacy candidate fallback.
    return proof.ready&&proof.link[0].offset==0x130&&proof.link[1].offset==0x13c&&
        proof.link[6].offset==gears_cover_surface::slots_offset&&proof.slot[5].offset==0xc&&
        proof.slot[6].offset==0x18&&proof.stride==gears_cover_surface::slot_stride;
}
struct NativeQuery {
    template<class Read,class Guard>
    bool operator()(uintptr_t module,uintptr_t link,int slot,unsigned thread,Read read,Guard guard,gears_cover_surface::Result& out)const{
        return gears_cover_surface::query(module,link,slot,thread,read,guard,out);
    }
};
template<class Query>
bool sample_native_with(gears_anchor::Receipt const* scene,EyeSample const& eye,Geometry& out,Query query){
    std::array<unsigned char,sizeof(gears_anchor::Receipt)> receipt_bytes{};
    if(scene)memcpy(receipt_bytes.data(),scene,receipt_bytes.size());
    return sample_with_impl(scene,eye,out,[&](neutral::Context const& before,Values const& first,EyeSample const& key,Geometry& candidate){
        stage="native_layout";if(!native_layout_matches())return reject(Reason::native_layout);
        // Both callbacks execute under the ALREADY held body mutex. They must
        // not recursively call sample_with/sample_native_with or take it again.
        bool guard_failed=false;
        auto guard=[&](){
            neutral::Context current{};Values observed{};
            if(memcmp(receipt_bytes.data(),scene,receipt_bytes.size())||scene->thread!=GetCurrentThreadId()||
               body::game_thread.load()!=scene->thread||!neutral::body_ready()||!neutral::proof.ready||!neutral::proof_current()||
               !native_layout_matches()||!proof_current()||!neutral::context(*scene,current)||!neutral::same_values(before,current)||
               !matches(current,key)||!matches(current,eye)||!calibration_matches(current,key)||!calibration_matches(current,eye)||
               eye.scene_invocation!=key.scene_invocation||!values(current,observed)||!same_values(first,observed)){
                guard_failed=true;return false;
            }
            return true;
        };
        stage="native_query";if(!guard())return reject(Reason::changed);
        gears_cover_surface::Result native{};
        if(!query(base,before.link.address,before.slot,scene->thread,safe_copy,guard,native))
            return reject(guard_failed?Reason::changed:Reason::native_getter);
        stage="native_closing_guard";if(!guard()||guard_failed)return reject(Reason::changed);
        if(native.slot!=before.slot||native.slots.data!=first.slots.data||native.slots.count!=first.slots.count||
           native.slots.capacity!=first.slots.capacity||!gears_cover_surface::valid(native.location))return reject(Reason::native_getter);
        candidate.native_slot_world={native.location.x,native.location.y,native.location.z};
        for(unsigned n=0;n<3;++n)candidate.native_slot_rotation[n]=native.rotation.values[n];
        // Verification refers ONLY to these native getter outputs. Candidate
        // std::sin/cos axes/top heights remain separate and uncertified.
        candidate.native_getter_verified=true;return true;
    });
}
bool sample_native_with(gears_anchor::Receipt const* scene,EyeSample const& eye,Geometry& out){
    return sample_native_with(scene,eye,out,NativeQuery{});
}
#else
bool sample_with(gears_anchor::Receipt const*,EyeSample const&,Geometry&){return reject(Reason::body_unavailable);}
template<class Query>bool sample_native_with(gears_anchor::Receipt const*,EyeSample const&,Geometry&,Query){return reject(Reason::body_unavailable);}
bool sample_native_with(gears_anchor::Receipt const*,EyeSample const&,Geometry&){return reject(Reason::body_unavailable);}
#endif
bool sample(EyeSample const& eye,Geometry& out){
    if(current_scene&&current_scene->poisoned())return reject(Reason::nested_scene);
    return sample_with(current_scene?current_scene->receipt():nullptr,eye,out);
}
} // namespace cover_geometry_live
