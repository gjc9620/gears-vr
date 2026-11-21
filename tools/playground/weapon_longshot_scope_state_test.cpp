#include "../vr-common/weapon_hand_participant.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
using uptr=std::uintptr_t;constexpr uptr base=0x100000;
namespace hand=gears_weapon_hand;
bool actors_live=true,fields_live=true,metadata_live=true;
bool target_pc{},target_pawn{},target_key{},zoomed{},ignore_controllers=true;
std::array<bool,4> hidden{};std::array<uptr,4> components{1024,1280,1536,0};
float default_fov=85;unsigned metadata_checks{},change_component_after{};
namespace gripdamage_metadata {
enum class Stage {final_check};struct Table {};
struct Property {unsigned offset{};};struct PointerField {uptr value{};};
struct Snapshot {uptr module_base{};Table objects{},names{};};
struct Reader {
    Snapshot& result;Stage stage;
    template<class T>bool at(uptr,unsigned offset,T& out){
        if constexpr(std::is_same_v<T,float>){out=default_fov;return offset==24&&fields_live;}else return false;
    }
};
bool tables(uptr,Table&,Table&){return fields_live;}
}
namespace weaponhand {
struct BoolField {gripdamage_metadata::Property property{};unsigned bits{},mask{4};};
bool current_gt(void*){return true;}
template<std::size_t N>bool boolean(gripdamage_metadata::Reader&,uptr at,wchar_t const(&name)[N],BoolField& out){
    out={};bool value{};bool found=false;
    if(!std::wcscmp(name,L"bOwnerNoSee"))for(unsigned i=0;i<components.size();++i)if(components[i]&&at==components[i]){value=hidden[i];found=true;}
    if(!std::wcscmp(name,L"bIgnoreControllersWhenNotRendered")&&at==components[1]){value=ignore_controllers;found=true;}
    if(!std::wcscmp(name,L"bIsTargeting")&&(at==48||at==64)){value=at==48?target_pc:target_pawn;found=true;}
    if(!std::wcscmp(name,L"bPressingTargetingKey")&&at==48){value=target_key;found=true;}
    if(!std::wcscmp(name,L"bZoomed")&&at==80){value=zoomed;found=true;}
    out.bits=value?4u:0u;return fields_live&&found;
}
template<std::size_t N,std::size_t K>bool field(gripdamage_metadata::Reader&,uptr at,wchar_t const(&name)[N],wchar_t const(&kind)[K],unsigned bytes,BoolField& out){
    out={};out.property.offset=24;return fields_live&&at==48&&!std::wcscmp(name,L"DefaultFOV")&&!std::wcscmp(kind,L"FloatProperty")&&bytes==4;
}
template<std::size_t N>bool read_pointer(gripdamage_metadata::Reader&,uptr at,wchar_t const(&name)[N],gripdamage_metadata::PointerField& out,bool is_component){
    out={};if(!fields_live||!is_component)return false;
    if(!std::wcscmp(name,L"Mesh")&&(at==64||at==80)){out.value=components[at==80?0:1];return true;}
    if(at==64&&!std::wcscmp(name,L"HeadSlotStaticMesh")){out.value=components[2];return true;}
    if(at==64&&!std::wcscmp(name,L"HeadSlotStaticMesh2")){out.value=components[3];return true;}
    return false;
}
}
namespace unified_weapon_fire {
bool retained_metadata(gripdamage_metadata::Reader&){
    ++metadata_checks;if(change_component_after==metadata_checks)components[2]+=16;return metadata_live;
}
}
namespace longshot_scope_original {
bool actors(hand::Frame const&){return actors_live;}
bool identify(gripdamage_metadata::Reader&,uptr at,hand::ObjectIdentity& out){out={at,0,16,unsigned(at/16),unsigned(at/16),0,1};return at!=0;}
}
#include "weapon_longshot_scope_state.inl"
namespace state=longshot_scope_state;
int main(){
    hand::Frame frame{};frame.controller.address=48;frame.pawn.address=64;frame.weapon.address=80;
    state::State observed{};
    assert(state::read(frame,observed)&&state::neutral(observed)&&observed.base_fov==85);
    assert(!observed.components[3].identity.address); // Absent optional head mesh is explicit.
    hidden[2]=true;assert(state::read(frame,observed)&&!state::neutral(observed));hidden[2]=false;
    target_pc=target_pawn=true;ignore_controllers=false;hidden={true,true,true,false};
    assert(state::read(frame,observed)&&state::scoped(observed));
    target_key=true;assert(state::read(frame,observed)&&!state::scoped(observed));target_key=false;
    hidden[1]=false;assert(state::read(frame,observed)&&!state::scoped(observed));hidden[1]=true;
    default_fov=NAN;assert(!state::read(frame,observed));default_fov=85;
    actors_live=false;assert(!state::read(frame,observed));actors_live=true;
    fields_live=false;assert(!state::read(frame,observed));fields_live=true;
    metadata_live=false;assert(!state::read(frame,observed));metadata_live=true;
    // Each full sample has one actor read + three nonempty component reads.
    // Change a binding after first sample's closing read; double-read rejects.
    metadata_checks=0;change_component_after=4;assert(!state::read(frame,observed));change_component_after=0;
    components[0]=0;assert(!state::read(frame,observed));
    std::puts("Longshot scope production state reader: actual component identities/visibility, optional heads, target/key/zoom/ignore flags, original base FOV, ownership baseline, two-read drift and unavailable metadata checks passed; reflection boundaries injected");
}
