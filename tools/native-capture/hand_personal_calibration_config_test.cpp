#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include "hand_personal_calibration_live.inl"

namespace {
namespace cfg=hand_personal_calibration_live;
unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",unsigned(__LINE__),#x);}}while(false)
template<class T> auto bytes(T const& value){std::array<unsigned char,sizeof(T)> out{};std::memcpy(out.data(),&value,sizeof(T));return out;}
void parse_cases(){
    cfg::Measurement output{1,1.73,-.16};
    CHECK(cfg::parse(L"",L"",false,false,output)==cfg::ConfigStatus::okay);CHECK(!output.valid());
    output={1,1.73,-.16};
    const auto before=bytes(output);
    for(auto pair:{std::pair{L"1.7",L""},std::pair{L"",L"-.15"}}){
        CHECK(cfg::parse(pair.first,pair.second,true,true,output)==cfg::ConfigStatus::partial);CHECK(bytes(output)==before);
    }
    for(auto flags:{std::pair{false,true},std::pair{true,false},std::pair{false,false}}){
        CHECK(cfg::parse(L"1.7",L"-.15",flags.first,flags.second,output)==cfg::ConfigStatus::inactive);CHECK(bytes(output)==before);
    }
    for(auto text:{L"garbage",L"1.7x",L"1.7 "}){
        CHECK(cfg::parse(text,L"-.15",true,true,output)==cfg::ConfigStatus::invalid_number);CHECK(bytes(output)==before);
        CHECK(cfg::parse(L"1.7",text,true,true,output)==cfg::ConfigStatus::invalid_number);CHECK(bytes(output)==before);
    }
    for(auto text:{L"nan",L"inf",L"-inf",L"0",L".899",L"2.401",L"1e999"}){
        CHECK(cfg::parse(text,L"-.15",true,true,output)==cfg::ConfigStatus::invalid_measurement);CHECK(bytes(output)==before);
    }
    for(auto text:{L"nan",L"inf",L"-.351",L".101"}){
        CHECK(cfg::parse(L"1.7",text,true,true,output)==cfg::ConfigStatus::invalid_measurement);CHECK(bytes(output)==before);
    }
    CHECK(cfg::parse(nullptr,L"-.15",true,true,output)==cfg::ConfigStatus::invalid_number);CHECK(bytes(output)==before);
    CHECK(cfg::parse(L"1.7",nullptr,true,true,output)==cfg::ConfigStatus::invalid_number);CHECK(bytes(output)==before);
    CHECK(cfg::parse(L"1.7",L"-.15",true,true,output)==cfg::ConfigStatus::okay);
    CHECK(output.valid()&&output.version==1&&output.armspan_metres==1.7&&output.hands_below_head_metres==-.15);
}
void ini_cases(){
    wchar_t directory[MAX_PATH+1]{},path[MAX_PATH+1]{};
    CHECK(GetTempPathW(MAX_PATH,directory)>0);CHECK(GetTempFileNameW(directory,L"ghc",0,path)!=0);
    CHECK(cfg::configure(path,false,false,nullptr));CHECK(!cfg::loaded&&!cfg::saved.valid());
    CHECK(WritePrivateProfileStringW(L"Capture",L"weapon_armspan_metres",L"1.7",path));
    CHECK(!cfg::configure(path,true,true,nullptr));CHECK(!cfg::loaded&&!cfg::saved.valid());
    CHECK(WritePrivateProfileStringW(L"Capture",L"weapon_hands_below_head_metres",L"-.15",path));
    // The caller can represent the shared continuous owner with has_consumer.
    // There is deliberately no dependency on the legacy observer global here.
    CHECK(cfg::configure(path,true,true,nullptr));CHECK(cfg::loaded&&cfg::saved.armspan_metres==1.7);
    const auto accepted=bytes(cfg::saved);
    CHECK(!cfg::configure(path,true,false,nullptr));CHECK(cfg::loaded&&bytes(cfg::saved)==accepted);
    CHECK(!cfg::configure(path,false,true,nullptr));CHECK(cfg::loaded&&bytes(cfg::saved)==accepted);
    const std::wstring long_number=L"1.7"+std::wstring(90,L'0');
    CHECK(WritePrivateProfileStringW(L"Capture",L"weapon_armspan_metres",long_number.c_str(),path));
    CHECK(!cfg::configure(path,true,true,nullptr));CHECK(cfg::loaded&&bytes(cfg::saved)==accepted);
    CHECK(WritePrivateProfileStringW(L"Capture",L"weapon_armspan_metres",nullptr,path));
    CHECK(WritePrivateProfileStringW(L"Capture",L"weapon_hands_below_head_metres",nullptr,path));
    CHECK(cfg::configure(path,true,true,nullptr));CHECK(!cfg::loaded&&!cfg::saved.valid());
    CHECK(DeleteFileW(path));
    CHECK(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES&&GetLastError()==ERROR_FILE_NOT_FOUND);
}
} // namespace
int main(){parse_cases();ini_cases();std::printf("HandPersonalCalibrationConfig: %u checks, %u failures; own temporary INI only\n",checks,failures);return failures?1:0;}
