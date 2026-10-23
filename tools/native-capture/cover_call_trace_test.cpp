#include "cover_call_trace.h"
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <utility>
#include <vector>

namespace trace=gears_cover_call_trace;
#define CHECK(condition) do{++checks;if(!(condition)){std::printf("failed line %d: %s\n",__LINE__,#condition);return 1;}}while(false)
struct Memory {
    std::map<std::uintptr_t,unsigned char> bytes;
    std::vector<std::pair<std::uintptr_t,std::size_t>> spans;
    unsigned reads{},fail_read{};
    std::function<void(std::uintptr_t,std::size_t)> on_read;
    void put(std::uintptr_t address,void const* value,std::size_t size){
        auto const* source=static_cast<unsigned char const*>(value);
        for(std::size_t i=0;i<size;++i)bytes[address+i]=source[i];
    }
    template<class T> void put(std::uintptr_t address,T const& value){put(address,&value,sizeof(value));}
    bool read(void* output,void const* source,std::size_t size){
        const auto address=reinterpret_cast<std::uintptr_t>(source);++reads;spans.emplace_back(address,size);
        if(reads==fail_read)return false;
        if(on_read)on_read(address,size);
        auto* destination=static_cast<unsigned char*>(output);
        for(std::size_t i=0;i<size;++i){auto item=bytes.find(address+i);if(item==bytes.end())return false;destination[i]=item->second;}
        return true;
    }
};
struct Fixture {
    static constexpr std::uintptr_t caller=0x1000,self=0x9000,locals=0x6000;
    Memory memory;
    trace::Function function{0x3000,0x4000,128,256,{0,1},{4,1},{200,1}};
    Fixture(){
        for(unsigned i=0;i<0x40;++i)memory.bytes[caller+i]=0;
        for(unsigned i=0;i<function.size;++i)memory.bytes[function.script+i]=static_cast<unsigned char>(i);
        for(unsigned i=0;i<function.locals_size;++i)memory.bytes[locals+i]=static_cast<unsigned char>(i);
        memory.put(caller+0x18,function.address);memory.put(caller+0x20,self);
        memory.put(caller+0x28,function.script+64);memory.put(caller+0x30,locals);
        memory.bytes[locals]=1;memory.bytes[locals+4]=2;memory.bytes[locals+200]=3;
    }
    trace::Result capture(){
        auto read=[&](void* out,void const* source,std::size_t size){return memory.read(out,source,size);};
        return trace::capture(read,caller,self,function);
    }
};
int main(){
    unsigned checks{};unsigned accepted_reads{};
    {
        Fixture f;const auto before=f.memory.bytes;auto result=f.capture();const auto& s=result.snapshot;
        CHECK(result&&result.status==trace::Status::accepted);accepted_reads=f.memory.reads;
        CHECK(s.frame.node==f.function.address&&s.frame.object==f.self&&s.locals==f.locals);
        CHECK(s.values.pawn_ct==1&&s.values.pawn_ca==2&&s.values.pawn_cd==3);
        CHECK(trace::equal(s.values,s.after_values));CHECK(s.code_offset==64&&s.script_window_offset==48&&s.script_window_size==32);
        CHECK(s.raw_locals_size==128&&s.raw_locals[0]==1&&s.raw_locals[4]==2&&s.raw_locals[127]==127);
        CHECK(s.frame_equal&&s.locals_pointer_equal&&s.storage_equal&&s.script_equal&&!s.mutation_authorized);
        CHECK(f.memory.bytes==before);CHECK(std::strcmp(trace::status_name(result.status),"accepted")==0);
        CHECK(std::strcmp(trace::status_name(static_cast<trace::Status>(999)),"unknown")==0);
    }
    for(unsigned offset:{0u,15u,16u,64u,127u}){
        Fixture f;f.memory.put(f.caller+0x28,f.function.script+offset);auto result=f.capture();const auto& s=result.snapshot;
        CHECK(result);CHECK(s.script_window_size<=32&&s.script_window_offset+s.script_window_size<=f.function.size);
        CHECK(s.script_window_offset<=offset&&offset<s.script_window_offset+s.script_window_size);
        for(unsigned i=0;i<s.script_window_size;++i)CHECK(s.script_window[i]==s.script_window_offset+i);
    }
    {
        Fixture f;f.function.locals_size=8;f.function.pawn_cd={6,1};f.function.size=8;
        f.memory.put(f.caller+0x28,f.function.script+7);auto result=f.capture();
        CHECK(result&&result.snapshot.raw_locals_size==8&&result.snapshot.script_window_size==8);
        CHECK(result.snapshot.script_window_offset==0&&result.snapshot.values.pawn_cd==6);
        for(auto const& span:f.memory.spans)if(span.first>=f.locals&&span.first<f.locals+256)CHECK(span.second<=8);
    }
    {
        Fixture baseline;std::vector<trace::Function> invalid;
        auto add=[&](auto change){auto function=baseline.function;change(function);invalid.push_back(function);};
        add([](auto& f){f.address=0;});add([](auto& f){++f.address;});add([](auto& f){f.script=0;});
        add([](auto& f){f.size=0;});add([](auto& f){f.size=trace::script_limit+1;});
        add([](auto& f){f.locals_size=0;});add([](auto& f){f.locals_size=trace::locals_limit+1;});
        add([](auto& f){f.script=(std::numeric_limits<std::uintptr_t>::max)()-7;f.size=16;});
        add([](auto& f){f.pawn_ct.width=0;});add([](auto& f){f.pawn_ca.width=2;});
        add([](auto& f){f.pawn_cd.offset=f.locals_size;});add([](auto& f){f.pawn_cd.offset=0;});
        for(auto const& function:invalid){Fixture f;f.function=function;CHECK(!f.capture());CHECK(f.memory.reads==0);}
    }
    {
        Fixture f;auto read=[&](void* out,void const* source,std::size_t size){return f.memory.read(out,source,size);};
        for(auto caller:{std::uintptr_t(0),std::uintptr_t(1),(std::numeric_limits<std::uintptr_t>::max)()-7}){
            auto result=trace::capture(read,caller,f.self,f.function);CHECK(result.status==trace::Status::invalid_caller);CHECK(f.memory.reads==0);}
        CHECK(trace::capture(read,f.caller,0,f.function).status==trace::Status::invalid_caller);
        CHECK(trace::capture(read,f.caller,f.self+1,f.function).status==trace::Status::invalid_caller);CHECK(f.memory.reads==0);
    }
    for(bool node:{false,true}){
        Fixture f;f.memory.put(f.caller+(node?0x18:0x20),node?f.function.address+8:f.self+8);
        CHECK(f.capture().status==trace::Status::caller_mismatch);
    }
    for(auto offset:{-1,128}){
        Fixture f;f.memory.put(f.caller+0x28,f.function.script+offset);CHECK(f.capture().status==trace::Status::code_outside_script);
    }
    for(auto pointer:{std::uintptr_t(0),Fixture::locals+1,(std::numeric_limits<std::uintptr_t>::max)()-7}){
        Fixture f;f.memory.put(f.caller+0x30,pointer);CHECK(f.capture().status==trace::Status::invalid_locals);
    }
    // Every actual reader call can fail, including the second endpoint cohort.
    for(unsigned fail=1;fail<=accepted_reads;++fail){Fixture f;f.memory.fail_read=fail;CHECK(!f.capture());}
    for(auto field:{0x18u,0x20u,0x28u,0x38u}){
        Fixture f;unsigned calls{};
        f.memory.on_read=[&](auto address,auto){if(address==f.caller+field&&++calls==2){
            std::uintptr_t changed=field==0x28?f.function.script+65:0xdead0000;f.memory.put(address,changed);}};
        auto result=f.capture();CHECK(result.status==trace::Status::frame_changed);CHECK(!result.snapshot.frame_equal);
    }
    {
        Fixture f;unsigned calls{};
        f.memory.on_read=[&](auto address,auto){if(address==f.caller+0x30&&++calls==2)f.memory.put(address,f.locals+0x1000);};
        auto result=f.capture();CHECK(result.status==trace::Status::locals_changed);
        CHECK(result.snapshot.frame_equal&&!result.snapshot.locals_pointer_equal);
    }
    for(bool selected_outside_prefix:{false,true}){
        Fixture f;unsigned calls{};const auto target=selected_outside_prefix?f.locals+200:f.locals;
        f.memory.on_read=[&](auto address,auto){if(address==target&&++calls==2)
            f.memory.bytes[selected_outside_prefix?f.locals+200:f.locals+10]^=0x40;};
        auto result=f.capture();CHECK(result.status==trace::Status::storage_changed);CHECK(!result.snapshot.storage_equal);
        if(selected_outside_prefix)CHECK(!trace::equal(result.snapshot.values,result.snapshot.after_values));
        else CHECK(trace::equal(result.snapshot.values,result.snapshot.after_values));
    }
    {
        Fixture f;unsigned calls{};
        f.memory.on_read=[&](auto address,auto){if(address==f.function.script+48&&++calls==2)f.memory.bytes[address]^=1;};
        auto result=f.capture();CHECK(result.status==trace::Status::script_changed);CHECK(!result.snapshot.script_equal);
    }
    {
        // A byte outside the retained prefix/selected fields is deliberately
        // unobserved. storage_equal must never be described as a whole-frame hash.
        Fixture f;unsigned calls{};
        f.memory.on_read=[&](auto address,auto){if(address==f.locals&&++calls==2)f.memory.bytes[f.locals+150]^=1;};
        auto result=f.capture();CHECK(result&&result.snapshot.storage_equal);
    }
    {
        // Entry and return snapshots are independent. Changing code/storage
        // between them is observable, without treating an old pointer as a lease.
        Fixture f;auto entry=f.capture();CHECK(entry);
        constexpr std::uintptr_t next_locals=0x7000;
        for(unsigned i=0;i<f.function.locals_size;++i)f.memory.bytes[next_locals+i]=0;
        f.memory.bytes[next_locals]=8;f.memory.bytes[next_locals+4]=9;f.memory.bytes[next_locals+200]=10;
        f.memory.put(f.caller+0x30,next_locals);f.memory.put(f.caller+0x28,f.function.script+80);f.memory.spans.clear();
        auto returned=f.capture();CHECK(returned);CHECK(!trace::equal(entry.snapshot.values,returned.snapshot.values));
        CHECK(returned.snapshot.locals==next_locals&&returned.snapshot.code_offset==80);
        for(auto const& span:f.memory.spans)CHECK(span.first<f.locals||span.first>=f.locals+f.function.locals_size);
    }
    std::printf("CoverCallTraceTest passed: %u checks; bounded readonly snapshots, endpoint consistency, invalid ranges and failures\n",checks);
    return 0;
}
