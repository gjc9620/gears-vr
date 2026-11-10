// Production rebind control and real transaction. Engine reads are owned stubs;
// real Frame freshness is covered separately by RestoreAdmissionTest.
#define main hammerburst_existing_park_ledger_tests
#include "weapon_hammerburst_reload_park_ledger_test.cpp"
#undef main
namespace hand {
enum class LeaseKind{none,magazine,ammo};
struct Lease{std::uint64_t id{};LeaseKind kind{};
    Lease()=default;Lease(unsigned value):id(value){};
    bool operator!()const noexcept{return !id;}bool operator==(unsigned value)const noexcept{return id==value;}};
struct Frame {inv::Identity identity{};std::uint64_t press_token{};bool left_tracked{true},left_grip_known{true},left_grip_down{};};
struct Decision{Lease lease{};bool fresh{true},granted{true};};
bool same(unsigned a,unsigned b)noexcept{return a==b;}
bool same(Lease const& a,Lease const& b)noexcept{return a.id==b.id&&a.kind==b.kind;}
}
namespace inv {
Identity identity(hand::Frame const& f)noexcept{return f.identity;}
struct ObservedInventory {
    Inventory value{};bool available{true};unsigned rebinds{};
    bool rebind_hold(hand::Frame const& f,Identity const& expected,Inventory& out)noexcept{
        if(!available||!(value.identity==expected))return false;
        ++rebinds;value.identity=f.identity;out=value;return true;
    }
    bool sample(hand::Frame const& f,Inventory& out)noexcept{if(!available||!(value.identity==f.identity))return false;out=value;return true;}
};
}
namespace nativeequipment {bool current_value{true};bool current(void*,hand::Frame const&)noexcept{return current_value;}}
namespace lancerreload_state {struct Lease{inv::Identity identity{};unsigned serial{1};bool acquired{};
    std::int64_t accepted_begin_ms{123},perfect_end_ms{4000};float reward_start{10};};Lease lease;}
namespace anim {enum class Acquire{rejected,acquired,recovery_required};
struct Ownership{bool active{},visual_only{},stopped{},seek_pending{};unsigned serial{};inv::Identity inventory{};};Ownership ownership;}
namespace visual {bool active{};struct Saved{unsigned weapon{7};};Saved saved;}
namespace nativelancerfollow {struct Key{unsigned weapon{7},component{31},asset{37};};Key bound_key;}
namespace hb_rebind_fixture {
namespace park_control {bool enabled_value{true};bool enabled()noexcept{return enabled_value;}}
Stage stage{};park_ledger::Plan parked_plan;inv::Transaction transaction;
inv::Inventory opening;inv::ObservedInventory observer;
lancerreload_state::Lease parked_native_lease;
struct Geometry {nativelancerfollow::Key key{};struct Latch{double open_angle{1.2};}latch;bool known{true};};Geometry geometry;
struct Stroke {unsigned resets{};void reset()noexcept{++resets;}}stroke,insertion,extraction;
unsigned old_item{1},serial{1};hand::Lease held{99};bool released{true},restore_display_ready{true};
unsigned restore_display_step{},restore_resume_step{};bool restoration_failed{},resume_grant_ready{};
bool restore_native_uncertain{};
bool fresh_value{true},eligible_value{true},discover_value{true},final_geometry{true},expire_in_discover{},replace_in_discover{};
unsigned discovers{};
bool fresh_restore_main(hand::Frame const&)noexcept{return fresh_value;}
bool eligible(hand::Frame const&)noexcept{return eligible_value;}
bool geometry_current(hand::Frame const&)noexcept{return final_geometry;}
bool discover(hand::Frame const&)noexcept{++discovers;if(replace_in_discover)++nativelancerfollow::bound_key.component;
    geometry.key=nativelancerfollow::bound_key;geometry.latch.open_angle=0;
    if(expire_in_discover)fresh_value=false;return discover_value;}
void log_restore(char const*,hand::Frame const* =nullptr) noexcept {}
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
    restore_display_step=0;restoration_failed=false;
    auto next=opening.identity;++next.hold;return {next};
}
}
