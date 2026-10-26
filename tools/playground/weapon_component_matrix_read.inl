// Actual typed LocalToWorld metadata read with stable descriptor/identity/matrix repeats.
namespace nativeequipment_component {
namespace hand=gears_weapon_hand;
namespace sampling=gears_weapon_component_samples;
namespace meta=weaponhand::meta;
using Matrix=std::array<float,16>;
template<std::size_t N>bool matrix(hand::ObjectIdentity const& object,wchar_t const (&label)[N],Matrix& out) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField descriptor{};uptr inner{};unsigned inner_slot{},inner_type{},extent{};Matrix before{},after{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,object)||
       !weaponhand::field(r,object.address,label,L"StructProperty",64,descriptor))return false;
    const auto property=schema.identities[descriptor.property.identity].address;
    if(!r.at(property,0xd0,inner)||!r.path(inner,inner_slot)||!r.named(inner_slot,L"Matrix")||
       !r.path(schema.identities[inner_slot].klass,inner_type)||!weaponhand::core_type(r,inner_type,L"ScriptStruct")||
       !r.at(inner,0x88,extent)||extent!=64||!r.at(object.address,descriptor.property.offset,before))return false;
    for(float v:before)if(!std::isfinite(v))return false;
    weaponhand::BoolField final_descriptor{};uptr final_inner{};unsigned final_extent{};
    if(!weaponhand::field(r,object.address,label,L"StructProperty",64,final_descriptor)||
       !meta::same_property(descriptor.property,final_descriptor.property)||descriptor.owner!=final_descriptor.owner||
       !r.at(property,0xd0,final_inner)||final_inner!=inner||!r.at(inner,0x88,final_extent)||final_extent!=extent||
       !r.at(object.address,descriptor.property.offset,after)||after!=before||!weaponhand::object(r,object))return false;
    const auto count=schema.identity_count;
    for(unsigned i=0;i<count;++i){unsigned current{};if(!r.identity(schema.identities[i].address,current)||current!=i)return false;}
    meta::Table objects{},names{};
    if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
       r.result.failure.reason!=meta::Reason::none)return false;
    out=before;return true;
}
}
