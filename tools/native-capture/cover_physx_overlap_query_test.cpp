#define GEARS_OWNED_SWEEP_TEST_MAIN PriorOwnedSweepFixtureMain
#include "cover_physx_owned_sweep_test.cpp"
namespace enumeration=gears_cover_physx_overlap_code;
enum class EnumerationMode {normal,empty,fault,untrusted,overrun,mismatch,source_change,code_change,catalog_change,unsupported,guard_change};
static EnumerationMode enumeration_mode{};static bool owner_allowed=true;static void(*enumeration_probe)(){};
struct EnumerationGuard:Guard {
    using Guard::operator();
    bool operator()(){return owner_allowed;}
    bool operator()(s::Snapshot const& scope){return owner_allowed&&Guard::operator()(scope);}
};
struct EnumerationCalls:HostCalls {
    static bool overlap_shapes(std::uintptr_t function,std::uintptr_t public_scene,q::Capsule const& geometry,
        std::uintptr_t* batch,q::detail::OverlapReporter& receiver,std::uint32_t& returned){
        ++original_calls;check(function==base+0x172160&&public_scene==0x180000,"exact original public capsule enumeration entry");
        check(geometry.first.x==1&&geometry.second.z==5&&geometry.radius==.5f,"original whole capsule geometry retained");
        if(enumeration_probe)enumeration_probe();
        if(enumeration_mode==EnumerationMode::fault)return false;
        if(enumeration_mode==EnumerationMode::source_change)live_memory->put<std::uintptr_t>(0x150088,actor1);
        if(enumeration_mode==EnumerationMode::code_change)live_memory->put<unsigned char>(base+0xaa3e0,0x90);
        if(enumeration_mode==EnumerationMode::catalog_change)live_memory->put<std::uintptr_t>(0x280000,0);
        if(enumeration_mode==EnumerationMode::unsupported)live_memory->put<std::uintptr_t>(shape1,base+0x241358);
        if(enumeration_mode==EnumerationMode::guard_change)owner_allowed=false;
        returned=0;if(enumeration_mode==EnumerationMode::empty)return true;
        if(enumeration_mode==EnumerationMode::untrusted){std::array<std::uintptr_t,2> other{shape0,shape1};
            receiver.vtable[0](&receiver,2,other.data());returned=2;return true;}
        if(enumeration_mode==EnumerationMode::overrun){receiver.vtable[0](&receiver,2,batch+63);returned=2;return true;}
        batch[0]=shape0;batch[1]=shape1;returned=2;
        if(!receiver.vtable[0](&receiver,2,batch))return true;
        batch[2]=shape0;returned=3;
        if(!receiver.vtable[0](&receiver,1,batch+2))return true;
        receiver.vtable[0](&receiver,0,batch+64);
        if(enumeration_mode==EnumerationMode::mismatch)++returned;
        return true;
    }
};
using EnumerationEngine=q::QueryEngine<HostImage,EnumerationCalls>;
static EnumerationEngine* enumeration_engine{};static q::Code nested_enumeration{};static bool enumeration_pending{},enumeration_closed{};
static q::Capsule endpoint{{1,2,3},{1,2,5},.5f};
static void reenter(){q::AllOverlap out{};nested_enumeration=enumeration_engine->overlap_all(source(),endpoint,{},
    [](void*,void const*,std::size_t){return false;},EnumerationGuard{},out);
    enumeration_pending=enumeration_engine->pending();enumeration_closed=enumeration_engine->close();}
