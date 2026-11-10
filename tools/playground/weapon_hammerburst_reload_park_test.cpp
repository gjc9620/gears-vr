// Actual production parking control + real transaction, owned service stubs.
#if !defined(HB_PARK_TEST_DISABLED)
#define GEARS_PLAYGROUND_HB_PARK_RESTORE 1
#endif
#define main hammerburst_park_ledger_existing_tests
#include "weapon_hammerburst_reload_park_ledger_test.cpp"
#undef main
#include <atomic>
namespace hand {
struct Frame {unsigned weapon{7};inv::Identity identity{};bool current{true};};
bool same(unsigned a,unsigned b)noexcept{return a==b;}
}
namespace inv {
Identity identity(hand::Frame const& source)noexcept{return source.identity;}
struct ObservedInventory {Inventory value{};bool available{true};
    bool sample_owned(Identity const& expected,Inventory& result)noexcept{
        if(!available||!(value.identity==expected))return false;result=value;return true;
    }};
}
namespace nativeequipment {bool current(void*,hand::Frame const& f)noexcept{return f.current;}}
namespace lancerreload_state {
struct Lease {inv::Identity identity{};std::uint64_t serial{1};bool acquired{};long long accepted_begin_ms{123},perfect_end_ms{4000};};
Lease lease;unsigned exits{};
bool retains_serial(std::uint64_t value)noexcept{return lease.acquired&&lease.serial==value;}
bool recover_serial(std::uint64_t value)noexcept{
    if(retains_serial(value)){++exits;lease.acquired=false;}return true;
}
bool recover_owned()noexcept{return recover_serial(lease.serial);}
}
namespace lancerreload_runtime {std::atomic<bool> outstanding{true};}
namespace anim {
struct Ownership {inv::Identity inventory{};bool active{true},seek_pending{},visual_only{};std::uint64_t serial{};};Ownership ownership;
bool seek_ok{true};bool reconcile_seek()noexcept{if(!seek_ok)return false;ownership.seek_pending=false;return true;}
}
namespace visual {
struct Saved {unsigned weapon{7};};Saved saved;bool active{true},restore_ok{true};
bool restore()noexcept{if(!restore_ok)return false;active=false;return true;}
}
namespace hb_fixture {
Stage stage{};
inv::Inventory opening{};
inv::Transaction transaction;
inv::ObservedInventory observer;
park_ledger::Plan parked_plan;
lancerreload_state::Lease parked_native_lease;
std::uint64_t serial{1},old_item{1},new_item{},feed_item{},feed_sequence{},held{9};
std::int64_t feed_ns{};
bool released{},switch_requested{},native_exited{};
bool restore_native_uncertain{};
struct Stroke {void reset()noexcept{}};Stroke stroke,insertion;
struct Extraction {bool done{};bool complete()const noexcept{return done;}void reset()noexcept{done=false;}} extraction;
bool animation_restore_ok{true};double angle{.6};
bool restore_animation()noexcept{if(!animation_restore_ok)return false;anim::ownership.active=false;return true;}
#include "weapon_hammerburst_reload_park.inl"

void setup(Fixture const& f,Stage phase){
    stage=phase;opening=f.initial;transaction=f.transaction;observer={transaction.inventory(),true};
    parked_plan={};parked_native_lease={};serial=old_item=1;new_item=phase==Stage::new_magazine?2:0;
    held=9;released=switch_requested=native_exited=false;extraction.done=false;angle=.6;
    lancerreload_state::lease={opening.identity,1,true,123,4000};lancerreload_state::exits=0;
    anim::ownership={opening.identity,true,false};anim::seek_ok=true;
    visual::saved={7};visual::active=visual::restore_ok=animation_restore_ok=true;
    lancerreload_runtime::outstanding=true;
}
}
int main(){
    using namespace hb_fixture;
#if defined(HB_PARK_TEST_DISABLED)
    checks=0;Fixture f;f.insert();setup(f,Stage::closing);bool changed=true;
    C(!park_control::enabled());C(!park_control::owns_stow(hand::Frame{}));
    C(!park_control::begin());C(park_control::prepare_stow(hand::Frame{},changed)&&!changed);
    C(stage==Stage::closing&&lancerreload_state::lease.acquired&&lancerreload_state::exits==0);
    C(transaction.inventory().loaded==73&&transaction.inventory().reserve==0&&!parked_plan.valid);
    std::printf("HammerburstParkDisabledTest: %u checks; default does not enter incomplete parking\n",checks);return 0;
#else
    checks=cases=0;
    {
        Fixture f;f.insert();setup(f,Stage::closing);C(park_control::begin());
        C(stage==Stage::parked&&parked_plan.valid&&!park_control::pending_resources());
        C(transaction.inventory().loaded==73&&transaction.inventory().reserve==0&&transaction.inserted()==3);
        C(!lancerreload_state::lease.acquired&&!anim::ownership.active&&!visual::active&&!held);
        C(parked_native_lease.accepted_begin_ms==123&&parked_native_lease.perfect_end_ms==4000&&angle==.6);
        C(lancerreload_state::exits==1&&!lancerreload_runtime::outstanding);C(park_control::begin()&&lancerreload_state::exits==1);++cases;
    }
    {
        Fixture f;f.reserve();setup(f,Stage::new_magazine);visual::restore_ok=false;
        C(!park_control::begin()&&stage==Stage::parking&&parked_plan.valid);
        C(transaction.reserved()==3&&observer.value.loaded==70&&observer.value.reserve==3);
        visual::restore_ok=true;C(park_control::finish());
        C(stage==Stage::parked&&!transaction.reserved()&&transaction.inventory().reserve==3);
        C(parked_plan.resume_stage==Stage::waiting_ammo&&lancerreload_state::exits==1);++cases;
    }
    for(bool extracted:{false,true}){
        Fixture f;setup(f,Stage::old_magazine);extraction.done=extracted;C(park_control::begin());
        C(parked_plan.original_at_feed==!extracted);
        C(parked_plan.resume_stage==(extracted?Stage::waiting_ammo:Stage::opened));
        C(transaction.inventory().loaded==70&&transaction.inventory().reserve==3);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);++lancerreload_state::lease.identity.world;
        C(!park_control::begin()&&stage==Stage::opening&&lancerreload_state::exits==0);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);++anim::ownership.inventory.world;
        C(!park_control::begin()&&stage==Stage::opening&&lancerreload_state::exits==0);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);visual::saved.weapon=8;
        C(!park_control::begin()&&stage==Stage::parking&&lancerreload_state::exits==0&&visual::active);++cases;
    }
    {
        Fixture f;f.reserve();C(f.transaction.request_insertion(2));setup(f,Stage::new_magazine);
        C(!park_control::begin()&&stage==Stage::new_magazine&&lancerreload_state::exits==0);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);hand::Frame source{};bool changed=true;source.current=false;
        C(!park_control::prepare_stow(source,changed)&&!changed&&stage==Stage::opening);
        source.current=true;C(park_control::prepare_stow(source,changed)&&changed&&stage==Stage::parked);
        C(park_control::prepare_stow(source,changed)&&!changed&&lancerreload_state::exits==1);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);observer.value.loaded=71;
        C(!park_control::begin()&&stage==Stage::parking);C(transaction.inventory().loaded==70);
        C(lancerreload_runtime::outstanding);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);visual::restore_ok=false;
        C(!park_control::begin()&&stage==Stage::parking&&lancerreload_state::exits==1);
        lancerreload_state::lease.acquired=true;++lancerreload_state::lease.identity.world;
        visual::restore_ok=true;C(!park_control::finish());
        C(lancerreload_state::lease.acquired&&lancerreload_state::exits==1&&stage==Stage::parking);++cases;
    }
    for(unsigned fault=0;fault<3;++fault){
        Fixture f;setup(f,Stage::opening);C(park_control::begin());
        const auto saved_serial=parked_native_lease.serial;const auto saved_begin=parked_native_lease.accepted_begin_ms;
        stage=Stage::restoring;anim::ownership={opening.identity,true,false};
        if(fault==1)++anim::ownership.inventory.world;
        if(fault==2){lancerreload_state::lease.acquired=true;++lancerreload_state::lease.identity.world;}
        C(park_control::begin()==(fault==0));
        C(parked_native_lease.serial==saved_serial&&parked_native_lease.accepted_begin_ms==saved_begin);
        C(transaction.inventory().loaded==70&&transaction.inventory().reserve==3);
        if(!fault)C(stage==Stage::parked&&!anim::ownership.active&&lancerreload_state::exits==1);
        else C(stage==Stage::restoring&&anim::ownership.active&&lancerreload_state::exits==1);
        ++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);C(park_control::begin());
        stage=Stage::restoring;lancerreload_state::lease={opening.identity,1,true,123,4000};
        anim::ownership={opening.identity,true,false};C(park_control::begin());
        C(stage==Stage::parked&&parked_plan.valid&&!lancerreload_state::lease.acquired);
        C(parked_native_lease.serial==1&&parked_native_lease.accepted_begin_ms==123&&lancerreload_state::exits==2);
        C(transaction.inventory().loaded==70&&transaction.inventory().reserve==3);++cases;
    }
    {
        Fixture f;f.insert();setup(f,Stage::closing);C(park_control::begin());
        C(transaction.resume(transaction.inventory(),2,1));stage=Stage::closing;
        anim::ownership={opening.identity,true,false,true,1};C(park_control::begin());
        C(stage==Stage::parked&&transaction.inventory().loaded==73&&transaction.inventory().reserve==0);
        C(parked_native_lease.serial==1&&parked_native_lease.accepted_begin_ms==123&&lancerreload_state::exits==1);++cases;
    }
    {
        Fixture f;setup(f,Stage::opening);restore_native_uncertain=true;
        C(!park_control::begin()&&stage==Stage::opening&&lancerreload_state::exits==0);
        C(transaction.inventory().loaded==70&&transaction.inventory().reserve==3);restore_native_uncertain=false;++cases;
    }
    std::printf("HammerburstParkControlTest: %u cases, %u checks passed; actual parking adapter and real ledger\n",cases,checks);return 0;
#endif
}
