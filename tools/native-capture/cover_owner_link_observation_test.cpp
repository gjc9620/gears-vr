#define main PriorOwnerLinkQueryFixtureMain
#include "cover_physx_query_test.cpp"
#undef main
#include <memory>
#include <thread>
namespace obs=gears_cover_owner_link_observation;
namespace aa=gears_cover_physx_asset_actors;
namespace cc=gears_cover_physx_collider;
namespace cp=gears_cover_physx_components;
int main(){
    const obs::Key key{11,22,33,44,GetCurrentThreadId()};
    obs::Report report{};
    obs::Row example{false,obs::Reader::asset,{},obs::Branch::reverse,key.thread,100,200,300,300,400,500,true};
    obs::note(example);check(!report.first.observed,"unarmed note ignored");
    {obs::Scope off(false,key,report);obs::PhaseScope phase(obs::Phase::before_dispatch);obs::note(example);}
    check(!report.first.observed,"disabled scope ignored");
    for(unsigned n=0;n<5;++n){auto bad=key;
        if(n==0)bad.world=0;if(n==1)bad.pawn=0;if(n==2)bad.scene=0;if(n==3)bad.compose=0;if(n==4)bad.thread=0;
        obs::Scope scope(true,bad,report);check(!obs::active,"invalid key not installed");}
    {obs::Scope scope(true,key,report);obs::note(example);check(!report.first.observed,"outside owner phase ignored");
        {obs::PhaseScope phase(obs::Phase::before_dispatch);
            auto wrong=example;wrong.thread++;obs::note(wrong);check(!report.first.observed,"wrong thread ignored");
            std::thread worker([&](){obs::note(example);});worker.join();check(!report.first.observed,"other thread isolated");
            obs::note(example);check(report.first.observed&&report.first.phase==obs::Phase::before_dispatch,"phase captured");
            auto later=example;later.actor++;obs::note(later);check(report.first.actor==100,"first failure frozen");
            obs::Report inner{};{obs::Scope nested(true,key,inner);obs::PhaseScope closing(obs::Phase::after_dispatch);obs::note(later);
                check(inner.first.actor==101&&inner.first.phase==obs::Phase::after_dispatch,"nested report separate");}
            check(obs::active->phase==obs::Phase::before_dispatch,"nested phase restored");}
        check(obs::active->phase==obs::Phase::none,"owner phase removed");}
    check(!obs::active,"query scope removed");
    // Exercise each actual reader's links return, not a copied predicate.
    for(unsigned reader=1;reader<=3;++reader)for(unsigned failure=0;failure<3;++failure){
        auto memory=fixture();memory.block(0x140000,0x98);memory.block(0x160000,0x48);memory.block(0x190000,0x200);memory.block(0x1a0000,0x18);
        memory.put<std::uintptr_t>(0x140088,0x160000);memory.put<std::uintptr_t>(0x140090,0);
        memory.put<std::uintptr_t>(0x190180,0x140000);
        memory.put<std::uintptr_t>(0x160000,base+0x2330a8);
        memory.put<std::uintptr_t>(0x160038,failure==0?0x180000:0x170000);
        memory.put<std::uintptr_t>(0x160040,failure==1?1:0x1a0000);
        memory.put<std::uintptr_t>(0x1a0010,28);
        memory.put<std::uintptr_t>(0x160030,failure==2?0x180000:0x160000);
        s::Snapshot scope{};scope.source.thread=key.thread;scope.source.body_index=0;scope.scene=0x170000;
        scope.bodies.count=1;scope.members[0]=0x140000;scope.actor=0x160000;
        auto read=[&](void* p,void const* address,std::size_t bytes){return memory.read(p,address,bytes);};
        auto invoke=[&](){
            if(reader==1){aa::Snapshot output{};return aa::detail::sample(base,scope,read,output)==aa::Code::links;}
            if(reader==2){cc::Source input{};input.component=0x190000;input.body_instance_offset=0x180;
                input.body=0x140000;input.body_data=0x160000;cc::Snapshot output{};
                return cc::detail::sample(base,scope,input,read,output)==cc::Code::links;}
            auto packet=std::make_unique<cp::Snapshot>();cp::Actor actor{};
            return cp::detail::actor(base,scope,0x160000,read,actor,*packet)==cp::Code::links;
        };
        memory.calls=0;check(invoke(),"unarmed actual reader refuses bad links");const auto baseline_reads=memory.calls;
        memory.calls=0;{obs::Scope recording(true,key,report);obs::PhaseScope phase(obs::Phase::before_dispatch);
            check(!report.first.observed,"fresh query clears old witness");check(invoke(),"armed reader retains refusal");}
        auto const& row=report.first;
        check(memory.calls==baseline_reads,"observation adds no reads");
        check(row.observed&&unsigned(row.reader)==reader&&row.actor==0x160000&&row.expected_scene==0x170000,
            "exact reader and already-read actor scene recorded");
        check(row.branch==(failure==2?obs::Branch::embedded_owner:obs::Branch::scene_or_internal),"exact failing branch recorded");
        check(row.reverse_read==(failure==2)&&row.reverse==(failure==2?0x180000u:0u),"unread reverse remains unknown");
        check(row.observed_scene==(failure==0?0x180000u:0x170000u)&&row.internal==(failure==1?1u:0x1a0000u),
            "actual mismatching values retained");
    }
    check(!GetModuleHandleW(L"PhysXCore.dll"),"no original DLL executed");
    printf("CoverOwnerLinkObservation checks=%u failed=%u actual reader branches; no game\n",checks,failed);
    return failed?1:0;
}