int main(int argc,char**argv){
    if(argc!=2)return 2;
    try{
        const auto initial=setup(argv[1]);auto memory=initial;live_memory=&memory;
        auto read=[&](void* out,void const* address,std::size_t bytes){return memory.read(out,address,bytes);};
        auto output=std::make_unique<q::AllOverlapOwners>();output->raw.raw_count=99;output->owners[0].actor=0xdead;output->self_sources[0]=0xff;
        auto saved=std::make_unique<q::AllOverlapOwners>(*output);
        auto equal_output=[&](){return !memcmp(output.get(),saved.get(),sizeof *output);};
        EnumerationGuard guard;owner_allowed=true;enumeration_mode=EnumerationMode::normal;
        auto engine=std::make_unique<EnumerationEngine>();enumeration_engine=engine.get();original_calls=0;
        check(engine->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::unavailable&&equal_output(),"unprimed enumeration leaves all output unchanged");
        check(engine->prime(),"Host image prime only");enumeration_probe=reenter;
        check(engine->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried,"typed source closed around one actual production enumeration transaction");
        check(nested_enumeration==q::Code::reentrant&&enumeration_pending&&!enumeration_closed&&original_calls==1,"one owner and image pin span original call, no reentry/unload");
        enumeration_probe=nullptr;
        check(output->raw.raw_count==3&&output->raw.shape_count==3&&output->raw.reports==3,"all shape batches including zero report and returned count");
        check(output->raw.shapes[0]==shape0&&output->raw.shapes[1]==shape1&&output->raw.shapes[2]==shape0&&!output->raw.shapes[3],"original pointer sequence duplicate and cleared tail");
        check(output->owners[0].actor==0x160000&&output->owners[1].actor==actor1&&output->owners[2].actor==0x160000&&!output->owners[3].actor,"every public shape receives exact original membership/backlink observation");
        check(!output->self_sources[0]&&!output->self_sources[1]&&!output->self_sources[2],"unproven primary mesh and foreign source remain unexcluded");
        check(parts::consistent(output->component_sources)&&output->asset_bodies.body_count==2,"same transaction carries actual complete declared typed catalogs");
        check(engine->observation().original_started==1&&engine->observation().original_returned==1&&engine->observation().stage==q::QueryStage::complete,"original start/normal return counted exactly");
        enumeration_mode=EnumerationMode::empty;
        check(engine->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::queried&&!output->raw.shape_count&&!output->owners[0].actor&&!output->self_sources[0],"empty observation clears previous shape and owner arrays; not clearance");
        check(engine->close()&&!engine->pending(),"normal enumeration owner fully closes");
        {memory=initial;enumeration_mode=EnumerationMode::normal;auto owner=std::make_unique<EnumerationEngine>();check(owner->prime(),"raw enumeration prime");q::AllOverlap raw{};raw.raw_count=99;
            check(owner->overlap_all(source(),endpoint,{},read,guard,raw)==q::Code::queried&&raw.shape_count==3&&raw.shapes[2]==shape0,"raw pointer observation also uses original source/code closure");
            const auto before=raw;const auto calls=original_calls;auto invalid=endpoint;invalid.second=invalid.first;
            check(owner->overlap_all(source(),invalid,{},read,guard,raw)==q::Code::request&&!memcmp(&raw,&before,sizeof raw)&&original_calls==calls,"zero-axis capsule still prohibited before original call");
            check(owner->close(),"raw observer closes normally");}
        for(auto const& chunk:enumeration::chunks){memory=initial;original_calls=0;*output=*saved;
            memory.blocks[base][chunk.rva+chunk.size-1]^=1;auto owner=std::make_unique<EnumerationEngine>();check(owner->prime(),"certificate test prime");
            check(owner->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::code&&equal_output()&&!original_calls,"changed selected enumeration callee tail rejected before dispatch");}
        for(auto const slot:{base+0x23ca98+0x2f0,base+0x22ccb8+0x248}){memory=initial;original_calls=0;*output=*saved;
            memory.put<std::uintptr_t>(slot,0);auto owner=std::make_unique<EnumerationEngine>();check(owner->prime(),"vtable test prime");
            check(owner->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::code&&equal_output()&&!original_calls,"changed public/internal enumeration vtable slot rejected");}
        const std::array<std::pair<EnumerationMode,q::Code>,9> cases{{{EnumerationMode::fault,q::Code::fault_unknown},{EnumerationMode::untrusted,q::Code::callback_rejected},
            {EnumerationMode::overrun,q::Code::callback_rejected},{EnumerationMode::mismatch,q::Code::callback_rejected},
            {EnumerationMode::source_change,q::Code::changed},{EnumerationMode::code_change,q::Code::code},{EnumerationMode::catalog_change,q::Code::changed},
            {EnumerationMode::unsupported,q::Code::owner_unsupported},{EnumerationMode::guard_change,q::Code::changed}}};
        for(auto const& item:cases){memory=initial;*output=*saved;owner_allowed=true;enumeration_mode=item.first;original_calls=0;
            auto owner=std::make_unique<EnumerationEngine>();check(owner->prime(),"failure test prime");
            check(owner->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==item.second&&equal_output(),"fault/incomplete/source/code/owner failure preserves every public output byte");
            if(item.first==EnumerationMode::fault){check(owner->uncertain()&&owner->pending()&&!owner->close()&&!owner->prime(),"uncertain original call pins image and remains pending");
                const auto count=original_calls;owner_allowed=true;check(owner->overlap_all(source(),endpoint,{},read,guard,*output,&collider_input,&parts_input)==q::Code::unavailable&&original_calls==count,"fault does not silently retry original call");}}
        owner_allowed=true;enumeration_mode=EnumerationMode::normal;
        for(auto budget:std::array<q::StreamLimits,3>{{{2,256},{1024,1},{3,3}}}){memory=initial;*output=*saved;
            auto owner=std::make_unique<EnumerationEngine>();check(owner->prime(),"budget test prime");
            const bool complete=budget.max_hits==3;const auto code=owner->overlap_all(source(),endpoint,budget,read,guard,*output,&collider_input,&parts_input);
            check(code==(complete?q::Code::queried:q::Code::truncated)&&(complete||equal_output()),"exact budgets pass; exhausted budgets never publish prefix");}
        check(!q::detail::active_query_reporter&&!q::detail::active_overlap_reporter,"no callback owner remains");
    }catch(...){check(false,"fixture exception");}
    printf("CoverOverlapQuery checks=%u failed=%u production transaction with Host module/dispatch; original DLL data only\n",checks,failed);return failed?1:0;
}
