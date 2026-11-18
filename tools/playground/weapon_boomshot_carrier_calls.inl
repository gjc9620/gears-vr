// Exact original carrier calls, included in boomshot_carrier_original.
struct Layout {
    uptr function{},exec{},component_class{};unsigned bytes{},flags{},native_index{},result{},mask{};
    std::array<unsigned,3> offsets{};
    bool operator==(Layout const& b)const noexcept {
        return function==b.function&&exec==b.exec&&bytes==b.bytes&&flags==b.flags&&native_index==b.native_index&&component_class==b.component_class&&result==b.result&&mask==b.mask&&offsets==b.offsets;
    }
};
template<class Call,std::size_t N,std::size_t K>bool signature(Call const& call,uptr receiver,const wchar_t (&label)[N],const wchar_t (&owner)[K],
    bool is_native,unsigned shape,meta::Reader& r,Layout& out) noexcept {
    out={};unsigned fn{},kind{},owner_index{},package{},flags{};std::uint16_t bytes{},returned{},native{};uptr exec{};
    meta::Structure structure{};
    if(!call.valid||call.r.size>32||!r.path(call.r.function,fn)||!r.named(fn,label)||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner_index)||!r.named(owner_index,owner)||
       !r.path(r.result.identities[owner_index].klass,kind)||!weaponhand::core_type(r,kind,L"Class")||
       !r.path(r.result.identities[owner_index].outer,package)||!(is_native?r.named(package,L"Engine"):r.named(package,L"WarfareGame"))||
       r.result.identities[package].outer||!unified_weapon_fire::owner_in_receiver(r,receiver,r.result.identities[owner_index].address)||
       !r.at(call.r.function,0xf4,flags)||bool(flags&0x400)!=is_native||!r.at(call.r.function,0xf8,native)||(!is_native&&native)||
       !r.at(call.r.function,0x114,exec)||!weaponhand::executable(reinterpret_cast<FARPROC>(exec),GetModuleHandleW(nullptr))||
       !r.at(call.r.function,0x106,bytes)||bytes!=call.r.size||!r.at(call.r.function,0x108,returned)||
       !r.structure(call.r.function,structure)||bytes>structure.extent)return false;
    Layout value{};value.function=call.r.function;value.exec=exec;value.bytes=bytes;value.flags=flags;value.native_index=native;value.result=returned;
    unsigned fields{},seen{};std::array<uptr,meta::max_fields> visited{};
    for(auto child=structure.children;child;){
        if(fields==visited.size())return false;for(unsigned i=0;i<fields;++i)if(visited[i]==child)return false;
        visited[fields++]=child;meta::Property property{};
        if(!r.property(child,property)||r.result.identities[property.identity].outer!=call.r.function||property.dimension!=1||
           !meta::property_fits(property,bytes)||r.result.field_count==r.result.fields.size())return false;
        r.result.fields[r.result.field_count++]=property;
        unsigned bit{},width{};
        if(shape==1&&r.named(property.identity,L"bAttach")&&weaponhand::core_type(r,property.type,L"BoolProperty")&&
           property.element_bytes==4&&(property.flags&0x580)==0x80){
            bit=1;width=4;value.offsets[0]=property.offset;
            if(!r.at(child,0xd0,value.mask)||!value.mask||(value.mask&(value.mask-1)))return false;
        }else if(shape==2&&r.named(property.identity,L"Component")&&weaponhand::core_type(r,property.type,L"ComponentProperty")&&
           property.element_bytes==8&&(property.flags&0x580)==0x80){
            bit=1;width=8;value.offsets[0]=property.offset;
            uptr component_class{};unsigned component{},component_kind{},component_package{};
            if(!r.at(child,0xd0,component_class)||!r.path(component_class,component)||
               !r.path(r.result.identities[component].klass,component_kind)||!weaponhand::core_type(r,component_kind,L"Class")||
               !r.path(r.result.identities[component].outer,component_package)||!r.named(component_package,L"Engine")||
               r.result.identities[component_package].outer)return false;value.component_class=component_class;
        }else if(shape==2&&r.named(property.identity,L"BoneName")&&weaponhand::core_type(r,property.type,L"NameProperty")&&
           property.element_bytes==8&&((property.flags&0x580)==0x80||(property.flags&0x580)==0x180)){
            bit=2;width=8;value.offsets[1]=property.offset;
        }else if(shape==2&&r.named(property.identity,L"ReturnValue")&&weaponhand::core_type(r,property.type,L"BoolProperty")&&
           property.element_bytes==4&&((property.flags&0x580)==0x480||(property.flags&0x580)==0x580)&&property.offset==returned){
            bit=4;width=4;value.offsets[2]=property.offset;
            if(!r.at(child,0xd0,value.mask)||!value.mask||(value.mask&(value.mask-1)))return false;
        }else return false;
        if(seen&bit||property.offset>bytes||width>bytes-property.offset)return false;seen|=bit;child=property.next;
    }
    if(shape==0){if(bytes||seen||returned!=0xffff)return false;}
    else if(shape==1){if(bytes!=4||seen!=1||returned!=0xffff||value.offsets[0])return false;}
    else if(shape==2){
        if(seen!=7)return false;
        for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j){const unsigned wi=i==2?4:8,wj=j==2?4:8;
            if(value.offsets[i]<value.offsets[j]+wj&&value.offsets[j]<value.offsets[i]+wi)return false;}
    }else return false;
    if(!unified_weapon_fire::retained_metadata(r))return false;out=value;return true;
}
