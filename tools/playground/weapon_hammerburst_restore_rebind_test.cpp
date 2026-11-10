// Production rebind control and real transaction. Engine reads are owned stubs;
// real Frame freshness is covered separately by RestoreAdmissionTest.
#define main hammerburst_existing_park_ledger_tests
#include "weapon_hammerburst_reload_park_ledger_test.cpp"
#undef main
namespace hand {
struct Frame {inv::Identity identity{};};
bool same(unsigned a,unsigned b)noexcept{return a==b;}
}
namespace inv {
Identity identity(hand::Frame const& f)noexcept{return f.identity;}
struct ObservedInventory {
    Inventory value{};bool available{true};unsigned rebinds{};
    bool rebind_hold(hand::Frame const& f,Identity const& expected,Inventory& out)noexcept{
        if(!available||!(value.identity==expected))return false;
        ++rebinds;value.identity=f.identity;out=value;return true;
    }
};
}
namespace nativeequipment {bool current_value{true};bool current(void*,hand::Frame const&)noexcept{return current_value;}}
namespace lancerreload_state {struct Lease{inv::Identity identity{};unsigned serial{1};bool acquired{};};Lease lease;}
namespace anim {struct Ownership{bool active{};};Ownership ownership;}
namespace visual {bool active{};}
namespace nativelancerfollow {struct Key{unsigned weapon{7},component{31},asset{37};};Key bound_key;}
namespace hb_rebind_fixture {
namespace park_control {bool enabled_value{true};bool enabled()noexcept{return enabled_value;}}
Stage stage{};park_ledger::Plan parked_plan;inv::Transaction transaction;
inv::Inventory opening;inv::ObservedInventory observer;
lancerreload_state::Lease parked_native_lease;
struct Geometry {nativelancerfollow::Key key{};struct Latch{double open_angle{1.2};}latch;bool known{true};};Geometry geometry;
struct Stroke {unsigned resets{};void reset()noexcept{++resets;}}stroke,insertion,extraction;
unsigned old_item{1},serial{1},held{99};bool released{true},restore_display_ready{true};
bool fresh_value{true},eligible_value{true},discover_value{true},final_geometry{true},expire_in_discover{},replace_in_discover{};
unsigned discovers{};
bool fresh_restore_main(hand::Frame const&)noexcept{return fresh_value;}
bool eligible(hand::Frame const&)noexcept{return eligible_value;}
bool geometry_current(hand::Frame const&)noexcept{return final_geometry;}
bool discover(hand::Frame const&)noexcept{++discovers;if(replace_in_discover)++nativelancerfollow::bound_key.component;
    geometry.key=nativelancerfollow::bound_key;geometry.latch.open_angle=0;
    if(expire_in_discover)fresh_value=false;return discover_value;}
#include "weapon_hammerburst_restore_rebind.inl"
hand::Frame setup(bool inserted=false){
    Fixture f;if(inserted)f.insert();
    stage=Stage::parked;C(park_ledger::prepare(inserted?Stage::closing:Stage::opening,f.transaction,1,0,false,parked_plan));
    transaction=parked_plan.transaction;opening=f.initial;observer={transaction.inventory(),true,0};
    geometry={};nativelancerfollow::bound_key={};
    parked_native_lease={opening.identity,1,false};lancerreload_state::lease={};
    anim::ownership={};visual::active=false;park_control::enabled_value=true;
    fresh_value=eligible_value=discover_value=final_geometry=nativeequipment::current_value=true;expire_in_discover=replace_in_discover=false;discovers=0;
    stroke=insertion=extraction={};held=99;released=restore_display_ready=true;
    auto next=opening.identity;++next.hold;return {next};
}
}
int main(){
    using namespace hb_rebind_fixture;checks=cases=0;
    for(bool inserted:{false,true}){
        auto f=setup(inserted);const auto original_loaded=opening.loaded;const auto saved=transaction.inventory();
        C(restore_control::rebind(f));C(stage==Stage::restoring);C(opening.identity==f.identity);
        C(opening.loaded==original_loaded);C(transaction.inventory().loaded==saved.loaded&&transaction.inventory().reserve==saved.reserve);
        C(parked_plan.transaction.inventory().identity==f.identity&&observer.value.identity==f.identity);
        C(parked_native_lease.identity==f.identity&&parked_native_lease.serial==1&&!parked_native_lease.acquired);
        C(!held&&!released&&!restore_display_ready&&stroke.resets==1&&insertion.resets==1&&extraction.resets==1);
        C(geometry.latch.open_angle==1.2&&discovers==1&&observer.rebinds==1);
        C(!lancerreload_state::lease.acquired&&!anim::ownership.active&&!visual::active);
        C(!restore_control::rebind(f));++cases;
    }
    for(unsigned fault=0;fault<20;++fault){
        auto f=setup();const auto original=opening.identity;const auto loaded=transaction.inventory().loaded;
        switch(fault){
        case 0:park_control::enabled_value=false;break;
        case 1:fresh_value=false;break;
        case 2:eligible_value=false;break;
        case 3:++f.identity.weapon;break;
        case 4:++f.identity.session;break;
        case 5:lancerreload_state::lease.acquired=true;break;
        case 6:anim::ownership.active=true;break;
        case 7:visual::active=true;break;
        case 8:++nativelancerfollow::bound_key.component;break;
        case 9:++nativelancerfollow::bound_key.asset;break;
        case 10:++nativelancerfollow::bound_key.weapon;break;
        case 11:observer.available=false;break;
        case 12:++observer.value.loaded;break;
        case 13:discover_value=false;break;
        case 14:expire_in_discover=true;break;
        case 15:final_geometry=false;break;
        case 16:nativeequipment::current_value=false;break;
        case 17:replace_in_discover=true;break;
        case 18:++parked_native_lease.serial;break;
        case 19:++parked_native_lease.identity.hold;break;
        }
        C(!restore_control::rebind(f));C(stage==Stage::parked&&opening.identity==original);
        C(transaction.inventory().identity==original&&transaction.inventory().loaded==loaded);
        C(observer.rebinds==0&&parked_plan.transaction.inventory().identity==original);
        C(geometry.latch.open_angle==1.2&&held==99&&released&&restore_display_ready);
        C(!stroke.resets&&!insertion.resets&&!extraction.resets);++cases;
    }
    std::printf("HammerburstRestoreRebindTest: %u cases, %u checks; atomic copies, identity/count guards, no engine action\n",cases,checks);return 0;
}
