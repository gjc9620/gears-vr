#include "../vr-common/weapon_hand_participant.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#ifdef NDEBUG
#error GnasherAnimationAcquireTest requires active checks
#endif
#define CHECK(v) do{if(!(v)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#v);std::abort();}}while(false)
namespace hand=gears_weapon_hand;
struct Name {unsigned index{},number{};};
namespace inv {
struct Identity {hand::ObjectIdentity weapon{};std::uint64_t world{};
bool operator==(Identity const& b)const noexcept{return hand::same(weapon,b.weapon)&&world==b.world;}};
Identity identity(hand::Frame const& f)noexcept{return {f.weapon,f.key.world_generation};}
}
hand::ObjectIdentity object(std::uintptr_t p){return {p,0x8000,0x9000,unsigned(p/8),7,0,1};}
namespace anim {
struct Observation {hand::ObjectIdentity weapon{},component{},asset{},node{},sequence{};
Name sequence_name{};float time{},rate{1},length{1};bool playing{true},looping{};};
struct Controls {hand::ObjectIdentity node{},stop{},seek{};};
bool controls_ok=true;
Controls controls{};
bool prepare_controls(hand::Frame const&,Observation const&,Controls& out,bool)noexcept{
    out=controls;return controls_ok;
}
}
namespace gnasher_animation {
#include "weapon_gnasher_animation_acquire.inl"
bool support=true,prepared_ok=true,select_result=true,select_applies=true,stop_result=true,stop_applies=true,context_alive=true;
bool select_enters=true,stop_enters=true;
bool original_shot=true;
unsigned selections{},stops{},support_calls{},expire_at{};
anim::Observation actual{};Selection preparation{};std::function<void()> on_select,on_stop;
bool support_granted(hand::Frame const&)noexcept{return ++support_calls!=expire_at&&support;}
bool shot_admitted(hand::Frame const&)noexcept{return original_shot&&context_alive;}
bool observe(hand::Frame const&,anim::Observation& out,bool chamber_only)noexcept{
    out=actual;return !chamber_only||hand::same(actual.sequence,preparation.target);
}
bool current(hand::Frame const& f,anim::Observation& out)noexcept{
    out=actual;return owner.active&&!owner.call_in_flight&&!owner.selection_pending&&owner.stop_dispatched&&
        owner.identity==inv::identity(f)&&!actual.playing&&hand::same(actual.sequence,owner.last.sequence)&&
        hand::same(actual.node,owner.last.node)&&hand::same(actual.component,owner.last.component)&&hand::same(actual.asset,owner.last.asset);
}
bool prepare_selection(hand::Frame const&,Selection& out)noexcept{out=preparation;return prepared_ok;}
bool dispatch_selection(hand::Frame const&,Selection const& prepared,bool& dispatched)noexcept{
    CHECK(owner.active&&owner.call_in_flight&&owner.selection_pending&&!owner.stop_dispatched&&last_serial==owner.serial);
    if(!select_enters)return false;dispatched=true;
    ++selections;if(on_select)on_select();
    if(select_applies){actual.sequence=prepared.target;actual.sequence_name=prepared.name;}
    return select_result;
}
bool observe_owned_target(anim::Observation& out)noexcept{
    out=actual;return context_alive&&owner.active&&hand::same(actual.weapon,owner.identity.weapon)&&
        hand::same(actual.node,owner.last.node)&&hand::same(actual.component,owner.last.component)&&hand::same(actual.asset,owner.last.asset)&&
        hand::same(actual.sequence,owner.requested_sequence)&&actual.sequence_name.index==owner.requested_name.index&&actual.sequence_name.number==owner.requested_name.number&&
        hand::same(anim::controls.stop,owner.controls.stop)&&hand::same(anim::controls.seek,owner.controls.seek);
}
bool dispatch_owned_stop(anim::Observation const&,anim::Controls const&,bool& dispatched)noexcept{
    CHECK(owner.active&&owner.call_in_flight&&!owner.selection_pending&&!owner.stop_dispatched);
    if(!stop_enters)return false;dispatched=true;++stops;
    if(on_stop)on_stop();if(stop_applies)actual.playing=false;return stop_result;
}
}
struct Fixture {
    hand::Frame f{};
    Fixture(){using namespace gnasher_animation;
        owner={};last_serial=0;support=prepared_ok=select_result=select_applies=stop_result=stop_applies=context_alive=true;
        select_enters=stop_enters=true;
        original_shot=true;
        selections=stops=support_calls=expire_at=0;on_select={};on_stop={};anim::controls_ok=true;
        f.weapon=object(0x1000);f.controller=object(0x1100);f.key.world_generation=1;f.controllers.profile_generation=2;f.reference_generation=3;
        actual={};actual.weapon=f.weapon;actual.component=object(0x2000);actual.asset=object(0x3000);
        actual.node=object(0x4000);actual.sequence=object(0x5000);actual.sequence_name={10,0};
        anim::controls={actual.node,object(0x6000),object(0x7000)};
        preparation={actual,anim::controls,object(0x5100),object(0x8000),{11,0},true};
    }
    bool begin(std::uint64_t serial=9){return gnasher_animation::begin(f,serial);}
};
int main(){using namespace gnasher_animation;
    {Fixture x;on_select=[&]{CHECK(!x.begin(10)&&!service_owned());};on_stop=[&]{CHECK(!x.begin(10)&&!service_owned());};
     CHECK(x.begin());CHECK(selections==1&&stops==1&&owner.active&&!owner.selection_pending);
     CHECK(x.begin()&&selections==1&&stops==1);CHECK(!x.begin(10));}
    {Fixture x;preparation.target=actual.sequence;preparation.name=actual.sequence_name;preparation.needed=false;
     CHECK(x.begin()&&!selections&&stops==1);}
    {Fixture x;actual.looping=true;preparation.before=actual;
     CHECK(x.begin()&&actual.looping&&!actual.playing&&selections==1&&stops==1);}
    for(bool applies:{false,true}){Fixture x;select_result=false;select_applies=applies;
     CHECK(x.begin()==applies);CHECK(owner.active&&owner.selection_pending==!applies&&selections==1&&stops==unsigned(applies));
     select_result=true;CHECK(x.begin()==applies);CHECK(selections==1&&stops==unsigned(applies));
     if(!applies)CHECK(!x.begin(10)&&owner.serial==9);}
    for(bool applies:{false,true}){Fixture x;stop_result=false;stop_applies=applies;
     CHECK(x.begin()==applies);CHECK(owner.active&&!owner.selection_pending&&owner.stop_dispatched);
     CHECK(selections==1&&stops==1);stop_result=true;
     CHECK(x.begin()==applies);CHECK(stops==1&&selections==1);}
    for(unsigned bad=0;bad<8;++bad){Fixture x;select_applies=false;CHECK(!x.begin());
     actual.sequence=preparation.target;actual.sequence_name=preparation.name;
     if(bad==0)++actual.node.registry_index;if(bad==1)++actual.component.registry_index;if(bad==2)++actual.asset.registry_index;
     if(bad==3)++actual.sequence_name.index;if(bad==4)++anim::controls.stop.registry_index;if(bad==5)++anim::controls.seek.registry_index;
     if(bad==6)++actual.sequence.registry_index;if(bad==7)++actual.sequence_name.number;
     CHECK(!x.begin()&&owner.selection_pending&&!stops&&selections==1);}
    for(unsigned bad=0;bad<5;++bad){Fixture x;select_applies=false;CHECK(!x.begin());
     actual.sequence=preparation.target;actual.sequence_name=preparation.name;
     if(bad==0)++x.f.weapon.registry_index;if(bad==1)++x.f.key.world_generation;
     if(bad==2)++x.f.controllers.profile_generation;if(bad==3)++x.f.reference_generation;
     if(bad==4)++x.f.controller.registry_index;
     CHECK(!x.begin()&&selections==1&&!stops);}
    for(unsigned denied=0;denied<6;++denied){Fixture x;
     if(denied==0)support=false;if(denied==1)prepared_ok=false;if(denied==2)expire_at=2;
     if(denied==3)last_serial=9;
     if(denied==5)x.f.controller={};
     CHECK(!x.begin(denied==4?0:9)&&!owner.active&&!selections&&!stops);}
    {Fixture x;select_applies=false;CHECK(!x.begin());support=false;
     actual.sequence=preparation.target;actual.sequence_name=preparation.name;
     CHECK(!x.begin()&&selections==1&&!stops);support=true;CHECK(x.begin()&&selections==1&&stops==1);}
    {Fixture x;CHECK(x.begin());owner={};CHECK(!x.begin()&&!owner.active&&selections==1&&stops==1);}
    {Fixture x;on_select=[&]{support=false;};CHECK(!x.begin());
     CHECK(!actual.playing&&owner.active&&!owner.selection_pending&&stops==1&&selections==1);
     CHECK(service_owned()&&!x.begin()&&stops==1);support=true;CHECK(x.begin()&&stops==1&&selections==1);}
    {Fixture x;on_select=[&]{context_alive=false;};CHECK(!x.begin());CHECK(owner.active&&owner.selection_pending&&!stops);
     support=false;context_alive=true;CHECK(service_owned()&&stops==1&&!actual.playing);CHECK(!x.begin()&&selections==1);}
    {Fixture x;stop_applies=false;CHECK(!x.begin());support=false;
     for(unsigned i=0;i<3;++i)CHECK(!service_owned());CHECK(stops==1&&owner.active);
     actual.playing=false;CHECK(service_owned()&&stops==1);CHECK(!x.begin());}
    {Fixture x;select_enters=false;CHECK(!x.begin()&&!owner.active&&!selections&&!stops&&last_serial==9);
     select_enters=true;CHECK(!x.begin());CHECK(x.begin(10)&&selections==1&&stops==1);}
    {Fixture x;stop_enters=false;CHECK(!x.begin()&&owner.active&&!owner.stop_dispatched&&selections==1&&!stops);
     support=false;CHECK(!service_owned()&&!stops);stop_enters=true;
     CHECK(service_owned()&&stops==1&&!actual.playing);CHECK(!x.begin());}
    for(unsigned field=0;field<3;++field){Fixture x;CHECK(x.begin()&&owner.stopped_observed);
     const auto baseline=owner.last;
     if(field==0)++actual.rate;if(field==1)++actual.length;if(field==2)actual.looping=!actual.looping;
     CHECK(!service_owned()&&!x.begin()&&stops==1);
     CHECK(owner.last.rate==baseline.rate&&owner.last.length==baseline.length&&owner.last.looping==baseline.looping);}
    {Fixture x;support=false;actual.sequence=preparation.target;actual.sequence_name=preparation.name;
     preparation.before=actual;preparation.needed=false;
     CHECK(hold_after_original_shot(x.f,9)&&stops==1&&!selections&&owner.stopped_observed);
     CHECK(!x.begin()&&stops==1);support=true;CHECK(x.begin()&&stops==1&&!selections);}
    {Fixture x;support=false;CHECK(!hold_after_original_shot(x.f,9)&&!owner.active&&!stops&&!selections&&!last_serial);
     actual.sequence=preparation.target;actual.sequence_name=preparation.name;preparation.before=actual;preparation.needed=false;
     original_shot=false;CHECK(!hold_after_original_shot(x.f,9)&&!owner.active&&!stops);}
    std::puts("Production Gnasher animation acquisition: original selection/stop journal before dispatch, unknown/reentrant no replay, exact target/controls/owner/source, stopped looped clip and same-serial observation-only continuation passed (injected engine/schema boundaries)");
}
