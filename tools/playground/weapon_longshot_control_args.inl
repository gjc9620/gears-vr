// Local metadata addresses only. No input, action permission or result cache.
// The caller still checks both native function identities and every argument.
bool longshot_control_args(uptr node,playground::Args& stop,playground::Args& seek) noexcept {
    if(!registered(node))return false;
    const auto cls=get<uptr>(node+0x50);
    const wchar_t* labels[]{L"StopAnim",L"SetPosition"};std::array<uptr,2> functions{};
    if(!playground::fields(cls,labels,functions))return false;
    // fields selects the first NAME, while Args uses the first matching
    // Function. A same-name property of another type must retain that original
    // typed walk, including derived-to-base shadowing and its existing bounds.
    for(unsigned i=0;i<functions.size();++i)
        if(!playground::named(get<uptr>(functions[i]+0x50),L"Function"))
            functions[i]=playground::field(cls,labels[i],L"Function");
    // No Engine call occurred. Reject a changed receiver class rather than
    // mixing two layouts into the same prepared operation.
    if(!registered(node)||get<uptr>(node+0x50)!=cls)return false;
    const auto bind=[&](playground::Args& call,uptr function) noexcept {
        call.r.object=node;call.r.function=function;
        call.r.size=function?get<unsigned short>(function+0x106):0;
        call.valid=function&&call.r.size<=4096;
    };
    bind(stop,functions[0]);bind(seek,functions[1]);return true;
}
