// Included inside playground, after get/name. One query-local, untyped first
// name selection in derived-to-base order. Descriptor/currentness checks are
// still the caller's responsibility. This is the unchanged shared fields walk.
struct PlainFieldTraversal {};
template<std::size_t N,bool Strict>
bool fields_query(uptr cls,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    out={};std::array<uptr,N> found{};std::size_t remaining=N,limit=128;
    for(auto label:labels)if(!label)return false;
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    limit=1;
    for(auto label:labels){
        limit=(std::max)(limit,component_names::query_limit(label));
    }
    if(!component_names::enabled)limit=128;
#endif
    // A shortened unmatched prefix is not an unreadable name. Strict source
    // selection needs the full bounded name before making that distinction.
    if constexpr(Strict)limit=128;
    [[maybe_unused]] std::array<uptr,40> parents{};
    for(unsigned depth=0;cls&&depth<40;++depth){
        if constexpr(Strict){for(unsigned p=0;p<depth;++p)if(parents[p]==cls)return false;parents[depth]=cls;}
        uptr f{};
        if constexpr(Strict){if(!copy(&f,cls+0x80,sizeof f))return false;}
        else f=get<uptr>(cls+0x80);
        [[maybe_unused]] std::array<uptr,4096> seen;
        [[maybe_unused]] unsigned count{};
        [[maybe_unused]] std::conditional_t<Strict,weapon_metadata_parse::Walk,PlainFieldTraversal> traversal;
        for(unsigned n=0;f&&n<4096;++n){
            if constexpr(Strict){if(traversal.seen(f,seen.data(),count))return false;seen[count++]=f;}
            wchar_t text[128]{};
            if(name(f,text,limit)){
                for(std::size_t i=0;i<N;++i){
                    if(found[i]||_wcsicmp(text,labels[i]))continue;
                    found[i]=f;--remaining;
                }
                if(!remaining){out=found;return true;}
            }else if constexpr(Strict)return false;
            if constexpr(Strict){uptr next{};if(!copy(&next,f+0x68,sizeof next))return false;f=next;}
            else f=get<uptr>(f+0x68);
        }
        if constexpr(Strict){
            if(f)return false;
            uptr next{};if(!copy(&next,cls+0x60,sizeof next))return false;cls=next;
        }else cls=get<uptr>(cls+0x60);
    }
    return false;
}

template<std::size_t N>
bool fields(uptr cls,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    return fields_query<N,false>(cls,labels,out);
}
// Same first-match traversal, with fail-closed reads/cycles for retained source
// links. Local addresses only; concrete types/layouts and closure stay with the
// caller. The existing fields entry keeps its historical behavior.
template<std::size_t N>
bool fields_strict(uptr cls,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    return fields_query<N,true>(cls,labels,out);
}
