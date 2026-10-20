#pragma once
#include "path_choice_position_trial.h"
#include "hook_entry_attestation.h"

// Read-only CloseScene experiment. No inactive-widget write permission is granted.
namespace gears_path_choice_close {
namespace position=gears_path_choice_position;
inline constexpr std::uintptr_t close_rva=0x346574,exec_rva=0x34698c,exec_cold_rva=0x90dbe6;
inline constexpr std::uintptr_t client_table_rva=0x1429d58,close_slot=0x228;
inline constexpr char close_sha[]="b8c82a830ef05c50a88489351c469e591b65a154310a2fefec91e57f66fb7b3a";
inline constexpr char exec_sha[]="1f03d742fcd859a664004bb62ff35c327d80b1364b235701e64184eee53e55c1";
template<class Read> bool read_own_prefix(gears_hook_entry::Proof const& proof,std::uintptr_t target,std::uintptr_t detour,
    void* output,void const* from,std::size_t size,Read read){
    return proof.target==target&&proof.detour==detour&&
        gears_hook_entry::read_original(proof,reinterpret_cast<std::uintptr_t>(from),output,size,read);
}
struct Request {
    std::uint64_t nonce{},pid{},creation_time{},started_qpc{},world_generation{};
    std::uintptr_t base{},client{},scene{},panel{},active_header{},function{},function_outer{},function_class{};
    std::uint32_t function_index{},function_name_index{},function_number{};
    position::RegistryHeader initial{};std::vector<std::uintptr_t> members;unsigned duration_ms{};
};
inline bool parse(std::string const& text,Request& out){
    if(text.empty()||text.size()>32768||text.find('\0')!=std::string::npos)return false;
    std::map<std::string,std::string> fields;bool section=false;
    for(std::size_t begin=0;begin<text.size();){auto end=text.find('\n',begin);if(end==std::string::npos)end=text.size();
        auto line=text.substr(begin,end-begin);begin=end+1;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;if(line=="[PathChoiceCloseObserver]"){if(section)return false;section=true;continue;}
        auto at=line.find('=');if(!section||at==std::string::npos||!at||line.size()>256||!fields.emplace(line.substr(0,at),line.substr(at+1)).second)return false;}
    Request r;unsigned schema{};
    auto u=[&](char const* key,auto& v){auto it=fields.find(key);return it!=fields.end()&&gears_ui_scene_trial::decimal(it->second,v);};
    auto a=[&](char const* key,std::uintptr_t& v){auto it=fields.find(key);return it!=fields.end()&&gears_ui_scene_trial::address(it->second,v);};
    if(!u("schema",schema)||schema!=1||!u("nonce",r.nonce)||!r.nonce||!u("pid",r.pid)||!r.pid||r.pid>0xffffffffull||
       !u("creation_time",r.creation_time)||!r.creation_time||!u("started_qpc",r.started_qpc)||!r.started_qpc||
       !u("world_generation",r.world_generation)||!r.world_generation||!a("base",r.base)||r.base>UINTPTR_MAX-0x2000000||
       !a("client",r.client)||r.client>UINTPTR_MAX-0x10000||!a("scene",r.scene)||!a("panel",r.panel)||
       r.scene==r.panel||r.scene==r.client||r.panel==r.client||r.panel>UINTPTR_MAX-0x1000||
       !a("active_header",r.active_header)||r.active_header<r.client||r.active_header-r.client>0x10000-16||
       !a("active_data",r.initial.data)||!u("active_num",r.initial.num)||!u("active_max",r.initial.max)||
       r.initial.num<1||r.initial.num>128||r.initial.max<r.initial.num||r.initial.max>4096||
       r.initial.data>UINTPTR_MAX-std::uintptr_t(r.initial.max)*8||!a("function",r.function)||r.function>UINTPTR_MAX-0x11c||
       !u("function_index",r.function_index)||!a("function_outer",r.function_outer)||!a("function_class",r.function_class)||
       r.function_outer>UINTPTR_MAX-144||r.function_class>UINTPTR_MAX-112||
       !u("function_name_index",r.function_name_index)||!u("function_number",r.function_number)||r.function_number||
       !u("duration_ms",r.duration_ms)||r.duration_ms<100||r.duration_ms>8000||fields.size()!=21u+unsigned(r.initial.num))return false;
    unsigned occurrences{};
    for(int i=0;i<r.initial.num;++i){std::uintptr_t member{};auto key="member_"+std::to_string(i);
        if(!a(key.c_str(),member)||std::find(r.members.begin(),r.members.end(),member)!=r.members.end())return false;
        occurrences+=member==r.scene;r.members.push_back(member);}
    if(occurrences!=1)return false;out=std::move(r);return true;
}
inline bool same(Request const& r,position::Config const& c){return r.nonce==c.nonce&&r.pid==c.pid&&
    r.creation_time==c.creation_time&&r.started_qpc==c.started_qpc&&r.world_generation==c.world_generation&&
    r.base==c.base&&r.client==c.client&&r.scene==c.scene&&r.panel==c.panel;}
// This is evidence coverage, never a matcher exemption or generic check mask.
inline bool covered(position::Config const& c,std::uintptr_t at,void const* expected,std::size_t size){
    auto bytes=static_cast<unsigned char const*>(expected);
    for(std::size_t i=0;i<size;++i){bool found=false;for(auto const& check:c.checks)
        if(at+i>=check.address&&at+i-check.address<check.bytes.size()){
            if(check.bytes[at+i-check.address]!=bytes[i])return false;found=true;}
        if(!found)return false;}return true;
}
template<class T> bool expected(position::Config const& c,std::uintptr_t at,T& value){
    auto bytes=reinterpret_cast<unsigned char*>(&value);
    for(std::size_t i=0;i<sizeof(T);++i){bool found=false;for(auto const& check:c.checks)
        if(at+i>=check.address&&at+i-check.address<check.bytes.size()){auto b=check.bytes[at+i-check.address];
            if(found&&bytes[i]!=b)return false;bytes[i]=b;found=true;}if(!found)return false;}return true;
}
inline bool descriptor(position::Config const& c,Request const& r){
    return same(r,c)&&covered(c,r.active_header,&r.initial,16)&&
        covered(c,r.initial.data,r.members.data(),r.members.size()*8);
}
template<class Read,class EntryRead,class Matches> bool certify(std::uintptr_t base,Read read,EntryRead entry,Matches matches){
    if(base>UINTPTR_MAX-0x2000000)return false;
    std::array<unsigned char,62> close{},again{};std::array<unsigned char,160> exec{},exec_again{};
    auto body=[&](auto& bytes){return entry(bytes.data(),reinterpret_cast<void const*>(base+close_rva),32)&&
        read(bytes.data()+32,reinterpret_cast<void const*>(base+close_rva+32),30);};
    std::uintptr_t slot{},slot_again{};
    return body(close)&&body(again)&&close==again&&matches(close.data(),62u,close_sha)&&
        read(exec.data(),reinterpret_cast<void const*>(base+exec_rva),128)&&
        read(exec.data()+128,reinterpret_cast<void const*>(base+exec_cold_rva),32)&&
        read(exec_again.data(),reinterpret_cast<void const*>(base+exec_rva),128)&&
        read(exec_again.data()+128,reinterpret_cast<void const*>(base+exec_cold_rva),32)&&exec==exec_again&&matches(exec.data(),160u,exec_sha)&&
        read(&slot,reinterpret_cast<void const*>(base+client_table_rva+close_slot),8)&&
        read(&slot_again,reinterpret_cast<void const*>(base+client_table_rva+close_slot),8)&&slot==slot_again&&slot==base+close_rva;
}
template<class Read> bool native_pointer(Request const& r,Read read){
    std::uintptr_t first{},second{};return read(&first,reinterpret_cast<void const*>(r.function+0x114),8)&&
        read(&second,reinterpret_cast<void const*>(r.function+0x114),8)&&first==second&&first==r.base+exec_rva;
}
template<class Read> bool registry(position::Config const& c,std::uintptr_t rva,Read read,position::RegistryHeader& initial,position::RegistryHeader& live){
    if(!expected(c,c.base+rva,initial)||!position::valid_registry_header(initial)||
       !read(&live,reinterpret_cast<void const*>(c.base+rva),16)||!position::valid_registry_header(live)||
       initial.data!=live.data||initial.max!=live.max||live.num<initial.num)return false;
    return c.schema==2||live.num==initial.num;
}
template<std::size_t N,class Read> bool name_is(position::Config const& c,unsigned index,char16_t const (&text)[N],Read read){
    position::RegistryHeader initial{},live{};std::uintptr_t entry{};unsigned entry_index{};std::array<char16_t,N> observed{};
    return registry(c,0x1c34800,read,initial,live)&&index<unsigned(initial.num)&&
        read(&entry,reinterpret_cast<void const*>(live.data+std::uintptr_t(index)*8),8)&&entry&&entry<=UINTPTR_MAX-20-sizeof(observed)&&
        read(&entry_index,reinterpret_cast<void const*>(entry),4)&&entry_index==index&&
        read(observed.data(),reinterpret_cast<void const*>(entry+20),sizeof(observed))&&std::memcmp(observed.data(),text,sizeof(observed))==0;
}
template<class Read> bool registered(position::RegistryHeader const& initial,position::RegistryHeader const& live,std::uintptr_t object,Read read){
    unsigned index{};std::uintptr_t slot{};
    return object&&object<=UINTPTR_MAX-112&&read(&index,reinterpret_cast<void const*>(object+8),4)&&index<unsigned(initial.num)&&
        read(&slot,reinterpret_cast<void const*>(live.data+std::uintptr_t(index)*8),8)&&slot==object;
}
template<class Read> bool function_identity(position::Config const& c,Request const& r,Read read){
    position::RegistryHeader initial{},live{};unsigned index{};std::array<std::uintptr_t,3> raw{};
    if(!registry(c,0x1c24730,read,initial,live)||!registered(initial,live,r.function,read)||
       !read(&index,reinterpret_cast<void const*>(r.function+8),4)||index!=r.function_index||
       !read(raw.data(),reinterpret_cast<void const*>(r.function+64),24)||raw[0]!=r.function_outer||raw[2]!=r.function_class||
       raw[1]!=(std::uint64_t(r.function_number)<<32|r.function_name_index)||!name_is(c,r.function_name_index,u"CloseScene",read)||
       !registered(initial,live,r.function_class,read))return false;
    std::array<unsigned,2> class_name{};
    if(!read(class_name.data(),reinterpret_cast<void const*>(r.function_class+72),8)||class_name[1]||!name_is(c,class_name[0],u"Function",read))return false;
    std::uintptr_t cls{};if(!expected(c,c.client+80,cls))return false;bool owner_found=false;
    for(unsigned i=0;i<64&&cls;++i){if(cls==r.function_outer){owner_found=true;break;}std::uintptr_t parent{};
        if(!expected(c,cls+96,parent)||parent==cls)return false;cls=parent;}
    if(!owner_found)return false;
    // Actual owned field-chain membership, bounded independently of old flattened checks.
    std::uintptr_t field{};std::array<std::uintptr_t,2048> seen{};unsigned used{};
    if(!read(&field,reinterpret_cast<void const*>(r.function_outer+128),8))return false;
    while(field&&used<seen.size()){
        if(std::find(seen.begin(),seen.begin()+used,field)!=seen.begin()+used||!registered(initial,live,field,read))return false;
        seen[used++]=field;std::uintptr_t outer{};
        if(!read(&outer,reinterpret_cast<void const*>(field+64),8)||outer!=r.function_outer)return false;
        if(field==r.function)return true;
        if(!read(&field,reinterpret_cast<void const*>(field+104),8))return false;
    }return false;
}
template<class Read> bool fixed_identity(position::Config const& c,std::uintptr_t object,Read read){
    position::RegistryHeader initial{},live{};
    if(!expected(c,c.base+0x1c24730,initial)||!position::valid_registry_header(initial)||
       !read(&live,reinterpret_cast<void const*>(c.base+0x1c24730),16)||
       !position::valid_registry_header(live)||live.data!=initial.data||live.max!=initial.max||live.num<initial.num)return false;
    if(c.schema==1&&live.num!=initial.num)return false;
    std::uint32_t index{},index_now{};std::uintptr_t slot{};
    if(!expected(c,object+8,index)||index>=unsigned(initial.num)||
       !read(&slot,reinterpret_cast<void const*>(live.data+std::uintptr_t(index)*8),8)||slot!=object||
       !read(&index_now,reinterpret_cast<void const*>(object+8),4)||index_now!=index)return false;
    std::array<unsigned char,24> identity{};
    return read(identity.data(),reinterpret_cast<void const*>(object+64),identity.size())&&covered(c,object+64,identity.data(),identity.size());
}
struct Snapshot {
    std::array<std::uint32_t,4> position{};std::uint32_t units{};std::uintptr_t vtable{};
    position::RegistryHeader active{};std::array<std::uintptr_t,128> members{};
    unsigned mask{},occurrences{};bool read_exception{};
};
// Mask: client/scene/panel fixed registry slot + raw ID tuple 1/2/4, vtable8,
// faces16, units32, stable bounded current ActiveScenes64. This does not prove
// names/reflection/liveness after close and grants no inactive write permission.
template<class Read> Snapshot snapshot(position::Config const& c,Request const& r,Read read)noexcept{
    Snapshot s;try{
        if(fixed_identity(c,c.client,read))s.mask|=1;if(fixed_identity(c,c.scene,read))s.mask|=2;
        if(fixed_identity(c,c.panel,read))s.mask|=4;
        if(s.mask&4){if(read(&s.vtable,reinterpret_cast<void const*>(c.panel),8))s.mask|=8;
            if(read(s.position.data(),reinterpret_cast<void const*>(c.panel+0x60),16))s.mask|=16;
            if(read(&s.units,reinterpret_cast<void const*>(c.panel+0x70),4))s.mask|=32;}
        if(!(s.mask&1)||!read(&s.active,reinterpret_cast<void const*>(r.active_header),16))return s;
        auto const& h=s.active;
        if(h.num<0||h.num>128||h.max<h.num||h.max>4096||(!h.data&&h.num)||h.data>UINTPTR_MAX-std::uintptr_t(h.max)*8)return s;
        if(h.num&&!read(s.members.data(),reinterpret_cast<void const*>(h.data),std::size_t(h.num)*8))return s;
        for(int i=0;i<h.num;++i){if(!s.members[unsigned(i)]||std::find(s.members.begin(),s.members.begin()+i,s.members[unsigned(i)])!=s.members.begin()+i)return s;
            s.occurrences+=s.members[unsigned(i)]==c.scene;}
        position::RegistryHeader last{};std::array<std::uintptr_t,128> again{};
        if(!read(&last,reinterpret_cast<void const*>(r.active_header),16)||std::memcmp(&last,&h,16)||
           (h.num&&!read(again.data(),reinterpret_cast<void const*>(h.data),std::size_t(h.num)*8))||again!=s.members||
           !read(&last,reinterpret_cast<void const*>(r.active_header),16)||std::memcmp(&last,&h,16))return s;
        if((s.mask&7)==7)s.mask|=64;
        for(unsigned i=0;i<3;++i){const std::uintptr_t object=i==0?c.client:i==1?c.scene:c.panel;
            if((s.mask&(1u<<i))&&!fixed_identity(c,object,read))s.mask&=~((1u<<i)|64u);}
    }catch(...){s.read_exception=true;s.mask&=~71u;}return s;
}
}
