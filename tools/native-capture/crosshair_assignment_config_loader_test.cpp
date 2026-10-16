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
    // Optional view_enabled may be omitted by old certificates.
    for(auto const& entry:valid){if(entry.first==L"view_enabled")continue;
        if(entry.first.rfind(L"view_",0)==0&&!good.assignment.view.enabled)continue;
        auto bad=valid;bad.erase(entry.first);reject(bad);}
    {auto legacy=valid;legacy.erase(L"view_enabled");OwnedConfig out;
        assert(parse_config(Reader{legacy},123,0x140000000,out)&&!out.assignment.view.enabled);}
    for(auto value:{L"2",L"-1",L"+1",L"1x",L" 1",L"1 ",L"4294967296",L"1\n2"}){
        auto bad=valid;bad[L"view_enabled"]=value;reject(bad);}
    // Build a serialized-only optional fixture when input uses the legacy path.
    auto view_valid=valid;
    if(!good.assignment.view.enabled){
        unsigned count=good.identities.identity_count;
        auto add=[&](const wchar_t* role,const wchar_t* name,std::uintptr_t cls,std::uintptr_t outer){
            const unsigned index=count++;const auto prefix=L"id"+std::to_wstring(index)+L"_";
            view_valid[prefix+L"address"]=std::to_wstring(0x70000000ull+index*0x1000ull);
            view_valid[prefix+L"index"]=std::to_wstring(3000000+index);
            view_valid[prefix+L"name_index"]=std::to_wstring(3000000+index);
            view_valid[prefix+L"number"]=L"0";view_valid[prefix+L"cls"]=std::to_wstring(cls);
            view_valid[prefix+L"outer"]=std::to_wstring(outer);view_valid[prefix+L"name"]=name;
            view_valid[L"view_"+std::wstring(role)]=std::to_wstring(index);
        };
        auto const& ids=good.identities;auto const& c=good.assignment;
        add(L"rotator_struct",L"Rotator",ids.identities[c.vector_struct].cls,0);
        const auto owner=ids.identities[c.writer].address,cls=ids.identities[c.lhs].cls;
        add(L"cam_loc",L"CamLoc",cls,owner);add(L"cam_rot",L"CamRot",cls,owner);
        add(L"start_trace",L"StartTrace",cls,owner);add(L"trace_dir",L"TraceDir",cls,owner);
        view_valid[L"identity_count"]=std::to_wstring(count);view_valid[L"view_enabled"]=L"1";
        for(auto pair:{std::pair<const wchar_t*,unsigned>{L"cam_loc",0xa0},{L"cam_rot",0xb0},{L"start_trace",0xc0},{L"trace_dir",0xd0}})
            view_valid[L"view_"+std::wstring(pair.first)+L"_offset"]=std::to_wstring(pair.second);
    }
    {OwnedConfig out;assert(parse_config(Reader{view_valid},123,0x140000000,out)&&out.assignment.view.enabled);}
    auto reject_view=[&](Values const& bad){OwnedConfig out;assert(!parse_config(Reader{bad},123,0x140000000,out));};
    for(auto const& entry:view_valid)if(entry.first.rfind(L"view_",0)==0&&entry.first!=L"view_enabled"){
        auto bad=view_valid;bad.erase(entry.first);reject_view(bad);}
    for(auto value:{L"65524",L"65535",L"-1",L"1x"}){
        auto bad=view_valid;bad[L"view_cam_loc_offset"]=value;reject_view(bad);}
    for(auto key:{L"view_cam_rot_offset",L"ww_offset",L"impact_offset"}){
        auto bad=view_valid;bad[L"view_cam_loc_offset"]=bad[key];reject_view(bad);}
    {auto bad=view_valid;bad[L"view_rotator_struct"]=bad[L"vector_struct"];reject_view(bad);}
    {auto bad=view_valid;bad[L"id"+bad[L"view_cam_loc"]+L"_outer"]=L"0";reject_view(bad);}
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
