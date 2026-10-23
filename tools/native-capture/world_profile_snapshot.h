#pragma once
#include <windows.h>
#include <algorithm>
#include <cwchar>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace gears_profile {
// A worker-local read of the already locked immutable candidate. Only the
// canonical ASCII subset emitted by our writer is accelerated. Other syntax
// keeps the original Windows profile API, including quoting/Unicode behavior.
class Snapshot {
    using Section=std::map<std::wstring,std::wstring>;
    std::map<std::wstring,Section> sections_;
    bool enabled_{};
    std::size_t entries_{};
    static bool key(std::wstring_view input,std::wstring& output){
        if(input.empty()||input.size()>128)return false;
        output.clear();output.reserve(input.size());
        for(wchar_t c:input){
            if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'_'))return false;
            output.push_back(c>=L'A'&&c<=L'Z'?wchar_t(c-L'A'+L'a'):c);
        }
        return true;
    }
    bool parse(std::wstring_view text){
        std::map<std::wstring,Section> parsed;
        std::wstring section,normalized;
        std::size_t start=0,entries=0;
        while(start<text.size()){
            auto end=text.find(L'\n',start);if(end==text.npos)end=text.size();
            auto line=text.substr(start,end-start);start=end==text.size()?end:end+1;
            if(!line.empty()&&line.back()==L'\r')line.remove_suffix(1);
            if(line.empty())continue;
            if(line.front()==L'['){
                if(line.size()<3||line.back()!=L']'||!key(line.substr(1,line.size()-2),section)||
                   parsed.size()>=32||!parsed.emplace(section,Section{}).second)return false;
                continue;
            }
            const auto equal=line.find(L'=');
            if(section.empty()||equal==line.npos||!key(line.substr(0,equal),normalized))return false;
            auto value=line.substr(equal+1);
            if(value.empty()||value.front()==L' '||value.back()==L' ')return false;
            for(wchar_t c:value)if(c<32||c>126||c==L'\''||c==L'"'||c==L';')return false;
            if(entries>=32768||!parsed.at(section).emplace(normalized,std::wstring(value)).second)return false;
            ++entries;
        }
        if(!entries)return false;
        sections_.swap(parsed);entries_=entries;enabled_=true;return true;
    }
public:
    static constexpr DWORD maximum_bytes=2*1024*1024;
    Snapshot()=default;
    Snapshot(Snapshot const&)=delete;
    Snapshot& operator=(Snapshot const&)=delete;
    bool enabled()const{return enabled_;}
    std::size_t entries()const{return entries_;}
    bool load(HANDLE file){
        static_assert(sizeof(wchar_t)==2,"Windows UTF-16 profile expected");
        enabled_=false;entries_=0;sections_.clear();
        LARGE_INTEGER size{},zero{};
        if(file==INVALID_HANDLE_VALUE||!file||!GetFileSizeEx(file,&size)||size.QuadPart<=2||
           size.QuadPart>maximum_bytes||(size.QuadPart&1)||!SetFilePointerEx(file,zero,nullptr,FILE_BEGIN))return false;
        std::vector<wchar_t> text(static_cast<std::size_t>(size.QuadPart/2));DWORD count{};
        if(!ReadFile(file,text.data(),static_cast<DWORD>(size.QuadPart),&count,nullptr)||
           count!=static_cast<DWORD>(size.QuadPart)||text.front()!=wchar_t(0xfeff))return false;
        return parse(std::wstring_view(text.data()+1,text.size()-1));
    }
    DWORD get(const wchar_t* section,const wchar_t* name,wchar_t* output,unsigned capacity,const wchar_t* path)const{
        std::wstring s,k;
        if(!enabled_||!section||!name||!output||!capacity||!key(section,s)||!key(name,k))
            return GetPrivateProfileStringW(section,name,L"",output,capacity,path);
        output[0]=0;
        const auto group=sections_.find(s);if(group==sections_.end())return 0;
        const auto found=group->second.find(k);if(found==group->second.end())return 0;
        const auto copied=(std::min)(found->second.size(),std::size_t(capacity-1));
        if(copied)std::wmemcpy(output,found->second.data(),copied);
        output[copied]=0;return static_cast<DWORD>(copied);
    }
};
inline DWORD get(Snapshot const* snapshot,const wchar_t* section,const wchar_t* key,wchar_t* output,unsigned capacity,const wchar_t* path){
    return snapshot?snapshot->get(section,key,output,capacity,path):GetPrivateProfileStringW(section,key,L"",output,capacity,path);
}
}
