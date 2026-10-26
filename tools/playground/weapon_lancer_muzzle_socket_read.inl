// Include after weapon_lancer_asset_read.inl and weapon_lancer_muzzle_observation.h.
// Metadata observation only: no bone/axis inference and no engine getter call.
// Non-Actor asset/component/socket liveness means current registered identity;
// this does not certify UObject pending-kill flags (no proven flags ABI here).
namespace nativeequipment_component {
namespace muzzle_detail {
namespace meta=weaponhand::meta;
using U32=std::uint32_t;
struct Reads {
    struct Value {uptr object{},offset{};std::size_t size{};std::array<unsigned char,32> bytes{};};
    std::array<Value,256> values{};U32 count{},cursor{};bool repeat{};
    std::array<weaponhand::BoolField,160> fields{};U32 field_count{},field_cursor{};
    std::array<meta::NameText,34> names{};U32 name_count{},name_cursor{};
    template<class T>bool at(meta::Reader& r,uptr object,uptr offset,T& out) noexcept {
        static_assert(sizeof(T)<=32);
        if(!r.at(object,offset,out))return false;
        if(repeat){if(cursor==count)return false;auto const& old=values[cursor++];
            return old.object==object&&old.offset==offset&&old.size==sizeof(T)&&!std::memcmp(old.bytes.data(),&out,sizeof(T));}
        if(count==values.size())return false;auto& value=values[count++];
        value.object=object;value.offset=offset;value.size=sizeof(T);std::memcpy(value.bytes.data(),&out,sizeof(T));return true;
    }
    template<std::size_t N,std::size_t K>bool field(meta::Reader& r,uptr object,wchar_t const(&label)[N],
        wchar_t const(&kind)[K],U32 bytes,weaponhand::BoolField& out) noexcept {
        if(!weaponhand::field(r,object,label,kind,bytes,out))return false;
        if(repeat){if(field_cursor==field_count)return false;auto const& old=fields[field_cursor++];
            return old.object==out.object&&old.owner==out.owner&&meta::same_property(old.property,out.property);}
        if(field_count==fields.size())return false;fields[field_count++]=out;return true;
    }
    bool name(meta::Reader& r,meta::RawName raw,meta::NameText& text) noexcept {
        if(raw.number||!meta::fresh_name(r.result.names,raw.index,text))return false;
        if(repeat)return name_cursor<name_count&&meta::same_name_text(names[name_cursor++],text);
        if(name_count==names.size())return false;names[name_count++]=text;return true;
    }
};
bool live(meta::Reader& r,uptr object) noexcept {
    gears_weapon_hand::ObjectIdentity identity{};return lancermain_asset::identify(r,object,identity);
}
template<std::size_t N>bool name_field(meta::Reader& r,Reads& reads,uptr object,wchar_t const(&label)[N],
    meta::RawName& raw,meta::NameText& text) noexcept {
    weaponhand::BoolField field{};
    return reads.field(r,object,label,L"NameProperty",8,field)&&reads.at(r,object,field.property.offset,raw)&&reads.name(r,raw,text);
}
template<std::size_t N,std::size_t K,class T>bool vector_field(meta::Reader& r,Reads& reads,uptr object,
    wchar_t const(&label)[N],wchar_t const(&structure_name)[K],T& out) noexcept {
    weaponhand::BoolField field{};uptr structure{};U32 slot{},type{},package{},extent{};
    if(!reads.field(r,object,label,L"StructProperty",12,field)||
       !reads.at(r,r.result.identities[field.property.identity].address,0xd0,structure)||
       !r.path(structure,slot)||!r.named(slot,structure_name)||
       !r.path(r.result.identities[slot].klass,type)||!weaponhand::core_type(r,type,L"ScriptStruct")||
       !r.path(r.result.identities[slot].outer,package)||!r.named(package,L"Core")||r.result.identities[package].outer||
       !reads.at(r,structure,0x88,extent)||extent!=12)return false;
    return reads.at(r,object,field.property.offset,out);
}
bool pass(meta::Reader& r,Reads& reads,gears_weapon_hand::Frame const& frame,lancermain_asset::Evidence const& asset,
    gears_weapon_lancer_muzzle::Socket& candidate) noexcept {
    if(!weaponhand::object(r,frame.weapon)||!weaponhand::object(r,asset.component)||!weaponhand::object(r,asset.asset))return false;
    weaponhand::BoolField deleted{};U32 mask{},bits{};
    if(!reads.field(r,frame.weapon.address,L"bDeleteMe",L"BoolProperty",4,deleted)||
       !reads.at(r,r.result.identities[deleted.property.identity].address,0xd0,mask)||!mask||(mask&(mask-1))||
       !reads.at(r,frame.weapon.address,deleted.property.offset,bits)||(bits&mask))return false;
    meta::RawName configured{};meta::NameText configured_text{};
    if(!name_field(r,reads,frame.weapon.address,L"MuzzleSocketName",configured,configured_text)||
       !meta::exact_text(configured_text,L"Muzzle"))return false;
    weaponhand::BoolField array_field{};meta::Table sockets{};uptr inner{};meta::Property inner_property{};
    U32 inner_slot{};
    if(!reads.field(r,asset.asset.address,L"Sockets",L"ArrayProperty",16,array_field)||
       !reads.at(r,r.result.identities[array_field.property.identity].address,0xd0,inner)||
       !r.property(inner,inner_property)||!weaponhand::core_type(r,inner_property.type,L"ObjectProperty")||
       r.result.identities[inner_property.identity].outer!=r.result.identities[array_field.property.identity].address||
       inner_property.dimension!=1||inner_property.element_bytes!=8||
       !r.path(inner,inner_slot)||!reads.at(r,asset.asset.address,array_field.property.offset,sockets)||
       !sockets.data||sockets.count<1||sockets.count>32||sockets.capacity<sockets.count||sockets.capacity>32)return false;
    // Reader.property does not retain descriptors. Retain the inner explicitly too.
    weaponhand::BoolField inner_witness{};inner_witness.object=inner;inner_witness.property=inner_property;
    if(reads.repeat){if(reads.field_cursor==reads.field_count)return false;auto const& old=reads.fields[reads.field_cursor++];
        if(old.object!=inner||!meta::same_property(old.property,inner_property))return false;}
    else {if(reads.field_count==reads.fields.size())return false;reads.fields[reads.field_count++]=inner_witness;}
    bool selected=false;
    for(int i=0;i<sockets.count;++i){uptr socket{};meta::RawName socket_name{};meta::NameText text{};
        if(!reads.at(r,sockets.data,uptr(i)*8,socket)||!live(r,socket)||
           !weaponhand::derives(r,socket,L"SkeletalMeshSocket",L"Engine")||
           !name_field(r,reads,socket,L"SocketName",socket_name,text))return false;
        if(socket_name.index!=configured.index||socket_name.number!=configured.number)continue;
        if(selected)return false;selected=true;meta::RawName bone{};meta::NameText bone_text{};
        if(!name_field(r,reads,socket,L"BoneName",bone,bone_text)||
           !vector_field(r,reads,socket,L"RelativeLocation",L"Vector",candidate.location)||
           !vector_field(r,reads,socket,L"RelativeRotation",L"Rotator",candidate.rotation))return false;
        for(float value:candidate.location)if(!std::isfinite(value))return false;
        candidate.object=socket;candidate.configured_name={configured.index,configured.number};
        candidate.socket_name={socket_name.index,socket_name.number};candidate.bone_name={bone.index,bone.number};
    }
    return selected;
}
} // namespace muzzle_detail
bool muzzle_socket(gears_weapon_hand::Frame const& frame,lancermain_asset::Evidence const& asset,
    gears_weapon_lancer_muzzle::Socket& out) noexcept {
    namespace md=muzzle_detail;namespace meta=weaponhand::meta;
    const auto gt=weaponhand::current_gt(nullptr);if(!gt||(frame.key.thread_id&&frame.key.thread_id!=gt))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};md::Reads reads{};
    gears_weapon_lancer_muzzle::Socket candidate{},fresh{};
    if(!gears_weapon_hand::identity_valid(frame.weapon)||!gears_weapon_hand::identity_valid(asset.component)||
       !gears_weapon_hand::identity_valid(asset.asset)||!meta::tables(base,schema.objects,schema.names)||
       !md::pass(r,reads,frame,asset,candidate))return false;
    reads.repeat=true;
    if(!md::pass(r,reads,frame,asset,fresh)||!gears_weapon_lancer_muzzle::same(candidate,fresh)||
       reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||reads.name_cursor!=reads.name_count)return false;
    const auto identities=schema.identity_count,structures=schema.structure_count;
    for(unsigned i=0;i<identities;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure current{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,current)||!meta::same_structure(current,schema.structures[i]))return false;}
    for(unsigned i=0;i<reads.field_count;++i){meta::Property current{};auto const& field=reads.fields[i].property;
        if(!r.property(schema.identities[field.identity].address,current)||!meta::same_property(field,current))return false;}
    for(unsigned i=0;i<reads.name_count;++i){meta::NameText current{};
        if(!meta::fresh_name(schema.names,reads.names[i].index,current)||!meta::same_name_text(reads.names[i],current))return false;}
    meta::Table objects{},names{};
    if(!meta::tables(base,objects,names)||!meta::same_table(objects,schema.objects)||!meta::same_table(names,schema.names)||
       base!=schema.module_base||weaponhand::current_gt(nullptr)!=gt||r.result.failure.reason!=meta::Reason::none)return false;
    out=candidate;return true;
}
} // namespace nativeequipment_component
