#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cwchar>
#include <limits>
#include <algorithm>

namespace gears_anchor {
struct Identity {std::uintptr_t address{},cls{},outer{},super{};unsigned index{},name_index{},number{},check_super{};
    std::array<wchar_t,64> name{};};
struct Config {std::uintptr_t objects_header{},names_header{};unsigned identity_count{};
    unsigned object_internal_index{},object_name{},object_class{},object_outer{},field_super{},name_entry_index{},name_entry_text{};
    unsigned role_controller{},role_local{},role_camera{},role_pawn{},role_level{};
    unsigned controller_pawn{},pawn_controller{},pawn_location{},pawn_rotation{},controller_rotation{},eye_height{},base_eye_height{},
        player_camera{},pc_owner{},player{},camera_view_target{},camera_timestamp{},camera_pov{};
    std::array<Identity,128> identities{};
};
struct Registry {std::uintptr_t data{};int count{},capacity{};};
inline bool equal_identity(Identity const& a,Identity const& b) {
    return a.address==b.address&&a.cls==b.cls&&a.outer==b.outer&&a.super==b.super&&a.index==b.index&&
        a.name_index==b.name_index&&a.number==b.number&&a.check_super==b.check_super&&a.name==b.name;
}
struct PawnBinding {Identity pawn{};std::uintptr_t controller{},local{},camera{},level{};};
inline bool equal_binding(PawnBinding const& a,PawnBinding const& b) {
    return equal_identity(a.pawn,b.pawn)&&a.controller==b.controller&&a.local==b.local&&a.camera==b.camera&&a.level==b.level;
}
// This is a narrow same-class/same-outer possession snapshot, not a new Level
// certificate or an absolute UObject serial/ABA proof. Config stays immutable.
template<class Read> bool capture_pawn_binding(Config const& c,Read read,PawnBinding& out) {
    if(!c.identity_count||c.identity_count>c.identities.size()||c.role_pawn>=c.identity_count||
        c.role_controller>=c.identity_count||c.role_local>=c.identity_count||c.role_camera>=c.identity_count||c.role_level>=c.identity_count)return false;
    auto bytes=[&](void* dest,std::uintptr_t address,std::size_t size){return address&&size&&
        address<=(std::numeric_limits<std::uintptr_t>::max)()-size&&read(dest,reinterpret_cast<const void*>(address),size);};
    auto link=[&](std::uintptr_t owner,unsigned offset,std::uintptr_t expected){std::uintptr_t actual{};
        return owner<=(std::numeric_limits<std::uintptr_t>::max)()-offset&&bytes(&actual,owner+offset,8)&&actual==expected;};
    Registry objects{},names{};
    auto registry=[&](Registry& r,std::uintptr_t address){return bytes(&r,address,sizeof(r))&&r.data&&r.count>1000&&r.count<=r.capacity&&r.capacity<4000000;};
    if(!registry(objects,c.objects_header)||!registry(names,c.names_header))return false;
    PawnBinding value{};value.controller=c.identities[c.role_controller].address;value.local=c.identities[c.role_local].address;
    value.camera=c.identities[c.role_camera].address;value.level=c.identities[c.role_level].address;
    if(!value.controller||!value.local||!value.camera||!value.level||
        value.controller>(std::numeric_limits<std::uintptr_t>::max)()-c.controller_pawn||
        !bytes(&value.pawn.address,value.controller+c.controller_pawn,8)||!value.pawn.address)return false;
    auto& id=value.pawn;std::array<unsigned char,96> raw{};
    if(!bytes(raw.data(),id.address,raw.size()))return false;
    auto field=[&](auto& v,unsigned offset){if(offset>raw.size()||sizeof(v)>raw.size()-offset)return false;
        std::memcpy(&v,raw.data()+offset,sizeof(v));return true;};
    if(!field(id.index,c.object_internal_index)||!field(id.name_index,c.object_name)||c.object_name>88||
        !field(id.number,c.object_name+4)||!field(id.cls,c.object_class)||!field(id.outer,c.object_outer)||
        id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||
        id.cls!=c.identities[c.role_pawn].cls||id.outer!=c.identities[c.role_pawn].outer)return false;
    for(unsigned i=0;i<c.identity_count;++i)if(i!=c.role_pawn&&
        (id.address==c.identities[i].address||id.index==c.identities[i].index))return false;
    auto slot_address=[&](std::uintptr_t data,unsigned index,std::uintptr_t& result){auto offset=std::uintptr_t(index)*8;
        if(data>(std::numeric_limits<std::uintptr_t>::max)()-offset)return false;result=data+offset;return true;};
    std::uintptr_t slot{},entry{},address{};unsigned entry_index{};std::array<wchar_t,64> text{};
    if(!slot_address(objects.data,id.index,address)||!bytes(&slot,address,8)||slot!=id.address||
        !slot_address(names.data,id.name_index,address)||!bytes(&entry,address,8)||
        entry>(std::numeric_limits<std::uintptr_t>::max)()-c.name_entry_index||
        !bytes(&entry_index,entry+c.name_entry_index,4)||entry_index!=id.name_index||
        entry>(std::numeric_limits<std::uintptr_t>::max)()-c.name_entry_text||
        !bytes(text.data(),entry+c.name_entry_text,sizeof(text)))return false;
    unsigned length=0;while(length<text.size()&&text[length])++length;
    if(!length||length==text.size()||!std::wcsncmp(text.data(),L"Default__",9))return false;
    std::copy(text.begin(),text.begin()+length,id.name.begin());
    if(id.address==c.identities[c.role_pawn].address&&!equal_identity(id,c.identities[c.role_pawn]))return false;
    auto links=[&](){return link(value.controller,c.controller_pawn,id.address)&&link(id.address,c.pawn_controller,value.controller)&&
        link(value.camera,c.camera_view_target,id.address)&&link(value.controller,c.player_camera,value.camera)&&
        link(value.camera,c.pc_owner,value.controller)&&link(value.controller,c.player,value.local);};
    if(!links())return false;
    std::array<unsigned char,96> after{};Registry objects_after{},names_after{};
    if(!bytes(after.data(),id.address,after.size())||after!=raw||
        !registry(objects_after,c.objects_header)||!registry(names_after,c.names_header)||
        std::memcmp(&objects,&objects_after,sizeof(objects))||std::memcmp(&names,&names_after,sizeof(names))||
        !slot_address(objects.data,id.index,address)||!bytes(&slot,address,8)||slot!=id.address||
        !slot_address(names.data,id.name_index,address)||!bytes(&slot,address,8)||slot!=entry||
        !bytes(&entry_index,entry+c.name_entry_index,4)||entry_index!=id.name_index||
        !bytes(text.data(),entry+c.name_entry_text,sizeof(text))||!links())return false;
    for(unsigned i=0;i<=length;++i)if(text[i]!=id.name[i])return false;
    out=value;return true;
}
enum Reason:unsigned {accepted=0,no_receipt,nested,ambiguous,source_mismatch,family_mismatch,
    matrix_mismatch,stamp_mismatch,identity_failed,read_failed,multiple_views,caller_mismatch,
    already_consumed,reason_count};
inline const char* reason_name(unsigned n){static const char* names[]={"accepted","no_receipt","nested","ambiguous",
    "source_mismatch","family_mismatch","matrix_mismatch","stamp_mismatch","identity_failed","read_failed",
    "multiple_views","caller_mismatch","already_consumed"};return n<reason_count?names[n]:"unknown";}
struct Key {std::uintptr_t local{},family{},source{},owner{};std::uint32_t stamp{};
    std::array<unsigned char,128> matrices{};};
struct Anchor {std::uintptr_t pawn{},controller{},camera{},level{};std::array<float,3> location{};
    float eye_height{},base_eye_height{};Identity pawn_identity{};};
inline bool scene_binding_matches(PawnBinding const& before,Anchor const& after) {
    return equal_identity(before.pawn,after.pawn_identity)&&before.pawn.address==after.pawn&&
        before.controller==after.controller&&before.camera==after.camera&&before.level==after.level;
}
template<class Read> bool strict_guard(Config const& c,Read read,Anchor& out,std::uint32_t& stamp){
    auto bytes=[&](void* dest,std::uintptr_t address,std::size_t size){return address&&read(dest,reinterpret_cast<const void*>(address),size);};
    Registry objects{},names{};
    auto registry=[&](Registry& h,std::uintptr_t address){return bytes(&h,address,sizeof(h))&&h.data&&h.count>1000&&h.count<=h.capacity&&h.capacity<4000000;};
    if(!c.identity_count||c.identity_count>c.identities.size()||!registry(objects,c.objects_header)||!registry(names,c.names_header))return false;
    if(c.role_controller>=c.identity_count||c.role_local>=c.identity_count||c.role_camera>=c.identity_count||c.role_pawn>=c.identity_count||c.role_level>=c.identity_count)return false;
    const auto pc=c.identities[c.role_controller].address,local=c.identities[c.role_local].address,
        camera=c.identities[c.role_camera].address,pawn=c.identities[c.role_pawn].address,level=c.identities[c.role_level].address;
    auto identity=[&](std::uintptr_t address)->Identity const*{for(unsigned i=0;i<c.identity_count;++i)
        if(c.identities[i].address==address)return &c.identities[i];return nullptr;};
    auto derives=[&](std::uintptr_t cls,const wchar_t* name){for(unsigned n=0;n<64&&cls;++n){auto id=identity(cls);
        if(!id||!id->check_super)return false;if(!std::wcscmp(id->name.data(),name))return true;cls=id->super;}return false;};
    if(!std::wcsncmp(c.identities[c.role_pawn].name.data(),L"Default__",9)||
        !derives(c.identities[c.role_pawn].cls,L"WarPawn")||!derives(c.identities[c.role_level].cls,L"Level")||
        !derives(c.identities[c.role_local].cls,L"WarLocalPlayer")||!derives(c.identities[c.role_camera].cls,L"WarPlayerCamera"))return false;
    auto outer=c.identities[c.role_pawn].outer;bool inlevel=false;
    for(unsigned n=0;n<16&&outer;++n){if(outer==level){inlevel=true;break;}auto id=identity(outer);if(!id)break;outer=id->outer;}
    if(!inlevel)return false;
    std::uint32_t before{};float time{};if(!bytes(&before,camera+c.camera_timestamp,4))return false;
    std::memcpy(&time,&before,4);if(!std::isfinite(time))return false;
    auto verify_identities=[&](){for(unsigned i=0;i<c.identity_count;++i){auto const& id=c.identities[i];std::array<unsigned char,96> raw{};
        if(id.index>=unsigned(objects.count)||id.name_index>=unsigned(names.count)||!bytes(raw.data(),id.address,raw.size()))return false;
        auto field=[&](auto expected,unsigned offset){decltype(expected) actual{};if(offset+sizeof(actual)>raw.size())return false;
            std::memcpy(&actual,raw.data()+offset,sizeof(actual));return actual==expected;};
        std::uintptr_t slot{},entry{};unsigned nameindex{};std::array<wchar_t,64> text{};
        if(!field(id.index,c.object_internal_index)||!field(id.name_index,c.object_name)||!field(id.number,c.object_name+4)||
            !field(id.cls,c.object_class)||!field(id.outer,c.object_outer)||
            !bytes(&slot,objects.data+id.index*8,8)||slot!=id.address||!bytes(&entry,names.data+id.name_index*8,8)||
            !bytes(&nameindex,entry+c.name_entry_index,4)||nameindex!=id.name_index||!bytes(text.data(),entry+c.name_entry_text,sizeof(text)))return false;
        bool equal=false;for(unsigned j=0;j<text.size();++j){if(text[j]!=id.name[j])return false;if(!text[j]){equal=true;break;}}
        if(!equal)return false;
        if(id.check_super){std::uintptr_t parent{};if(!bytes(&parent,id.address+c.field_super,8)||parent!=id.super)return false;}
    }return true;};
    if(!verify_identities())return false;
    auto link=[&](std::uintptr_t owner,unsigned offset,std::uintptr_t expected){std::uintptr_t actual{};return bytes(&actual,owner+offset,8)&&actual==expected;};
    auto links=[&](){return link(pc,c.controller_pawn,pawn)&&link(pawn,c.pawn_controller,pc)&&link(pc,c.player_camera,camera)&&
        link(camera,c.pc_owner,pc)&&link(pc,c.player,local)&&link(camera,c.camera_view_target,pawn);};
    if(!links())return false;
    Anchor anchor{};anchor.pawn=pawn;anchor.controller=pc;anchor.camera=camera;anchor.level=level;
    anchor.pawn_identity=c.identities[c.role_pawn];
    if(!bytes(anchor.location.data(),pawn+c.pawn_location,12)||!bytes(&anchor.eye_height,pawn+c.eye_height,4)||
        !bytes(&anchor.base_eye_height,pawn+c.base_eye_height,4))return false;
    for(float v:anchor.location)if(!std::isfinite(v))return false;
    if(!std::isfinite(anchor.eye_height)||!std::isfinite(anchor.base_eye_height)||!std::isfinite(anchor.location[2]+anchor.eye_height))return false;
    Registry afterobjects{},afternames{};std::uint32_t after{};
    if(!verify_identities()||!registry(afterobjects,c.objects_header)||!registry(afternames,c.names_header)||
        std::memcmp(&objects,&afterobjects,sizeof(objects))||std::memcmp(&names,&afternames,sizeof(names))||
        !bytes(&after,camera+c.camera_timestamp,4)||before!=after||!links())return false;
    // Registry growth between observations is legal. A change inside this
    // finite bracket rejects this observation only; the next reads fresh headers.
    out=anchor;stamp=before;return true;
}
template<class Read> bool guard(Config const& startup,Read read,Anchor& out,std::uint32_t& stamp){
    PawnBinding binding{};if(!capture_pawn_binding(startup,read,binding))return false;
    Config candidate=startup;candidate.identities[candidate.role_pawn]=binding.pawn;
    Anchor anchor{};std::uint32_t observed{};
    if(!strict_guard(candidate,read,anchor,observed))return false;
    PawnBinding final{};if(!capture_pawn_binding(startup,read,final)||!equal_binding(binding,final))return false;
    std::uint32_t final_stamp{};
    if(binding.camera>(std::numeric_limits<std::uintptr_t>::max)()-startup.camera_timestamp||
        !read(&final_stamp,reinterpret_cast<const void*>(binding.camera+startup.camera_timestamp),4)||final_stamp!=observed)return false;
    out=anchor;stamp=observed;return true;
}
struct Receipt {Key key{};Anchor anchor{};std::uint64_t invocation{};std::uint32_t thread{};
    std::int64_t scene_qpc{},family_qpc{},renderer_qpc{};
    std::array<unsigned char,128> family_matrices{},renderer_matrices{};};
// Pure binding check against an already-readable current source snapshot.
// The caller obtains this receipt from its current one-use Context scope;
// no latest/global receipt, new memory read, or persistent failure state lives here.
inline bool submission_matches(Receipt const* receipt,std::uintptr_t family,
    std::uintptr_t source,std::uintptr_t local,std::uint32_t thread,
    const void* matrices128,std::int64_t qpc) {
    if(!receipt||!receipt->invocation||!thread||receipt->thread!=thread||
        !family||!source||!local||!matrices128||
        receipt->key.family!=family||receipt->key.owner!=family||
        receipt->key.source!=source||receipt->key.local!=local||
        receipt->scene_qpc<=0||receipt->family_qpc<receipt->scene_qpc||
        receipt->renderer_qpc<receipt->family_qpc||qpc<receipt->renderer_qpc||
        std::memcmp(receipt->renderer_matrices.data(),matrices128,128))return false;
    float stamp{};std::memcpy(&stamp,&receipt->key.stamp,sizeof(stamp));
    if(!std::isfinite(stamp))return false;
    for(float value:receipt->anchor.location)if(!std::isfinite(value))return false;
    if(!std::isfinite(receipt->anchor.eye_height)||!std::isfinite(receipt->anchor.base_eye_height)||
        !std::isfinite(receipt->anchor.location[2]+receipt->anchor.eye_height))return false;
    for(unsigned i=0;i<32;++i){float value{};
        std::memcpy(&value,receipt->renderer_matrices.data()+i*sizeof(float),sizeof(value));
        if(!std::isfinite(value))return false;
    }
    return true;
}
inline bool equal_anchor(Anchor const& a,Anchor const& b){return a.pawn==b.pawn&&a.controller==b.controller&&
    a.camera==b.camera&&a.level==b.level&&a.location==b.location&&a.eye_height==b.eye_height&&a.base_eye_height==b.base_eye_height&&
    equal_identity(a.pawn_identity,b.pawn_identity);}
inline Reason match(Key const& a,Key const& b){if(a.local!=b.local)return identity_failed;
    if(a.family!=b.family||b.owner!=b.family||a.owner!=a.family)return family_mismatch;
    if(a.source!=b.source)return source_mismatch;
    // Verified upper function can change projection after SceneView returns.
    // View64 remains strict; all three projection snapshots are evidence only.
    if(std::memcmp(a.matrices.data(),b.matrices.data(),64))return matrix_mismatch;
    if(a.stamp!=b.stamp)return stamp_mismatch;return accepted;}
struct FamilyState {Receipt receipt{};bool valid{},consumed{},poisoned{};};
class Context {
    Receipt pending_{};bool pending_valid_{};unsigned scene_depth_{};bool scene_dirty_{};
    FamilyState* active_{};
public:
    std::uint64_t sequence{};
    void begin_scene(){if(!scene_depth_)scene_dirty_=false;else scene_dirty_=true;
        ++scene_depth_;if(active_)active_->poisoned=true;}
    void end_scene(){if(scene_depth_)--scene_depth_;}
    Reason offer(Receipt receipt){if(scene_dirty_||scene_depth_!=1||active_){pending_valid_=false;return nested;}
        if(pending_valid_){pending_valid_=false;return ambiguous;}
        pending_=receipt;pending_valid_=true;return accepted;}
    void discard(){pending_valid_=false;}
    class FamilyScope {
        Context& context_;FamilyState state_{};FamilyState* previous_{};
    public:
        Reason reason{no_receipt};
        FamilyScope(Context& c,Key const& key,Anchor const& anchor,std::uint32_t thread,std::int64_t qpc,
            bool guard,unsigned count,std::uintptr_t caller):context_(c),previous_(c.active_){
            if(previous_){previous_->poisoned=true;c.discard();reason=nested;}
            else if(caller!=0x2c8909){c.discard();reason=caller_mismatch;}
            else if(count!=1){c.discard();reason=multiple_views;}
            else if(!guard){c.discard();reason=identity_failed;}
            else if(c.pending_valid_){state_.receipt=c.pending_;c.discard();
                reason=state_.receipt.thread!=thread?no_receipt:match(state_.receipt.key,key);
                if(reason==accepted&&!equal_anchor(state_.receipt.anchor,anchor))reason=identity_failed;
                if(reason==accepted){state_.valid=true;state_.receipt.family_qpc=qpc;state_.receipt.family_matrices=key.matrices;}}
            c.active_=&state_;
        }
        ~FamilyScope(){context_.discard();context_.active_=previous_;}
        FamilyScope(FamilyScope const&)=delete;
        bool valid()const{return state_.valid;}
        Receipt const& receipt()const{return state_.receipt;}
    };
    Reason consume(Key const& key,Anchor const& anchor,std::uint32_t thread,std::int64_t qpc,
        bool guard,unsigned count,std::uintptr_t caller,Receipt& out){
        if(!active_)return no_receipt;
        // FamilyScope is the sole owner of the active state. The same checks
        // are used here so production calls need no global/latest sidecar.
        if(!active_->valid)return no_receipt;if(active_->poisoned)return nested;
        if(active_->consumed)return already_consumed;active_->consumed=true;
        if(caller!=0x3a1595)return caller_mismatch;if(count!=1)return multiple_views;
        if(thread!=active_->receipt.thread)return no_receipt;
        if(!guard||!equal_anchor(active_->receipt.anchor,anchor))return identity_failed;
        auto result=match(active_->receipt.key,key);if(result!=accepted)return result;
        out=active_->receipt;out.renderer_qpc=qpc;out.renderer_matrices=key.matrices;return accepted;
    }
};
}
