#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

// Fresh characters only. No text, descriptor or permission cache. A failed
// read is terminal (notably PAGE_GUARD); never retry it with a smaller copy.
namespace fresh_wide_name {
template<class Read> bool text(std::uintptr_t from,wchar_t* out,std::size_t count,
                              std::size_t page,Read&& read) noexcept {
    static_assert(sizeof(wchar_t)==2);
    if(!from||!out||!count||page<sizeof(wchar_t)||
       count>(std::numeric_limits<std::uintptr_t>::max()-from)/sizeof(wchar_t))return false;
    for(std::size_t at=0;at<count;){
        const auto address=from+at*sizeof(wchar_t);
        const auto room=page-address%page;
        // Read the bounded remainder of THIS page once. Splitting the same
        // readable page into eight-character reads adds no identity or fault
        // protection; every character and terminator still comes from memory.
        auto chars=(std::min)(count-at,room/sizeof(wchar_t));
        // Preserve the scalar read for an unaligned wchar crossing a page.
        if(!chars)chars=1;
        if(!read(out+at,address,chars*sizeof(wchar_t)))return false;
        for(std::size_t i=0;i<chars;++i)if(!out[at+i])return true;
        at+=chars;
    }
    out[count-1]=0;return false;
}
}
