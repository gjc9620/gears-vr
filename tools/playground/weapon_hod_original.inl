// HOD original-call adapter. Composition stays on the existing GT dispatcher.
// No Hook, skeletal writer, target validity override, timer or ammo mutation.
namespace hod_original {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
enum class Method:unsigned {press,release,end_fire};
enum class Button:unsigned {fire,aim};
struct Entry {
    Method method{};hand::ObjectIdentity receiver{},function{},parameter{};
    unsigned bytes{},offset{};std::uint64_t fingerprint{};
};
const wchar_t* label(Method m) noexcept {
    return m==Method::press?L"ButtonPress":m==Method::release?L"ButtonRelease":L"EndFire";
}
bool identify(meta::Reader& r,uptr address,hand::ObjectIdentity& out) noexcept {
    unsigned slot{};if(!r.path(address,slot))return false;
    out=weaponshot::identity(r.result.identities[slot]);return hand::identity_valid(out);
}
bool bind(hand::ObjectIdentity const& receiver,Method method,Entry& out) noexcept {
    out={};if(!weaponhand::current_gt(nullptr))return false;
    const bool end=method==Method::end_fire;
    playground::Args args(receiver.address,label(method));
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    unsigned fn{},kind{},owner{},owner_kind{},package{};
    if(!args.valid||!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,receiver)||
       !r.path(args.r.function,fn)||
       !(end?r.named(fn,L"EndFire"):method==Method::press?r.named(fn,L"ButtonPress"):r.named(fn,L"ButtonRelease"))||
       !r.path(schema.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(schema.identities[fn].outer,owner)||!r.path(schema.identities[owner].klass,owner_kind)||
       !weaponhand::core_type(r,owner_kind,L"Class")||!r.path(schema.identities[owner].outer,package)||
       schema.identities[package].outer||
       !(end?(r.named(owner,L"WEAP_HammerOfDawn")&&r.named(package,L"warfaregamecontentweapons")):
              (r.named(owner,L"WarPC")&&r.named(package,L"WarfareGame"))))return false;
    Entry value{};value.method=method;value.receiver=receiver;
    meta::Structure body{};meta::Table script{};std::uint16_t bytes{},ret{},native{};unsigned flags{};uptr exec{};
    std::uint64_t hash{};
    if(!identify(r,args.r.function,value.function)||!r.structure(args.r.function,body)||
       !r.at(args.r.function,0x106,bytes)||bytes!=(end?1:8)||bytes!=args.r.size||
       !r.at(args.r.function,0x108,ret)||ret!=0xffff||!r.at(args.r.function,0xf8,native)||native||
       !r.at(args.r.function,0xf4,flags)||(flags&0x400)||!r.at(args.r.function,0x114,exec)||exec!=base+0x1d6ca0||
       !r.at(args.r.function,0x8c,script)||script.count<=0||script.count>script.capacity||script.capacity>131072||
       !weaponshot::hash_script(script.data,unsigned(script.count),hash))return false;
    std::array<uptr,meta::max_fields> seen{};unsigned count{},parameters{};
    for(auto child=body.children;child;){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;
        seen[count++]=child;meta::Property p{};
        if(!r.property(child,p)||schema.identities[p.identity].outer!=args.r.function||
           !meta::property_fits(p,body.extent))return false;
        if(p.flags&0x80){
            if(++parameters!=1||p.dimension!=1||(p.flags&0x590)!=0x80||
               !meta::property_fits(p,bytes)||p.element_bytes!=(end?1u:8u)||
               !(end?(r.named(p.identity,L"FireModeNum")&&weaponhand::core_type(r,p.type,L"ByteProperty")):
                      (r.named(p.identity,L"ButtonName")&&weaponhand::core_type(r,p.type,L"NameProperty")))||
               !identify(r,child,value.parameter))return false;
            value.offset=p.offset;
        }
        child=p.next;
    }
    if(parameters!=1||!unified_weapon_fire::retained_metadata(r))return false;
    playground::Args closing(receiver.address,label(method));
    if(!closing.valid||closing.r.function!=args.r.function||closing.r.size!=bytes)return false;
    unified_weapon_fire::Fingerprint fingerprint;unified_weapon_fire::schema_fingerprint(schema,fingerprint);
    fingerprint.word(unsigned(method));fingerprint.word(hash);fingerprint.word(bytes);fingerprint.word(flags);
    fingerprint.word(exec);fingerprint.word(script.data);fingerprint.word(unsigned(script.count));
    fingerprint.word(unsigned(script.capacity));fingerprint.word(value.offset);
    value.bytes=bytes;value.fingerprint=fingerprint.value;out=value;return true;
}
bool same(Entry const& a,Entry const& b) noexcept {
    return a.fingerprint&&a.fingerprint==b.fingerprint&&a.method==b.method&&a.bytes==b.bytes&&a.offset==b.offset&&
        hand::same(a.receiver,b.receiver)&&hand::same(a.function,b.function)&&hand::same(a.parameter,b.parameter);
}
bool actors(hand::Frame const& f) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField pc{},world{},instigator{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,f.world)&&
        weaponhand::object(r,f.controller)&&weaponhand::object(r,f.pawn)&&weaponhand::object(r,f.weapon)&&
        weaponhand::derives(r,f.weapon.address,L"WEAP_HammerOfDawn",L"warfaregamecontentweapons")&&
        weaponhand::pointer(r,f.controller.address,L"Pawn",f.pawn.address,pc)&&
        weaponhand::pointer(r,f.pawn.address,L"WorldInfo",f.world.address,world)&&
        weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,instigator)&&
        unified_weapon_fire::retained_metadata(r);
}
// Caller journals the original lifetime BEFORE dispatch. Only OFF operations
// can use that exact retained lifetime after tracking loss or a weapon switch.
// EndFire is a same-selected-owner stop fallback, never a new-owner cleanup.
bool execute(hand::Frame const& source,Entry const& entry,Name r2,bool& dispatched,
             hand::ExistingActionLifetime const* cleanup=nullptr,Button button=Button::fire) noexcept {
    dispatched=false;const bool end=entry.method==Method::end_fire;
    if(unsigned(button)>1||(end&&button!=Button::fire))return false;
    if(!hand::same(entry.receiver,end?source.weapon:source.controller))return false;
    const auto allowed=[&]() noexcept {
        return cleanup?entry.method!=Method::press&&nativeequipment::restoration_quiet()&&
            weaponhand::client.existing_action_lifetime_on_gt(*cleanup):nativeequipment::current(nullptr,source);
    };
    if(!weaponhand::current_gt(nullptr)||!allowed()||!actors(source))return false;
    // EndFire also routes through BeamManager by PAWN, not weapon. An old
    // HOD object cannot stop after selection has moved: it could abort the
    // new owner's beam. Equipment must yield BEFORE changing this link.
    // A retained lifetime alone does not prove ownership of a current beam.
    if(playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address)return false;
    Entry current{};if(!bind(entry.receiver,entry.method,current)||!same(entry,current))return false;
    Request request{};request.object=entry.receiver.address;request.function=entry.function.address;request.size=entry.bytes;
    if(!end){
        meta::Table object_table{},names{};meta::NameText text{};
        if(r2.number||!meta::tables(base,object_table,names)||!meta::fresh_name(names,r2.index,text)||
           !(button==Button::aim?meta::exact_text(text,L"L2"):meta::exact_text(text,L"R2")))return false;
        std::memcpy(request.params+entry.offset,&r2,sizeof r2);
    } // EndFire mode zero is the authenticated stock primary mode.
    if(!allowed())return false;dispatched=true;return invoke(&request);
}
} // namespace hod_original
