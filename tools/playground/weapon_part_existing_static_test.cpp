#include "../vr-common/weapon_hand_participant.h"
#include <atomic>
#include <cassert>
#include <climits>
#include <vector>
#include <cstdio>
namespace h=gears_weapon_hand;
namespace {
using uptr=std::uintptr_t;
namespace fixture {bool gt=true,current=true,hide_ok=true,mesh_live=true,material_live=true;unsigned hide{},detach{},unbind{},component_unroot{},mesh_unroot{};uptr bound_mesh{};}
namespace playground {
struct Vector {float x{},y{},z{};};
bool instance(uptr address,wchar_t const* label) noexcept {
 return (address==16&&!std::wcscmp(label,L"StaticMesh")&&fixture::mesh_live)||
        (address==24&&!std::wcscmp(label,L"MaterialInterface")&&fixture::material_live);
}
}
namespace nativeequipment {bool current(void*,h::Frame const&) noexcept {return fixture::current;}}
namespace pool {
namespace hand=h;using Identity=h::ObjectIdentity;
struct Owner {Identity world{},pawn{};std::uint64_t game{},native{},registration{},world_generation{},generation{};};
struct Handle {unsigned slot{},generation{};};
enum class Stage {empty,reserved,mesh_created,mesh_rooted,lod_created,prepared,resources_queued,component_created,ready,visible,retiring,failed,original_mesh_ready};
enum class Creation {rejected,pending,ready};
struct Resource {
 Owner owner{};Identity mesh{},component{};unsigned generation{};Stage stage{};
 bool borrowed_mesh{},mesh_root{},component_root{},mesh_bound{},attached{},publication_withdrawn{},
     lighting_bound{},lighting_assigned{},lod_allocation_dispatched{},lod_constructed{};
 playground::Vector original_scale{1,1,1};std::vector<Identity> materials;
};
std::array<Resource,2> resources{};std::atomic<unsigned> outstanding{};bool inside_create{},faulted{};
std::atomic<bool> stop_requested{};unsigned next_generation{1},failure{};
bool gt() noexcept {return fixture::gt;}
bool object(Identity const& id) noexcept {
 return h::identity_valid(id)&&((id.address==16&&fixture::mesh_live)||(id.address==24&&fixture::material_live)||id.address==32);
}
bool hidden(Resource&,bool value) noexcept {assert(value);++fixture::hide;return fixture::hide_ok;}
bool attachment(Resource& r,bool value) noexcept {assert(!value);++fixture::detach;r.attached=false;return true;}
bool pointer_field(Identity const&,wchar_t const* name,uptr& value,bool=false) noexcept {
 value=!std::wcscmp(name,L"StaticMesh")?fixture::bound_mesh:0;return true;
}
bool object_call(Identity const& id,wchar_t const* name,wchar_t const*,wchar_t const*,wchar_t const*,uptr mesh) noexcept {
 if(!std::wcscmp(name,L"SetStaticMesh")){assert(id.address==32);fixture::bound_mesh=mesh;if(!mesh)++fixture::unbind;}return true;
}
bool unroot(Identity const& id,bool& root) noexcept {
 assert(root);root=false;if(id.address==32)++fixture::component_unroot;else if(id.address==16)++fixture::mesh_unroot;else assert(false);return true;
}
#include "weapon_part_bind_mesh.inl"
#include "weapon_part_asset_cleanup.inl"
#include "weapon_part_existing_static_reserve.inl"
}
}
int main(){
 namespace p=pool;
 h::Frame f{};f.key={1,2,3,4,5,6,7,8};f.world={40,0,256,1,2,0,1};f.pawn={48,0,256,2,3,0,1};
 const h::ObjectIdentity mesh{16,0,256,3,4,0,1},material{24,0,256,4,5,0,1},component{32,0,256,5,6,0,1};
 p::Handle handle{};
 fixture::current=false;assert(p::create_existing_static(f,9,mesh,{material},{1,1,1},handle)==p::Creation::rejected);
 fixture::current=true;fixture::mesh_live=false;assert(p::create_existing_static(f,9,mesh,{material},{1,1,1},handle)==p::Creation::rejected);
 fixture::mesh_live=true;fixture::material_live=false;assert(p::create_existing_static(f,9,mesh,{material},{1,1,1},handle)==p::Creation::rejected);
 fixture::material_live=true;assert(p::create_existing_static(f,9,mesh,{material},{1,0,1},handle)==p::Creation::rejected);
 assert(!p::outstanding&&!fixture::mesh_unroot&&!fixture::component_unroot);
 assert(p::create_existing_static(f,9,mesh,{material},{.5f,.5f,.5f},handle)==p::Creation::pending);
 auto& r=p::resources[handle.slot];assert(r.borrowed_mesh&&!r.mesh_root&&r.stage==p::Stage::original_mesh_ready);
 assert(h::same(r.mesh,mesh)&&r.owner.generation==9&&r.original_scale.x==.5f&&p::outstanding==1);
 r.component=component;r.component_root=r.attached=true;r.stage=p::Stage::visible;
 assert(p::bind_mesh(r,mesh.address)&&r.mesh_bound&&fixture::bound_mesh==mesh.address);
 fixture::hide_ok=false;assert(!p::cleanup(r)&&r.component_root&&p::outstanding==1&&!fixture::mesh_unroot);
 fixture::hide_ok=true;assert(p::cleanup(r)&&!p::outstanding);
 assert(fixture::component_unroot==1&&!fixture::mesh_unroot&&fixture::unbind==1&&fixture::detach==1);
 // Existing reconstructed meshes retain their exact established cleanup.
 r={};r.mesh=mesh;r.component=component;r.mesh_root=r.component_root=true;p::outstanding=1;
 assert(p::bind_mesh(r,mesh.address)&&r.mesh_bound);
 assert(p::cleanup(r));assert(fixture::component_unroot==2&&fixture::mesh_unroot==1&&!p::outstanding);
 // Even corrupted mixed responsibility never unroots the original mesh.
 r={};r.mesh=mesh;r.component=component;r.component_root=r.borrowed_mesh=r.mesh_root=true;p::outstanding=1;
 assert(!p::bind_mesh(r,mesh.address));
 assert(!p::cleanup(r)&&r.stage==p::Stage::failed&&p::faulted&&p::failure==81&&p::outstanding==1);
 assert(fixture::mesh_unroot==1);r={};p::outstanding=0;p::faulted=false;
 // A reservation can retire without constructing anything; it never acquired
 // an original root or allocated an original LOD/RHI resource.
 assert(p::create_existing_static(f,10,mesh,{material},{1,1,1},handle)==p::Creation::pending);
 assert(p::cleanup(p::resources[handle.slot]));assert(!p::outstanding&&fixture::mesh_unroot==1);
 std::puts("Existing static part: actual CPU reservation/cleanup, source/scale/material rejection, borrowed mesh never unrooted, owned mesh cleanup retained, mixed responsibility quarantined and failed component cleanup retained; injected engine backend only.");
}
