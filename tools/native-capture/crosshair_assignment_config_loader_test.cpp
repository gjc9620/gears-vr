#include "crosshair_assignment_config_loader.h"
#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <iostream>
using namespace gears_crosshair_assignment;
using Values=std::map<std::wstring,std::wstring>;
struct Reader {
    const Values& values;
    unsigned operator()(const wchar_t* key,wchar_t* out,unsigned capacity)const{
        auto it=values.find(key);if(it==values.end()){out[0]=0;return 0;}
        const auto n=(std::min)(unsigned(it->second.size()),capacity-1);
        std::copy_n(it->second.data(),n,out);out[n]=0;return n;
    }
};
int main(int argc,char** argv){
    assert(argc==2);Values valid;std::ifstream input(argv[1]);assert(input);
    for(std::string line;std::getline(input,line);){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        auto at=line.find('=');if(at==std::string::npos)continue;
        std::wstring key(line.begin(),line.begin()+at),value(line.begin()+at+1,line.end());
        assert(valid.emplace(key,value).second);
    }
    OwnedConfig good;assert(parse_config(Reader{valid},123,0x140000000,good));
    assert(good.assignment.identities==&good.identities&&good.tick_root_count==2);
    OwnedConfig copied=good;assert(copied.assignment.identities==&copied.identities);
    OwnedConfig moved=std::move(copied);assert(moved.assignment.identities==&moved.identities);
    OwnedConfig assigned;assigned=std::move(moved);assert(assigned.assignment.identities==&assigned.identities);
    auto reject=[&](Values const& bad){OwnedConfig out=good;out.pid=777;
        const bool accepted=parse_config(Reader{bad},123,0x140000000,out);
        if(accepted)for(auto const& kv:valid){auto at=bad.find(kv.first);
            if(at==bad.end()||at->second!=kv.second)std::wcerr<<L"Unexpected acceptance: "<<kv.first<<L"\n";}
        assert(!accepted);
        assert(out.pid==777&&out.assignment.script==good.assignment.script&&out.assignment.identities==&out.identities);
    };
    // Every emitted field is required: omissions cannot become default zeros.
    for(auto const& entry:valid){auto bad=valid;bad.erase(entry.first);reject(bad);}
    for(auto value:{L"-1",L"+1",L" 1",L"1 ",L"1x",L"18446744073709551616",L"4294967296",L"",L"0x",L"1\n2"}){
        auto bad=valid;bad[L"pid"]=value;reject(bad);
    }
    for(auto key:{L"identity_count",L"super_count",L"writer",L"script_size",L"property_structure"}){
        auto bad=valid;bad[key]=L"999999999";reject(bad);
    }
    for(auto key:{L"schema",L"native_contract",L"base",L"sha256",L"root1_function",L"tick_root_count"}){
        auto bad=valid;bad[key]=L"999";reject(bad);
    }
    {auto bad=valid;bad[L"root1_size"]=L"0";reject(bad);}
    {auto bad=valid;bad[L"pawn"]=bad[L"controller"];reject(bad);}
    {auto bad=valid;bad[L"id1_address"]=bad[L"id0_address"];reject(bad);}
    {auto bad=valid;bad[L"id1_index"]=bad[L"id0_index"];reject(bad);}
    {auto bad=valid;bad[L"id0_name"]=std::wstring(100,L'A');reject(bad);}
    {auto bad=valid;bad[L"script"]=L"18446744073709551615";reject(bad);}
    {auto bad=valid;bad[L"super1_identity"]=bad[L"super0_identity"];reject(bad);}
    {auto no_roots=valid;no_roots[L"tick_root_count"]=L"0";
        for(unsigned i=0;i<2;++i)for(auto suffix:{L"owner",L"function",L"script",L"size"})
            no_roots.erase(L"root"+std::to_wstring(i)+L"_"+suffix);
        OwnedConfig out;assert(parse_config(Reader{no_roots},123,0x140000000,out)&&out.tick_root_count==0);}
    {OwnedConfig out;assert(!parse_config(Reader{valid},124,0x140000000,out));
        assert(!parse_config(Reader{valid},123,0x140001000,out));}
    std::cout<<"crosshair assignment INI loader tests passed\n";
}
