#include "graphics_lock.h"
#include <cstdio>
#include <map>
#include <vector>

#define CHECK(c) do{if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return __LINE__;}}while(0)
using namespace gears_graphics_lock;
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> regions;
    std::vector<std::uintptr_t> writes;
    unsigned reads{};bool reject_read{},reject_write{},race{},bad_readback{};
    unsigned char* at(std::uintptr_t address,std::size_t size){
        for(auto& region:regions)if(address>=region.first&&address-region.first<=region.second.size()&&
            size<=region.second.size()-(address-region.first))return region.second.data()+address-region.first;
        return nullptr;
    }
    template<class T> void put(std::uintptr_t address,T value){std::memcpy(at(address,sizeof(value)),&value,sizeof(value));}
    template<class T> T get(std::uintptr_t address){T value{};std::memcpy(&value,at(address,sizeof(value)),sizeof(value));return value;}
    bool read(std::uintptr_t address,void* output,std::size_t size){
        ++reads;if(reject_read)return false;auto* data=at(address,size);if(!data)return false;
        std::memcpy(output,data,size);return true;
    }
    bool cas(std::uintptr_t address,std::uint32_t expected,std::uint32_t replacement){
        writes.push_back(address);if(reject_write||race)return false;
        auto* data=at(address,4);if(!data||get<std::uint32_t>(address)!=expected)return false;
        put(address,bad_readback?std::uint32_t{1}:replacement);return true;
    }
};
struct Fixture {
    static constexpr std::uintptr_t base=0x140000000,command=0x50000000;
    Memory mem;Context context{};
    Fixture(){
        for(const auto& entry:entries)mem.regions[base+entry.rva]={entry.bytes.begin(),entry.bytes.end()};
        mem.regions[base+singleton_rva]=std::vector<unsigned char>(0x600,0x5a);
        mem.regions[base+settings_vtable_rva]=std::vector<unsigned char>(0x200);
        mem.regions[base+command_vtable_rva]=std::vector<unsigned char>(16);
        mem.regions[command]=std::vector<unsigned char>(command_size,0x6b);
        mem.put(base+singleton_rva,base+settings_vtable_rva);
        mem.put(base+settings_vtable_rva+0x18,base+std::uintptr_t{0x26a9dc});
        mem.put(base+settings_vtable_rva+0x20,base+std::uintptr_t{0x269168});
        mem.put(base+settings_vtable_rva+0x190,base+menu_rva);
        mem.put(base+command_vtable_rva+8,base+commit_rva);
        mem.put(command,base+command_vtable_rva);
        mem.put(base+singleton_rva+normal_dlss_offset,std::uint32_t{1});
        mem.put(command+command_dlss_offset,std::uint32_t{1});
        auto reader=[&](auto a,auto* p,auto n){return mem.read(a,p,n);};
        context.admission=admit(base,true,reader);
        context.enabled=true;context.owner_thread=11;context.consumer_thread=22;
        context.owner_thread_verified=context.consumer_thread_verified=true;
        context.normal_write_authorized=context.command_write_authorized=context.command_layout_verified=true;
        mem.reads=0;
    }
};
int main(){
    unsigned cases{};
    {Fixture f;CHECK(f.context.admission.valid());++cases;
        auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto cas=[&](auto a,auto old,auto value){return f.mem.cas(a,old,value);};
        Result result{};unsigned calls{};auto original=[&](auto p){++calls;return p;};
        // A single four-byte normal-field clear precedes the original collector.
        auto before=f.mem.regions[Fixture::base+singleton_rva];
        auto returned=invoke_collector(f.context,f.context.admission.singleton,11,read,cas,original,result);
        CHECK(result.protected_now()&&result.modified&&calls==1&&returned==f.context.admission.singleton);
        CHECK(f.mem.writes==std::vector<std::uintptr_t>{f.context.admission.singleton+normal_dlss_offset});
        auto expected=before;std::memset(expected.data()+normal_dlss_offset,0,4);
        CHECK(f.mem.regions[Fixture::base+singleton_rva]==expected);++cases;
        // The original may inspect the same address/opaque data, with only DLSS changed.
        auto command_before=f.mem.regions[Fixture::command];unsigned rebuilds{};
        auto commit=[&](auto p)->std::uint32_t{CHECK(p==Fixture::command);
            if(f.mem.get<std::uint32_t>(p+command_dlss_offset)==0)++rebuilds;
            return commit_stride_eax;};
        CHECK(invoke_commit(f.context,Fixture::command,22,read,cas,commit,result)==commit_stride_eax);
        CHECK(result.protected_now()&&result.modified&&rebuilds==1);
        expected=command_before;std::memset(expected.data()+command_dlss_offset,0,4);
        CHECK(f.mem.regions[Fixture::command]==expected);++cases;
        auto count=f.mem.writes.size();
        CHECK(collector_before(f.context,f.context.admission.singleton,11,read,cas).protected_now());
        CHECK(commit_before(f.context,Fixture::command,22,read,cas).protected_now());
        CHECK(f.mem.writes.size()==count);++cases;
        unsigned menu_calls{};auto menu_original=[&](auto,std::int32_t mode){++menu_calls;return std::uint32_t(mode+10);};
        auto bytes=f.mem.regions;
        CHECK(invoke_menu(f.context,f.context.admission.singleton,3,11,read,menu_original,result)==menu_success_eax);
        CHECK(result.bypass_original&&result.protected_now()&&menu_calls==0&&f.mem.regions==bytes);++cases;
        CHECK(invoke_menu(f.context,f.context.admission.singleton,2,11,read,menu_original,result)==12);
        CHECK(result.reason==Reason::other_mode&&menu_calls==1&&f.mem.regions==bytes);++cases;
    }
    // Disabled is a literal pass-through, including unreadable memory and unknown threads.
    {Fixture f;f.context.enabled=false;f.mem.reject_read=true;
        auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto cas=[&](auto a,auto old,auto value){return f.mem.cas(a,old,value);};
        Result result{};unsigned calls{};auto bytes=f.mem.regions;
        auto original=[&](auto p){++calls;return p;};
        CHECK(invoke_collector(f.context,123,0,read,cas,original,result)==123);
        CHECK(result.reason==Reason::disabled&&!result.modified);
        CHECK(invoke_commit(f.context,123,0,read,cas,original,result)==123);
        CHECK(result.reason==Reason::disabled&&!result.modified);
        auto setter=[&](auto,auto){++calls;return 77;};
        CHECK(invoke_menu(f.context,123,3,0,read,setter,result)==77);
        CHECK(calls==3&&f.mem.reads==0&&f.mem.writes.empty()&&f.mem.regions==bytes);++cases;
    }
    // Activation requires external module identity, all entry bytes, and the concrete vtables.
    for(unsigned trial=0;trial<8;++trial){Fixture f;
        if(trial<3)++f.mem.regions[Fixture::base+entries[trial].rva][0];
        if(trial==3)f.mem.put(Fixture::base+singleton_rva,std::uintptr_t{1});
        if(trial==4)f.mem.put(Fixture::base+settings_vtable_rva+0x190,std::uintptr_t{1});
        if(trial==5)f.mem.put(Fixture::base+command_vtable_rva+8,std::uintptr_t{1});
        if(trial==6)f.mem.reject_read=true;
        auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto admission=admit(Fixture::base,trial!=7,read);
        CHECK(!admission.valid()&&f.mem.writes.empty());++cases;
    }
    // No field change and no protection claim when identity/thread/permission/read/CAS proof fails.
    for(unsigned trial=0;trial<11;++trial){Fixture f;auto settings=f.context.admission.singleton;auto thread=11U;
        if(trial==0)++settings;
        if(trial==1)f.context.owner_thread_verified=false;
        if(trial==2)thread=22;
        if(trial==3)f.context.normal_write_authorized=false;
        if(trial==4)f.mem.reject_read=true;
        if(trial==5)f.mem.put(settings,std::uintptr_t{1});
        if(trial==6)f.mem.reject_write=true;
        if(trial==7)f.mem.race=true;
        if(trial==8)f.mem.put(settings+normal_dlss_offset,std::uint32_t{2});
        if(trial==9)f.context.admission.reason=Reason::entry_mismatch;
        if(trial==10)thread=0;
        auto before=f.mem.regions;auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto cas=[&](auto a,auto old,auto value){return f.mem.cas(a,old,value);};
        unsigned calls{};Result result{};auto original=[&](auto p){++calls;return p;};
        CHECK(invoke_collector(f.context,settings,thread,read,cas,original,result)==settings);
        CHECK(result.fault()&&!result.protected_now()&&!result.modified&&calls==1&&f.mem.regions==before);++cases;
    }
    for(unsigned trial=0;trial<9;++trial){Fixture f;auto command=Fixture::command;auto thread=22U;
        if(trial==0)f.context.consumer_thread_verified=false;
        if(trial==1)thread=11;
        if(trial==2)f.context.command_layout_verified=false;
        if(trial==3)f.context.command_write_authorized=false;
        if(trial==4)++command;
        if(trial==5)f.mem.put(command,std::uintptr_t{1});
        if(trial==6)f.mem.reject_write=true;
        if(trial==7)f.mem.put(command+command_dlss_offset,std::uint32_t{0xffffffff});
        if(trial==8)f.mem.reject_read=true;
        auto before=f.mem.regions;auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto cas=[&](auto a,auto old,auto value){return f.mem.cas(a,old,value);};
        unsigned calls{};Result result{};auto original=[&](auto p){++calls;return p;};
        CHECK(invoke_commit(f.context,command,thread,read,cas,original,result)==command);
        CHECK(result.fault()&&!result.protected_now()&&!result.modified&&calls==1&&f.mem.regions==before);++cases;
    }
    {Fixture f;f.mem.bad_readback=true;
        auto read=[&](auto a,auto* p,auto n){return f.mem.read(a,p,n);};
        auto cas=[&](auto a,auto old,auto value){return f.mem.cas(a,old,value);};
        auto result=collector_before(f.context,f.context.admission.singleton,11,read,cas);
        CHECK(result.reason==Reason::verification_failed&&result.modified&&!result.protected_now());++cases;
        auto overflow=admit(std::numeric_limits<std::uintptr_t>::max()-8,true,read);
        CHECK(overflow.reason==Reason::address_overflow);++cases;
        // Void original collectors are supported without inventing a native return type.
        Result output{};unsigned calls{};auto original=[&](auto){++calls;};
        invoke_collector(Context{},1,0,read,cas,original,output);CHECK(calls==1&&output.reason==Reason::disabled);++cases;
    }
    std::printf("GraphicsLockTest: %u cases passed\n",cases);return 0;
}
