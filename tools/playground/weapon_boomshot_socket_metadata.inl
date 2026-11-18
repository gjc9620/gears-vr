// Current original Magazine socket, not an assumed static-carrier transform.
// Metadata and world getter are read-only normal-GT operations.
namespace boomshot_socket {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using U32=std::uint32_t;
// Local bounded repeat journal; the existing component-probe reader is
// diagnostic-flag-only and is not enabled as a production dependency.
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
struct Source {
    hand::ObjectIdentity component{},asset{},socket{};Name name{},bone{};
    playground::Vector relative{},scale{};playground::Rotation rotation{};
    meta::Property inner{};
};
bool metadata(hand::Frame const& f,lancermain_asset::Evidence const& asset,meta::Reader& r,
    Reads& reads,Source& out) noexcept {
    Source value{};value.component=asset.component;value.asset=asset.asset;
    if(!weaponhand::object(r,f.weapon)||!weaponhand::object(r,asset.component)||!weaponhand::object(r,asset.asset))return false;
    weaponhand::BoolField field{};meta::Table table{};uptr inner{};
    if(!reads.field(r,asset.asset.address,L"Sockets",L"ArrayProperty",16,field)||
       !reads.at(r,asset.asset.address,field.property.offset,table)||!table.data||table.count<1||table.count>32||
       table.capacity<table.count||table.capacity>32||
       !reads.at(r,r.result.identities[field.property.identity].address,0xd0,inner)||!r.property(inner,value.inner)||
       r.result.identities[value.inner.identity].outer!=r.result.identities[field.property.identity].address||
       value.inner.dimension!=1||value.inner.element_bytes!=8||!weaponhand::core_type(r,value.inner.type,L"ObjectProperty"))return false;
    bool found{};
    for(int i=0;i<table.count;++i){
        uptr socket{};hand::ObjectIdentity identity{};meta::RawName raw{};meta::NameText text{};U32 slot{};
        if(!reads.at(r,table.data,std::size_t(i)*8,socket)||!lancermain_asset::identify(r,socket,identity)||
           !r.path(socket,slot)||r.result.identities[slot].outer!=asset.asset.address||
           !weaponhand::derives(r,socket,L"SkeletalMeshSocket",L"Engine")||
           !reads.field(r,socket,L"SocketName",L"NameProperty",8,field)||
           !reads.at(r,socket,field.property.offset,raw)||!reads.name(r,raw,text))return false;
        if(!meta::exact_text(text,L"Magazine"))continue;
        if(found)return false;found=true;value.socket=identity;value.name={raw.index,raw.number};
        if(!reads.field(r,socket,L"BoneName",L"NameProperty",8,field)||
           !reads.at(r,socket,field.property.offset,raw)||!reads.name(r,raw,text)||
           !meta::exact_text(text,L"Boomshot_Magazine"))return false;
        value.bone={raw.index,raw.number};
        if(!reads.field(r,socket,L"RelativeLocation",L"StructProperty",12,field)||
           !unified_weapon_fire::core_struct(r,field.property,L"Vector")||!reads.at(r,socket,field.property.offset,value.relative)||
           !reads.field(r,socket,L"RelativeRotation",L"StructProperty",12,field)||
           !unified_weapon_fire::core_struct(r,field.property,L"Rotator")||!reads.at(r,socket,field.property.offset,value.rotation)||
           !reads.field(r,socket,L"RelativeScale",L"StructProperty",12,field)||
           !unified_weapon_fire::core_struct(r,field.property,L"Vector")||!reads.at(r,socket,field.property.offset,value.scale))return false;
        for(float v:{value.relative.x,value.relative.y,value.relative.z,value.scale.x,value.scale.y,value.scale.z})
            if(!std::isfinite(v))return false;
        if(value.scale.x<=0||value.scale.y<=0||value.scale.z<=0)return false;
    }
    if(!found||!unified_weapon_fire::retained_metadata(r))return false;
    out=value;return true;
}
}
