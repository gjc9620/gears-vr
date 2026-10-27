#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
bool deny_chinese{},fail_glyph_query{};
// Host font-availability seam only. Drawing, fonts, memory DC/DIB, file I/O
// and cleanup still use actual Win32. No copied renderer or real XR session.
DWORD WINAPI observed_glyph_indices(HDC dc,LPCWSTR text,int count,LPWORD output,DWORD flags){
    if(fail_glyph_query)return GDI_ERROR;
    if(deny_chinese&&std::any_of(text,text+count,[](wchar_t c){return c>=0x3000;})){
        std::fill(output,output+count,WORD(0xffff));return DWORD(count);
    }
    return ::GetGlyphIndicesW(dc,text,count,output,flags);
}
}
#define GetGlyphIndicesW observed_glyph_indices
#include "armspan_card_pixels.h"
#undef GetGlyphIndicesW

namespace card=armspan_card_pixels;
namespace {
unsigned checks{};
void check(bool okay){++checks;if(!okay){std::fprintf(stderr,"armspan card check %u failed\n",checks);std::exit(1);}}
template<class T>std::array<unsigned char,sizeof(T)> bytes(T const& object){
    std::array<unsigned char,sizeof(T)> result{};std::memcpy(result.data(),&object,sizeof(object));return result;
}
using RGB=std::array<std::uint8_t,3>;
RGB at(std::vector<std::uint8_t> const& image,unsigned x,unsigned y){
    const auto i=(std::size_t(y)*card::width+x)*4;return {{image[i],image[i+1],image[i+2]}};
}
void inspect(std::vector<std::uint8_t> const& image,card::Content const& content){
    check(image.size()==std::size_t(card::width)*card::height*4);
    bool opaque=true;unsigned heading{},body{},green{},yellow{};
    for(std::size_t i=0;i<image.size();i+=4){
        opaque=opaque&&image[i+3]==255;const unsigned x=unsigned(i/4%card::width),y=unsigned(i/4/card::width);
        const RGB rgb{{image[i],image[i+1],image[i+2]}};
        if(x>=252&&x<732&&y>=36&&y<82&&rgb==card::detail::title)++heading;
        if(x>=252&&x<732&&y>=98&&y<165&&rgb==card::detail::hint)++body;
        if(rgb==card::detail::green)++green;if(rgb==card::detail::yellow)++yellow;
    }
    check(opaque&&heading>150&&body>150);
    check(at(image,0,0)==card::detail::background&&at(image,20,20)==card::detail::border);
    bool right_margin=true;
    for(unsigned y=21;y<235;++y)right_margin=right_margin&&at(image,733,y)==card::detail::background;
    check(right_margin);
    const bool saved=content.phase==card::Phase::saved;
    const bool stopped=content.phase==card::Phase::cancelled||content.phase==card::Phase::failed;
    const RGB accent=saved?card::detail::green:stopped?card::detail::muted:card::detail::yellow;
    check(at(image,55,138)==accent&&at(image,199,138)==accent);
    check(saved?green>5000:green==0);
    check(stopped?yellow==0:saved||yellow>150);
    if(content.phase==card::Phase::waiting||stopped)
        check(at(image,252,212)==card::detail::track&&at(image,731,212)==card::detail::track);
    if(content.phase==card::Phase::holding){
        if(content.progress_permille==0)check(at(image,252,212)==card::detail::track);
        if(content.progress_permille==500)check(at(image,490,212)==card::detail::yellow&&at(image,492,212)==card::detail::track);
        if(content.progress_permille==999)check(at(image,730,212)==card::detail::yellow&&at(image,731,212)==card::detail::track);
    }
    if(saved)check(at(image,252,212)==card::detail::green&&at(image,731,212)==card::detail::green);
}
std::vector<unsigned char> load(std::filesystem::path const& path){
    std::ifstream stream(path,std::ios::binary);return {std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
}
void invalid_inputs(){
    using card::Content;using card::Phase;using card::Language;
    std::vector<std::uint8_t> image{11,22,33,44};const auto original=image;
    card::Metadata metadata{Language::chinese,true};const auto old_metadata=bytes(metadata);
    for(Content content:{Content{Phase::waiting,1,Language::english},Content{Phase::holding,1000,Language::english},
        Content{Phase::saved,999,Language::english},Content{Phase::cancelled,500,Language::english},
        Content{Phase::failed,1000,Language::english},Content{static_cast<Phase>(0),0,Language::english},
        Content{Phase::waiting,0,static_cast<Language>(2)},Content{Phase::holding,0xffffffffu,Language::english}}){
        check(!card::rgba(content,image,&metadata));check(image==original&&bytes(metadata)==old_metadata);
    }
    check(!card::save_ppm("unused-armspan-invalid.ppm",image));
    check(!card::save_ppm("",image));
    fail_glyph_query=true;
    check(!card::rgba({Phase::waiting,0,Language::english},image,&metadata));
    fail_glyph_query=false;check(image==original&&bytes(metadata)==old_metadata);
}
void fallback(){
    using card::Language;using card::Phase;
    std::vector<std::uint8_t> english,missing;card::Metadata meta{};
    check(card::rgba({Phase::holding,500,Language::english},english));
    deny_chinese=true;check(card::rgba({Phase::holding,500,Language::chinese},missing,&meta));deny_chinese=false;
    check(meta.rendered_language==Language::english&&meta.fallback&&missing==english);
    check(card::rgba({Phase::waiting,0,Language::english},english,&meta));
    check(meta.rendered_language==Language::english&&!meta.fallback);
}
void previews(std::filesystem::path const& directory){
    using card::Phase;using card::Language;
    const std::array<std::pair<card::Content,char const*>,7> cases{{
        {{Phase::waiting,0,Language::english},"waiting"},{{Phase::holding,0,Language::english},"holding-0"},
        {{Phase::holding,500,Language::english},"holding-500"},{{Phase::holding,999,Language::english},"holding-999"},
        {{Phase::saved,1000,Language::english},"saved"},{{Phase::cancelled,0,Language::english},"cancelled"},
        {{Phase::failed,0,Language::english},"failed"}}};
    for(auto language:{Language::english,Language::chinese})for(auto test:cases){
        test.first.language=language;std::vector<std::uint8_t> image,repeat;card::Metadata metadata{},again{};
        check(card::rgba(test.first,image,&metadata));check(card::rgba(test.first,repeat,&again));
        check(image==repeat&&metadata.rendered_language==again.rendered_language&&metadata.fallback==again.fallback);
        check(metadata.rendered_language==language||(language==Language::chinese&&metadata.rendered_language==Language::english&&metadata.fallback));
        inspect(image,test.first);
        const auto prefix=language==Language::english?"en-":"zh-";
        if(!directory.empty()){
            const auto path=directory/(std::string(prefix)+test.second+".ppm");check(card::save_ppm(path.string(),image));
            const auto raw=load(path);const std::string header="P6\n768 256\n255\n";
            check(raw.size()==header.size()+std::size_t(card::width)*card::height*3);
            check(std::equal(header.begin(),header.end(),raw.begin()));
            const auto at_rgb=header.size()+(std::size_t(212)*card::width+731)*3;
            const auto expected=at(image,731,212);check(std::equal(expected.begin(),expected.end(),raw.begin()+std::ptrdiff_t(at_rgb)));
        }
        std::printf("preview=%s%s requested_language=%u rendered_language=%u fallback=%u progress=%u\n",prefix,test.second,
            unsigned(language),unsigned(metadata.rendered_language),unsigned(metadata.fallback),test.first.progress_permille);
    }
    if(!directory.empty()){
        const auto existing=directory/"invalid-save-preserves.txt";{std::ofstream file(existing,std::ios::binary);file<<"existing preview bytes";}
        const auto before=load(existing);check(!card::save_ppm(existing.string(),{}));check(load(existing)==before);
        std::vector<std::uint8_t> image;check(card::rgba({Phase::saved,1000,Language::english},image));image[3]=0;
        check(!card::save_ppm(existing.string(),image));check(load(existing)==before);
        image[3]=255;check(!card::save_ppm(directory.string(),image));
        bool temporary=false;for(auto const& entry:std::filesystem::directory_iterator(directory.parent_path()))
            temporary=temporary||entry.path().filename().string().find(directory.filename().string()+".armspan-")==0;
        check(!temporary);
    }
}
void no_leaks(){
    std::vector<std::uint8_t> pixels;card::Metadata metadata{};
    check(card::rgba({card::Phase::waiting,0,card::Language::chinese},pixels));
    const DWORD gdi_before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);DWORD handles_before{};
    check(GetProcessHandleCount(GetCurrentProcess(),&handles_before)!=FALSE);
    for(unsigned i=0;i<48;++i){
        const bool failed=i%4==0;fail_glyph_query=failed;deny_chinese=i%4==1;
        check(card::rgba({card::Phase::holding,i*20,card::Language::chinese},pixels,&metadata)!=failed);
        fail_glyph_query=deny_chinese=false;
    }
    check(GdiFlush()!=FALSE);const DWORD gdi_after=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);DWORD handles_after{};
    check(GetProcessHandleCount(GetCurrentProcess(),&handles_after)!=FALSE);
    check(gdi_after==gdi_before&&handles_after==handles_before);
    std::printf("resources GDI=%lu->%lu process_handles=%lu->%lu after48 actual/failed/fallback renders\n",gdi_before,gdi_after,handles_before,handles_after);
}
}
int main(int argc,char* argv[]){
    std::filesystem::path directory;
    if(argc==3&&!std::strcmp(argv[1],"--preview-dir")){directory=argv[2];if(!std::filesystem::is_directory(directory))return 2;}
    else if(argc!=1)return 2;
    invalid_inputs();fallback();previews(directory);no_leaks();
    std::printf("Armspan card %ux%u opaque RGBA: %u checks passed; memory GDI only, no windows/session/measurement records\n",card::width,card::height,checks);return 0;
}
