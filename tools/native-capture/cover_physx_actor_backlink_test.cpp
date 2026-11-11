#define main PriorActorBacklinkQueryMain
#include "cover_physx_query_test.cpp"
#undef main
#include <memory>
namespace aa=gears_cover_physx_asset_actors;
namespace cc=gears_cover_physx_collider;
namespace cp=gears_cover_physx_components;
int main(){
    for(unsigned reader=1;reader<=3;++reader)for(unsigned scenario=0;scenario<4;++scenario){
        auto memory=fixture();memory.block(0x140000,0x98);memory.block(0x160000,0x48);
        memory.block(0x190000,0x200);memory.block(0x1a0000,0x18);
        memory.put<std::uintptr_t>(0x140088,0x160000);memory.put<std::uintptr_t>(0x140090,0);
        memory.put<std::uintptr_t>(0x190180,0x140000);
        memory.put<std::uintptr_t>(0x160040,scenario==2?0:0x1a0000);
        // The r172 value 28 is not an Actor pointer. Old fixtures invented one.
        memory.put<std::uintptr_t>(0x1a0010,scenario==0?28:0x160000);
        if(scenario==1)memory.put<std::uintptr_t>(0x160030,0x180000);
        if(scenario==3)memory.put<std::uintptr_t>(0x160028,base+0x2330a8);
        s::Snapshot scope{};scope.source.thread=GetCurrentThreadId();scope.source.body_index=0;
        scope.scene=0x170000;scope.bodies.count=1;scope.members[0]=0x140000;scope.actor=0x160000;
        auto read=[&](void* p,void const* at,std::size_t n){return memory.read(p,at,n);};
        auto run=[&](){
            if(reader==1){aa::Snapshot out{};return aa::detail::sample(base,scope,read,out)==aa::Code::observed;}
            if(reader==2){cc::Source in{};in.component=0x190000;in.body_instance_offset=0x180;
                in.body=0x140000;in.body_data=0x160000;cc::Snapshot out{};return cc::detail::sample(base,scope,in,read,out)==cc::Code::observed;}
            auto out=std::make_unique<cp::Snapshot>();cp::Actor value{};
            return cp::detail::actor(base,scope,0x160000,read,value,*out)==cp::Code::observed;
        };
#ifdef EXPECT_BROKEN_BACKLINK
        const bool expected=scenario==1||scenario==3;
#else
        const bool expected=scenario==0||scenario==2;
#endif
        check(run()==expected,"actual reader accepts only the correctly typed public owner; baseline contrast retained");
    }
    printf("CoverPhysXActorBacklink checks=%u failed=%u actual reader contrast; no game/original calls\n",checks,failed);
    return failed?1:0;
}
