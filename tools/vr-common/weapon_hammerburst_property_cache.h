#pragma once
#include "weapon_hand_participant.h"
// Reader Property.identity/type are indices LOCAL TO A SNAPSHOT. Never use
// those ordinals as persistent property identity between preflight and firing.
namespace gears_hammerburst_property_cache {
template<class Property>bool layout(Property const& a,Property const& b) noexcept {
    return a.next==b.next&&a.flags==b.flags&&a.dimension==b.dimension&&
        a.element_bytes==b.element_bytes&&a.offset==b.offset;
}
template<class Reader,class Property>
bool same(Reader& r,Property const& current,Property const& saved,
          gears_weapon_hand::ObjectIdentity const& descriptor,
          gears_weapon_hand::ObjectIdentity const& type) noexcept {
    if(descriptor.known!=1||type.known!=1||!descriptor.address||!type.address||
       current.identity>=r.result.identity_count||current.type>=r.result.identity_count||!layout(current,saved))return false;
    const auto id=[&](unsigned slot){auto const& v=r.result.identities[slot];
        return gears_weapon_hand::ObjectIdentity{v.address,v.outer,v.klass,v.index,v.name.index,v.name.number,1};};
    return gears_weapon_hand::same(id(current.identity),descriptor)&&gears_weapon_hand::same(id(current.type),type);
}
}
