#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "world_profile_snapshot.h"
#include <array>
#include <cstdio>
#include <cwctype>
#include <utility>

static unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
using Key=std::pair<std::wstring,std::wstring>;
std::wstring full(const wchar_t* name){wchar_t cwd[MAX_PATH]{};GetCurrentDirectoryW(MAX_PATH,cwd);return std::wstring(cwd)+L"\\"+name;}
HANDLE open(const std::wstring& path){return CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);}
bool write(const std::wstring& path,std::wstring const& content){
    HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;DWORD written{};
    const auto size=static_cast<DWORD>(content.size()*sizeof(wchar_t));
    const bool ok=WriteFile(file,content.data(),size,&written,nullptr)&&written==size;CloseHandle(file);return ok;
}
std::vector<Key> keys(const std::wstring& path){
    std::vector<Key> result;std::vector<wchar_t> sections(8192),values(65536);
    const auto count=GetPrivateProfileSectionNamesW(sections.data(),static_cast<DWORD>(sections.size()),path.c_str());
    if(!count||count>=sections.size()-2)return {};
    for(auto section=sections.data();*section;section+=std::wcslen(section)+1){
        const auto n=GetPrivateProfileSectionW(section,values.data(),static_cast<DWORD>(values.size()),path.c_str());
        if(!n||n>=values.size()-2)return {};
        for(auto value=values.data();*value;value+=std::wcslen(value)+1){
            const auto equal=std::wcschr(value,L'=');if(!equal)return {};
            result.emplace_back(section,std::wstring(value,static_cast<std::size_t>(equal-value)));
        }
    }
    return result;
}
bool equal(gears_profile::Snapshot const& snapshot,std::wstring const& path,const wchar_t* section,const wchar_t* key,unsigned capacity){
    std::vector<wchar_t> expected(capacity+1,L'!'),actual(capacity+1,L'!');
    const auto a=GetPrivateProfileStringW(section,key,L"",expected.data(),capacity,path.c_str());
    const auto b=snapshot.get(section,key,actual.data(),capacity,path.c_str());
    return a==b&&std::wstring(expected.data())==std::wstring(actual.data())&&expected[capacity]==L'!'&&actual[capacity]==L'!';
}
std::wstring upper(std::wstring text){for(auto& c:text)c=static_cast<wchar_t>(std::towupper(c));return text;}
double ms(){LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);return double(q.QuadPart)*1000/double(f.QuadPart);}
int wmain(){
    for(auto name:{L"mounted.ini",L"streamed.ini",L"streamed_cinematic.ini",L"normal.ini"}){
        auto path=full(name);HANDLE file=open(path);CHECK(file!=INVALID_HANDLE_VALUE);
        gears_profile::Snapshot snapshot;const auto load_start=ms();CHECK(snapshot.load(file)&&snapshot.enabled());const auto load_ms=ms()-load_start;
        const auto fields=keys(path);CHECK(fields.size()>500&&snapshot.entries()==fields.size());
        for(auto const& field:fields)for(auto capacity:{1u,2u,7u,64u,128u}){
            CHECK(equal(snapshot,path,field.first.c_str(),field.second.c_str(),capacity));
            const auto section=upper(field.first),key=upper(field.second);
            CHECK(equal(snapshot,path,section.c_str(),key.c_str(),capacity));
        }
        CHECK(equal(snapshot,path,L"Missing",L"Missing",128));
        CHECK(equal(snapshot,path,L"AnchorObserver",L"Missing",128));
        CHECK(equal(snapshot,path,L"AnchorObserver",L"\x212a",128)); // Unsupported query uses the API.
        HANDLE writer=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
        CHECK(writer==INVALID_HANDLE_VALUE);CHECK(!DeleteFileW(path.c_str()));
        if(!std::wcscmp(name,L"mounted.ini")){
            std::array<double,5> old_times{},new_times{};DWORD sink=0;wchar_t text[128]{};
            for(unsigned round=0;round<5;++round){
                auto before=ms();
                for(auto const& field:fields)sink+=GetPrivateProfileStringW(field.first.c_str(),field.second.c_str(),L"",text,128,path.c_str());
                old_times[round]=ms()-before;before=ms();
                for(auto const& field:fields)sink+=snapshot.get(field.first.c_str(),field.second.c_str(),text,128,path.c_str());
                new_times[round]=ms()-before;
            }
            std::sort(old_times.begin(),old_times.end());std::sort(new_times.begin(),new_times.end());
            std::printf("{\"profile_lookup_cost\":true,\"fixture_keys\":%zu,\"api_median_ms\":%.6f,\"snapshot_load_ms\":%.6f,\"snapshot_median_ms\":%.6f,\"sink\":%lu,\"game_latency_measured\":false}\n",fields.size(),old_times[2],load_ms,new_times[2],sink);
        }
        CHECK(CloseHandle(file));
    }
    const std::array<std::wstring,10> unsupported{{
        L"\xfeff[One]\nkey=\"quoted value\"\n",L"\xfeff[One]\nkey= trailing \n",
        L"\xfeff[One]\n;comment\nkey=1\n",L"\xfeff[One]\nkey=1\nKEY=2\n",
        L"\xfeff[One]\nkey=1\n[ONE]\nkey=2\n",L"\xfeff[One]\nkey=\x4e2d\x6587\n",
        L"\xfeff[One]\nkey=\n",L"\xfeff[One]\n key=1\n",L"\xfeff[ One ]\nkey=1\n",
        L"[One]\nkey=1\n"}};
    unsigned ordinal{};
    for(auto const& text:unsupported){
        const auto path=full((L"fallback-"+std::to_wstring(++ordinal)+L".ini").c_str());CHECK(write(path,text));
        HANDLE file=open(path);CHECK(file!=INVALID_HANDLE_VALUE);gears_profile::Snapshot snapshot;
        CHECK(!snapshot.load(file)&&!snapshot.enabled()&&!snapshot.entries());
        for(auto capacity:{1u,2u,7u,128u})CHECK(equal(snapshot,path,L"One",L"key",capacity));
        CHECK(CloseHandle(file));CHECK(DeleteFileW(path.c_str()));
    }
    const auto path=full(L"canonical-extra.ini");
    CHECK(write(path,L"\xfeff[One]\r\nkey=a=b # text\r\nempty_name=0\r\n\r\n[Two]\nkey=42"));
    HANDLE file=open(path);CHECK(file!=INVALID_HANDLE_VALUE);gears_profile::Snapshot snapshot;CHECK(snapshot.load(file)&&snapshot.entries()==3);
    CHECK(equal(snapshot,path,L"one",L"KEY",128));CHECK(equal(snapshot,path,L"two",L"key",2));
    CHECK(!snapshot.load(INVALID_HANDLE_VALUE)&&!snapshot.enabled()&&!snapshot.entries());
    CHECK(equal(snapshot,path,L"one",L"key",128));CHECK(CloseHandle(file));CHECK(DeleteFileW(path.c_str()));
    CHECK(write(path,std::wstring(gears_profile::Snapshot::maximum_bytes/2+1,L'x')));file=open(path);CHECK(file!=INVALID_HANDLE_VALUE);
    CHECK(!snapshot.load(file)&&!snapshot.enabled());CHECK(CloseHandle(file));CHECK(DeleteFileW(path.c_str()));
    std::printf("WorldProfileSnapshotTest passed: %u API equivalence, fallback, truncation, lease and lifetime checks\n",checks);return 0;
}
