#include "world_rebind_metadata.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
using namespace gears_world_rebind;
static void require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
static bool equal(Metadata const& a,Metadata const& b){
    return a.previous_generation==b.previous_generation&&a.generation==b.generation&&a.local_player==b.local_player&&
        a.controller==b.controller&&a.camera==b.camera&&a.timestamp==b.timestamp&&a.camera_yaw==b.camera_yaw;
}
struct Fixture {
    Expected expected{11,22,33,44,5,0x10000000,0x20000000};
    std::map<std::pair<std::wstring,std::wstring>,std::wstring> values;
    Fixture(){
        set(L"WorldRebind",L"schema",L"1");set(L"WorldRebind",L"game_pid",L"11");set(L"WorldRebind",L"game_creation",L"33");
        set(L"WorldRebind",L"owner_pid",L"22");set(L"WorldRebind",L"owner_creation",L"44");set(L"WorldRebind",L"previous_generation",L"5");
        set(L"WorldRebind",L"generation",L"6");set(L"WorldRebind",L"module_base",L"0x10000000");set(L"WorldRebind",L"game_sha256",game_sha256);
        set(L"Capture",L"pid",L"11");set(L"Capture",L"local_player",L"0x20000000");set(L"Capture",L"controller",L"0xABCDEF");
        set(L"Capture",L"camera",L"123456");set(L"Capture",L"timestamp",L"0X123456");set(L"Capture",L"camera_yaw",L"7654321");
    }
    void set(wchar_t const* section,wchar_t const* key,std::wstring value){values[{section,key}]=std::move(value);}
    std::size_t read(wchar_t const* section,wchar_t const* key,wchar_t* output,std::size_t capacity){
        auto found=values.find({section,key});if(found==values.end())return 0;
        const auto length=(std::min)(capacity-1,found->second.size());
        std::copy_n(found->second.data(),length,output);output[length]=0;return length;
    }
    bool parse_into(Metadata& output){return parse([&](auto section,auto key,auto buffer,auto capacity){return read(section,key,buffer,capacity);},expected,output);}
    void rejects(){Metadata output{101,102,103,104,105,106,107};const auto before=output;
        require(!parse_into(output),"invalid metadata accepted");require(equal(output,before),"failed parse mutated output");}
};
int main(){try{
    Fixture valid;Metadata output{};require(valid.parse_into(output),"valid packet rejected");
    require(output.previous_generation==5&&output.generation==6&&output.local_player==0x20000000&&output.controller==0xabcdef&&
        output.camera==123456&&output.timestamp==0x123456&&output.camera_yaw==7654321,"valid fields differ");
    for(auto key:{L"schema",L"game_pid",L"game_creation",L"owner_pid",L"owner_creation",L"previous_generation",L"generation",L"module_base"}){
        Fixture f;f.set(L"WorldRebind",key,L"999");f.rejects();
        f=Fixture{};f.values.erase({L"WorldRebind",key});f.rejects();
    }
    for(auto key:{L"pid",L"local_player"}){Fixture f;f.set(L"Capture",key,L"999");f.rejects();}
    for(auto key:{L"local_player",L"controller",L"camera",L"timestamp",L"camera_yaw"}){Fixture f;f.set(L"Capture",key,L"0");f.rejects();}
    for(auto generation:{L"0",L"4",L"5",L"7",L"18446744073709551616"}){Fixture f;f.set(L"WorldRebind",L"generation",generation);f.rejects();}
    for(auto previous:{L"0",L"4",L"6"}){Fixture f;f.set(L"WorldRebind",L"previous_generation",previous);f.rejects();}
    for(auto bad:{L"",L"-1",L"+1",L" 1",L"1 ",L"\t1",L"1\n",L"0x",L"0xG1",L"1junk",L"1.0",L"18446744073709551616",L"0x10000000000000000"}){
        Fixture f;f.set(L"Capture",L"camera",bad);f.rejects();
    }
    {Fixture f;f.set(L"WorldRebind",L"game_pid",L"4294967307");f.rejects();}
    {Fixture f;f.set(L"Capture",L"camera",std::wstring(95,L'1'));f.rejects();}
    {Fixture f;f.set(L"Capture",L"camera",std::wstring(200,L'1'));f.rejects();}
    {Fixture f;f.set(L"Capture",L"camera",std::wstring{L'1',L'2',L'3',wchar_t(0),L'4',L'5',L'6'});f.rejects();}
    for(auto hash:{std::wstring(64,L'0'),std::wstring(game_sha256)+L"0",std::wstring(game_sha256,63)}){
        Fixture f;f.set(L"WorldRebind",L"game_sha256",hash);f.rejects();
    }
    for(unsigned field=0;field<8;++field){Fixture f;switch(field){
        case 0:f.expected.game_pid=0;break;case 1:f.expected.owner_pid=0;break;
        case 2:f.expected.game_creation=0;break;case 3:f.expected.owner_creation=0;break;
        case 4:f.expected.generation=0;break;case 5:f.expected.module_base=0;break;
        case 6:f.expected.local_player=0;break;case 7:f.expected.owner_pid=f.expected.game_pid;break;}
        f.rejects();
    }
    {Fixture f;f.expected.generation=UINT64_MAX;f.set(L"WorldRebind",L"previous_generation",L"18446744073709551615");
        f.set(L"WorldRebind",L"generation",L"0");f.rejects();}
    {Fixture f;f.expected.generation=UINT64_MAX-1;f.set(L"WorldRebind",L"previous_generation",L"18446744073709551614");
        f.set(L"WorldRebind",L"generation",L"18446744073709551615");require(f.parse_into(output),"maximum valid generation rejected");}
    // Readers claiming a full or unterminated buffer are rejected atomically.
    for(unsigned mode=0;mode<3;++mode){Fixture f;Metadata result{101,102,103,104,105,106,107};const auto before=result;
        auto malformed=[&](auto section,auto key,wchar_t* buffer,std::size_t capacity){
            if(std::wstring(key)==L"camera"){
                if(mode==0)return capacity;
                if(mode==1){buffer[0]=L'1';buffer[1]=L'2';return std::size_t(1);}
                return std::size_t(-1);
            }
            return f.read(section,key,buffer,capacity);
        };
        require(!parse(malformed,f.expected,result)&&equal(result,before),"malformed reader accepted or output changed");
    }
    puts("WorldRebindMetadataTest passed (prepare identity only)");return 0;
}catch(std::exception const& error){fprintf(stderr,"WorldRebindMetadataTest failed: %s\n",error.what());return 1;}}
