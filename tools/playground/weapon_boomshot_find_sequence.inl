// Included in boomshot_inspection_source. Stack-local original getter binding.
struct FindLayout {
    uptr function{},exec{};unsigned bytes{},flags{},native{},input{},result{},return_offset{};
    bool operator==(FindLayout const& b) const noexcept {
        return function==b.function&&exec==b.exec&&bytes==b.bytes&&flags==b.flags&&native==b.native&&
            input==b.input&&result==b.result&&return_offset==b.return_offset;
    }
};
template<class Call>bool find_schema(Call const& call,uptr receiver,meta::Reader& r,FindLayout& out) noexcept {
    out={};unsigned fn{},kind{},owner{},package{},flags{};std::uint16_t bytes{},returned{},native{};
    uptr exec{};meta::Structure structure{};
    if(!call.valid||!call.r.size||call.r.size>64||call.r.size>sizeof(call.r.params)||
       !r.path(call.r.function,fn)||!r.named(fn,L"FindAnimSequence")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"SkeletalMeshComponent")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||r.result.identities[package].outer||
       !unified_weapon_fire::owner_in_receiver(r,receiver,r.result.identities[owner].address)||
       !r.at(call.r.function,0xf4,flags)||!(flags&0x400)||!r.at(call.r.function,0xf8,native)||
       !r.at(call.r.function,0x114,exec)||
       !weaponhand::executable(reinterpret_cast<FARPROC>(exec),GetModuleHandleW(nullptr))||
       !r.at(call.r.function,0x106,bytes)||bytes!=call.r.size||!r.at(call.r.function,0x108,returned)||
       !r.structure(call.r.function,structure)||bytes>structure.extent)return false;
    FindLayout value{};value.function=call.r.function;value.exec=exec;value.bytes=bytes;
    value.flags=flags;value.native=native;value.return_offset=returned;
    std::array<uptr,meta::max_fields> visited{};unsigned count{},seen{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return false;
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;
        visited[count++]=child;meta::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||p.dimension!=1||
           p.element_bytes!=8||!meta::property_fits(p,bytes))return false;
        bool retained=false;
        for(unsigned i=0;i<r.result.field_count;++i)if(r.result.fields[i].identity==p.identity){
            if(!meta::same_property(r.result.fields[i],p))return false;retained=true;break;
        }
        if(!retained){if(r.result.field_count==r.result.fields.size())return false;r.result.fields[r.result.field_count++]=p;}
        unsigned bit{};
        if(r.named(p.identity,L"AnimSeqName")&&weaponhand::core_type(r,p.type,L"NameProperty")&&(p.flags&0x580)==0x80){
            bit=1;value.input=p.offset;
        }else if(r.named(p.identity,L"ReturnValue")&&weaponhand::core_type(r,p.type,L"ObjectProperty")&&
            (p.flags&0x580)==0x480&&p.offset==returned){
            uptr result_class{};unsigned result_identity{},class_kind{},result_package{};
            if(!r.at(child,0xd0,result_class)||!r.path(result_class,result_identity)||!r.named(result_identity,L"AnimSequence")||
               !r.path(r.result.identities[result_identity].klass,class_kind)||!weaponhand::core_type(r,class_kind,L"Class")||
               !r.path(r.result.identities[result_identity].outer,result_package)||!r.named(result_package,L"Engine")||
               r.result.identities[result_package].outer)return false;
            bit=2;value.result=p.offset;
        }else return false;
        if(seen&bit)return false;seen|=bit;child=p.next;
    }
    if(seen!=3||!(value.input+8<=value.result||value.result+8<=value.input))return false;
    out=value;return true;
}
