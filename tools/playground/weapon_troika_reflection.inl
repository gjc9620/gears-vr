// Troika-local ScriptStruct reader. Do not widen the shared Class/Function
// reader: a reflected value layout is a different metadata category.
namespace troika_reflection {
namespace meta=weaponhand::meta;
struct ScriptStruct {meta::Property container{};meta::Structure layout{};uptr address{};};
bool retain(meta::Reader& reader,meta::Property const& property) noexcept {
    for(unsigned i=0;i<reader.result.field_count;++i)if(reader.result.fields[i].identity==property.identity)
        return meta::same_property(reader.result.fields[i],property);
    if(reader.result.field_count==reader.result.fields.size())return false;
    reader.result.fields[reader.result.field_count++]=property;return true;
}
template<std::size_t L,std::size_t O,std::size_t P>bool read(meta::Reader& reader,meta::Property const& container,
    wchar_t const (&label)[L],wchar_t const (&declaring_class)[O],wchar_t const (&package_name)[P],unsigned bytes,ScriptStruct& output) noexcept {
    ScriptStruct result{};result.container=container;unsigned kind{},owner{},owner_kind{},package{},package_kind{};
    if(container.dimension!=1||container.element_bytes!=bytes||!weaponhand::core_type(reader,container.type,L"StructProperty")||
       !retain(reader,container)||!reader.at(reader.result.identities[container.identity].address,0xd0,result.address)||
       !reader.path(result.address,result.layout.identity)||!reader.named(result.layout.identity,label)||
       !reader.path(reader.result.identities[result.layout.identity].klass,kind)||!weaponhand::core_type(reader,kind,L"ScriptStruct")||
       !reader.path(reader.result.identities[result.layout.identity].outer,owner)||!reader.named(owner,declaring_class)||
       !reader.path(reader.result.identities[owner].klass,owner_kind)||!weaponhand::core_type(reader,owner_kind,L"Class")||
       !reader.path(reader.result.identities[owner].outer,package)||!reader.named(package,package_name)||reader.result.identities[package].outer||
       !reader.path(reader.result.identities[package].klass,package_kind)||!weaponhand::core_type(reader,package_kind,L"Package")||
       !reader.at(result.address,0x60,result.layout.super)||result.layout.super||
       !reader.at(result.address,0x80,result.layout.children)||!reader.at(result.address,0x88,result.layout.extent)||result.layout.extent!=bytes)return false;
    output=result;return true;
}
template<std::size_t L,std::size_t O,std::size_t P>bool current(meta::Reader& reader,ScriptStruct const& original,
    wchar_t const (&label)[L],wchar_t const (&declaring_class)[O],wchar_t const (&package_name)[P]) noexcept {
    meta::Property container{};ScriptStruct closing{};
    return reader.property(reader.result.identities[original.container.identity].address,container)&&meta::same_property(original.container,container)&&
        read(reader,container,label,declaring_class,package_name,original.layout.extent,closing)&&closing.address==original.address&&
        meta::same_structure(original.layout,closing.layout);
}
template<std::size_t N>bool object_member(meta::Reader& reader,ScriptStruct const& structure,uptr container,
    wchar_t const (&label)[N],uptr expected) noexcept {
    auto child=structure.layout.children;std::array<uptr,32> visited{};unsigned count{},matches{};
    while(child){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        meta::Identity id{};meta::NameText text{};
        if(!meta::raw_identity(reader.result.objects,reader.result.names,child,id,text)||id.outer!=structure.address)return false;
        if(meta::exact_text(text,label)){
            meta::Property field{};uptr member_value{};
            if(id.name.number||++matches!=1||!reader.property(child,field)||field.dimension!=1||field.element_bytes!=8||
               !weaponhand::core_type(reader,field.type,L"ObjectProperty")||!meta::property_fits(field,structure.layout.extent)||
               !retain(reader,field)||!reader.at(container,field.offset,member_value)||member_value!=expected)return false;
        }
        if(!reader.at(child,0x68,child))return false;
    }
    return matches==1;
}
template<std::size_t N,std::size_t L,std::size_t S>bool numeric(meta::Reader& reader,meta::Property const& container,
    wchar_t const (&label)[L],wchar_t const (&scalar)[S],std::array<wchar_t const*,N> const& fields,ScriptStruct& reflected) noexcept {
    static_assert(N&&N<32);
    if(!read(reader,container,label,L"Object",L"Core",unsigned(N*4),reflected))return false;
    auto child=reflected.layout.children;std::array<uptr,32> visited{};unsigned count{},mask{};
    while(child){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
        meta::Identity id{};meta::NameText text{};meta::Property property{};
        if(!meta::raw_identity(reader.result.objects,reader.result.names,child,id,text)||id.outer!=reflected.address||id.name.number||
           !reader.property(child,property)||property.dimension!=1||property.element_bytes!=4||
           !weaponhand::core_type(reader,property.type,scalar)||!meta::property_fits(property,reflected.layout.extent)||!retain(reader,property))return false;
        unsigned selected=unsigned(N);
        for(unsigned i=0;i<N;++i){const auto length=std::wcslen(fields[i]);
            if(text.length==length&&!std::wmemcmp(text.units.data(),fields[i],length+1))selected=i;}
        if(selected==N||property.offset!=selected*4||(mask&(1u<<selected)))return false;
        mask|=1u<<selected;if(!reader.at(child,0x68,child))return false;
    }
    return mask==((1u<<N)-1);
}
} // namespace troika_reflection
