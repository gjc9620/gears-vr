#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "../vr-common/armspan_ui_state.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <utility>
#include <vector>

// Literal display pixels only. Memory GDI, no window/screen/game capture and
// no measurement, persistence, tracking, control or session authority.
namespace armspan_card_pixels {
using Content=gears_armspan_ui::Content;
using Phase=gears_armspan_ui::Phase;
using Language=gears_armspan_ui::Language;
inline constexpr unsigned width=768,height=256;
struct Metadata {Language rendered_language{Language::english};bool fallback{};};
namespace detail {
using Color=std::array<std::uint8_t,3>;
inline constexpr Color background{{29,32,38}},border{{62,68,78}},title{{238,242,247}},hint{{178,188,200}};
inline constexpr Color yellow{{245,199,74}},green{{99,216,147}},muted{{136,148,163}},track{{55,62,74}};
struct Selection {
    HDC dc{};HGDIOBJ previous{};
    Selection(HDC target,HGDIOBJ object):dc(target),previous(SelectObject(target,object)){}
    bool valid() const noexcept {return previous&&previous!=HGDI_ERROR;}
    ~Selection(){if(valid())SelectObject(dc,previous);}
    Selection(Selection const&)=delete;Selection& operator=(Selection const&)=delete;
};
struct Surface {
    HDC dc{};HBITMAP bitmap{};HGDIOBJ previous{};std::uint8_t* bits{};
    Surface(){
        dc=CreateCompatibleDC(nullptr);if(!dc)return;
        BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-static_cast<LONG>(height);
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* memory{};bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&memory,nullptr,0);
        if(!bitmap||!memory)return;bits=static_cast<std::uint8_t*>(memory);previous=SelectObject(dc,bitmap);
    }
    bool valid() const noexcept {return dc&&bitmap&&bits&&previous&&previous!=HGDI_ERROR;}
    ~Surface(){if(dc&&previous&&previous!=HGDI_ERROR)SelectObject(dc,previous);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);}
    Surface(Surface const&)=delete;Surface& operator=(Surface const&)=delete;
    void pixel(int x,int y,Color color){
        if(x<0||y<0||x>=int(width)||y>=int(height))return;
        const auto i=(std::size_t(y)*width+std::size_t(x))*4;
        bits[i]=color[2];bits[i+1]=color[1];bits[i+2]=color[0];bits[i+3]=255;
    }
    void rect(int x,int y,int w,int h,Color color){for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)pixel(xx,yy,color);}
    void disk(int x,int y,int radius,Color color){
        for(int yy=-radius;yy<=radius;++yy)for(int xx=-radius;xx<=radius;++xx)
            if(xx*xx+yy*yy<=radius*radius)pixel(x+xx,y+yy,color);
    }
    void line(int x,int y,int end_x,int end_y,int radius,Color color){
        const int dx=std::abs(end_x-x),sx=x<end_x?1:-1,dy=-std::abs(end_y-y),sy=y<end_y?1:-1;int error=dx+dy;
        for(;;){disk(x,y,radius,color);if(x==end_x&&y==end_y)break;const int twice=2*error;
            if(twice>=dy){error+=dy;x+=sx;}if(twice<=dx){error+=dx;y+=sy;}}
    }
};
struct Fonts {
    std::array<HFONT,3> values{};
    ~Fonts(){clear();}
    void clear(){for(auto& value:values){if(value)DeleteObject(value);value=nullptr;}}
    Fonts()=default;Fonts(Fonts const&)=delete;Fonts& operator=(Fonts const&)=delete;
};
struct Text {wchar_t const* heading;wchar_t const* first;wchar_t const* second;wchar_t const* footer;};
inline Text text(Phase phase,Language language){
    if(language==Language::chinese){
        switch(phase){
        case Phase::waiting:return {L"\u81c2\u5c55\u6821\u51c6",L"\u53cc\u81c2\u5411\u4e24\u4fa7\u4f38\u5c55",L"\u53cc\u624b\u63e1\u6301\u624b\u67c4\uff0c\u4fdd\u6301 2 \u79d2",L"\u53cc\u624b\u63e1\u6301"};
        case Phase::holding:return {L"\u4fdd\u6301\u7a33\u5b9a",L"\u53cc\u81c2\u4e0e\u53cc\u624b\u4fdd\u6301\u7a33\u5b9a",L"\u7ee7\u7eed\u53cc\u624b\u63e1\u6301\u624b\u67c4",L"\u4fdd\u6301\u63e1\u6301"};
        case Phase::saved:return {L"\u6d4b\u91cf\u5df2\u4fdd\u5b58",L"\u81c2\u5c55\u6d4b\u91cf\u7ed3\u679c\u5df2\u4fdd\u5b58",L"\u73b0\u5728\u53ef\u4ee5\u653e\u4e0b\u53cc\u81c2",L"\u5df2\u4fdd\u5b58"};
        case Phase::cancelled:return {L"\u6d4b\u91cf\u5df2\u505c\u6b62",L"\u672c\u6b21\u6d4b\u91cf\u5df2\u505c\u6b62",L"\u8bf7\u8fd4\u56de\u542f\u52a8\u5668\u91cd\u8bd5",L"\u8fd4\u56de\u542f\u52a8\u5668"};
        case Phase::failed:return {L"\u6d4b\u91cf\u5df2\u505c\u6b62",L"\u672a\u80fd\u5b8c\u6210\u672c\u6b21\u6d4b\u91cf",L"\u8bf7\u8fd4\u56de\u542f\u52a8\u5668\u91cd\u8bd5",L"\u8fd4\u56de\u542f\u52a8\u5668"};
        default:break;
        }
    }
    switch(phase){
    case Phase::waiting:return {L"ARM SPAN SETUP",L"Stretch both arms out to the sides.",L"Hold both grips for 2 seconds.",L"HOLD BOTH GRIPS"};
    case Phase::holding:return {L"HOLD STEADY",L"Keep your arms and hands steady.",L"Keep holding both grips.",L"KEEP HOLDING"};
    case Phase::saved:return {L"MEASUREMENT SAVED",L"Your arm span measurement is saved.",L"You can lower your arms.",L"SAVED"};
    case Phase::cancelled:return {L"MEASUREMENT STOPPED",L"Measurement stopped.",L"Return to the launcher to try again.",L"RETURN TO LAUNCHER"};
    case Phase::failed:return {L"MEASUREMENT STOPPED",L"Could not finish the measurement.",L"Return to the launcher to try again.",L"RETURN TO LAUNCHER"};
    default:return {};
    }
}
inline bool coverage(HDC dc,HFONT font,wchar_t const* value){
    const auto count=std::wcslen(value);if(!count||count>255)return false;
    Selection selected(dc,font);if(!selected.valid())return false;
    std::array<WORD,256> glyphs{};
    if(GetGlyphIndicesW(dc,value,int(count),glyphs.data(),GGI_MARK_NONEXISTING_GLYPHS)!=count)return false;
    for(std::size_t i=0;i<count;++i)if(glyphs[i]==0xffff)return false;return true;
}
inline bool make_fonts(HDC dc,wchar_t const* face,Language language,Fonts& fonts){
    fonts.clear();constexpr int sizes[]={32,22,19};
    for(unsigned i=0;i<3;++i){fonts.values[i]=CreateFontW(-sizes[i],0,0,0,i?FW_NORMAL:FW_SEMIBOLD,FALSE,FALSE,FALSE,
        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,face);
        if(!fonts.values[i]){fonts.clear();return false;}}
    // Check every phase and the progress digits, avoiding tofu or silent mixed
    // language output when a requested Chinese face was substituted by GDI.
    for(auto phase:{Phase::waiting,Phase::holding,Phase::saved,Phase::cancelled,Phase::failed}){
        const auto value=text(phase,language);
        if(!coverage(dc,fonts.values[0],value.heading)||!coverage(dc,fonts.values[1],value.first)||
           !coverage(dc,fonts.values[1],value.second)||!coverage(dc,fonts.values[2],value.footer)){
            fonts.clear();return false;}}
    if(!coverage(dc,fonts.values[2],L"0123456789%")){fonts.clear();return false;}return true;
}
inline bool select_fonts(HDC dc,Language requested,Fonts& fonts,Metadata& metadata){
    if(requested==Language::chinese)for(auto face:{L"Microsoft YaHei UI",L"Microsoft YaHei",L"SimSun"})
        if(make_fonts(dc,face,Language::chinese,fonts)){metadata={Language::chinese,false};return true;}
    for(auto face:{L"Segoe UI",L"Arial"})if(make_fonts(dc,face,Language::english,fonts)){
        metadata={Language::english,requested==Language::chinese};return true;}
    return false;
}
inline bool write_text(Surface& surface,HFONT font,wchar_t const* value,RECT rectangle,Color color,bool right=false){
    Selection selected(surface.dc,font);if(!selected.valid())return false;
    const int count=int(std::wcslen(value));SIZE size{};
    if(!GetTextExtentPoint32W(surface.dc,value,count,&size)||size.cx>rectangle.right-rectangle.left||
       size.cy>rectangle.bottom-rectangle.top)return false;
    if(SetTextColor(surface.dc,RGB(color[0],color[1],color[2]))==CLR_INVALID)return false;
    const auto x=right?rectangle.right-size.cx:rectangle.left;
    return ExtTextOutW(surface.dc,x,rectangle.top,ETO_CLIPPED,&rectangle,value,UINT(count),nullptr)!=FALSE;
}
inline void illustration(Surface& surface,Color accent){
    surface.rect(36,40,192,182,{{34,38,45}});
    surface.disk(132,72,17,title);surface.disk(132,72,12,{{34,38,45}});
    surface.line(132,91,132,164,4,hint);
    surface.line(60,116,204,116,4,hint);
    surface.line(132,164,109,208,4,hint);surface.line(132,164,155,208,4,hint);
    for(int x:{60,204}){
        surface.disk(x,116,13,accent);surface.disk(x,116,8,background);
        surface.rect(x-5,116,11,25,accent);surface.rect(x-2,121,5,14,background);
    }
}
} // namespace detail

