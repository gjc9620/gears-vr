// Original Frag declarations and normal-GT calls. No alternate input producer,
// pose publication, projectile implementation or arbitrary native entry address.
// Include after the existing unified-fire/metadata and original-call helpers.
namespace frag_original {
namespace hand=gears_weapon_hand;
namespace meta=gripdamage_metadata;
enum class Method:unsigned {prepare,stop,notify,base_throw,origin,velocity,finished,empty,press,release,target,mode};
enum class Type:unsigned {name,boolean,vector,projectile,byte};
struct Parameter {
    hand::ObjectIdentity identity{};Type type{};unsigned offset{},bytes{},mask{};
    std::uint64_t flags{};
};
struct Entry {
    Method method{};hand::ObjectIdentity function{},owner{},declaring{};
    uptr script{};unsigned script_bytes{},parameter_bytes{},return_offset{0xffff},count{};
    std::uint64_t script_hash{},fingerprint{};
    std::array<Parameter,8> parameters{};
};
template<std::size_t N>bool named(meta::Reader& r,unsigned slot,wchar_t const(&label)[N]) noexcept {
    return r.named(slot,label);
}
bool identify(meta::Reader& r,uptr address,hand::ObjectIdentity& out) noexcept {
    unsigned slot{};if(!r.path(address,slot))return false;
    const auto& i=r.result.identities[slot];out={i.address,i.outer,i.klass,i.index,i.name.index,i.name.number,1};
    return hand::identity_valid(out);
}
template<std::size_t N>bool child(meta::Reader& r,uptr owner,wchar_t const(&label)[N],uptr& out) noexcept {
    out=0;meta::Structure structure{};
    unsigned identity{},kind{};if(!r.path(owner,identity)||!r.path(r.result.identities[identity].klass,kind))return false;
    if(weaponhand::core_type(r,kind,L"State")){
        // UState shares these UStruct members, but the existing generic
        // Reader deliberately accepts only Class/Function. Keep its policy
        // unchanged; admit only this explicitly selected original state.
        structure.identity=identity;
        if(!r.at(owner,0x60,structure.super)||!r.at(owner,0x80,structure.children)||!r.at(owner,0x88,structure.extent))return false;
    }else if(!r.structure(owner,structure))return false;
    std::array<uptr,meta::max_search> seen{};unsigned count{};
    for(auto p=structure.children;p;){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;
        seen[count++]=p;unsigned slot{};uptr next{};
        // WarPC contains hundreds of fields before its button methods. Match
        // skipped children through the existing bounded raw metadata reader,
        // as Reader::member does, instead of filling its retained identity/
        // name capacity with unrelated declarations. Retain the selected path;
        // the closing selector traverses the same actual owner again.
        meta::Identity id{};meta::NameText text{};
        if(!meta::raw_identity(r.result.objects,r.result.names,p,id,text)||id.outer!=owner||
           !r.at(p,0x68,next))return false;
        if(meta::exact_text(text,label)){
            if(!r.path(p,slot)||r.result.identities[slot].outer!=owner||!r.named(slot,label))return false;
            out=p;return true;
        }p=next;
    }
    return false;
}
template<std::size_t N,std::size_t K>bool declaring_in(meta::Reader& r,uptr receiver,wchar_t const(&label)[N],
    wchar_t const(&package_label)[K],uptr& out) noexcept {
    out=0;unsigned object{};if(!r.path(receiver,object))return false;
    auto p=r.result.identities[object].klass;std::array<uptr,meta::max_parents> seen{};unsigned count{};
    while(p){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;
        seen[count++]=p;meta::Structure structure{};unsigned kind{},package{};
        if(!r.structure(p,structure)||!r.path(r.result.identities[structure.identity].klass,kind)||
           !weaponhand::core_type(r,kind,L"Class"))return false;
        if(r.named(structure.identity,label)){
            if(!r.path(r.result.identities[structure.identity].outer,package)||!r.named(package,package_label)||
               r.result.identities[package].outer)return false;out=p;return true;
        }
        p=structure.super;
    }
    return false;
}
template<std::size_t N>bool declaring_class(meta::Reader& r,uptr receiver,wchar_t const(&label)[N],uptr& out) noexcept {
    return declaring_in(r,receiver,label,L"WarfareGame",out);
}
bool projectile_property(meta::Reader& r,meta::Property const& p) noexcept {
    if(!weaponhand::core_type(r,p.type,L"ObjectProperty")||p.element_bytes!=8)return false;
    uptr type{};unsigned slot{},kind{},package{};
    return r.at(r.result.identities[p.identity].address,0xd0,type)&&r.path(type,slot)&&
        (r.named(slot,L"Proj_Grenade")||r.named(slot,L"Proj_FragGrenade"))&&
        r.path(r.result.identities[slot].klass,kind)&&weaponhand::core_type(r,kind,L"Class")&&
        r.path(r.result.identities[slot].outer,package)&&r.named(package,L"WarfareGame")&&!r.result.identities[package].outer;
}
bool selector(meta::Reader& r,uptr receiver,Method method,uptr& owner,uptr& fn,uptr& declaring) noexcept {
    owner=fn=declaring=0;
    if(method==Method::press||method==Method::release||method==Method::target){
        if(!declaring_class(r,receiver,L"WarPC",declaring))return false;owner=declaring;
        return method==Method::press?child(r,owner,L"ButtonPress",fn):
            method==Method::release?child(r,owner,L"ButtonRelease",fn):child(r,owner,L"SetTargetingMode",fn);
    }
    if(!declaring_class(r,receiver,L"Weap_FragGrenade",declaring))return false;
    if(method==Method::mode){
        if(!declaring_in(r,receiver,L"Weapon",L"Engine",declaring))return false;owner=declaring;
        return child(r,owner,L"SetCurrentFireMode",fn);
    }
    if(method==Method::notify){owner=declaring;return child(r,owner,L"AnimNotifyThrow",fn);}
    if(method==Method::empty){
        if(!declaring_class(r,receiver,L"WarWeapon",declaring)||!child(r,declaring,L"Active",owner))return false;
        return child(r,owner,L"PlayClickClickEmptyClip",fn);
    }
    if(!declaring_class(r,receiver,L"Weap_Grenade",declaring))return false;owner=declaring;
    switch(method){
    case Method::prepare:return child(r,owner,L"StartGrenadeSpin",fn);
    case Method::stop:return child(r,owner,L"StopGrenadeSpin",fn);
    case Method::base_throw:return child(r,owner,L"ThrowGrenade",fn);
    case Method::origin:return child(r,owner,L"GetPhysicalFireStartLoc",fn);
    case Method::velocity:return child(r,owner,L"GetGrenadeTossVelocity",fn);
    case Method::finished:return child(r,declaring,L"ThrowingGrenade",owner)&&child(r,owner,L"FinishedThrow",fn);
    default:return false;
    }
}
bool bind(uptr receiver,Method method,Entry& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_path};
    uptr fn{},owner{},declaring{};unsigned slot{},kind{},owner_slot{},owner_kind{};
    if(!weaponhand::current_gt(nullptr)||!meta::tables(base,schema.objects,schema.names)||
       !selector(r,receiver,method,owner,fn,declaring)||!r.path(fn,slot)||r.result.identities[slot].outer!=owner||
       !r.path(r.result.identities[slot].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(owner,owner_slot)||!r.path(r.result.identities[owner_slot].klass,owner_kind))return false;
    if(method==Method::finished||method==Method::empty){if(!weaponhand::core_type(r,owner_kind,L"State"))return false;}
    else if(!weaponhand::core_type(r,owner_kind,L"Class"))return false;
    Entry value{};value.method=method;
    meta::Structure body{};meta::Table script{};unsigned flags{};std::uint16_t bytes{},ret{},native_index{};uptr exec{};
    if(!identify(r,fn,value.function)||!identify(r,owner,value.owner)||!identify(r,declaring,value.declaring)||
       !r.structure(fn,body)||!r.at(fn,0x106,bytes)||bytes>4096||!r.at(fn,0x108,ret)||
       !r.at(fn,0xf4,flags)||!r.at(fn,0xf8,native_index)||!r.at(fn,0x114,exec)||
       (flags&0x400)||native_index||exec!=base+0x1d6ca0||!r.at(fn,0x8c,script)||
       script.count<=0||script.count>script.capacity||script.capacity>131072||
       !weaponshot::hash_script(script.data,unsigned(script.count),value.script_hash))return false;
    value.script=script.data;value.script_bytes=unsigned(script.count);value.parameter_bytes=bytes;value.return_offset=ret;
    unsigned found{};std::array<uptr,meta::max_fields> seen{};unsigned count{};
    for(auto p=body.children;p;){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        meta::Property prop{};
        if(!r.property(p,prop)||schema.identities[prop.identity].outer!=fn||prop.dimension!=1||
           !meta::property_fits(prop,body.extent))return false;
        if(prop.flags&0x80){
            if(!meta::property_fits(prop,bytes)||value.count==value.parameters.size())return false;
            Parameter parameter{};parameter.offset=prop.offset;parameter.bytes=prop.element_bytes;parameter.flags=prop.flags;
            if(!identify(r,p,parameter.identity))return false;
            const bool returned=(prop.flags&0x400)!=0;unsigned bit{};
            if(returned){
                if(!r.named(prop.identity,L"ReturnValue")||(prop.flags&0x580)!=0x580||prop.offset!=ret)return false;
                if((method==Method::origin||method==Method::velocity)&&unified_weapon_fire::core_struct(r,prop,L"Vector"))parameter.type=Type::vector;
                else if(method==Method::base_throw&&projectile_property(r,prop))parameter.type=Type::projectile;
                else return false;bit=1;
            }else if((method==Method::press||method==Method::release)&&r.named(prop.identity,L"ButtonName")&&
                weaponhand::core_type(r,prop.type,L"NameProperty")&&prop.element_bytes==8&&(prop.flags&0x580)==0x80){parameter.type=Type::name;bit=2;}
            else if(method==Method::target&&r.named(prop.identity,L"bNewIsTargeting")&&
                weaponhand::core_type(r,prop.type,L"BoolProperty")&&prop.element_bytes==4&&(prop.flags&0x580)==0x80){
                if(!r.at(p,0xd0,parameter.mask)||!parameter.mask||(parameter.mask&(parameter.mask-1)))return false;
                parameter.type=Type::boolean;bit=2;
            }else if(method==Method::mode&&r.named(prop.identity,L"FiringModeNum")&&
                weaponhand::core_type(r,prop.type,L"ByteProperty")&&prop.element_bytes==1&&(prop.flags&0x580)==0x80){
                parameter.type=Type::byte;bit=2;
            }else if(method==Method::origin&&r.named(prop.identity,L"AimDir")&&
                unified_weapon_fire::core_struct(r,prop,L"Vector")&&prop.flags==0x90){
                // Retail Weap_Grenade inherits this optional Vector argument.
                // The observer never supplies it; preserve the original caller
                // argument and authenticate it separately from ReturnValue.
                parameter.type=Type::vector;bit=2;
            }else if(method==Method::velocity&&(prop.flags&0x580)==0x80){
                if(r.named(prop.identity,L"Start")&&unified_weapon_fire::core_struct(r,prop,L"Vector")){parameter.type=Type::vector;bit=2;}
                else if(r.named(prop.identity,L"TossDir")&&unified_weapon_fire::core_struct(r,prop,L"Vector")){parameter.type=Type::vector;bit=4;}
                else if(r.named(prop.identity,L"NadeProj")&&projectile_property(r,prop)){parameter.type=Type::projectile;bit=8;}
                else return false;
            }else return false;
            if(found&bit)return false;found|=bit;
            for(unsigned i=0;i<value.count;++i){const auto& q=value.parameters[i];
                if(parameter.offset<q.offset+q.bytes&&q.offset<parameter.offset+parameter.bytes)return false;}
            value.parameters[value.count++]=parameter;
        }
        p=prop.next;
    }
    const unsigned expected=method==Method::velocity?15u:(method==Method::origin?3u:
        (method==Method::base_throw?1u:
        (method==Method::target||method==Method::press||method==Method::release||method==Method::mode?2u:0u)));
    uptr closing_fn{},closing_owner{},closing_declaring{};
    if(found!=expected||(!(expected&1)&&ret!=0xffff)||
       !selector(r,receiver,method,closing_owner,closing_fn,closing_declaring)||
       closing_fn!=fn||closing_owner!=owner||closing_declaring!=declaring||!unified_weapon_fire::retained_metadata(r))return false;
    unified_weapon_fire::Fingerprint hash;hash.word(value.function.address);hash.word(value.function.klass);hash.word(value.function.outer);
    hash.word(value.owner.address);hash.word(value.declaring.address);hash.word(flags);hash.word(bytes);hash.word(ret);
    hash.word(script.data);hash.word(unsigned(script.count));hash.word(value.script_hash);
    for(unsigned i=0;i<value.count;++i){const auto& p=value.parameters[i];hash.word(p.identity.address);hash.word(p.identity.klass);
        hash.word(p.offset);hash.word(p.bytes);hash.word(p.mask);hash.word(p.flags);hash.word(unsigned(p.type));}
    value.fingerprint=hash.value;out=value;return true;
}
bool same(Entry const& a,Entry const& b) noexcept {
    return a.method==b.method&&hand::same(a.function,b.function)&&hand::same(a.owner,b.owner)&&hand::same(a.declaring,b.declaring)&&
        a.fingerprint&&a.fingerprint==b.fingerprint;
}
// Existing original dispatcher, including its normal hand/stow restoration
// boundary. No cached entry is callable after its actual declaration changes.
bool call(hand::Frame const& source,Entry const& entry,void const* arguments=nullptr,unsigned argument_bytes=0,
    uptr* projectile=nullptr,bool* invoked=nullptr) noexcept {
    if(projectile)*projectile=0;
    if(invoked)*invoked=false;
    const auto receiver=entry.method==Method::press||entry.method==Method::release||entry.method==Method::target?
        source.controller.address:source.weapon.address;
    Entry current{};
    if(!nativeequipment::current(nullptr,source)||!bind(receiver,entry.method,current)||!same(entry,current)||
       (entry.method==Method::velocity||entry.method==Method::origin)||
       argument_bytes>entry.parameter_bytes||(argument_bytes&&!arguments))return false;
    Request request{};request.object=receiver;request.function=entry.function.address;request.size=entry.parameter_bytes;
    if(argument_bytes)std::memcpy(request.params,arguments,argument_bytes);
    if(!nativeequipment::current(nullptr,source))return false;
    if(invoked)*invoked=true;
    if(!invoke(&request))return false;
    if(projectile){
        if(entry.method!=Method::base_throw||entry.return_offset>request.size||sizeof(uptr)>request.size-entry.return_offset)return false;
        std::memcpy(projectile,request.params+entry.return_offset,sizeof(uptr));
    }
    return true;
}
bool button(hand::Frame const& source,Entry const& entry,Name value,bool* invoked=nullptr) noexcept {
    if((entry.method!=Method::press&&entry.method!=Method::release)||entry.count!=1||
       entry.parameters[0].type!=Type::name||entry.parameter_bytes>4096)return false;
    std::array<unsigned char,4096> arguments{};
    std::memcpy(arguments.data()+entry.parameters[0].offset,&value,sizeof value);
    return call(source,entry,arguments.data(),entry.parameter_bytes,nullptr,invoked);
}
bool targeting(hand::Frame const& source,Entry const& entry,bool value,bool* invoked=nullptr) noexcept {
    if(entry.method!=Method::target||entry.count!=1||entry.parameters[0].type!=Type::boolean||entry.parameter_bytes>4096)return false;
    std::array<unsigned char,4096> arguments{};const unsigned bits=value?entry.parameters[0].mask:0;
    std::memcpy(arguments.data()+entry.parameters[0].offset,&bits,sizeof bits);
    return call(source,entry,arguments.data(),entry.parameter_bytes,nullptr,invoked);
}
bool throw_mode(hand::Frame const& source,Entry const& entry,bool* invoked=nullptr) noexcept {
    if(entry.method!=Method::mode||entry.count!=1||entry.parameters[0].type!=Type::byte||entry.parameter_bytes>4096)return false;
    // The original IncrementFlashCount default selects fire mode0 when the
    // stock throw starts. This physical route selects that same mode through
    // its original setter, without starting a second notifying animation.
    std::array<unsigned char,4096> arguments{};
    if(!call(source,entry,arguments.data(),entry.parameter_bytes,nullptr,invoked))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField mode{};unsigned char current{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,source.weapon)&&
        weaponhand::field(r,source.weapon.address,L"CurrentFireMode",L"ByteProperty",1,mode)&&
        r.at(source.weapon.address,mode.property.offset,current)&&current==0&&
        unified_weapon_fire::retained_metadata(r)&&nativeequipment::current(nullptr,source);
}
// Preparation/stop effect modes7/8 have no projectile slot. Read the actual
// mode0 class declaration without changing an idle/prepared weapon's mode.
bool projectile_class(hand::Frame const& source,hand::ObjectIdentity& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField array{};uptr weapon_class{},frag_class{},inner{},class_type{},projectile_base{},value{},closing{};
    meta::Property element{},last_array{},last_element{};meta::Table items{},last_items{};
    unsigned kind{},class_slot{},base_slot{},package{},slot{};uptr last_inner{},last_class{},last_base{};
    if(!nativeequipment::current(nullptr,source)||!meta::tables(base,schema.objects,schema.names)||
       !weaponhand::object(r,source.weapon)||!declaring_class(r,source.weapon.address,L"Weap_FragGrenade",frag_class)||
       !declaring_in(r,source.weapon.address,L"Weapon",L"Engine",weapon_class)||
       !weaponhand::field(r,source.weapon.address,L"WeaponProjectiles",L"ArrayProperty",16,array)||
       schema.identities[array.owner].address!=weapon_class)return false;
    const auto property=schema.identities[array.property.identity].address;
    if(!r.at(property,0xd0,inner)||!r.property(inner,element)||
       schema.identities[element.identity].outer!=property||!r.named(element.identity,L"WeaponProjectiles")||
       !weaponhand::core_type(r,element.type,L"ClassProperty")||element.dimension!=1||element.element_bytes!=8||
       !r.at(inner,0xd0,class_type)||!r.path(class_type,class_slot)||!weaponhand::core_type(r,class_slot,L"Class")||
       !r.at(inner,0xd8,projectile_base)||!r.path(projectile_base,base_slot)||!r.named(base_slot,L"Projectile")||
       !r.path(schema.identities[base_slot].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(schema.identities[base_slot].outer,package)||!r.named(package,L"Engine")||schema.identities[package].outer||
       !r.at(source.weapon.address,array.property.offset,items)||!items.data||items.count<1||
       items.count>items.capacity||items.capacity>64||!r.at(items.data,0,value)||!r.path(value,slot)||
       !r.named(slot,L"Proj_FragGrenade")||!r.path(schema.identities[slot].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(schema.identities[slot].outer,package)||!r.named(package,L"WarfareGame")||schema.identities[package].outer||
       !identify(r,value,out)||!unified_weapon_fire::retained_metadata(r)||
       !r.property(property,last_array)||!meta::same_property(array.property,last_array)||
       !r.property(inner,last_element)||!meta::same_property(element,last_element)||
       !r.at(property,0xd0,last_inner)||last_inner!=inner||!r.at(inner,0xd0,last_class)||last_class!=class_type||
       !r.at(inner,0xd8,last_base)||last_base!=projectile_base||
       !r.at(source.weapon.address,array.property.offset,last_items)||!meta::same_table(items,last_items)||
       !r.at(items.data,0,closing)||closing!=value||!nativeequipment::current(nullptr,source)){out={};return false;}
    return true;
}
// Release only an already-owned preparation. No expired Frame is renewed into
// a new press, throw, equip or pose permit. Retain the original actor/lifetime
// and declaration checks even when a busy animation cleared its publication.
bool release_existing(hand::Frame const& source,hand::ExistingActionLifetime const& lifetime,
    Entry const& entry,Name button_name={}) noexcept {
    const auto m=entry.method;
    if(m!=Method::release&&m!=Method::target&&m!=Method::stop)return false;
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       !weaponhand::client.existing_action_lifetime_on_gt(lifetime))return false;
    for(const auto& id:{source.world,source.controller,source.pawn,source.weapon})
        if(!weapon_part_assets::object(id))return false;
    if(playground::pointer(source.controller.address,L"Pawn")!=source.pawn.address||
       playground::pointer(source.pawn.address,L"WorldInfo")!=source.world.address||
       playground::pointer(source.weapon.address,L"Instigator")!=source.pawn.address)return false;
    const auto receiver=m==Method::stop?source.weapon.address:source.controller.address;
    Entry fresh{};if(!bind(receiver,m,fresh)||!same(entry,fresh))return false;
    Request request{};request.object=receiver;request.function=entry.function.address;request.size=entry.parameter_bytes;
    if(m==Method::release){
        if(entry.count!=1||entry.parameters[0].type!=Type::name)return false;
        std::memcpy(request.params+entry.parameters[0].offset,&button_name,sizeof button_name);
    }else if(m==Method::target){
        if(entry.count!=1||entry.parameters[0].type!=Type::boolean)return false;
        // The zero-filled actual BoolProperty argument can only turn OFF.
    }else if(entry.count)return false;
    return weaponhand::client.existing_action_lifetime_on_gt(lifetime)&&invoke(&request);
}
// Completion of a proven, already dispatched physical throw. It cannot spawn
// another grenade. The caller owns the once-only completion journal; the
// existing action lifetime retains the exact original actor/reference tuple.
bool reenter_exhausted_active(hand::Frame const& source,hand::ExistingActionLifetime const& lifetime) noexcept {
    lancerreload_inventory::Counts a{},b{};
    if(!weaponhand::client.existing_action_lifetime_on_gt(lifetime)||
       !weapon_part_assets::object(source.weapon)||
       !lancerreload_inventory::query(source.weapon.address,a)||
       !lancerreload_inventory::query(source.weapon.address,b)||!(a==b))return false;
    if(a.infinite||a.reserve||a.used<a.capacity)return true;
    if(playground::pointer(source.controller.address,L"Pawn")!=source.pawn.address||
       playground::pointer(source.pawn.address,L"WorldInfo")!=source.world.address)return false;
    // Stock FinishedThrow leaves ThrowingGrenade for Active. This authored
    // physical throw starts no stock throw animation and was ALREADY Active;
    // an ordinary same-state GotoState skips Active.BeginState and its original
    // exhausted-weapon auto-switch. Replay only those original state events,
    // after the exact once-only throw/completion and proven zero ammunition.
    // No inventory, selection, ammunition or VR hold is authored here.
    if(playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address)return true;
    playground::Args state(source.weapon.address,L"GetStateName");
    if(!weapon_part_assets::native_function(state,L"GetStateName",L"Object",L"Core")||
       get<std::uint16_t>(state.r.function+0xf8)!=284||!vrequip::result_property(state,L"NameProperty",8)||
       !weaponhand::client.existing_action_lifetime_on_gt(lifetime)||!state.run())return false;
    const auto active=state.output<Name>();meta::Table objects{},names{};meta::NameText text{};
    if(active.number||!meta::tables(base,objects,names)||!meta::fresh_name(names,active.index,text)||
       !meta::exact_text(text,L"Active"))return false;
    playground::Args enter(source.weapon.address,L"GotoState");
    const auto parameter=[&](const wchar_t* name,const wchar_t* type,unsigned bytes,unsigned offset) noexcept {
        const auto p=enter.prop(name);
        return p&&playground::named(get<uptr>(p+0x50),type)&&get<unsigned>(p+0x70)==1&&
            get<unsigned>(p+0x74)==bytes&&get<unsigned>(p+0x94)==offset&&
            (get<std::uint64_t>(p+0x78)&0x590)==0x90;
    };
    if(!weapon_part_assets::native_function(enter,L"GotoState",L"Object",L"Core")||
       get<std::uint16_t>(enter.r.function+0xf8)!=113||enter.r.size!=24||
       get<std::uint16_t>(enter.r.function+0x108)!=0xffff||
       !parameter(L"NewState",L"NameProperty",8,0)||!parameter(L"Label",L"NameProperty",8,8)||
       !parameter(L"bForceEvents",L"BoolProperty",4,16)||!parameter(L"bKeepStack",L"BoolProperty",4,20))return false;
    const auto mask=get<unsigned>(enter.prop(L"bForceEvents")+0xd0);
    if(!mask||(mask&(mask-1)))return false;
    std::memcpy(enter.r.params,&active,sizeof active);std::memcpy(enter.r.params+16,&mask,sizeof mask);
    for(const auto& id:{source.world,source.controller,source.pawn,source.weapon})if(!weapon_part_assets::object(id))return false;
    if(playground::pointer(source.controller.address,L"Pawn")!=source.pawn.address||
       playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address||
       playground::pointer(source.weapon.address,L"Instigator")!=source.pawn.address||
       !weaponhand::client.existing_action_lifetime_on_gt(lifetime)||!nativeequipment::restoration_quiet())return false;
    return enter.run();
}
bool complete_existing(hand::Frame const& source,hand::ExistingActionLifetime const& lifetime,
    Entry const& entry,hand::ObjectIdentity const& projectile,bool* invoked) noexcept {
    if(invoked)*invoked=false;
    if(entry.method!=Method::finished||entry.count||!weaponhand::current_gt(nullptr)||
       !nativeequipment::restoration_quiet()||
       !weaponhand::client.existing_action_lifetime_on_gt(lifetime))return false;
    for(const auto& id:{source.world,source.controller,source.pawn,source.weapon,projectile})
        if(!weapon_part_assets::object(id))return false;
    if(playground::pointer(source.controller.address,L"Pawn")!=source.pawn.address||
       playground::pointer(source.pawn.address,L"Weapon")!=source.weapon.address||
       playground::pointer(projectile.address,L"WorldInfo")!=source.world.address||
       playground::pointer(projectile.address,L"Instigator")!=source.pawn.address)return false;
    Entry current{};if(!bind(source.weapon.address,Method::finished,current)||!same(entry,current))return false;
    Request request{};request.object=source.weapon.address;request.function=entry.function.address;request.size=entry.parameter_bytes;
    if(!weaponhand::client.existing_action_lifetime_on_gt(lifetime))return false;
    if(invoked)*invoked=true;
    if(!invoke(&request))return false;
    return reenter_exhausted_active(source,lifetime);
}
} // namespace frag_original
