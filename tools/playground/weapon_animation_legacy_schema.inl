// Shared read-only Engine.AnimSequence layout proof; no weapon state or action authority.
namespace weapon_animation_schema {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
// This supported Engine.AnimSequence predates KeyEncodingFormat. Admit its
// known uniform stream by exact native class and a complete, stable UField
// chain proving that selector absent. A failed lookup is never such proof.
bool legacy_uniform_schema(meta::Reader& r,hand::ObjectIdentity const& sequence) noexcept {
    meta::U32 object{},type{},package{},class_type{};
    if(!weaponhand::object(r,sequence)||!r.path(sequence.address,object))return false;
    const auto klass=r.result.identities[object].klass;
    meta::Structure initial{};
    if(!r.structure(klass,initial)||!r.named(initial.identity,L"AnimSequence")||
       !r.path(r.result.identities[initial.identity].klass,class_type)||!weaponhand::core_type(r,class_type,L"Class")||
       !r.path(r.result.identities[initial.identity].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer)return false;
    struct Link {uptr address{},next{};};
    std::array<Link,meta::max_fields> links{};meta::U32 count{};
    auto child=initial.children;
    while(child){
        if(count==links.size())return false;
        for(meta::U32 i=0;i<count;++i)if(links[i].address==child)return false;
        if(!r.path(child,type)||r.result.identities[type].outer!=klass||
           !weaponhand::derives(r,child,L"Field",L"Core")||r.named(type,L"KeyEncodingFormat",false))return false;
        auto& link=links[count++];link.address=child;
        if(!r.at(child,0x68,link.next))return false;
        child=link.next;
    }
    if(!count)return false;
    // Close the absence proof: a changed interior link, class/outer/name or
    // selector inserted during the read cannot inherit the old conclusion.
    for(meta::U32 i=0;i<count;++i){uptr next{};
        if(!r.path(links[i].address,type)||r.result.identities[type].outer!=klass||
           !weaponhand::derives(r,links[i].address,L"Field",L"Core")||r.named(type,L"KeyEncodingFormat",false)||
           !r.at(links[i].address,0x68,next)||next!=links[i].next)return false;
    }
    meta::Structure after{};
    return r.path(sequence.address,object)&&r.result.identities[object].klass==klass&&
        r.structure(klass,after)&&after.super==initial.super&&after.children==initial.children&&after.extent==initial.extent;
}

}
