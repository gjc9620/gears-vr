// Included in the muzzle namespace after its metadata/weaponhand aliases.
// A binding belongs to ONE stack-local Args object/schema observation. It
// contains offsets only: no engine object/input/pose permission is cached.
struct SocketGetterLayout {
    enum Index:U32 {input_name,output_location,output_rotation,result,count};
    uptr function{};U32 bytes{};std::array<U32,count> offsets{};bool valid{};
    template<class Call>bool storage(Call const& call,Index index,std::size_t size) const noexcept {
        constexpr U32 widths[count]={8,12,12,4};
        return valid&&call.valid&&function==call.r.function&&bytes==call.r.size&&
            bytes<=sizeof(call.r.params)&&index<count&&size==widths[index]&&
            offsets[index]<=bytes&&size<=bytes-offsets[index];
    }
    template<class Call,class T>bool write(Call& call,Index index,T const& value) const noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        if(index!=input_name||!storage(call,index,sizeof(T)))return false;
        std::memcpy(call.r.params+offsets[index],&value,sizeof(T));return true;
    }
    template<class Call,class T>bool read(Call const& call,Index index,T& out) const noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        if(index==input_name||!storage(call,index,sizeof(T)))return false;
        T value{};std::memcpy(&value,call.r.params+offsets[index],sizeof(T));out=value;return true;
    }
};
bool retained_metadata(metadata::Reader& r) noexcept {
    auto& schema=r.result;
    const auto identities=schema.identity_count,structures=schema.structure_count,fields=schema.field_count;
    for(U32 i=0;i<identities;++i){U32 slot{};
        if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(U32 i=0;i<structures;++i){metadata::Structure current{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,current)||
           !metadata::same_structure(current,schema.structures[i]))return false;}
    for(U32 i=0;i<fields;++i){metadata::Property current{};
        if(!r.property(schema.identities[schema.fields[i].identity].address,current)||
           !metadata::same_property(current,schema.fields[i]))return false;}
    for(U32 i=0;i<schema.name_count;++i){metadata::NameText current{};
        if(!metadata::fresh_name(schema.names,schema.name_text[i].index,current)||
           !metadata::same_name_text(current,schema.name_text[i]))return false;}
    metadata::Table objects{},names{};
    return base==schema.module_base&&metadata::tables(base,objects,names)&&
        metadata::same_table(objects,schema.objects)&&metadata::same_table(names,schema.names)&&
        schema.failure.reason==metadata::Reason::none;
}
template<std::size_t N>bool core_struct(metadata::Reader& r,metadata::Property const& property,
    wchar_t const(&label)[N]) noexcept {
    uptr structure{};U32 slot{},kind{},owner{},owner_kind{},package{},extent{};
    return weaponhand::core_type(r,property.type,L"StructProperty")&&property.element_bytes==12&&
        r.at(r.result.identities[property.identity].address,0xd0,structure)&&r.path(structure,slot)&&r.named(slot,label)&&
        r.path(r.result.identities[slot].klass,kind)&&weaponhand::core_type(r,kind,L"ScriptStruct")&&
        r.path(r.result.identities[slot].outer,owner)&&r.named(owner,L"Object")&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&r.named(package,L"Core")&&
        !r.result.identities[package].outer&&r.at(structure,0x88,extent)&&extent==12;
}
template<class Call>bool socket_getter_schema(Call const& call,metadata::Reader& r,U32& return_mask,
    SocketGetterLayout* output=nullptr) noexcept {
    SocketGetterLayout layout{};layout.function=call.r.function;layout.bytes=call.r.size;
    if(!call.valid||!call.r.function||call.r.size>4096)return false;
    U32 fn{},kind{},owner{},package{};metadata::Structure structure{};
    if(!r.path(call.r.function,fn)||!r.named(fn,L"GetSocketWorldLocationAndRotation")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"SkeletalMeshComponent")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||
       r.result.identities[package].outer||!r.structure(call.r.function,structure))return false;
    std::uint16_t parameter_bytes{},return_offset{};
    if(!r.at(call.r.function,0x106,parameter_bytes)||parameter_bytes!=call.r.size||
       !r.at(call.r.function,0x108,return_offset))return false;
    unsigned found{};std::array<uptr,16> visited{};unsigned count{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;metadata::Property property{};
        if(!r.property(child,property)||r.result.identities[property.identity].outer!=call.r.function||
           property.dimension!=1||!metadata::property_fits(property,structure.extent)||
           ((property.flags&0x80)&&!metadata::property_fits(property,parameter_bytes)))return false;
        // Pre/post walks still read every descriptor afresh. Retain one
        // closing-check entry per identity, rejecting any changed descriptor
        // immediately rather than appending the same entry a second time.
        // This binding is local to this getter query, never another frame.
        bool retained=false;
        for(U32 i=0;i<r.result.field_count;++i){
            if(r.result.fields[i].identity!=property.identity)continue;
            if(!metadata::same_property(r.result.fields[i],property))return false;
            retained=true;break;
        }
        if(!retained){
            if(r.result.field_count==r.result.fields.size())return false;
            r.result.fields[r.result.field_count++]=property;
        }
        if(property.flags&0x80){
            unsigned bit{};
            if(r.named(property.identity,L"InSocketName")&&weaponhand::core_type(r,property.type,L"NameProperty")&&
               property.element_bytes==8&&(property.flags&0x580)==0x80)bit=1;
            else if(r.named(property.identity,L"OutLocation")&&core_struct(r,property,L"Vector")&&
               (property.flags&0x580)==0x180)bit=2;
            else if(r.named(property.identity,L"OutRotation")&&core_struct(r,property,L"Rotator")&&
               (property.flags&0x580)==0x180)bit=4;
            else if(r.named(property.identity,L"ReturnValue")&&weaponhand::core_type(r,property.type,L"BoolProperty")&&
               property.element_bytes==4&&(property.flags&0x480)==0x480&&property.offset==return_offset){
                if(!r.at(child,0xd0,return_mask)||!return_mask||(return_mask&(return_mask-1)))return false;
                bit=8;
            }else if(r.named(property.identity,L"Space")&&weaponhand::core_type(r,property.type,L"IntProperty")&&
               property.element_bytes==4&&(property.flags&0x590)==0x90){
                // Optional coordinate-space argument stays at its ordinary
                // zero/world default in Args' zero-initialized parameter block.
                bit=16;
            }else return false;
            if(found&bit)return false;found|=bit;
            if(bit!=16){
                const auto index=bit==1?SocketGetterLayout::input_name:bit==2?SocketGetterLayout::output_location:
                    bit==4?SocketGetterLayout::output_rotation:SocketGetterLayout::result;
                layout.offsets[index]=property.offset;
            }
        }
        child=property.next;
    }
    if((found&15)!=15||!retained_metadata(r))return false;
    layout.valid=true;
    if(output)*output=layout;
    return true;
}