// Validation/GDI/allocation failure leaves both out and optional meta unchanged.
// holding is always <1000 and displayed with a floored percentage; only an
// explicit valid saved state may draw a completed green bar.
inline bool rgba(Content const& value,std::vector<std::uint8_t>& out,Metadata* meta=nullptr){
    if(!gears_armspan_ui::valid(value))return false;
    try{
        detail::Surface surface;if(!surface.valid())return false;
        detail::Fonts fonts;Metadata metadata{};
        if(!detail::select_fonts(surface.dc,value.language,fonts,metadata))return false;
        if(!SetBkMode(surface.dc,TRANSPARENT)||SetTextAlign(surface.dc,TA_LEFT|TA_TOP|TA_NOUPDATECP)==GDI_ERROR)return false;
        surface.rect(0,0,width,height,detail::background);
        surface.rect(20,20,width-40,1,detail::border);surface.rect(20,height-21,width-40,1,detail::border);
        surface.rect(20,20,1,height-40,detail::border);surface.rect(width-21,20,1,height-40,detail::border);
        const bool saved=value.phase==Phase::saved,stopped=value.phase==Phase::cancelled||value.phase==Phase::failed;
        const auto accent=saved?detail::green:stopped?detail::muted:detail::yellow;
        detail::illustration(surface,accent);const auto text=detail::text(value.phase,metadata.rendered_language);
        if(!detail::write_text(surface,fonts.values[0],text.heading,{252,36,732,82},detail::title)||
           !detail::write_text(surface,fonts.values[1],text.first,{252,98,732,130},detail::hint)||
           !detail::write_text(surface,fonts.values[1],text.second,{252,132,732,165},detail::hint)||
           !detail::write_text(surface,fonts.values[2],text.footer,{252,175,660,203},accent))return false;
        if(!stopped){
            wchar_t percent[8]{};if(swprintf_s(percent,L"%u%%",value.progress_permille/10)<0)return false;
            if(!detail::write_text(surface,fonts.values[2],percent,{664,175,732,203},accent,true))return false;
        }
        surface.rect(252,207,480,12,detail::track);
        if(!stopped)surface.rect(252,207,int(480*value.progress_permille/1000),12,accent);
        if(!GdiFlush())return false;
        std::vector<std::uint8_t> candidate(std::size_t(width)*height*4);
        for(std::size_t i=0;i<candidate.size();i+=4){candidate[i]=surface.bits[i+2];candidate[i+1]=surface.bits[i+1];
            candidate[i+2]=surface.bits[i];candidate[i+3]=255;}
        out.swap(candidate);if(meta)*meta=metadata;return true;
    }catch(...){return false;}
}

