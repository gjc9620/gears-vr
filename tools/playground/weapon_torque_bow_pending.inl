// Original InventoryManager.PendingFire is an array of 32-bit integers.
// This read grants no input/action permission. The caller retains both
// properties and brackets array identity/values in its complete double read.
namespace torquebow_pending {
namespace meta=gripdamage_metadata;
struct Observed {
    meta::Property property{},element{};meta::Table values{};
    std::int32_t primary{};unsigned gate{};
};
bool read(meta::Reader& r,uptr manager,Observed& out) noexcept {
    out={};weaponhand::BoolField field{};uptr inner{};
    out.gate=1;
    if(!weaponhand::field(r,manager,L"PendingFire",L"ArrayProperty",16,field))return false;
    out.property=field.property;
    out.gate=2;if(!r.at(manager,field.property.offset,out.values))return false;
    const auto& a=out.values;
    out.gate=3;
    if(a.count<1||a.count>32||a.capacity<a.count||a.capacity>128||!a.data)return false;
    const auto property=r.result.identities[field.property.identity].address;
    out.gate=4;
    if(!r.at(property,0xd0,inner)||!r.property(inner,out.element))return false;
    out.gate=5;
    if(r.result.identities[out.element.identity].outer!=property||
       !weaponhand::core_type(r,out.element.type,L"IntProperty")||
       out.element.dimension!=1||out.element.element_bytes!=4||out.element.offset)return false;
    out.gate=6;
    if(!copy(&out.primary,a.data,sizeof out.primary)||out.primary<0||out.primary>1)return false;
    out.gate=0;return true;
}
}
