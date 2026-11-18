#pragma once
#include <array>
#include <cstddef>

// Query-local first-match selection. Partial results deliberately survive a
// missing label, matching independent untyped field() queries returning zero.
// The reader always fetches live links/names; no descriptor survives this call.
namespace query_partial_fields {
template<class Address,std::size_t N,class Reader>
std::array<Address,N> select(Address cls,wchar_t const* const (&labels)[N],Reader&& reader) {
    std::array<Address,N> result{};std::size_t remaining=N;
    for(unsigned depth=0;cls&&depth<40;++depth,cls=reader.super(cls)){
        auto field=reader.children(cls);
        for(unsigned count=0;field&&count<4096;++count,field=reader.next(field)){
            wchar_t text[128]{};
            if(!reader.name(field,text,128))continue;
            for(std::size_t i=0;i<N;++i)
                if(!result[i]&&reader.equal(text,labels[i])){result[i]=field;--remaining;}
            if(!remaining)return result;
        }
    }
    return result;
}
}