// Preview only. Validate before touching disk, then replace through a private
// same-directory temp file so a failed save preserves an existing preview.
inline bool save_ppm(std::string const& path,std::vector<std::uint8_t> const& pixels){
    if(path.empty()||pixels.size()!=std::size_t(width)*height*4)return false;
    for(std::size_t i=3;i<pixels.size();i+=4)if(pixels[i]!=255)return false;
    try{
        static std::atomic<std::uint64_t> serial{};std::string temporary;HANDLE file=INVALID_HANDLE_VALUE;
        for(unsigned attempt=0;attempt<8&&file==INVALID_HANDLE_VALUE;++attempt){
            temporary=path+".armspan-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++serial)+".tmp";
            file=CreateFileA(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file==INVALID_HANDLE_VALUE&&GetLastError()!=ERROR_FILE_EXISTS)return false;
        }
        if(file==INVALID_HANDLE_VALUE)return false;
        char header[64]{};const int size=sprintf_s(header,"P6\n%u %u\n255\n",width,height);DWORD written{};
        bool okay=size>0&&WriteFile(file,header,DWORD(size),&written,nullptr)&&written==DWORD(size);
        std::array<std::uint8_t,width*3> row{};
        for(unsigned y=0;okay&&y<height;++y){
            for(unsigned x=0;x<width;++x){const auto i=(std::size_t(y)*width+x)*4;row[x*3]=pixels[i];row[x*3+1]=pixels[i+1];row[x*3+2]=pixels[i+2];}
            okay=WriteFile(file,row.data(),DWORD(row.size()),&written,nullptr)&&written==row.size();
        }
        if(okay&&!FlushFileBuffers(file))okay=false;
        if(!CloseHandle(file))okay=false;
        if(okay)okay=MoveFileExA(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
        if(!okay)DeleteFileA(temporary.c_str());return okay;
    }catch(...){return false;}
}
} // namespace armspan_card_pixels
