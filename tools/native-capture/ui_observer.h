#pragma once
#include "anchor_observer.h"

namespace gears_ui {
using Identity=gears_anchor::Identity;
using Registry=gears_anchor::Registry;
inline constexpr std::array<std::uintptr_t,4> binding_rvas{0x1d6ca0,0x1d52b4,0x3a4d3c,0x2482b8};
inline constexpr std::array<std::array<unsigned char,16>,4> binding_bytes{{
    {0x40,0x53,0x55,0x56,0x41,0x56,0x48,0x81,0xec,0xa8,0x00,0x00,0x00,0x48,0x8b,0x05},
    {0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x55,0x56,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81},
    {0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x18,0x55,0x57,0x41,0x56,0x48},
    {0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x55,0x56,0x57,0x41,0x56,0x41,0x57,0x48,0x8d}
}};
#define GEARS_UI_FIELDS(X) \
    X(controller_pawn) X(controller_hud) X(pawn_inventory) X(player_camera) X(camera_owner) \
    X(hud_owner) X(pawn_controller) X(hud_canvas) X(weapon_instigator) \
    X(canvas_clip_x) X(canvas_clip_y) X(canvas_cur_x) X(canvas_cur_y) \
    X(canvas_org_x) X(canvas_org_y) X(canvas_size_x) X(canvas_size_y)
struct Super {unsigned identity{};std::uintptr_t parent{};};
struct Scope {unsigned kind{},owner{},function{};std::uintptr_t script{};unsigned size{};};
struct Binding {std::uintptr_t rva{};std::array<unsigned char,16> bytes{};unsigned function{};};
struct Config {
    std::uintptr_t base{},objects_header{},names_header{};
    unsigned object_internal_index{},object_name{},object_class{},object_outer{},field_super{},name_entry_index{},name_entry_text{},property_offset_internal{};
    unsigned identity_count{},super_count{},scope_count{};
    std::array<Identity,96> identities{};std::array<Super,96> supers{};
    std::array<Scope,8> scopes{};std::array<Binding,4> bindings{};
    unsigned role_controller{},role_camera{},role_pawn{},role_hud{},role_inventory{},role_canvas_class{};
    bool mounted{};unsigned role_controlled{},controlled_driver{},controlled_driver_property{},pawn_driven_vehicle{},pawn_driven_vehicle_property{},controlled_controller{},controlled_controller_property{};
#define GEARS_UI_FIELD_MEMBER(name) unsigned name{},name##_property{};
    GEARS_UI_FIELDS(GEARS_UI_FIELD_MEMBER)
#undef GEARS_UI_FIELD_MEMBER
};
struct ScopeContext {unsigned kind{},scope_index{};std::uintptr_t function{},object{},code{};};
struct CanvasSample {
    ScopeContext scope{};std::uintptr_t canvas{},frame_node{},frame_object{},frame_code{},controller{},pawn{},hud{},inventory{},camera{},hud_canvas{};
    float clip_x{},clip_y{},cur_x{},cur_y{},org_x{},org_y{};std::int32_t size_x{},size_y{};
};
template<class Read> struct Reader {
    Read read;
    bool bytes(void* out,std::uintptr_t address,std::size_t size) const {
        return address&&size&&address<=(std::numeric_limits<std::uintptr_t>::max)()-size&&read(out,reinterpret_cast<const void*>(address),size);
    }
    template<class T> bool field(T& out,std::uintptr_t address,unsigned offset) const {
        return address<=(std::numeric_limits<std::uintptr_t>::max)()-offset&&bytes(&out,address+offset,sizeof(out));
    }
};
inline const Identity* known(Config const& c,std::uintptr_t address){
    for(unsigned i=0;i<c.identity_count;++i)if(c.identities[i].address==address)return &c.identities[i];return nullptr;
}
inline bool derives(Config const& c,std::uintptr_t cls,std::uintptr_t owner){
    for(unsigned depth=0;depth<40&&cls;++depth){if(cls==owner)return known(c,cls)!=nullptr;
        bool found=false;for(unsigned i=0;i<c.super_count;++i)if(c.identities[c.supers[i].identity].address==cls){cls=c.supers[i].parent;found=true;break;}
        if(!found)return false;
    }return false;
}
inline bool semantic_scope(Config const& c,Scope const& s){
    if(s.owner>=c.identity_count||s.function>=c.identity_count)return false;
    const auto owner=c.identities[s.owner].name.data(),function=c.identities[s.function].name.data();
    if(!std::wcscmp(owner,L"WarHUD"))return s.kind==1&&(!std::wcscmp(function,L"DrawWeaponCrosshair")||
        !std::wcscmp(function,L"DrawWeaponCrosshairGOW3"))||(s.kind==3&&!std::wcscmp(function,L"PostRender"));
    if(!std::wcscmp(owner,L"WarInventoryManager"))return s.kind==2&&!std::wcscmp(function,L"DrawWeaponSlots");
    return s.kind==1&&!std::wcscmp(function,L"DrawAdditionalCrosshair")&&
        (!std::wcscmp(owner,L"WarWeapon")||!std::wcscmp(owner,L"Weap_AssaultRifle")||!std::wcscmp(owner,L"Weap_LocustAssaultRifle"));
}
template<class Read> bool registry(Reader<Read> const& r,std::uintptr_t address,Registry& out){
    return r.bytes(&out,address,sizeof(out))&&out.data&&out.count>1000&&out.count<=out.capacity&&out.capacity<4000000;
}
template<class Read> bool fresh_identity(Config const& c,Reader<Read> const& r,Registry const& objects,Registry const& names,Identity const& id){
    std::uintptr_t slot{},cls{},outer{},entry{},parent{};unsigned index{},name{},number{},entry_index{};std::array<wchar_t,64> text{};
    std::array<unsigned char,96> raw{};
    if(!r.bytes(raw.data(),id.address,raw.size()))return false;
    auto object_field=[&](auto& value,unsigned offset){if(offset>raw.size()||sizeof(value)>raw.size()-offset)return false;
        std::memcpy(&value,raw.data()+offset,sizeof(value));return true;};
    if(!id.address||!id.cls||id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||
       !object_field(index,c.object_internal_index)||index!=id.index||
       !object_field(name,c.object_name)||name!=id.name_index||!object_field(number,c.object_name+4)||number!=id.number||
       !object_field(cls,c.object_class)||cls!=id.cls||!object_field(outer,c.object_outer)||outer!=id.outer||
       !r.field(slot,objects.data,id.index*8)||slot!=id.address||!r.field(entry,names.data,id.name_index*8)||!entry||
       !r.field(entry_index,entry,c.name_entry_index)||entry_index!=id.name_index||
       !r.field(text,entry,c.name_entry_text))return false;
    unsigned length=0;for(;length<text.size();++length){if(text[length]!=id.name[length])return false;if(!text[length])break;}
    if(length==text.size()||!length)return false;
    return !id.check_super||(r.field(parent,id.address,c.field_super)&&parent==id.super);
}
// Controlled Turret and human inventory Pawn remain distinct semantic roles.
template<class Read> bool possession_environment(Config const& c,Reader<Read> const& r){
    if(c.role_controller>=c.identity_count||c.role_pawn>=c.identity_count)return false;
    const auto pc=c.identities[c.role_controller].address,pawn=c.identities[c.role_pawn].address;
    auto link=[&](std::uintptr_t address,unsigned offset,std::uintptr_t expected){std::uintptr_t actual{};return r.field(actual,address,offset)&&actual==expected;};
    if(!c.mounted)return link(pc,c.controller_pawn,pawn)&&link(pawn,c.pawn_controller,pc);
    if(c.role_controlled>=c.identity_count||c.role_controlled==c.role_pawn)return false;
    Registry objects{},names{};if(!registry(r,c.objects_header,objects)||!registry(r,c.names_header,names)||
        !fresh_identity(c,r,objects,names,c.identities[c.role_controlled]))return false;
    auto const& controlled=c.identities[c.role_controlled];
    auto has_class=[&](std::uintptr_t cls,wchar_t const* name){for(unsigned n=0;n<40&&cls;++n){auto id=known(c,cls);if(!id)return false;
        if(!std::wcscmp(id->name.data(),name))return derives(c,controlled.cls,id->address);bool found=false;
        for(unsigned i=0;i<c.super_count;++i)if(c.supers[i].identity<c.identity_count&&c.identities[c.supers[i].identity].address==cls){cls=c.supers[i].parent;found=true;break;}
        if(!found)return false;}return false;};
    if(!has_class(controlled.cls,L"Turret")||!has_class(controlled.cls,L"Vehicle"))return false;
    const unsigned props[]={c.controlled_driver_property,c.pawn_driven_vehicle_property,c.controlled_controller_property};
    const unsigned offsets[]={c.controlled_driver,c.pawn_driven_vehicle,c.controlled_controller};
    const wchar_t* labels[]={L"Driver",L"DrivenVehicle",L"Controller"};
    const wchar_t* owners[]={L"Vehicle",L"Pawn",L"Pawn"};
    for(unsigned i=0;i<3;++i){if(props[i]>=c.identity_count||offsets[i]>0xfff8)return false;auto const& prop=c.identities[props[i]];
        auto kind=known(c,prop.cls),owner=known(c,prop.outer);unsigned dim{},size{},offset{};
        if(prop.number||std::wcscmp(prop.name.data(),labels[i])||!kind||std::wcscmp(kind->name.data(),L"ObjectProperty")||
           !owner||std::wcscmp(owner->name.data(),owners[i])||!derives(c,i==1?c.identities[c.role_pawn].cls:controlled.cls,prop.outer)||
           !fresh_identity(c,r,objects,names,prop)||!r.field(dim,prop.address,0x70)||dim!=1||!r.field(size,prop.address,0x74)||size!=8||
           !r.field(offset,prop.address,c.property_offset_internal)||offset!=offsets[i])return false;
    }
    return link(pc,c.controller_pawn,controlled.address)&&link(controlled.address,c.controlled_controller,pc)&&
        link(controlled.address,c.controlled_driver,pawn)&&link(pawn,c.pawn_driven_vehicle,controlled.address)&&link(pawn,c.pawn_controller,0);
}

template<class Read> bool validate(Config const& c,Read read,bool check_entry_bytes=true){
    Reader<Read> r{read};Registry objects{},names{};
    if(!c.base||!c.identity_count||c.identity_count>c.identities.size()||c.super_count>c.supers.size()||!c.scope_count||c.scope_count>c.scopes.size())return false;
    const unsigned layout[]{c.object_internal_index,c.object_name,c.object_class,c.object_outer,c.field_super,c.name_entry_index,c.name_entry_text,c.property_offset_internal};
    for(auto offset:layout)if(offset>=0x10000)return false;
    const unsigned roles[]{c.role_controller,c.role_camera,c.role_pawn,c.role_hud,c.role_inventory,c.role_canvas_class};
    for(auto role:roles)if(role>=c.identity_count)return false;
    if(!registry(r,c.objects_header,objects)||!registry(r,c.names_header,names))return false;
    for(unsigned i=0;i<c.identity_count;++i){auto const& id=c.identities[i];
        if(!fresh_identity(c,r,objects,names,id)||!known(c,id.cls)||!id.name[0]||id.name.back())return false;
        for(unsigned j=0;j<i;++j)if(id.address==c.identities[j].address||id.index==c.identities[j].index)return false;
    }
    for(unsigned i=0;i<c.super_count;++i){auto const& s=c.supers[i];std::uintptr_t actual{};
        if(s.identity>=c.identity_count||(s.parent&&!known(c,s.parent))||
           !r.field(actual,c.identities[s.identity].address,c.field_super)||actual!=s.parent)return false;
        for(unsigned j=0;j<i;++j)if(s.identity==c.supers[j].identity)return false;
    }
    for(unsigned i=0;i<c.bindings.size();++i){auto const& b=c.bindings[i];std::array<unsigned char,16> actual{};
        if(b.rva!=binding_rvas[i]||b.bytes!=binding_bytes[i]||
           (check_entry_bytes&&!r.field(actual,c.base,unsigned(b.rva)))||(check_entry_bytes&&actual!=b.bytes))return false;
        if(i){if(b.function>=c.identity_count)return false;auto const& f=c.identities[b.function];auto cls=known(c,f.cls);std::uintptr_t entry{};
            const wchar_t* expected=i==1?L"DrawTile":i==2?L"DrawMaterialTile":L"DrawText";
            if(!cls||std::wcscmp(cls->name.data(),L"Function")||std::wcscmp(f.name.data(),expected)||f.number||
               f.outer!=c.identities[c.role_canvas_class].address||!r.field(entry,f.address,0x114)||entry!=c.base+b.rva)return false;
        }
    }
    for(unsigned i=0;i<c.scope_count;++i){auto const& s=c.scopes[i];Registry script{};std::uintptr_t entry{};
        if(!semantic_scope(c,s)||!s.script||!s.size||s.size>131072||
           s.script>(std::numeric_limits<std::uintptr_t>::max)()-s.size)return false;
        auto const& f=c.identities[s.function];auto cls=known(c,f.cls);auto owner_cls=known(c,c.identities[s.owner].cls);
        if(!cls||std::wcscmp(cls->name.data(),L"Function")||!owner_cls||std::wcscmp(owner_cls->name.data(),L"Class")||
           f.outer!=c.identities[s.owner].address||f.number||!r.field(script,f.address,0x8c)||script.data!=s.script||
           script.count!=int(s.size)||script.count>script.capacity||script.capacity>131072||
           !r.field(entry,f.address,0x114)||entry!=c.base+binding_rvas[0])return false;
        for(unsigned j=0;j<i;++j)if(s.function==c.scopes[j].function)return false;
    }
#define GEARS_UI_CHECK_FIELD(name) {unsigned actual{};if(c.name>=0x10000||c.name##_property>=c.identity_count||!r.field(actual,c.identities[c.name##_property].address,c.property_offset_internal)||actual!=c.name)return false;}
    GEARS_UI_FIELDS(GEARS_UI_CHECK_FIELD)
#undef GEARS_UI_CHECK_FIELD
    const auto pc=c.identities[c.role_controller].address,pawn=c.identities[c.role_pawn].address,hud=c.identities[c.role_hud].address,
        inventory=c.identities[c.role_inventory].address,camera=c.identities[c.role_camera].address;
    auto link=[&](std::uintptr_t address,unsigned offset,std::uintptr_t expected){std::uintptr_t actual{};return r.field(actual,address,offset)&&actual==expected;};
    if(!possession_environment(c,r)||!link(pc,c.controller_hud,hud)||!link(pawn,c.pawn_inventory,inventory)||
       !link(pc,c.player_camera,camera)||!link(camera,c.camera_owner,pc)||!link(hud,c.hud_owner,pc))return false;
    Registry after_objects{},after_names{};
    return registry(r,c.objects_header,after_objects)&&registry(r,c.names_header,after_names)&&
        objects.data==after_objects.data&&objects.count==after_objects.count&&objects.capacity==after_objects.capacity&&
        names.data==after_names.data&&names.count==after_names.count&&names.capacity==after_names.capacity;
}
template<class Read> bool dynamic_object(Config const& c,Reader<Read> const& r,std::uintptr_t address,std::uintptr_t& cls){
    Registry objects{},names{};unsigned index{},name{},entry_index{};std::uintptr_t slot{},entry{};
    bool valid=registry(r,c.objects_header,objects)&&registry(r,c.names_header,names)&&
        r.field(index,address,c.object_internal_index)&&index<unsigned(objects.count)&&r.field(slot,objects.data,index*8)&&slot==address&&
        r.field(name,address,c.object_name)&&name<unsigned(names.count)&&r.field(entry,names.data,name*8)&&entry&&
        r.field(entry_index,entry,c.name_entry_index)&&entry_index==name&&r.field(cls,address,c.object_class)&&known(c,cls);
    return valid&&fresh_identity(c,r,objects,names,*known(c,cls));
}
template<class Read> bool quick_environment(Config const& c,Reader<Read> const& r){
    if(!c.identity_count||c.identity_count>c.identities.size()||c.scope_count>c.scopes.size()||c.super_count>c.supers.size())return false;
    const unsigned roles[]{c.role_controller,c.role_pawn,c.role_hud,c.role_inventory,c.role_camera,c.role_canvas_class};
    for(auto role:roles)if(role>=c.identity_count)return false;
    Registry objects{},names{};if(!registry(r,c.objects_header,objects)||!registry(r,c.names_header,names))return false;
    for(auto role:{c.role_controller,c.role_pawn,c.role_hud})if(!fresh_identity(c,r,objects,names,c.identities[role]))return false;
    const auto pc=c.identities[c.role_controller].address,pawn=c.identities[c.role_pawn].address,hud=c.identities[c.role_hud].address;
    auto link=[&](std::uintptr_t address,unsigned offset,std::uintptr_t expected){std::uintptr_t actual{};return r.field(actual,address,offset)&&actual==expected;};
    return possession_environment(c,r)&&link(pc,c.controller_hud,hud)&&
        link(hud,c.hud_owner,pc)&&link(pawn,c.pawn_inventory,c.identities[c.role_inventory].address)&&
        link(pc,c.player_camera,c.identities[c.role_camera].address);
}
template<class Read> bool live_derives(Config const& c,Reader<Read> const& r,std::uintptr_t cls,std::uintptr_t owner){
    Registry objects{},names{};if(!registry(r,c.objects_header,objects)||!registry(r,c.names_header,names))return false;
    for(unsigned depth=0;depth<40&&cls;++depth){auto id=known(c,cls);if(!id||!fresh_identity(c,r,objects,names,*id))return false;
        if(cls==owner)return true;bool found=false;
        for(unsigned i=0;i<c.super_count;++i){auto const& parent=c.supers[i];if(parent.identity>=c.identity_count)return false;
            if(c.identities[parent.identity].address==cls){std::uintptr_t actual{};
                if(!r.field(actual,cls,c.field_super)||actual!=parent.parent)return false;cls=actual;found=true;break;}}
        if(!found)return false;
    }return false;
}
template<class Read> bool live_scope_object(Config const& c,Reader<Read> const& r,unsigned scope_index,std::uintptr_t self){
    if(scope_index>=c.scope_count)return false;auto const& s=c.scopes[scope_index];
    if(s.owner>=c.identity_count||s.function>=c.identity_count)return false;
    auto const& owner=c.identities[s.owner];std::uintptr_t cls{};
    if(!semantic_scope(c,s)||!dynamic_object(c,r,self,cls)||!live_derives(c,r,cls,owner.address))return false;
    if(!std::wcscmp(owner.name.data(),L"WarHUD"))return self==c.identities[c.role_hud].address;
    if(!std::wcscmp(owner.name.data(),L"WarInventoryManager"))return self==c.identities[c.role_inventory].address;
    std::uintptr_t instigator{};return r.field(instigator,self,c.weapon_instigator)&&instigator==c.identities[c.role_pawn].address;
}
template<class Read> bool match_scope(Config const& c,Read read,std::uintptr_t self,std::uintptr_t frame,ScopeContext& out){
    Reader<Read> r{read};std::uintptr_t node{},object{},code{};
    if(c.scope_count>c.scopes.size()||c.identity_count>c.identities.size()||!r.field(node,frame,0x18))return false;
    for(unsigned i=0;i<c.scope_count;++i){auto const& s=c.scopes[i];if(s.function>=c.identity_count||node!=c.identities[s.function].address)continue;
        Registry objects{},names{},script{};std::uintptr_t entry{};
        if(!quick_environment(c,r)||s.owner>=c.identity_count||!registry(r,c.objects_header,objects)||!registry(r,c.names_header,names)||
           !fresh_identity(c,r,objects,names,c.identities[s.function])||!fresh_identity(c,r,objects,names,c.identities[s.owner])||
           c.identities[s.function].outer!=c.identities[s.owner].address||!r.field(script,node,0x8c)||script.data!=s.script||
           script.count!=int(s.size)||script.count<=0||script.count>script.capacity||script.capacity>131072||
           !r.field(entry,node,0x114)||entry!=c.base+binding_rvas[0]||
           !r.field(object,frame,0x20)||object!=self||!r.field(code,frame,0x28)||code<s.script||code-s.script>=s.size||
           !live_scope_object(c,r,i,self))return false;
        out={s.kind,i,node,self,code};return true;
    }return false; // Caller retains any enclosing TLS scope on an unmatched call.
}
// Nested script calls execute an inline opcode loop in module+0x1d6f30,
// bypassing the shared VM binding. Its FFrame stores PreviousFrame at +0x38.
// Canvas wrappers receive that script caller frame. Authenticate the whole
// bounded ancestry through the active VM root before committing a child scope.
inline constexpr unsigned canvas_frame_limit=12;
template<class Read> bool classify_canvas_scope(Config const& c,Read read,ScopeContext const& root,
    std::uintptr_t root_frame,std::uintptr_t canvas_frame,ScopeContext& out,std::uintptr_t& matched_frame,unsigned* matched_depth=nullptr){
    if(!root_frame||!canvas_frame||root.scope_index>=c.scope_count||c.scope_count>c.scopes.size()||
       !c.identity_count||c.identity_count>c.identities.size())return false;
    const auto& configured=c.scopes[root.scope_index];
    if(configured.function>=c.identity_count||root.kind!=configured.kind||
       root.function!=c.identities[configured.function].address||root.code<configured.script||
       root.code-configured.script>=configured.size)return false;
    Reader<Read> r{read};std::array<std::uintptr_t,canvas_frame_limit> visited{};
    ScopeContext selected{};std::uintptr_t selected_frame{};unsigned selected_depth{};
    for(unsigned depth=0;depth<canvas_frame_limit;++depth){
        if(!canvas_frame||(canvas_frame&7))return false;
        for(unsigned i=0;i<depth;++i)if(visited[i]==canvas_frame)return false;
        visited[depth]=canvas_frame;
        std::array<std::uintptr_t,3> fields{}; // Node, Object, Code; never Locals.
        if(!r.field(fields,canvas_frame,0x18)||!fields[0]||!fields[1]||!fields[2])return false;
        bool recognized=false;
        for(unsigned i=0;i<c.scope_count;++i){
            if(c.scopes[i].function>=c.identity_count)return false;
            if(fields[0]==c.identities[c.scopes[i].function].address){recognized=true;break;}
        }
        ScopeContext current{};
        if(recognized){
            if(!match_scope(c,read,fields[1],canvas_frame,current)||current.function!=fields[0]||
               current.object!=fields[1]||current.code!=fields[2])return false;
            if(!selected_frame){selected=current;selected_frame=canvas_frame;selected_depth=depth;}
        }
        if(canvas_frame==root_frame){
            // Code advances after entry. Require the same authenticated root
            // identity and an in-range current Code, not equality to entry Code.
            if(!recognized||current.kind!=root.kind||current.scope_index!=root.scope_index||
               current.function!=root.function||current.object!=root.object)return false;
            out=selected;matched_frame=selected_frame;if(matched_depth)*matched_depth=selected_depth;return true;
        }
        if(!r.field(canvas_frame,canvas_frame,0x38))return false;
    }
    return false;
}
template<class Read> bool sample_canvas(Config const& c,Read read,ScopeContext const& scope,std::uintptr_t canvas,std::uintptr_t frame,CanvasSample& out){
    Reader<Read> r{read};std::uintptr_t cls{};
    if(!quick_environment(c,r)||scope.scope_index>=c.scope_count)return false;
    auto const& selected=c.scopes[scope.scope_index];
    if(selected.function>=c.identity_count||scope.kind!=selected.kind||scope.function!=c.identities[selected.function].address||
       scope.code<selected.script||scope.code-selected.script>=selected.size||
       !live_scope_object(c,r,scope.scope_index,scope.object)||
       !dynamic_object(c,r,canvas,cls)||!live_derives(c,r,cls,c.identities[c.role_canvas_class].address))return false;
    CanvasSample sample{};sample.scope=scope;sample.canvas=canvas;
    sample.controller=c.identities[c.role_controller].address;sample.pawn=c.identities[c.role_pawn].address;
    sample.hud=c.identities[c.role_hud].address;sample.inventory=c.identities[c.role_inventory].address;sample.camera=c.identities[c.role_camera].address;
    if(!r.field(sample.frame_node,frame,0x18)||!r.field(sample.frame_object,frame,0x20)||!r.field(sample.frame_code,frame,0x28)||
       !r.field(sample.hud_canvas,sample.hud,c.hud_canvas)||!r.field(sample.clip_x,canvas,c.canvas_clip_x)||
       !r.field(sample.clip_y,canvas,c.canvas_clip_y)||!r.field(sample.cur_x,canvas,c.canvas_cur_x)||!r.field(sample.cur_y,canvas,c.canvas_cur_y)||
       !r.field(sample.org_x,canvas,c.canvas_org_x)||!r.field(sample.org_y,canvas,c.canvas_org_y)||
       !r.field(sample.size_x,canvas,c.canvas_size_x)||!r.field(sample.size_y,canvas,c.canvas_size_y))return false;
    const float scalars[]{sample.clip_x,sample.clip_y,sample.cur_x,sample.cur_y,sample.org_x,sample.org_y};
    for(auto value:scalars)if(!std::isfinite(value))return false;
    if(sample.clip_x<=0||sample.clip_y<=0||sample.size_x<=0||sample.size_y<=0||sample.size_x>65536||sample.size_y>65536||
       (sample.hud_canvas&&sample.hud_canvas!=canvas))return false;
    out=sample;return true;
}
// A copyable ScopeContext lets the hook's RAII save/restore its TLS on every
// exit, including nested known scopes. This module owns no TLS or game memory.
}
