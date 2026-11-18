// Current-instance Smoke class/ammo/effect declarations only. No engine call,
// object construction, timer, inventory mutation or emitter implementation.
namespace smoke_metadata {
namespace meta=gripdamage_metadata;namespace hand=gears_weapon_hand;
struct Evidence {
    hand::ObjectIdentity weapon_type{},projectile_type{},ammo_type{},emitter_type{};
    std::uint64_t effects{},audio{};
};
template<std::size_t N>bool exact_class(meta::Reader& r,uptr value,wchar_t const(&label)[N],
                                      hand::ObjectIdentity& out) noexcept {
    unsigned slot{},kind{},package{};
    return value&&r.path(value,slot)&&r.named(slot,label)&&
        r.path(r.result.identities[slot].klass,kind)&&weaponhand::core_type(r,kind,L"Class")&&
        r.path(r.result.identities[slot].outer,package)&&r.named(package,L"WarfareGame")&&
        !r.result.identities[package].outer&&weaponhand::derives(r,r.result.identities[slot].outer,L"Package",L"Core")&&
        smoke_original::identify(r,value,out);
}
bool emitter_class(meta::Reader& r,hand::ObjectIdentity& out) noexcept {
    static hand::ObjectIdentity cached{};
    hand::ObjectIdentity current{};
    if(cached.address&&exact_class(r,cached.address,L"Emit_SmokeGrenade",current)&&hand::same(cached,current)){
        out=current;return true;
    }
    const auto& table=r.result.objects;
    if(!table.data||table.count<=0||table.count>1000000||table.count>table.capacity)return false;
    for(unsigned i=0;i<unsigned(table.count);++i){
        uptr object{};meta::Identity raw{};meta::NameText name{};
        if(!r.at(table.data,i*8,object))return false;
        if(!object||!meta::raw_identity(r.result.objects,r.result.names,object,raw,name)||
           !meta::exact_text(name,L"Emit_SmokeGrenade"))continue;
        if(!exact_class(r,object,L"Emit_SmokeGrenade",current))continue;
        cached=current;out=current;return true;
    }
    return false;
}
template<std::size_t N>bool script(meta::Reader& r,uptr owner,wchar_t const(&label)[N],
                                 unified_weapon_fire::Fingerprint& fingerprint) noexcept {
    uptr function{},exec{};unsigned slot{},kind{},flags{};std::uint16_t native{};
    meta::Table body{};std::uint64_t hash{};
    if(!smoke_original::child(r,owner,label,function)||!r.path(function,slot)||
       r.result.identities[slot].outer!=owner||!r.path(r.result.identities[slot].klass,kind)||
       !weaponhand::core_type(r,kind,L"Function")||!r.at(function,0xf4,flags)||(flags&0x400)||
       !r.at(function,0xf8,native)||native||!r.at(function,0x114,exec)||exec!=base+0x1d6ca0||
       !r.at(function,0x8c,body)||body.count<=0||body.count>body.capacity||body.capacity>131072||
       !weaponshot::hash_script(body.data,unsigned(body.count),hash))return false;
    fingerprint.word(function);fingerprint.word(owner);fingerprint.word(body.data);
    fingerprint.word(unsigned(body.count));fingerprint.word(hash);return true;
}
bool read_after_current_check(hand::Frame const& frame,Evidence& out,unsigned* refusal_stage=nullptr) noexcept {
    // Precondition: the adjacent caller has already accepted the exact entry
    // current() guard for this same Frame. This body preserves all metadata
    // work and its closing current() check; it never grants action authority.
    out={};if(refusal_stage)*refusal_stage=1;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    Evidence value{};unsigned weapon{};weaponhand::BoolField ammo{};uptr ammo_value{},again{},war_weapon{};
    if(refusal_stage)*refusal_stage=2;
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,frame.weapon)||
       !r.path(frame.weapon.address,weapon)||
       !exact_class(r,schema.identities[weapon].klass,L"Weap_SmokeGrenade",value.weapon_type)||
       !smoke_original::projectile_class(frame,value.projectile_type)||
       !smoke_original::declaring_class(r,frame.weapon.address,L"WarWeapon",war_weapon)||
       !weaponhand::field(r,frame.weapon.address,L"AmmoTypeClass",L"ClassProperty",8,ammo)||
       schema.identities[ammo.owner].address!=war_weapon||
       !r.at(frame.weapon.address,ammo.property.offset,ammo_value)||
       !exact_class(r,ammo_value,L"WarAmmoType_GrenadeSmoke",value.ammo_type)||
       !emitter_class(r,value.emitter_type))return false;
    unified_weapon_fire::Fingerprint effects,audio;
    // These declarations remain engine-owned. Their fingerprint authenticates
    // the same observed code across this binding; it does not establish actual
    // later emitter visibility/audio output, which needs a game recording.
    if(refusal_stage)*refusal_stage=3;
    if(!script(r,value.projectile_type.address,L"StopBounce",effects)||
       !script(r,value.projectile_type.address,L"GrenadeExplosion",effects)||
       !script(r,value.projectile_type.address,L"AttachInit",effects)||
       !script(r,value.emitter_type.address,L"PostBeginPlay",effects)||
       !script(r,value.emitter_type.address,L"ForceSmokeOff",effects)||
       !script(r,value.emitter_type.address,L"OnParticleSystemFinished",effects)||
       !script(r,value.emitter_type.address,L"TurnOnSmokeSpewSound",audio)||
       !script(r,value.emitter_type.address,L"TurnOffSmokeSpewSound",audio))return false;
    if(refusal_stage)*refusal_stage=4;
    if(!r.at(frame.weapon.address,ammo.property.offset,again)||again!=ammo_value||
       !unified_weapon_fire::retained_metadata(r))return false;
    if(refusal_stage)*refusal_stage=5;
    if(!nativeequipment::current(nullptr,frame))return false;
    value.effects=effects.value;value.audio=audio.value;out=value;
    if(refusal_stage)*refusal_stage=0;
    return true;
}
bool read(hand::Frame const& frame,Evidence& out,unsigned* refusal_stage=nullptr) noexcept {
    out={};if(refusal_stage)*refusal_stage=1;
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::current(nullptr,frame))return false;
    return read_after_current_check(frame,out,refusal_stage);
}
bool same(Evidence const& a,Evidence const& b) noexcept {
    return hand::same(a.weapon_type,b.weapon_type)&&hand::same(a.projectile_type,b.projectile_type)&&
        hand::same(a.ammo_type,b.ammo_type)&&hand::same(a.emitter_type,b.emitter_type)&&
        a.effects&&a.effects==b.effects&&a.audio&&a.audio==b.audio;
}
} // namespace smoke_metadata
