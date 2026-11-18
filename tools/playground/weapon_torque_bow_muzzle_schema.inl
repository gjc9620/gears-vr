// Saved Engine.u names five parameters. Every live offset/type remains required.
enum class SchemaGate:unsigned {none,call,function,owner,native,parameters,cycle,property,name,duplicate,type,overlap,missing,return_offset,changed};
enum class TypeGate:unsigned {none,vector,vector_flags,bool_class,bool_width,bool_mask_read,bool_mask,bool_flags};
struct SchemaObservation {
    SchemaGate gate{};unsigned seen{},children{},bytes{},flags{},native{},return_offset{};
    std::array<unsigned,5> offsets{};
    TypeGate type_gate{};unsigned field_index{},field_type{},element_bytes{},mask{},mask_read{};
    uptr field_address{};std::uint64_t field_flags{};
};
struct Layout {
    enum Index:unsigned {trace_end,trace_start,box_extent,trace_bullet,result,count};
    uptr function{},exec{};unsigned bytes{},flags{},native{},return_offset{};
    std::array<unsigned,count> offsets{};unsigned bullet_mask{},result_mask{};
    bool operator==(Layout const& b) const noexcept {
        return function==b.function&&exec==b.exec&&bytes==b.bytes&&flags==b.flags&&native==b.native&&
            return_offset==b.return_offset&&offsets==b.offsets&&bullet_mask==b.bullet_mask&&result_mask==b.result_mask;
    }
};
template<class Call>bool schema(Call const& call,uptr receiver,meta::Reader& r,Layout& out,
    SchemaObservation* observation=nullptr) noexcept {
    out={};SchemaObservation trace_observation{};
    const auto decline=[&](SchemaGate reason){trace_observation.gate=reason;if(observation)*observation=trace_observation;return false;};
    unsigned fn{},kind{},owner{},package{};meta::Structure structure{};
    unsigned flags{};std::uint16_t native{},parameters{},return_offset{};uptr exec{};
    if(!call.valid||!call.r.size||call.r.size>64||call.r.size>sizeof(call.r.params))return decline(SchemaGate::call);
    if(!r.path(call.r.function,fn)||!r.named(fn,L"FastTrace")||
       !r.path(r.result.identities[fn].klass,kind)||!weaponhand::core_type(r,kind,L"Function"))return decline(SchemaGate::function);
    if(!r.path(r.result.identities[fn].outer,owner)||!r.named(owner,L"Actor")||
       !r.path(r.result.identities[owner].outer,package)||!r.named(package,L"Engine")||
       !f::owner_in_receiver(r,receiver,r.result.identities[owner].address)||r.result.identities[package].outer)
        return decline(SchemaGate::owner);
    if(!r.at(call.r.function,0xf4,flags)||!r.at(call.r.function,0xf8,native)||!r.at(call.r.function,0x114,exec))
        return decline(SchemaGate::native);
    trace_observation.flags=flags;trace_observation.native=native;
    if((flags&0x401)!=0x401||native!=548||
       !weaponhand::executable(reinterpret_cast<FARPROC>(exec),GetModuleHandleW(nullptr)))return decline(SchemaGate::native);
    if(!r.at(call.r.function,0x106,parameters)||!r.at(call.r.function,0x108,return_offset)||
       !r.structure(call.r.function,structure))return decline(SchemaGate::parameters);
    trace_observation.bytes=parameters;trace_observation.return_offset=return_offset;
    if(parameters!=call.r.size||parameters>structure.extent)return decline(SchemaGate::parameters);
    Layout value{};value.function=call.r.function;value.exec=exec;value.bytes=parameters;
    value.flags=flags;value.native=native;value.return_offset=return_offset;
    std::array<uptr,meta::max_fields> visited{};unsigned count{},seen{};std::uint64_t occupied{};
    for(auto child=structure.children;child;){
        if(count==visited.size())return decline(SchemaGate::cycle);
        for(unsigned i=0;i<count;++i)if(visited[i]==child)return decline(SchemaGate::cycle);
        visited[count++]=child;trace_observation.children=count;meta::Property p{};
        if(!r.property(child,p)||r.result.identities[p.identity].outer!=call.r.function||
           !(p.flags&0x80)||p.dimension!=1||!meta::property_fits(p,parameters)||!meta::property_fits(p,structure.extent))
            return decline(SchemaGate::property);
        bool retained=false;
        for(unsigned i=0;i<r.result.field_count;++i){
            if(r.result.fields[i].identity!=p.identity)continue;
            if(!meta::same_property(r.result.fields[i],p))return decline(SchemaGate::changed);
            retained=true;break;
        }
        if(!retained){
            if(r.result.field_count==r.result.fields.size())return decline(SchemaGate::property);
            r.result.fields[r.result.field_count++]=p;
        }
        unsigned index{};
        if(r.named(p.identity,L"TraceEnd"))index=Layout::trace_end;
        else if(r.named(p.identity,L"TraceStart"))index=Layout::trace_start;
        else if(r.named(p.identity,L"BoxExtent"))index=Layout::box_extent;
        else if(r.named(p.identity,L"bTraceBullet"))index=Layout::trace_bullet;
        else if(r.named(p.identity,L"ReturnValue"))index=Layout::result;
        else return decline(SchemaGate::name);
        if(seen&(1u<<index))return decline(SchemaGate::duplicate);
        seen|=1u<<index;trace_observation.seen=seen;trace_observation.offsets[index]=p.offset;
        trace_observation.field_index=index;trace_observation.field_type=p.type;
        trace_observation.field_address=child;trace_observation.field_flags=p.flags;
        trace_observation.element_bytes=p.element_bytes;trace_observation.mask=trace_observation.mask_read=0;
        const auto type_decline=[&](TypeGate reason){trace_observation.type_gate=reason;return decline(SchemaGate::type);};
        if(index<Layout::trace_bullet){
            if(!f::core_struct(r,p,L"Vector"))return type_decline(TypeGate::vector);
            if((p.flags&0x580)!=0x80)return type_decline(TypeGate::vector_flags);
        }else{
            unsigned mask{};
            // Keep the original short-circuit read order and exact admission rules.
            if(!weaponhand::core_type(r,p.type,L"BoolProperty"))return type_decline(TypeGate::bool_class);
            if(p.element_bytes!=4)return type_decline(TypeGate::bool_width);
            if(!r.at(child,0xd0,mask))return type_decline(TypeGate::bool_mask_read);
            trace_observation.mask_read=1;trace_observation.mask=mask;
            if(!mask||(mask&(mask-1)))return type_decline(TypeGate::bool_mask);
            // Live FastTrace ReturnValue (53860): Parm|OutParm|ReturnParm.
            if((p.flags&0x580)!=(index==Layout::result?0x580u:0x80u))return type_decline(TypeGate::bool_flags);
            if(index==Layout::trace_bullet)value.bullet_mask=mask;else value.result_mask=mask;
        }
        // Distinct storage only; no speculative packed-bool ABI fallback.
        const unsigned width=index<Layout::trace_bullet?12:4;
        for(unsigned i=0;i<width;++i){
            const auto bit=std::uint64_t(1)<<(p.offset+i);
            if(occupied&bit)return decline(SchemaGate::overlap);occupied|=bit;
        }
        value.offsets[index]=p.offset;child=p.next;
    }
    if(seen!=31)return decline(SchemaGate::missing);
    if(value.offsets[Layout::result]!=return_offset)return decline(SchemaGate::return_offset);
    out=value;if(observation)*observation=trace_observation;return true;
}
