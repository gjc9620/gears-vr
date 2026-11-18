#pragma once
// Original R2 PreButtonPress owns both bFire and bHoldingButton_R2. Only the
// existing journalled non-firing cancel owner may clear this request byte.
namespace torquebow_controller_fire {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using Result=gears_weapon::TorqueBowCallResult;
struct Value {weaponhand::BoolField field{};unsigned char byte{};};
bool read(meta::Reader& r,hand::ObjectIdentity const& controller,Value& out) noexcept {
    out={};Value value{};unsigned package{};
    if(!weaponhand::object(r,controller)||
       !weaponhand::field(r,controller.address,L"bFire",L"ByteProperty",1,value.field)||
       !r.named(value.field.owner,L"Controller")||
       !r.path(r.result.identities[value.field.owner].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer||!r.at(controller.address,value.field.property.offset,value.byte))return false;
    bool retained=false;
    for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==value.field.property.identity){
        if(!meta::same_property(r.result.fields[i],value.field.property))return false;
        retained=true;break;
    }
    if(!retained){
        if(r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=value.field.property;
    }
    out=value;return true;
}
template<class Current>
Result clear_owned(hand::ObjectIdentity const& controller,Current const& current) noexcept {
    if(!weaponhand::current_gt(nullptr)||!current())return Result::rejected;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    Value before{};unsigned slot{};meta::Structure receiver{};
    if(!meta::tables(base,schema.objects,schema.names)||!read(r,controller,before)||
       !r.path(controller.address,slot)||!r.structure(schema.identities[slot].klass,receiver))return Result::rejected;
    const auto offset=before.field.property.offset,word_offset=offset&~3u;
    if(word_offset>receiver.extent||receiver.extent-word_offset<4||
       controller.address>UINTPTR_MAX-word_offset||((controller.address+word_offset)&3))return Result::rejected;
    const auto shift=(offset&3u)*8u;const meta::U32 mask=0xffu<<shift;
    meta::U32 checked{};
    if(!r.at(controller.address,word_offset,checked)||((checked&mask)>>shift)!=before.byte||
       !current()||!unified_weapon_fire::retained_metadata(r))return Result::rejected;
    const auto wanted=checked&~mask;const bool attempted=wanted!=checked;
    if(attempted){
        bool exchanged{};
        // Same compare/exchange primitive as the existing packed R2-bit stop.
        // Preserve every neighbouring byte; a changed word refuses the store.
        if(!unified_weapon_fire::compare_word(controller.address+word_offset,checked,wanted,exchanged))return Result::unknown;
        if(!exchanged)return Result::rejected;
    }
    Value after{};
    const bool verified=read(r,controller,after)&&after.byte==0&&
        after.field.object==before.field.object&&after.field.owner==before.field.owner&&
        meta::same_property(after.field.property,before.field.property)&&
        unified_weapon_fire::retained_metadata(r)&&current();
    return verified?Result::accepted:attempted?Result::unknown:Result::rejected;
}
}
