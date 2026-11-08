// Included inside playground, after get/name. One query-local, untyped first
// name selection in derived-to-base order. Descriptor/currentness checks are
// still the caller's responsibility. This is the unchanged shared fields walk.
template<std::size_t N>
bool fields(uptr cls,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    out={};std::array<uptr,N> found{};std::size_t remaining=N,limit=1;
    for(auto label:labels){
        if(!label)return false;
        limit=(std::max)(limit,component_names::query_limit(label));
    }
    if(!component_names::enabled)limit=128;
    for(unsigned depth=0;cls&&depth<40;++depth,cls=get<uptr>(cls+0x60)){
        auto f=get<uptr>(cls+0x80);
        for(unsigned n=0;f&&n<4096;++n,f=get<uptr>(f+0x68)){
            wchar_t text[128]{};
            if(!name(f,text,limit))continue;
            for(std::size_t i=0;i<N;++i){
                if(found[i]||_wcsicmp(text,labels[i]))continue;
                found[i]=f;--remaining;
            }
            if(!remaining){out=found;return true;}
        }
    }
    return false;
}
