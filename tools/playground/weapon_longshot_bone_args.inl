// One local class traversal for the two getters. No Engine call or cache.
// The original reader still validates descriptors, fields, bone indices,
// returned matrices, receiver identity and final source currentness.
bool longshot_bone_args(uptr component,playground::Args& name,playground::Args& matrix) noexcept {
    if(!registered(component))return false;
    const auto cls=get<uptr>(component+0x50);
    const wchar_t* labels[]{L"MatchRefBone",L"GetBoneMatrix"};std::array<uptr,2> functions{};
    if(!playground::fields(cls,labels,functions))return false;
    for(unsigned i=0;i<functions.size();++i)
        if(!playground::named(get<uptr>(functions[i]+0x50),L"Function"))
            functions[i]=playground::field(cls,labels[i],L"Function");
    if(!registered(component)||get<uptr>(component+0x50)!=cls)return false;
    const auto bind=[&](playground::Args& call,uptr function) noexcept {
        call.r.object=component;call.r.function=function;
        call.r.size=function?get<unsigned short>(function+0x106):0;
        call.valid=function&&call.r.size<=4096;
    };
    bind(name,functions[0]);bind(matrix,functions[1]);return true;
}
