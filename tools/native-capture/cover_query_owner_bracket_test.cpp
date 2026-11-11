#define GEARS_QUERY_SESSION_TEST_MAIN PriorOwnerBracketMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
#include <functional>
static std::function<void()> read_change,recover_owner;
static bool retry_rejected{};
static uintptr_t read_trigger{};static bool fired{};static unsigned host_actions{};
static void change_on_read(uintptr_t address,size_t){
    if(!fired&&address==read_trigger){fired=true;read_change();}
}
struct BracketEngine:HostEngine {
    template<class Shape,class Read,class Guard>q::Code sweep_all(q::source::Source const&,Shape const&,q::Vector const&,
        q::StreamLimits const&,Read,Guard current,q::AllSweepOwners& out,
        gears_cover_physx_collider::Source const*,gears_cover_physx_components::Source const*){
        // Replaces only original dispatch. The real SourceGuard executes the
        // actual frozen physics/collider readers before this Host action.
        if(read_change)read_hook=change_on_read;
        const bool accepted=current();read_hook=nullptr;
        if(!accepted){if(recover_owner){recover_owner();retry_rejected=!current();}return q::Code::source;}
        ++host_actions;out.raw.raw_count=out.raw.hit_count=1;return q::Code::queried;
    }
};
using BracketSession=service::Session<BracketEngine>;
static q::Vector bracket_movement{1,0,0};static q::StreamLimits bracket_limits{32,8};
static void setup_bracket(){reset();catalog_setup();host_actions=0;read_change={};recover_owner={};retry_rejected=false;read_trigger=fixture_mesh+0x248;fired=false;}
int main(){
    if(PriorOwnerBracketMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    unsigned complete_calls{};
    {
        setup_bracket();auto session=std::make_unique<BracketSession>();C(session->start(true,true));
        auto output=std::make_unique<q::AllSweepOwners>();unsigned calls{};
        auto current=[&](){++calls;return guard();};const auto before=reads;
        C(session->sweep_all(actor,physics_expected,test_box,bracket_movement,bracket_limits,source_read,current,*output)==q::Code::queried&&host_actions==1);
        complete_calls=calls;printf("owner_bracket full_calls=%u reads=%u\n",calls,reads-before);
        session->request_drain();C(session->poll_stop());
    }
    // A rejection at EVERY full-owner boundary must leave caller output
    // unpublished. Boundaries after the Host action still reject its result.
    for(unsigned reject_call=1;reject_call<=complete_calls;++reject_call){
        setup_bracket();auto session=std::make_unique<BracketSession>();C(session->start(true,true));
        auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=77;unsigned calls{};
        auto current=[&](){return ++calls!=reject_call&&guard();};
        C(session->sweep_all(actor,physics_expected,test_box,bracket_movement,bracket_limits,source_read,current,*output)!=q::Code::queried&&output->raw.raw_count==77);
        session->request_drain();C(session->poll_stop());
    }
    for(unsigned bad=0;bad<8;++bad){
        setup_bracket();auto session=std::make_unique<BracketSession>();C(session->start(true,true));
        auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=77;bool source_fresh=true;unsigned token=1;
        const auto expected_actor=actor;
        auto current=[&](){return session->enabled()&&source_fresh&&token==1&&h::same_key(expected_actor,actor)&&
            expected_actor.scene_invocation==actor.scene_invocation&&guard();};
        read_change=[&](){
            if(bad==0)source_fresh=false;
            if(bad==1)++token; // release/repress invalidates this operation
            if(bad==2)put(fixture_pc+0x100,fixture_weapon);
            if(bad==3)session->request_drain();
            if(bad==4)++committed_world_generation;
            if(bad==5)++actor.scene_invocation;
            if(bad==6)h::game_thread=GetCurrentThreadId()+1;
            if(bad==7)put(physics_instance+0x7c,0);
        };
        C(session->sweep_all(actor,physics_expected,test_box,bracket_movement,bracket_limits,source_read,current,*output)!=q::Code::queried);
        C(fired&&host_actions==0&&output->raw.raw_count==77);
        read_change={};read_hook=nullptr;session->request_drain();C(session->poll_stop());
    }
    // An entered read failure consumes its recheck readiness. A recovered
    // owner cannot reuse that operation without a new initial sample.
    {
        setup_bracket();auto session=std::make_unique<BracketSession>();C(session->start(true,true));
        auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=77;bool source_fresh=true;
        auto current=[&](){return source_fresh&&guard();};
        read_change=[&](){source_fresh=false;};recover_owner=[&](){source_fresh=true;};
        C(session->sweep_all(actor,physics_expected,test_box,bracket_movement,bracket_limits,source_read,current,*output)!=q::Code::queried);
        C(fired&&retry_rejected&&host_actions==0&&output->raw.raw_count==77);
        read_change={};recover_owner={};session->request_drain();C(session->poll_stop());
    }
    // Exercise the initial read group as well, before the Host is entered.
    for(unsigned bad=0;bad<5;++bad){
        setup_bracket();auto session=std::make_unique<BracketSession>();C(session->start(true,true));
        auto output=std::make_unique<q::AllSweepOwners>();output->raw.raw_count=77;bool source_fresh=true;
        auto current=[&](){return session->enabled()&&source_fresh&&guard();};
        read_change=[&](){
            if(bad==0)source_fresh=false;
            if(bad==1)session->request_drain();
            if(bad==2)put(fixture_pc+0x100,fixture_weapon);
            if(bad==3)++committed_world_generation;
            if(bad==4)put(physics_instance+0x7c,99);
        };
        read_hook=change_on_read;
        C(session->sweep_all(actor,physics_expected,test_box,bracket_movement,bracket_limits,source_read,current,*output)!=q::Code::queried);
        C(fired&&host_actions==0&&output->raw.raw_count==77);
        read_change={};read_hook=nullptr;session->request_drain();C(session->poll_stop());
    }
    C(native_calls==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverOwnerBracket checks=%u failures=%u actual typed readers; Host dispatch only\n",checks,failures);return failures?1:0;
}
