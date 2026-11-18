// Exact stock scope declarations on the existing GT dispatcher. This layer
// owns neither input nor camera/rig publication. The scope lifecycle caller
// must journal BEFORE setters and retain its original action lifetime.
namespace longshot_scope_original {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
enum class Method:unsigned {target,zoom,visibility,fov};
struct Parameter {hand::ObjectIdentity id{};unsigned offset{},mask{};std::uint64_t flags{};};
struct Entry {
    Method method{};hand::ObjectIdentity function{},owner{},receiver{};
    std::array<Parameter,2> parameters{};unsigned count{},bytes{},returned{0xffff};
    std::uint64_t fingerprint{};
};
const wchar_t* label(Method m) noexcept {
    switch(m){case Method::target:return L"SetTargetingMode";case Method::zoom:return L"SetZoomed";
        case Method::visibility:return L"SetOwnerNoSee";default:return L"GetTargetingFOV";}
}
bool function_named(meta::Reader& r,unsigned slot,Method m) noexcept {
    switch(m){case Method::target:return r.named(slot,L"SetTargetingMode");case Method::zoom:return r.named(slot,L"SetZoomed");
        case Method::visibility:return r.named(slot,L"SetOwnerNoSee");default:return r.named(slot,L"GetTargetingFOV");}
}
bool identify(meta::Reader& r,uptr at,hand::ObjectIdentity& out) noexcept {
    unsigned slot{};if(!r.path(at,slot))return false;const auto& id=r.result.identities[slot];
    out={id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};return hand::identity_valid(out);
}
bool bind(hand::ObjectIdentity const& receiver,Method method,Entry& out) noexcept {
    out={};
    if(!weaponhand::current_gt(nullptr))return false;
    playground::Args args(receiver.address,label(method));
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    unsigned fn{},kind{},owner{},package{},owner_kind{};
    if(!args.valid||!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,receiver)||
       !r.path(args.r.function,fn)||!function_named(r,fn,method)||!r.path(schema.identities[fn].klass,kind)||
       !weaponhand::core_type(r,kind,L"Function")||!r.path(schema.identities[fn].outer,owner)||
       !(method==Method::target?r.named(owner,L"WarPC"):r.named(owner,L"Weap_SniperRifle"))||!r.path(schema.identities[owner].klass,owner_kind)||
       !weaponhand::core_type(r,owner_kind,L"Class")||!r.path(schema.identities[owner].outer,package)||
       !r.named(package,L"WarfareGame")||schema.identities[package].outer)return false;
    Entry value{};value.method=method;value.receiver=receiver;
    meta::Structure body{};meta::Table script{};unsigned flags{};std::uint16_t bytes{},ret{},native{};uptr exec{};
    std::uint64_t script_hash{};
    if(!identify(r,args.r.function,value.function)||!identify(r,schema.identities[owner].address,value.owner)||
       !r.structure(args.r.function,body)||!r.at(args.r.function,0x106,bytes)||!bytes||bytes>4096||
       bytes!=args.r.size||!r.at(args.r.function,0x108,ret)||!r.at(args.r.function,0xf4,flags)||
       !r.at(args.r.function,0xf8,native)||native||(flags&0x400)||
       !r.at(args.r.function,0x114,exec)||exec!=base+0x1d6ca0||!r.at(args.r.function,0x8c,script)||
       script.count<=0||script.count>script.capacity||script.capacity>131072||
       !weaponshot::hash_script(script.data,unsigned(script.count),script_hash))return false;
    value.bytes=bytes;value.returned=ret;
    unsigned found{},count{};std::array<uptr,meta::max_fields> seen{};
    for(auto p=body.children;p;){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        meta::Property prop{};
        if(!r.property(p,prop)||schema.identities[prop.identity].outer!=args.r.function||prop.dimension!=1||
           !meta::property_fits(prop,body.extent))return false;
        if(prop.flags&0x80){
            if(value.count==value.parameters.size()||prop.element_bytes!=4||!meta::property_fits(prop,bytes))return false;
            Parameter parameter{};parameter.offset=prop.offset;parameter.flags=prop.flags;unsigned bit{};
            if(!identify(r,p,parameter.id))return false;
            if(method==Method::fov){
                if(!weaponhand::core_type(r,prop.type,L"FloatProperty"))return false;
                if(r.named(prop.identity,L"BaseFOV")&&(prop.flags&0x590)==0x80)bit=1;
                else if(r.named(prop.identity,L"ReturnValue")&&(prop.flags&0x590)==0x580&&prop.offset==ret)bit=2;
                else return false;
            }else{
                const bool named=method==Method::target?r.named(prop.identity,L"bNewIsTargeting"):
                    method==Method::zoom?r.named(prop.identity,L"bNewZoomed"):r.named(prop.identity,L"bNewOwnerSee");
                if(!named||!weaponhand::core_type(r,prop.type,L"BoolProperty")||
                   (prop.flags&0x590)!=0x80||!r.at(p,0xd0,parameter.mask)||
                   !parameter.mask||(parameter.mask&(parameter.mask-1)))return false;
                bit=1;
            }
            if(found&bit)return false;found|=bit;
            for(unsigned i=0;i<value.count;++i){const auto& q=value.parameters[i];
                if(parameter.offset<q.offset+4&&q.offset<parameter.offset+4)return false;}
            value.parameters[value.count++]=parameter;
        }
        p=prop.next;
    }
    if(found!=(method==Method::fov?3u:1u)||(method!=Method::fov&&ret!=0xffff)||
       !unified_weapon_fire::retained_metadata(r))return false;
    // The class lookup must still resolve this exact stock method. This
    // rejects a state/derived override rather than invoking an unproved body.
    playground::Args closing(receiver.address,label(method));
    if(!closing.valid||closing.r.function!=args.r.function||closing.r.size!=bytes)return false;
    unified_weapon_fire::Fingerprint hash;
    for(const auto& id:{value.receiver,value.owner,value.function}){
        hash.word(id.address);hash.word(id.outer);hash.word(id.klass);hash.word(id.registry_index);hash.word(id.name_index);hash.word(id.name_number);}
    hash.word(unsigned(method));hash.word(bytes);hash.word(ret);hash.word(flags);hash.word(script.data);
    hash.word(unsigned(script.count));hash.word(script_hash);
    for(unsigned i=0;i<value.count;++i){const auto& p=value.parameters[i];
        hash.word(p.id.address);hash.word(p.offset);hash.word(p.mask);hash.word(p.flags);}
    value.fingerprint=hash.value;out=value;return true;
}
bool same(Entry const& a,Entry const& b) noexcept {
    return a.fingerprint&&a.fingerprint==b.fingerprint&&a.method==b.method&&
        hand::same(a.receiver,b.receiver)&&hand::same(a.function,b.function)&&hand::same(a.owner,b.owner);
}
bool actors(hand::Frame const& source) noexcept {
    for(const auto& id:{source.world,source.controller,source.pawn,source.weapon})if(!weapon_part_assets::object(id))return false;
    return playground::pointer(source.controller.address,L"Pawn")==source.pawn.address&&
        playground::pointer(source.pawn.address,L"WorldInfo")==source.world.address&&
        playground::pointer(source.weapon.address,L"Instigator")==source.pawn.address;
}
bool receiver_matches(hand::Frame const& source,Entry const& e) noexcept {
    return hand::same(e.receiver,e.method==Method::target?source.controller:source.weapon);
}
bool set(hand::Frame const& source,Entry const& entry,bool value,bool* dispatched=nullptr,
    hand::ExistingActionLifetime const* cleanup=nullptr) noexcept {
    if(dispatched)*dispatched=false;
    if(entry.method==Method::fov||entry.count!=1||!entry.parameters[0].mask||!receiver_matches(source,entry))return false;
    const auto permitted=[&]() noexcept {
        // An old lifetime permits only an already-journaled OFF restoration.
        // It cannot enable targeting, zoom, or visibility after source loss.
        return cleanup?!value&&nativeequipment::restoration_quiet()&&weaponhand::client.existing_action_lifetime_on_gt(*cleanup):
            nativeequipment::current(nullptr,source);
    };
    Entry current{};
    if(!weaponhand::current_gt(nullptr)||!permitted()||!actors(source)||
       (!cleanup&&playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address)||
       !bind(entry.receiver,entry.method,current)||!same(entry,current))return false;
    Request request{};request.object=entry.receiver.address;request.function=entry.function.address;request.size=entry.bytes;
    const unsigned bits=value?entry.parameters[0].mask:0;
    std::memcpy(request.params+entry.parameters[0].offset,&bits,sizeof bits);
    if(!permitted())return false;
    if(dispatched)*dispatched=true;
    return invoke(&request);
}
bool fov(hand::Frame const& source,Entry const& entry,float base_fov,float& out) noexcept {
    out=0;
    if(entry.method!=Method::fov||entry.count!=2||!receiver_matches(source,entry)||
       !std::isfinite(base_fov)||base_fov<2||base_fov>179||!nativeequipment::current(nullptr,source)||
       !actors(source)||playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address)return false;
    Entry current{};if(!bind(entry.receiver,entry.method,current)||!same(entry,current))return false;
    Request request{};request.object=entry.receiver.address;request.function=entry.function.address;request.size=entry.bytes;
    for(unsigned i=0;i<entry.count;++i)if(!(entry.parameters[i].flags&0x400))
        std::memcpy(request.params+entry.parameters[i].offset,&base_fov,sizeof base_fov);
    if(!nativeequipment::current(nullptr,source)||!invoke(&request))return false;
    float result{};std::memcpy(&result,request.params+entry.returned,sizeof result);
    if(!std::isfinite(result)||result<2||result>90||!nativeequipment::current(nullptr,source))return false;
    out=result;return true;
}
} // namespace longshot_scope_original
