#include "weapon_component_pose_cohort.h"
namespace nativeequipment_component {
namespace cohort=gears_weapon_component_pose;
bool pose_cohort(hand::ObjectIdentity const& mesh,Matrix const& ctw,cohort::Snapshot& out) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField selected{},again{};hand::ObjectIdentity asset{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,mesh)||
       !weaponhand::derives(r,mesh.address,L"SkeletalMeshComponent",L"Engine")||
       !weaponhand::read_pointer(r,mesh.address,L"SkeletalMesh",selected)||selected.property.offset!=0x218||
       !lancermain_asset::identify(r,selected.value,asset)||
       !weaponhand::derives(r,asset.address,L"SkeletalMesh",L"Engine"))return false;
    cohort::Snapshot candidate{};
    if(!cohort::capture([](cohort::Address address,void* destination,std::size_t bytes){return copy(destination,address,bytes);},
        mesh.address,asset.address,ctw,candidate)||
       !weaponhand::read_pointer(r,mesh.address,L"SkeletalMesh",again)||!meta::same_pointer(selected,again)||
       !weaponhand::object(r,mesh)||!weaponhand::object(r,asset))return false;
    const auto count=schema.identity_count;
    for(unsigned i=0;i<count;++i){unsigned current{};if(!r.identity(schema.identities[i].address,current)||current!=i)return false;}
    meta::Table objects{},names{};
    if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
       r.result.failure.reason!=meta::Reason::none)return false;
    out=candidate;return true;
}
unsigned pose_change_mask(cohort::Snapshot const& a,cohort::Snapshot const& b) noexcept {
    unsigned mask{};
    if(a.epoch!=b.epoch)mask|=1;
    if(a.space_values!=b.space_values)mask|=2;
    if(a.local_values!=b.local_values)mask|=4;
    if(a.reference_values!=b.reference_values)mask|=8;
    if(a.component!=b.component)mask|=16;
    if(a.mesh!=b.mesh||a.asset!=b.asset||a.master!=b.master)mask|=32;
    if(!cohort::same(a,b)&&!mask)mask|=64;
    return mask;
}
} // namespace nativeequipment_component
