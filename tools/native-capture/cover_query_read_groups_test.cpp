#define GEARS_QUERY_SESSION_TEST_MAIN PriorReadGroupSessionMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
static unsigned group_runs{},owner_checks{},fail_owner{},mutation{};
static bool expired{},inside_reads{};
static void(*late_group_action)(){};
static bool group_owner(){++owner_checks;return !expired&&owner_checks!=fail_owner&&guard();}
static void change_after_reads(){
    switch(mutation){
    case 1:put(body_instances[0]+0x60,7);break;
    case 2:put(body_instances[0]+0x68,uintptr_t{});break;
    case 3:put(props[7]+0x94,0x98u);break;
    case 4:put(audio+0x170,fixture_pawn);break;
    case 5:put(arena+0xd0000,uintptr_t{});break;
    case 6:put(cylinder+0x1a0,uintptr_t{});break;
    case 7:expired=true;break;
    case 8:++h::metadata_generation;break;
    case 9:if(late_group_action)late_group_action();break;
    }
}
struct GroupEngine:HostEngine {
    gears_cover_physx_asset_actors::Workspace group_assets{};
    template<class Shape,class Read,class Guard>q::Code sweep_all(q::source::Source const& source,Shape const& shape,q::Vector const& delta,q::StreamLimits const& limits,
        Read read,Guard current,q::AllSweepOwners& out,gears_cover_physx_collider::Source const* collider,gears_cover_physx_components::Source const* components){
        auto pure_reads=[&](auto& borrowed){
            ++group_runs;inside_reads=true;struct End{~End(){inside_reads=false;}}end;
            q::source::Snapshot scope{};
            if(q::source::capture(0x180000000,0x300000,source,read,borrowed,scope)!=q::source::Code::current)return q::Code::source;
            gears_cover_physx_asset_actors::Snapshot assets{};gears_cover_physx_collider::Snapshot body{};
            if(gears_cover_physx_asset_actors::observe(0x180000000,0x300000,scope,read,borrowed,group_assets,assets)!=gears_cover_physx_asset_actors::Code::observed)return q::Code::source;
            if(gears_cover_physx_collider::observe(0x180000000,0x300000,scope,*collider,read,borrowed,body)!=gears_cover_physx_collider::Code::observed)return q::Code::source;
            if(gears_cover_physx_components::observe(0x180000000,0x300000,scope,*components,read,borrowed,parts_work,parts_value)!=gears_cover_physx_components::Code::observed)return q::Code::source;
            for(unsigned i=0;i<64;++i)if(!borrowed(scope))return q::Code::source;
            C(query_calls==0);change_after_reads();return q::Code::queried;
        };
        const auto code=q::detail::read_group(current,pure_reads,0);if(code!=q::Code::queried)return code;
        C(!inside_reads);return HostEngine::sweep_all(source,shape,delta,limits,read,current,out,collider,components);
    }
};
using GroupSession=service::Session<GroupEngine>;
static GroupSession* group_session{};
static void drain_group(){group_session->request_drain();}
static void reset_group(){reset();catalog_setup();group_runs=owner_checks=fail_owner=mutation=0;expired=inside_reads=false;late_group_action=nullptr;}
static void original_boundary(){C(!inside_reads);replace_root();}
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=777;const auto saved=std::make_unique<q::AllSweepOwners>(*output);unsigned full_checks{};
    reset_group();
    {auto owner=std::make_unique<GroupSession>();C(owner->start(true,true));
     C(owner->sweep_all(actor,physics_expected,test_box,{1,0,0},q::StreamLimits{},source_read,group_owner,*output)==q::Code::queried);
     C(output->raw.hit_count==2&&query_calls==1&&group_runs==1&&owner->observation().query_read_guards>=64);
     full_checks=owner_checks;owner->request_drain();C(owner->poll_stop());}
    for(unsigned rejected_owner=1;rejected_owner<=full_checks;++rejected_owner){
      reset_group();fail_owner=rejected_owner;*output=*saved;auto owner=std::make_unique<GroupSession>();C(owner->start(true,true));
      C(owner->sweep_all(actor,physics_expected,test_box,{1,0,0},q::StreamLimits{},source_read,group_owner,*output)!=q::Code::queried);
      C(output->raw.raw_count==777&&!inside_reads&&query_calls<=1);owner->request_drain();C(owner->poll_stop());
    }
    for(unsigned bad=1;bad<=9;++bad){
      reset_group();mutation=bad;*output=*saved;auto owner=std::make_unique<GroupSession>();C(owner->start(true,true));
      group_session=owner.get();late_group_action=drain_group;
      C(owner->sweep_all(actor,physics_expected,test_box,{1,0,0},q::StreamLimits{},source_read,group_owner,*output)!=q::Code::queried);
      C(group_runs==1&&query_calls==0&&output->raw.raw_count==777&&!inside_reads);owner->request_drain();C(owner->poll_stop());
    }
    reset_group();during_query=original_boundary;*output=*saved;
    {auto owner=std::make_unique<GroupSession>();C(owner->start(true,true));
     C(owner->sweep_all(actor,physics_expected,test_box,{1,0,0},q::StreamLimits{},source_read,group_owner,*output)!=q::Code::queried);
     C(query_calls==1&&output->raw.raw_count==777);owner->request_drain();C(owner->poll_stop());}
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("QueryReadGroups checks=%u failures=%u full_owner=%u Host dispatch only; complete real typed/raw readers\n",checks,failures,full_checks);return failures?1:0;
}
