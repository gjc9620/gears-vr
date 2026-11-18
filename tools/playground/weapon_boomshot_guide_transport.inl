// Existing original Actor.DrawDebugLine, transient and bounded. No render hook,
// persistent primitive, material mutation, collision or part write is added.
namespace boomshot_guide {
bool line_call(hand::Frame const& f,playground::Args& call,hand::ObjectIdentity& fn) noexcept {
    if(!current(f)||!call.valid||!registered(call.r.function)||
       !playground::named(get<uptr>(call.r.function+0x50),L"Function")||!call.r.size||call.r.size>64)return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    if(!registered(owner)||!registered(package)||!playground::named(owner,L"Actor")||!playground::named(package,L"Engine")||
       get<uptr>(package+0x40)||!nativeequipment::identify(nullptr,call.r.function,fn)||
       (get<unsigned>(call.r.function+0xf4)&0x2400)!=0x2400||get<std::uint16_t>(call.r.function+0x108)!=0xffff||
       !weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr)))return false;
    const wchar_t* labels[]={L"LineStart",L"LineEnd",L"R",L"G",L"B",L"bPersistentLines"};
    const wchar_t* types[]={L"StructProperty",L"StructProperty",L"ByteProperty",L"ByteProperty",L"ByteProperty",L"BoolProperty"};
    const unsigned widths[]={12,12,1,1,1,4};std::uint64_t occupied{};std::array<uptr,6> properties{};
    for(unsigned i=0;i<6;++i){const auto p=call.prop(labels[i]);const auto offset=get<unsigned>(p+0x94);
        if(!registered(p)||get<uptr>(p+0x40)!=call.r.function||!playground::named(get<uptr>(p+0x50),types[i])||
           get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=widths[i]||
           (get<unsigned long long>(p+0x78)&0x580)!=0x80||offset>call.r.size||widths[i]>call.r.size-offset)return false;
        properties[i]=p;
        for(unsigned j=0;j<widths[i];++j){const auto bit=std::uint64_t(1)<<(offset+j);if(occupied&bit)return false;occupied|=bit;}
        if(i<2){const auto type=get<uptr>(p+0xd0);if(!registered(type)||!playground::named(type,L"Vector")||get<unsigned>(type+0x88)!=12)return false;}
    }
    std::array<uptr,32> seen{};unsigned count{},params{};
    for(auto p=get<uptr>(call.r.function+0x80);p;p=get<uptr>(p+0x68)){
        if(count==seen.size()||!registered(p))return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==p)return false;seen[count++]=p;
        if(!(get<unsigned long long>(p+0x78)&0x80))continue;
        if(std::find(properties.begin(),properties.end(),p)==properties.end())return false;++params;
    }
    return params==6&&call.set(L"R",static_cast<unsigned char>(255)).set(L"G",static_cast<unsigned char>(220))
        .set(L"B",static_cast<unsigned char>(0)).valid&&nativelancerfollow::bool_arg(call,L"bPersistentLines",false);
}
bool draw(hand::Frame const& f,guide::Shape const& shape,std::int64_t began) noexcept {
    if(!shape.count||shape.count>shape.lines.size()||weaponhand::clock_ns(nullptr)-began>=8000000||!current(f))return false;
    playground::Args call(f.world.address,L"DrawDebugLine");hand::ObjectIdentity function{};
    if(!line_call(f,call,function)||weaponhand::clock_ns(nullptr)-began>=8000000)return false;
    // Consume the batch before any original dispatch; unknown return/reentry
    // cannot repeat it. Another authenticated frame may draw its own transient.
    drawn_source=f.key;drawn_known=true;unsigned drawn{};
    for(unsigned i=0;i<shape.count;++i){hand::ObjectIdentity actual{};
        if(weaponhand::clock_ns(nullptr)-began>=8000000||!transport_current(f)||!registered(f.world.address)||
           !nativeequipment::identify(nullptr,function.address,actual)||!hand::same(function,actual))break;
        const auto& edge=shape.lines[i];
        const playground::Vector start{float(edge.start.x),float(edge.start.y),float(edge.start.z)},end{float(edge.end.x),float(edge.end.y),float(edge.end.z)};
        if(!call.set(L"LineStart",start).set(L"LineEnd",end).set(L"R",edge.red).set(L"G",edge.green).set(L"B",edge.blue).valid||!call.run())break;++drawn;
    }
    return drawn==shape.count&&current(f);
}
}
