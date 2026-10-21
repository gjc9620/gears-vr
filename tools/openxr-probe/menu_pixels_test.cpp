#include "menu_pixels.h"
#include <cassert>
#include <cstdio>

int main(){
    menu_pixels::Sampler sampler;
    // Reuse one sampler across changes in aspect, size and odd widths. Compare
    // every byte against the old scalar image, including padded source rows.
    for(const auto sw:{1u,7u,127u,1280u,1920u,3840u})for(const auto sh:{1u,13u,37u}){
        const auto w=std::min(sw,1920u),h=sh;const std::size_t pitch=std::size_t(sw)*4+28;
        std::vector<unsigned char> source(pitch*sh),actual(std::size_t(w)*h*4),expected(actual.size());
        for(std::size_t i=0;i<source.size();++i)source[i]=static_cast<unsigned char>((i*73+i/17)%256);
        sampler.copy(actual.data(),w,h,source.data(),pitch,sw,sh);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)
            std::memcpy(expected.data()+(std::size_t(y)*w+x)*4,
                source.data()+std::size_t(std::uint64_t(y)*sh/h)*pitch+std::size_t(std::uint64_t(x)*sw/w)*4,4);
        assert(actual==expected);
        for(bool rgba:{false,true}){
            std::vector<unsigned char> uploaded(actual.size()+31,0xcd);
            menu_pixels::upload_row(uploaded.data()+3,actual.data(),w,rgba);
            for(unsigned x=0;x<w;++x)for(unsigned channel=0;channel<4;++channel){
                const auto source_channel=rgba&&channel!=1?2-channel:channel;
                assert(uploaded[3+x*4+channel]==(channel==3?255:actual[x*4+source_channel]));
            }
            assert(uploaded[2]==0xcd&&uploaded[3+std::size_t(w)*4]==0xcd);
        }
    }
    // Non-integer vertical downsample and sampler reconfiguration.
    for(unsigned repeat=0;repeat<3;++repeat){
        std::vector<unsigned char> input(59*33*4),out(19*11*4);
        for(std::size_t i=0;i<input.size();++i)input[i]=static_cast<unsigned char>(i);
        sampler.copy(out.data(),19,11,input.data(),59*4,59,33);
        for(unsigned y=0;y<11;++y)for(unsigned x=0;x<19;++x)
            assert(!std::memcmp(out.data()+(y*19+x)*4,input.data()+((y*33/11)*59+x*59/19)*4,4));
    }
    std::vector<unsigned char> pixels(103*4,0);
    const auto hash=menu_pixels::fingerprint(pixels.data(),103);
    for(std::size_t i=0;i<pixels.size();++i){pixels[i]=1;assert(menu_pixels::fingerprint(pixels.data(),103)!=hash);pixels[i]=0;}
    std::puts("MenuPixelsTest: exact scalar reference, format conversion, padding, tails, resize and fingerprint passed");
}
