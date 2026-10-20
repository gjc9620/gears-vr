#pragma once
#include "ui_observer.h"
namespace gears_ui_binding {
struct Binding {gears_anchor::Identity pawn{},hud{},inventory{};std::uintptr_t controller{},camera{},level{};};
inline bool same(Binding const& a,Binding const& b){return gears_anchor::equal_identity(a.pawn,b.pawn)&&
    gears_anchor::equal_identity(a.hud,b.hud)&&gears_anchor::equal_identity(a.inventory,b.inventory)&&
    a.controller==b.controller&&a.camera==b.camera&&a.level==b.level;}
inline gears_ui::Config config(gears_ui::Config const& startup,Binding const& b){auto c=startup;
    c.identities[c.role_pawn]=b.pawn;c.identities[c.role_hud]=b.hud;c.identities[c.role_inventory]=b.inventory;return c;}
template<class Read> bool object(gears_ui::Config const& c,gears_ui::Reader<Read> const& r,gears_ui::Registry const& objects,
    gears_ui::Registry const& names,std::uintptr_t address,gears_anchor::Identity const& expected,std::uintptr_t outer,gears_anchor::Identity& out){
    gears_anchor::Identity id{};id.address=address;std::uintptr_t entry{};
    if(!address||!r.field(id.index,address,c.object_internal_index)||!r.field(id.name_index,address,c.object_name)||
       !r.field(id.number,address,c.object_name+4)||!r.field(id.cls,address,c.object_class)||!r.field(id.outer,address,c.object_outer)||
       id.cls!=expected.cls||id.outer!=outer||id.name_index>=unsigned(names.count)||
       !r.field(entry,names.data,id.name_index*8)||!entry||!r.field(id.name,entry,c.name_entry_text)||
       !id.name[0]||id.name.back()||!std::wcsncmp(id.name.data(),L"Default__",9)||!gears_ui::fresh_identity(c,r,objects,names,id)||
       !gears_ui::known(c,id.cls)||!gears_ui::fresh_identity(c,r,objects,names,*gears_ui::known(c,id.cls)))return false;
    out=id;return true;
}
// Startup certificate stays immutable. The anchor guard supplies the same-level,
// same-class/outer Pawn policy and full camera/possession bracket on THIS thread.
template<class Read> bool acquire(gears_ui::Config const& c,gears_anchor::Config const& a,Read read,Binding& out,gears_anchor::GuardDiagnostic* diagnostic=nullptr){
    if(c.identity_count>c.identities.size()||c.role_pawn>=c.identity_count||c.role_hud>=c.identity_count||c.role_inventory>=c.identity_count||
       c.role_controller>=c.identity_count||c.role_camera>=c.identity_count||a.identity_count>a.identities.size()||
       a.role_controller>=a.identity_count||a.role_camera>=a.identity_count||a.role_pawn>=a.identity_count||
       c.objects_header!=a.objects_header||c.names_header!=a.names_header||
       c.identities[c.role_controller].address!=a.identities[a.role_controller].address||
       c.identities[c.role_camera].address!=a.identities[a.role_camera].address||
       c.mounted!=a.mounted_enabled||(c.mounted&&a.mounted_role_driver>=a.identity_count)||
       c.identities[c.role_pawn].address!=a.identities[c.mounted?a.mounted_role_driver:a.role_pawn].address||
       (c.mounted&&(c.role_controlled>=c.identity_count||c.identities[c.role_controlled].address!=a.identities[a.role_pawn].address))||
       c.object_internal_index!=a.object_internal_index||c.object_name!=a.object_name||c.object_class!=a.object_class||
       c.object_outer!=a.object_outer||c.name_entry_index!=a.name_entry_index||c.name_entry_text!=a.name_entry_text||
       c.controller_pawn!=a.controller_pawn||c.pawn_controller!=a.pawn_controller||c.player_camera!=a.player_camera||c.camera_owner!=a.pc_owner)return false;
    gears_anchor::Anchor anchor{};std::uint32_t stamp{};
    if(!gears_anchor::guard(a,read,anchor,stamp,diagnostic))return false;
    Binding b{};b.pawn=c.mounted?a.identities[a.mounted_role_driver]:anchor.pawn_identity;b.controller=anchor.controller;b.camera=anchor.camera;b.level=anchor.level;
    gears_ui::Reader<Read> r{read};gears_ui::Registry objects{},names{};std::uintptr_t hud{},inventory{};
    if(!gears_ui::registry(r,c.objects_header,objects)||!gears_ui::registry(r,c.names_header,names)||
       !r.field(hud,b.controller,c.controller_hud)||!r.field(inventory,b.pawn.address,c.pawn_inventory))return false;
    auto mapped_outer=[&](gears_anchor::Identity const& id){return id.outer==c.identities[c.role_pawn].address?b.pawn.address:id.outer;};
    if(!object(c,r,objects,names,hud,c.identities[c.role_hud],mapped_outer(c.identities[c.role_hud]),b.hud)||
       !object(c,r,objects,names,inventory,c.identities[c.role_inventory],mapped_outer(c.identities[c.role_inventory]),b.inventory))return false;
    for(unsigned i=0;i<c.identity_count;++i)if(i!=c.role_pawn&&i!=c.role_hud&&i!=c.role_inventory&&
       (c.identities[i].address==b.pawn.address||c.identities[i].address==hud||c.identities[i].address==inventory))return false;
    if(b.pawn.address==hud||hud==inventory||b.pawn.address==inventory)return false;
    const auto candidate=config(c,b);
    if(!gears_ui::quick_environment(candidate,r)||!gears_ui::fresh_identity(candidate,r,objects,names,b.inventory))return false;
    gears_anchor::Anchor after{};
    if(!gears_anchor::guard(a,read,after,stamp,diagnostic)||!gears_anchor::equal_identity(anchor.pawn_identity,after.pawn_identity)||
       after.controller!=b.controller||after.camera!=b.camera||after.level!=b.level)return false;
    gears_ui::Registry end_objects{},end_names{};
    if(!gears_ui::registry(r,c.objects_header,end_objects)||!gears_ui::registry(r,c.names_header,end_names)||
       std::memcmp(&objects,&end_objects,sizeof(objects))||std::memcmp(&names,&end_names,sizeof(names))||
       !gears_ui::quick_environment(candidate,r)||!gears_ui::fresh_identity(candidate,r,objects,names,b.inventory)||
       !gears_ui::fresh_identity(candidate,r,objects,names,*gears_ui::known(candidate,b.hud.cls))||
       !gears_ui::fresh_identity(candidate,r,objects,names,*gears_ui::known(candidate,b.inventory.cls)))return false;
    out=b;return true;
}
enum class ReceiptStatus {current,pending,retired,changed};
// External mutex protects this bounded value ledger. Overwrite only causes a
// conservative loss of semantics; token equality prevents ABA/relabeling.
class Receipts {
    struct Entry {std::uint64_t token{};Binding binding{};bool complete{},valid{};};
    std::array<Entry,512> entries_{};
public:
    void retire_all(){entries_={};}
    void begin(std::uint64_t token,Binding const& binding){if(token)entries_[token%entries_.size()]={token,binding,false,false};}
    void finish(std::uint64_t token,bool valid){auto& e=entries_[token%entries_.size()];if(token&&e.token==token){e.complete=true;e.valid=valid;}}
    ReceiptStatus check(std::uint64_t token,Binding const& current)const{
        auto const& e=entries_[token%entries_.size()];if(!token||e.token!=token)return ReceiptStatus::retired;
        if(!e.complete)return ReceiptStatus::pending;
        return e.valid&&same(e.binding,current)?ReceiptStatus::current:ReceiptStatus::changed;
    }
};
}
