#pragma once
#include "ui_observer.h"
#include "crosshair_writer_view.h"

// Read-only evidence around ONE caller-owned original execLet invocation.
// This proves assignment completion, not tick, camera or scene freshness.
namespace gears_crosshair_assignment {
inline constexpr std::array<unsigned char,32> execlet_entry{{
    0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
    0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x30,0x48,0x8b,0xda,0x4c,0x8d,0x35,0x8e,0xe7}};
// StructProperty non-constructor copy route: size = ArrayDim * ElementSize,
// destination in RDX, source in R8, then original memcpy call.
inline constexpr std::array<unsigned char,21> copy_route{{
    0x8b,0x41,0x74,0x48,0x8b,0xd6,0x0f,0xaf,0x41,0x70,0x48,0x8b,0xcd,
    0x4c,0x63,0xc0,0xe8,0x4f,0x1f,0x3a,0x00}};
struct Field {unsigned property{},offset{};};
struct AimStateConfig {bool enabled{};unsigned property{},offset{},mask{};};
struct WriterViewConfig {
    bool enabled{};
    unsigned cam_loc{},cam_rot{},start_trace{},trace_dir{},rotator_struct{};
    unsigned cam_loc_offset{},cam_rot_offset{},start_trace_offset{},trace_dir_offset{};
};
struct Config {
    // Certificate storage must remain immutable and alive across both calls.
    const gears_ui::Config* identities{};
    unsigned writer{},war_pc{},controller{},pawn{},weapon{},war_pawn{},war_weapon{};
    unsigned lhs{},member{},impact{},impact_struct{},vector_struct{},ww{};
    std::uintptr_t script{};unsigned script_size{};
    unsigned impact_offset{},member_offset{},ww_offset{},impact_size{};
    // Structure-reference metadata offset is provided by the fresh resolver;
    // its position is not inferred from the decompiler's malformed prototype.
    unsigned property_structure{};
    Field controller_pawn{},pawn_controller{},pawn_weapon{},weapon_instigator{};
    WriterViewConfig view{};
    AimStateConfig aim{};
};
struct Snapshot {
    std::uintptr_t self{},frame{},node{},locals{},pawn{},weapon{},destination{},source{},script{};
    std::array<unsigned char,12> source_bytes{},previous_cache{};
    gears_crosshair_view::Sample view{};
};
struct CompletedAssignmentReceipt {
    std::uintptr_t controller{},pawn{},weapon{},writer{},frame{},locals{},destination{};
    std::array<unsigned char,12> cache_bytes{};
    std::array<float,3> point{};
    gears_crosshair_view::Sample view{};
};
namespace detail {
template<class Read> bool certificate(Config const& c,gears_ui::Reader<Read> const& r) {
    if(!c.identities)return false;auto const& ids=*c.identities;
    if(!ids.base||!ids.identity_count||ids.identity_count>ids.identities.size()||ids.super_count>ids.supers.size())return false;
    const unsigned layout[]{ids.object_internal_index,ids.object_name,ids.object_class,ids.object_outer,
        ids.field_super,ids.name_entry_index,ids.name_entry_text,ids.property_offset_internal};
    for(auto offset:layout)if(offset>=0x10000)return false;
    if(ids.property_offset_internal!=0x94||c.property_structure<0x60||c.property_structure>=0x10000)return false;
    gears_ui::Registry objects{},names{},after_objects{},after_names{};
    if(!gears_ui::registry(r,ids.objects_header,objects)||!gears_ui::registry(r,ids.names_header,names))return false;
    for(unsigned i=0;i<ids.identity_count;++i){auto const& id=ids.identities[i];
        if(!id.name[0]||id.name.back()||!gears_ui::known(ids,id.cls)||
           !gears_ui::fresh_identity(ids,r,objects,names,id))return false;
        for(unsigned j=0;j<i;++j)if(id.address==ids.identities[j].address||id.index==ids.identities[j].index)return false;
    }
    for(unsigned i=0;i<ids.super_count;++i){auto const& s=ids.supers[i];std::uintptr_t actual{};
        if(s.identity>=ids.identity_count||(s.parent&&!gears_ui::known(ids,s.parent))||
           !r.field(actual,ids.identities[s.identity].address,ids.field_super)||actual!=s.parent)return false;
        for(unsigned j=0;j<i;++j)if(s.identity==ids.supers[j].identity)return false;
    }
    return gears_ui::registry(r,ids.objects_header,after_objects)&&gears_ui::registry(r,ids.names_header,after_names)&&
        objects.data==after_objects.data&&objects.count==after_objects.count&&objects.capacity==after_objects.capacity&&
        names.data==after_names.data&&names.count==after_names.count&&names.capacity==after_names.capacity;
}
inline const gears_ui::Identity* id(Config const& c,unsigned index) {
    return c.identities&&index<c.identities->identity_count?&c.identities->identities[index]:nullptr;
}
inline bool named(Config const& c,unsigned index,const wchar_t* name,const wchar_t* cls,std::uintptr_t outer=0,bool check_outer=false) {
    auto value=id(c,index);if(!value||value->number||std::wcscmp(value->name.data(),name))return false;
    auto type=gears_ui::known(*c.identities,value->cls);
    return type&&!std::wcscmp(type->name.data(),cls)&&(!check_outer||value->outer==outer);
}
template<class Read> bool schema(Config const& c,gears_ui::Reader<Read> const& r) {
    const unsigned roles[]{c.writer,c.war_pc,c.controller,c.pawn,c.weapon,c.war_pawn,c.war_weapon,
        c.lhs,c.member,c.impact,c.impact_struct,c.vector_struct,c.ww};
    for(auto role:roles)if(!id(c,role))return false;
    auto addr=[&](unsigned i){return id(c,i)->address;};
    auto const& ids=*c.identities;
    if(!named(c,c.war_pc,L"WarPC",L"Class")||!named(c,c.war_pawn,L"WarPawn",L"Class")||
       !named(c,c.war_weapon,L"WarWeapon",L"Class")||
       !named(c,c.writer,L"UpdateHUDActions",L"Function",addr(c.war_pc),true)||
       !named(c,c.lhs,L"LastCrosshairHitLocation",L"StructProperty",addr(c.war_pc),true)||
       !named(c,c.member,L"HitLocation",L"StructProperty",addr(c.impact_struct),true)||
       !named(c,c.impact,L"Impact",L"StructProperty",addr(c.writer),true)||
       !named(c,c.ww,L"WW",L"ObjectProperty",addr(c.writer),true)||
       !named(c,c.impact_struct,L"ImpactInfo",L"ScriptStruct")||!named(c,c.vector_struct,L"Vector",L"ScriptStruct")||
       !gears_ui::derives(ids,id(c,c.controller)->cls,addr(c.war_pc))||
       !gears_ui::derives(ids,id(c,c.pawn)->cls,addr(c.war_pawn))||
       !gears_ui::derives(ids,id(c,c.weapon)->cls,addr(c.war_weapon)))return false;
    for(auto index:{c.controller,c.pawn,c.weapon})if(!std::wcsncmp(id(c,index)->name.data(),L"Default__",9))return false;
    if(!c.script||c.script_size<0x380||c.script_size>131072||c.impact_size<12||c.impact_size>65536||
       c.impact_offset>=65536||c.ww_offset>=65536||c.member_offset>c.impact_size-12)return false;
    gears_ui::Registry script{};std::uintptr_t entry{};
    if(!r.field(script,addr(c.writer),0x8c)||script.data!=c.script||script.count!=int(c.script_size)||
       script.count>script.capacity||script.capacity>131072||!r.field(entry,addr(c.writer),0x114)||entry!=ids.base+0x1d6ca0)return false;
    auto prop=[&](unsigned index,unsigned offset,unsigned size,std::uintptr_t structure,std::uint64_t excluded){
        unsigned actual{},dimension{},element{};std::uint64_t flags{};std::uintptr_t type{};
        return r.field(actual,addr(index),0x94)&&actual==offset&&r.field(dimension,addr(index),0x70)&&dimension==1&&
            r.field(element,addr(index),0x74)&&element==size&&r.field(flags,addr(index),0x78)&&!(flags&excluded)&&
            r.field(type,addr(index),c.property_structure)&&type==structure;
    };
    if(!prop(c.lhs,0x136c,12,addr(c.vector_struct),0x20)||
       !prop(c.member,c.member_offset,12,addr(c.vector_struct),0x400000)||
       !prop(c.impact,c.impact_offset,c.impact_size,addr(c.impact_struct),0)||
       !prop(c.ww,c.ww_offset,8,addr(c.war_weapon),0))return false;
    if(c.view.enabled){
        auto const& v=c.view;
        for(auto index:{v.cam_loc,v.cam_rot,v.start_trace,v.trace_dir,v.rotator_struct})if(!id(c,index))return false;
        if(!named(c,v.rotator_struct,L"Rotator",L"ScriptStruct"))return false;
        const unsigned indices[]{v.cam_loc,v.cam_rot,v.start_trace,v.trace_dir};
        const unsigned offsets[]{v.cam_loc_offset,v.cam_rot_offset,v.start_trace_offset,v.trace_dir_offset};
        const wchar_t* names[]{L"CamLoc",L"CamRot",L"StartTrace",L"TraceDir"};
        for(unsigned i=0;i<4;++i){
            if(offsets[i]>65524||!named(c,indices[i],names[i],L"StructProperty",addr(c.writer),true)||
               !prop(indices[i],offsets[i],12,addr(i==1?v.rotator_struct:c.vector_struct),0))return false;
            auto overlaps=[&](unsigned begin,unsigned size){return offsets[i]<begin+size&&begin<offsets[i]+12;};
            if(overlaps(c.ww_offset,8)||overlaps(c.impact_offset,c.impact_size))return false;
            for(unsigned j=0;j<i;++j)if(offsets[i]<offsets[j]+12&&offsets[j]<offsets[i]+12)return false;
        }
    }
    if(c.aim.enabled){auto const& a=c.aim;unsigned dimension{},size{},offset{},mask{};
        std::uintptr_t vtable{},accessor{},dispatch{};
        if(!c.view.enabled||a.offset>65532||!a.mask||(a.mask&(a.mask-1))||
           !named(c,a.property,L"bIsTargeting",L"BoolProperty")||
           !gears_ui::derives(ids,ids.identities[c.controller].cls,id(c,a.property)->outer)||
           !r.field(dimension,addr(a.property),0x70)||dimension!=1||
           !r.field(size,addr(a.property),0x74)||size!=4||
           !r.field(offset,addr(a.property),0x94)||offset!=a.offset||
           !r.field(mask,addr(a.property),0xd0)||mask!=a.mask||
           !r.field(vtable,addr(a.property),0)||!vtable||
           !r.field(accessor,vtable,0x2a0)||accessor!=ids.base+0x1daba0||
           !r.field(dispatch,ids.base,0x1b86280+0x2d*8)||dispatch!=ids.base+0x1d6ea4)return false;
    }
    auto link=[&](std::uintptr_t object,Field field,std::uintptr_t target,const wchar_t* name){
        auto property=id(c,field.property);unsigned offset{};std::uintptr_t actual{};
        auto cls=property?gears_ui::known(ids,property->cls):nullptr;
        auto obj=gears_ui::known(ids,object);
        return property&&obj&&cls&&!property->number&&!std::wcscmp(property->name.data(),name)&&
            !std::wcscmp(cls->name.data(),L"ObjectProperty")&&gears_ui::derives(ids,obj->cls,property->outer)&&field.offset<65536&&
            r.field(offset,property->address,0x94)&&offset==field.offset&&r.field(actual,object,field.offset)&&actual==target;
    };
    return link(addr(c.controller),c.controller_pawn,addr(c.pawn),L"Pawn")&&link(addr(c.pawn),c.pawn_controller,addr(c.controller),L"Controller")&&
        link(addr(c.pawn),c.pawn_weapon,addr(c.weapon),L"Weapon")&&link(addr(c.weapon),c.weapon_instigator,addr(c.pawn),L"Instigator");
}
template<class Read> bool bindings(Config const& c,gears_ui::Reader<Read> const& r,bool entry_bytes) {
    if(!c.identities||!id(c,c.member))return false;const auto base=c.identities->base;
    if(!base||base>(std::numeric_limits<std::uintptr_t>::max)()-0x577e65)return false;
    std::uintptr_t actual{},table{};
    if(base>(std::numeric_limits<std::uintptr_t>::max)()-0x1b9cc18)return false;table=base+0x1b86280;
    const std::pair<unsigned,unsigned> slots[]{{0x0f,0x1d7ad0},{0x35,0x1d6750},{0,0x1d7eb0},{1,0x1d8570}};
    for(auto slot:slots)if(!r.field(actual,table,slot.first*8)||actual!=base+slot.second)return false;
    std::uintptr_t vtable{};
    if(!r.field(vtable,id(c,c.member)->address,0)||!r.field(actual,vtable,0x2a8)||actual!=base+0x1d5ec8)return false;
    std::array<unsigned char,21> copy{};
    if(!r.field(copy,base,0x1d5efb)||copy!=copy_route)return false;
    // The guarded flags branch must remain the verified non-constructor path.
    // Entry code between flag-load and BT includes register copies; verify the
    // exact BT/JB, rather than inventing a contiguous short prefix.
    std::array<unsigned char,3> flag_load{};
    if(!r.field(flag_load,base,0x1d5ee5)||flag_load!=std::array<unsigned char,3>{0x8b,0x41,0x78})return false;
    std::array<unsigned char,7> branch{};
    constexpr std::array<unsigned char,7> expected_branch{{0x48,0x0f,0xba,0xe0,0x16,0x72,0x34}};
    if(!r.field(branch,base,0x1d5ef4)||branch!=expected_branch)return false;
    if(entry_bytes){std::array<unsigned char,32> actual_entry{};
        if(!r.field(actual_entry,base,0x1d7ad0)||actual_entry!=execlet_entry)return false;}
    return true;
}
template<class Read> bool state(Config const& c,gears_ui::Reader<Read> const& r,std::uintptr_t self,std::uintptr_t frame,
                              unsigned code_offset,Snapshot& out) {
    if(!certificate(c,r)||!schema(c,r)||!bindings(c,r,false))return false;
    auto addr=[&](unsigned i){return id(c,i)->address;};
    Snapshot value{};value.self=self;value.frame=frame;value.pawn=addr(c.pawn);value.weapon=addr(c.weapon);value.script=c.script;
    std::uintptr_t object{},code{},ww{};unsigned scratch{};
    if(self!=addr(c.controller)||!r.field(value.node,frame,0x18)||value.node!=addr(c.writer)||
       !r.field(object,frame,0x20)||object!=self||!r.field(code,frame,0x28)||
       c.script>(std::numeric_limits<std::uintptr_t>::max)()-c.script_size||code!=c.script+code_offset||
       !r.field(value.locals,frame,0x30)||!value.locals||
       !r.field(scratch,c.identities->base,0x1b9cc18)||(scratch&1))return false;
    std::array<unsigned char,38> tokens{};
    if(!r.field(tokens,c.script,0x359)||tokens[0]!=0x0f||tokens[1]!=1||tokens[10]!=0x35||
       tokens[27]||tokens[28]||tokens[29]!=0)return false;
    auto pointer=[&](unsigned index,std::uintptr_t expected){std::uintptr_t actual{};
        std::memcpy(&actual,tokens.data()+index,8);return actual==expected;};
    if(!pointer(2,addr(c.lhs))||!pointer(11,addr(c.member))||!pointer(19,addr(c.impact_struct))||!pointer(30,addr(c.impact)))return false;
    if(!r.field(ww,value.locals,c.ww_offset)||ww!=value.weapon||
       self>(std::numeric_limits<std::uintptr_t>::max)()-0x136c||
       value.locals>(std::numeric_limits<std::uintptr_t>::max)()-c.impact_offset-c.member_offset)return false;
    value.destination=self+0x136c;value.source=value.locals+c.impact_offset+c.member_offset;
    if(!r.bytes(value.source_bytes.data(),value.source,12)||!r.bytes(value.previous_cache.data(),value.destination,12))return false;
    if(c.view.enabled){
        auto const& v=c.view;
        if(!r.field(value.view.location,value.locals,v.cam_loc_offset)||
           !r.field(value.view.rotation,value.locals,v.cam_rot_offset)||
           !r.field(value.view.start,value.locals,v.start_trace_offset)||
           !r.field(value.view.direction,value.locals,v.trace_dir_offset))return false;
        for(auto const& vector:{value.view.location,value.view.start,value.view.direction})
            for(float f:vector)if(!std::isfinite(f))return false;
        value.view.valid=true;
        if(c.aim.enabled){unsigned flags{};if(!r.field(flags,self,c.aim.offset))return false;
            value.view.aiming_known=true;value.view.aiming=(flags&c.aim.mask)!=0;}
    }
    std::uintptr_t final_node{},final_object{},final_code{},final_locals{},final_ww{};unsigned final_scratch{};
    if(!r.field(final_node,frame,0x18)||final_node!=value.node||!r.field(final_object,frame,0x20)||final_object!=self||
       !r.field(final_code,frame,0x28)||final_code!=code||!r.field(final_locals,frame,0x30)||final_locals!=value.locals||
       !r.field(final_ww,value.locals,c.ww_offset)||final_ww!=ww||
       !r.field(final_scratch,c.identities->base,0x1b9cc18)||(final_scratch&1)||
       !certificate(c,r)||!schema(c,r)||!bindings(c,r,false))return false;
    out=value;return true;
}
}
// Activation only: originals must still have their reserved bytes. Hot begin /
// end cannot compare a detoured execLet prologue against its original bytes.
enum class ValidationFailure {none,certificate,schema,bindings};
inline const char* validation_failure_name(ValidationFailure failure){
    switch(failure){
    case ValidationFailure::certificate:return "certificate";
    case ValidationFailure::schema:return "schema";
    case ValidationFailure::bindings:return "bindings";
    default:return "none";
    }
}
template<class Read> bool validate_bindings(Config const& c,Read read,bool check_execlet_entry=true,ValidationFailure* failure=nullptr) {
    gears_ui::Reader<Read> r{read};
    if(failure)*failure=ValidationFailure::none;
    if(!detail::certificate(c,r)){if(failure)*failure=ValidationFailure::certificate;return false;}
    if(!detail::schema(c,r)){if(failure)*failure=ValidationFailure::schema;return false;}
    if(!detail::bindings(c,r,check_execlet_entry)){if(failure)*failure=ValidationFailure::bindings;return false;}
    return true;
}
template<class Read> bool snapshot(Config const& c,Read read,std::uintptr_t self,std::uintptr_t frame,Snapshot& out) {
    gears_ui::Reader<Read> r{read};Snapshot a{},b{};
    if(!detail::state(c,r,self,frame,0x35a,a)||!detail::state(c,r,self,frame,0x35a,b)||
       a.locals!=b.locals||a.source!=b.source||a.source_bytes!=b.source_bytes||a.previous_cache!=b.previous_cache||
       !gears_crosshair_view::equal(a.view,b.view))return false;
    out=b;return true;
}
template<class Read> bool completed(Config const& c,Read read,Snapshot const& before,CompletedAssignmentReceipt& out) {
    gears_ui::Reader<Read> r{read};Snapshot a{},b{};
    if(!detail::state(c,r,before.self,before.frame,0x37f,a)||!detail::state(c,r,before.self,before.frame,0x37f,b)||
       a.node!=before.node||a.locals!=before.locals||a.pawn!=before.pawn||a.weapon!=before.weapon||a.script!=before.script||
       a.destination!=before.destination||a.source!=before.source||a.source_bytes!=before.source_bytes||
       a.previous_cache!=a.source_bytes||b.locals!=a.locals||b.source_bytes!=a.source_bytes||b.previous_cache!=a.previous_cache||
       !gears_crosshair_view::equal(a.view,b.view)||!gears_crosshair_view::equal(before.view,b.view))return false;
    CompletedAssignmentReceipt receipt{};receipt.controller=a.self;receipt.pawn=a.pawn;receipt.weapon=a.weapon;
    receipt.writer=a.node;receipt.frame=a.frame;receipt.locals=a.locals;receipt.destination=a.destination;receipt.cache_bytes=b.previous_cache;
    std::memcpy(receipt.point.data(),receipt.cache_bytes.data(),12);
    receipt.view=b.view;
    for(float v:receipt.point)if(!std::isfinite(v))return false;
    out=receipt;return true;
}
}
