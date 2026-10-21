#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <emmintrin.h>

namespace menu_pixels {
// Keep the existing nearest-neighbour image exactly, but calculate horizontal
// divisions only when dimensions change. Equal-sized rows need just memcpy.
class Sampler {
    unsigned sw_=0,sh_=0,w_=0,h_=0;
    std::vector<std::size_t> columns_;
public:
    void copy(unsigned char* dst,unsigned w,unsigned h,const unsigned char* src,
              std::size_t pitch,unsigned sw,unsigned sh){
        if(sw!=sw_||sh!=sh_||w!=w_||h!=h_){
            sw_=sw;sh_=sh;w_=w;h_=h;columns_.resize(w);
            for(unsigned x=0;x<w;++x)columns_[x]=std::size_t(std::uint64_t(x)*sw/w)*4;
        }
        for(unsigned y=0;y<h;++y){
            const auto* row=src+std::size_t(std::uint64_t(y)*sh/h)*pitch;
            auto* out=dst+std::size_t(y)*w*4;
            if(w==sw)std::memcpy(out,row,std::size_t(w)*4);
            else for(unsigned x=0;x<w;++x)std::memcpy(out+std::size_t(x)*4,row+columns_[x],4);
        }
    }
};

// Diagnostic fingerprint of every source pixel, four independent FNV lanes.
// It is not the old serial byte-FNV value or a cryptographic content identity.
inline std::uint64_t fingerprint(const unsigned char* pixels,std::size_t count){
    std::uint64_t a=14695981039346656037ULL,b=a,c=a,d=a;
    constexpr std::uint64_t prime=1099511628211ULL;
    std::size_t i=0;
    for(;i+4<=count;i+=4){
        std::uint32_t words[4];std::memcpy(words,pixels+i*4,sizeof(words));
        a=(a^words[0])*prime;b=(b^words[1])*prime;c=(c^words[2])*prime;d=(d^words[3])*prime;
    }
    a=(((a^b)*prime^c)*prime^d)*prime;
    for(;i<count;++i){std::uint32_t word;std::memcpy(&word,pixels+i*4,4);a=(a^word)*prime;}
    return a;
}

// x64 SSE2 is part of the existing target baseline. Unaligned loads/stores and
// a scalar tail cover arbitrary WGC widths and D3D12 row pitches.
inline void upload_row(unsigned char* dst,const unsigned char* src,unsigned width,bool rgba){
    const auto alpha=_mm_set1_epi32(-16777216);
    const auto rb=_mm_set1_epi32(0x00ff00ff);
    const auto green=_mm_set1_epi32(0x0000ff00);
    unsigned x=0;
    for(;x+4<=width;x+=4){
        auto value=_mm_loadu_si128(reinterpret_cast<const __m128i*>(src+std::size_t(x)*4));
        if(rgba){
            const auto swapped=_mm_or_si128(_mm_slli_epi32(_mm_and_si128(value,rb),16),
                                           _mm_and_si128(_mm_srli_epi32(value,16),rb));
            value=_mm_or_si128(_mm_and_si128(value,green),swapped);
        }
        value=_mm_or_si128(value,alpha);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst+std::size_t(x)*4),value);
    }
    for(;x<width;++x){
        dst[x*4]=src[x*4+(rgba?2:0)];dst[x*4+1]=src[x*4+1];
        dst[x*4+2]=src[x*4+(rgba?0:2)];dst[x*4+3]=255;
    }
}
}
