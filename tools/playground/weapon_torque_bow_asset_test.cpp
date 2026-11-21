#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_lancer_asset_read.inl"
struct BowMemory:Memory {
 uptr component{},asset{},package{};
 BowMemory(){
  auto bow=cls(L"Weap_Bow_base",warfare,weapon_class);
  auto asset_class=cls(L"SkeletalMesh",engine);
  property(weapon_class,L"Mesh",component_type,0x118,8);
  property(mesh_class,L"HiddenGame",bool_type,0x110,4,2);
  property(mesh_class,L"SkeletalMesh",object_type,0x120,8);
  component=node(L"LiveBowMesh",mesh_class,weapon);package=node(L"Locust_Torquebow",package_type);
  asset=node(L"TorqueBow",asset_class,package);
  put(weapon,0x50,bow);put(weapon,0x118,component);put(component,0x120,asset);
  frame.weapon=identity(weapon);frame.key.thread_id=GetCurrentThreadId();tables();
 }
};
}
int main(){
 auto mem=std::make_unique<BowMemory>();auto& m=*mem;lancermain_asset::Evidence out{};
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::basic_hold)==lancermain_asset::Status::ready);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::torque_bow_projectile)==lancermain_asset::Status::ready);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::supported_r2_fire)==lancermain_asset::Status::unsupported_weapon);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::boomshot_projectile)==lancermain_asset::Status::unsupported_weapon);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::supported_instant_fire)==lancermain_asset::Status::unsupported_weapon);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::lancer_only)==lancermain_asset::Status::unsupported_weapon);
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::frag_hold)==lancermain_asset::Status::unsupported_weapon);
 Memory::put(m.package,0x48,m.text(L"WrongBowPackage"));m.tables();
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::basic_hold)==lancermain_asset::Status::unsupported_asset);
 Memory::put(m.package,0x48,m.text(L"Locust_Torquebow"));Memory::put(m.asset,0x48,m.text(L"Locust_Torquebow_Rocket"));m.tables();
 assert(lancermain_asset::read(m.frame,out,lancermain_asset::AssetScope::basic_hold)==lancermain_asset::Status::unsupported_asset);
 std::puts("Torque Bow asset identity/scope regression passed; owned memory only.");
}
