// Included inside troika_mounted_parts after socket_contract. Read only the
// two ORIGINAL cached controls; no guessed enum ordinals or axis certification.
namespace control_axes {
namespace metadata=weaponhand::meta;namespace hand=gears_weapon_hand;
struct EnumByte {hand::ObjectIdentity enumeration{};unsigned char value{};};
struct Control {
    hand::ObjectIdentity object{},component{},object_reference{},component_reference{};
    // LookAtAxis, UpAxis, AllowRotationSpace, TargetLocationSpace. These are
    // ORIGINAL enum identities/raw values, not our own numeric convention.
    std::array<EnumByte,4> bytes{};
    metadata::RawName name{},target_bone{},other_bone{};
    // X/Y/Z permission, inversion/up policy, original limit policy.
    std::uint32_t flags{};std::array<std::uint32_t,11> bool_masks{};
    float max_angle{},dead_zone{},strength{},alpha{};
    std::array<float,3> target{},base_look{},limit_look{},base_position{};
};
struct Snapshot {std::array<Control,2> controls{};};
static_assert(std::is_trivially_copyable_v<Snapshot>);

bool class_children(metadata::Reader& r) noexcept {
    std::array<uptr,metadata::max_fields> owners{};unsigned owner_count{};
    for(unsigned i=0;i<r.result.field_count;++i){
        auto const owner=r.result.identities[r.result.fields[i].identity].outer;
        bool visited=false;for(unsigned j=0;j<owner_count;++j)visited=visited||owners[j]==owner;
        if(visited)continue;if(owner_count==owners.size())return false;owners[owner_count++]=owner;
        unsigned slot{},kind{};
        if(!r.path(owner,slot)||!r.path(r.result.identities[slot].klass,kind))return false;
        // Vector members are independently walked/closed by numeric().
        if(weaponhand::core_type(r,kind,L"ScriptStruct"))continue;
        // Native arrays retain one typed inner descriptor, whose outer is the
        // ArrayProperty rather than a Class. Authenticate that exact link.
        if(weaponhand::core_type(r,kind,L"ArrayProperty")){
            metadata::Property array{};uptr inner{};
            if(!r.property(owner,array)||!weaponhand::core_type(r,array.type,L"ArrayProperty")||
               array.dimension!=1||array.element_bytes!=16||!r.at(owner,0xd0,inner)||
               inner!=r.result.identities[r.result.fields[i].identity].address||!troika_reflection::retain(r,array))return false;
            continue;
        }
        if(!weaponhand::core_type(r,kind,L"Class"))return false;
        metadata::Structure layout{};if(!r.structure(owner,layout))return false;
        std::array<uptr,512> addresses{};std::array<metadata::RawName,512> names{};unsigned count{};
        for(auto child=layout.children;child;){
            if(count==addresses.size())return false;
            for(unsigned j=0;j<count;++j)if(addresses[j]==child)return false;
            metadata::Identity id{};metadata::NameText text{};
            if(!metadata::raw_identity(r.result.objects,r.result.names,child,id,text)||id.outer!=owner||id.name.number)return false;
            for(unsigned j=0;j<count;++j)if(names[j].index==id.name.index&&names[j].number==id.name.number)return false;
            addresses[count]=child;names[count++]=id.name;if(!r.at(child,0x68,child))return false;
        }
    }
    return true;
}

bool identity(metadata::Reader& r,uptr address,hand::ObjectIdentity& output) noexcept {
    unsigned slot{};if(!r.path(address,slot))return false;
    auto const& id=r.result.identities[slot];
    hand::ObjectIdentity value{id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
    if(!weaponhand::object(r,value))return false;output=value;return true;
}
bool class_derives(metadata::Reader& r,uptr klass,uptr expected) noexcept {
    std::array<uptr,metadata::max_parents> seen{};unsigned count{};
    while(klass){
        if(count==seen.size())return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==klass)return false;seen[count++]=klass;
        metadata::Structure value{};if(!r.structure(klass,value))return false;
        if(klass==expected)return true;klass=value.super;
    }
    return false;
}
template<std::size_t N,std::size_t O,std::size_t P>bool declared(metadata::Reader& r,uptr address,
    wchar_t const (&label)[N],wchar_t const (&owner_name)[O],wchar_t const (&package_name)[P]) noexcept {
    unsigned value{},kind{},owner{},owner_kind{},package{};
    return r.path(address,value)&&r.named(value,label)&&!r.result.identities[value].name.number&&
        r.path(r.result.identities[value].klass,kind)&&weaponhand::core_type(r,kind,L"Enum")&&
        r.path(r.result.identities[value].outer,owner)&&r.named(owner,owner_name)&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&r.named(package,package_name)&&!r.result.identities[package].outer;
}
template<std::size_t N>bool byte(metadata::Reader& r,uptr object,wchar_t const (&label)[N],bool axis,EnumByte& output) noexcept {
    weaponhand::BoolField field{};uptr enumeration{};EnumByte value{};std::memset(&value,0,sizeof value);
    if(!weaponhand::field(r,object,label,L"ByteProperty",1,field)||!troika_reflection::retain(r,field.property)||
       !r.at(r.result.identities[field.property.identity].address,0xd0,enumeration)||
       !(axis?declared(r,enumeration,L"EAxis",L"Object",L"Core"):
                declared(r,enumeration,L"EBoneControlSpace",L"SkelControlBase",L"Engine"))||
       !identity(r,enumeration,value.enumeration)||!r.at(object,field.property.offset,value.value))return false;
    output=value;return true;
}
template<std::size_t N>bool name(metadata::Reader& r,uptr object,wchar_t const (&label)[N],metadata::RawName& output,
    wchar_t const* expected=nullptr) noexcept {
    weaponhand::BoolField field{};metadata::RawName value{};metadata::NameText text{};
    if(!weaponhand::field(r,object,label,L"NameProperty",8,field)||!troika_reflection::retain(r,field.property)||
       !r.at(object,field.property.offset,value)||value.number||!metadata::fresh_name(r.result.names,value.index,text))return false;
    if(expected){const auto length=std::wcslen(expected);
        if(text.length!=length||std::wmemcmp(text.units.data(),expected,length+1))return false;}
    output=value;return true;
}
template<std::size_t N>bool floating(metadata::Reader& r,uptr object,wchar_t const (&label)[N],float& output) noexcept {
    weaponhand::BoolField field{};float value{};
    if(!weaponhand::field(r,object,label,L"FloatProperty",4,field)||!troika_reflection::retain(r,field.property)||
       !r.at(object,field.property.offset,value)||!std::isfinite(value))return false;
    output=value;return true;
}
template<std::size_t N>bool vector(metadata::Reader& r,uptr object,wchar_t const (&label)[N],std::array<float,3>& output) noexcept {
    weaponhand::BoolField field{};troika_reflection::ScriptStruct layout{};std::array<float,3> value{};
    if(!weaponhand::field(r,object,label,L"StructProperty",12,field)||
       !troika_reflection::numeric(r,field.property,L"Vector",L"FloatProperty",std::array<wchar_t const*,3>{L"X",L"Y",L"Z"},layout)||
       !r.at(object,field.property.offset,value)||!troika_reflection::current(r,layout,L"Vector",L"Object",L"Core"))return false;
    for(auto x:value)if(!std::isfinite(x))return false;output=value;return true;
}
template<std::size_t N>bool pointer(metadata::Reader& r,uptr owner,wchar_t const (&label)[N],bool component,
    hand::ObjectIdentity& output,hand::ObjectIdentity& declaration) noexcept {
    metadata::PointerField field{};uptr reference{};unsigned declared_type{},kind{},package{};
    if(!weaponhand::read_pointer(r,owner,label,field,component)||!troika_reflection::retain(r,field.property)||
       !r.at(r.result.identities[field.property.identity].address,0xd0,reference)||!r.path(reference,declared_type)||
       !(component?r.named(declared_type,L"SkeletalMeshComponent"):r.named(declared_type,L"SkelControlLookAt"))||
       !r.path(r.result.identities[declared_type].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(r.result.identities[declared_type].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer)return false;
    return field.value&&identity(r,reference,declaration)&&identity(r,field.value,output)&&class_derives(r,output.klass,reference);
}
template<std::size_t N>bool bit(metadata::Reader& r,uptr object,wchar_t const (&label)[N],unsigned index,
    Control& output,std::array<weaponhand::BoolField,11>& fields) noexcept {
    weaponhand::BoolField field{};
    if(index>=fields.size()||!weaponhand::boolean(r,object,label,field)||!troika_reflection::retain(r,field.property))return false;
    for(unsigned i=0;i<index;++i){auto const& previous=fields[i];
        if(field.property.offset==previous.property.offset){if(field.mask&previous.mask)return false;}
        else if(field.property.offset<previous.property.offset+4&&previous.property.offset<field.property.offset+4)return false;
    }
    fields[index]=field;output.bool_masks[index]=field.mask;
    if(field.bits&field.mask)output.flags|=1u<<index;return true;
}
template<std::size_t N>bool control(metadata::Reader& r,uptr turret,hand::ObjectIdentity const& mesh,
    wchar_t const (&cached)[N],Control& output) noexcept {
    Control value{};std::memset(&value,0,sizeof value);
    std::array<weaponhand::BoolField,11> fields{};
    if(!pointer(r,turret,cached,false,value.object,value.object_reference)||
       !weaponhand::derives(r,value.object.address,L"SkelControlLookAt",L"Engine")||
       !pointer(r,value.object.address,L"SkelComponent",true,value.component,value.component_reference)||!hand::same(value.component,mesh)||
       !name(r,value.object.address,L"ControlName",value.name,cached))return false;
    auto const object=value.object.address;
    if(!byte(r,object,L"LookAtAxis",true,value.bytes[0])||!byte(r,object,L"UpAxis",true,value.bytes[1])||
       !byte(r,object,L"AllowRotationSpace",false,value.bytes[2])||!byte(r,object,L"TargetLocationSpace",false,value.bytes[3])||
       !name(r,object,L"TargetSpaceBoneName",value.target_bone)||!name(r,object,L"AllowRotationOtherBoneName",value.other_bone)||
       !bit(r,object,L"bAllowRotationX",0,value,fields)||!bit(r,object,L"bAllowRotationY",1,value,fields)||
       !bit(r,object,L"bAllowRotationZ",2,value,fields)||!bit(r,object,L"bInvertLookAtAxis",3,value,fields)||
       !bit(r,object,L"bDefineUpAxis",4,value,fields)||!bit(r,object,L"bInvertUpAxis",5,value,fields)||
       !bit(r,object,L"bEnableLimit",6,value,fields)||!bit(r,object,L"bLimitBasedOnRefPose",7,value,fields)||
       !bit(r,object,L"bDisableBeyondLimit",8,value,fields)||!bit(r,object,L"bNotifyBeyondLimit",9,value,fields)||
       !bit(r,object,L"bIgnoreWhenNotRendered",10,value,fields)||
       !floating(r,object,L"MaxAngle",value.max_angle)||!floating(r,object,L"DeadZoneAngle",value.dead_zone)||
       !floating(r,object,L"ControlStrength",value.strength)||!floating(r,object,L"LookAtAlpha",value.alpha)||
       !vector(r,object,L"TargetLocation",value.target)||!vector(r,object,L"BaseLookDir",value.base_look)||
       !vector(r,object,L"LimitLookDir",value.limit_look)||!vector(r,object,L"BaseBonePos",value.base_position))return false;
    output=value;return true;
}
// The consumer supplies the SAME original mounted-source currentness check.
// Two complete reads and closing metadata checks detect ordinary reuse/races;
// this is not an atomic engine snapshot or original native-control execution.
template<class Current>bool read(hand::ObjectIdentity const& turret,hand::ObjectIdentity const& mesh,
    Current&& current,Snapshot& output) noexcept {
    try{
        if(!current())return false;
        metadata::Snapshot schema{};schema.module_base=base;metadata::Reader r{schema,metadata::Stage::final_check};
        if(!metadata::tables(base,schema.objects,schema.names)||!weaponhand::object(r,turret)||!weaponhand::object(r,mesh)||
           !weaponhand::derives(r,turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||
           !weaponhand::derives(r,mesh.address,L"SkeletalMeshComponent",L"Engine"))return false;
        Snapshot first{},closing{};std::memset(&first,0,sizeof first);std::memset(&closing,0,sizeof closing);
        const auto sample=[&](Snapshot& s){return control(r,turret.address,mesh,L"Pivot_Latitude",s.controls[0])&&
            control(r,turret.address,mesh,L"Pivot_Longitude",s.controls[1])&&
            s.controls[0].object.address!=s.controls[1].object.address;};
        if(!sample(first)||!class_children(r)||!socket_contract::retained_metadata(r)||!current()||!sample(closing)||
           std::memcmp(&first,&closing,sizeof first)||!class_children(r)||!socket_contract::retained_metadata(r)||!current())return false;
        output=first;return true;
    }catch(...){return false;}
}
} // namespace control_axes
