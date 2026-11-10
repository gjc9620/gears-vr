#define main hammerburst_restore_frame_fixture
#include "weapon_rig_binding_test.cpp"
#undef main
#include "../vr-common/weapon_reload_transaction.h"
#include "../vr-common/weapon_hammerburst_latch_curve.h"
namespace inv {
struct Identity {
    std::uint64_t weapon{},world{},game_start{},lifecycle{};
    bool valid()const noexcept{return weapon&&world&&game_start&&lifecycle;}
    bool operator==(Identity const& b)const noexcept{return weapon==b.weapon&&world==b.world&&game_start==b.game_start&&lifecycle==b.lifecycle;}
};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;
using Transaction=gears_weapon::ReloadTransaction<Identity>;
Identity identity(wh::Frame const& f)noexcept{return {f.weapon.address,f.world.address,f.key.game_start,f.key.lifecycle};}
bool counts_equal(Inventory const& a,Inventory const& b)noexcept{return a.identity==b.identity&&a.capacity==b.capacity&&a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
}
namespace weaponhand {std::int64_t clock_value=now;std::int64_t clock_ns(void*)noexcept{return clock_value;}}
namespace nativeequipment {bool current_value=true;bool current(void*,wh::Frame const&)noexcept{return current_value;}}
namespace hb_restore_fixture {
namespace hand=gears_weapon_hand;
enum class Stage{parked,restoring,closing};Stage stage=Stage::restoring;
namespace park_control {bool enabled_value=true;bool enabled()noexcept{return enabled_value;}}
struct ParkedPlan{inv::Transaction transaction;bool valid{};};ParkedPlan parked_plan;
inv::Transaction transaction;inv::Inventory opening;
double angle=.5;std::uint64_t current_token=11;
bool geometry_value=true,eligible_value=true;
bool geometry_current(hand::Frame const&)noexcept{return geometry_value;}
bool eligible(hand::Frame const&)noexcept{return eligible_value;}
std::uint64_t animation_serial()noexcept{return current_token;}
namespace anim {
struct Latch{gears_weapon::HammerburstLatchCurve curve;gears_weapon::HammerburstLatchCurve::Sampling sampling{};};
struct Ownership{bool active{};std::uint64_t serial{};inv::Identity inventory{};Latch latch;};Ownership ownership;
}
#include "weapon_hammerburst_restore_admission.inl"
void setup(wh::Frame const& f){
    stage=Stage::restoring;park_control::enabled_value=true;geometry_value=eligible_value=true;
    nativeequipment::current_value=true;weaponhand::clock_value=now;angle=.5;current_token=11;
    opening={inv::identity(f),1,78,70,3};transaction={};CHECK(transaction.bind(opening));
    CHECK(transaction.begin(opening,1,gears_weapon::ReloadIntentSource::mechanism_open,1,gears_weapon::ReloadTxnMode::magazine,78,0,1));
    parked_plan={transaction,true};restoration_admission::cancel();
    anim::ownership={};anim::ownership.active=true;anim::ownership.serial=11;anim::ownership.inventory=opening.identity;
    const double keys[]={0,.5,1.,0};CHECK(anim::ownership.latch.curve.bind(keys,4,2.,true));
}
}
int main(){
    using namespace hb_restore_fixture;unsigned cases{};
    for(unsigned source=0;source<2;++source){
        Fixture f(source);setup(f.hand);
        CHECK(!restoration_owned(f.hand));
        {
            restoration_admission::Scope scope(f.hand);CHECK(bool(scope));CHECK(restoration_owned(f.hand));
            restoration_admission::Scope nested(f.hand);CHECK(!bool(nested)&&bool(scope));
            float expected{};CHECK(anim::ownership.latch.curve.time_for_angle(angle,expected));
            CHECK(restoration_seek_owned(f.hand,11,expected));
            CHECK(!restoration_seek_owned(f.hand,12,expected));
            CHECK(!restoration_seek_owned(f.hand,11,expected+.01f));
            auto different=f.hand;++different.key.frame;CHECK(!restoration_owned(different));
            weaponhand::clock_value=now+101000000;CHECK(!restoration_owned(f.hand));
            weaponhand::clock_value=now;CHECK(!restoration_owned(f.hand)&&!bool(scope));
        }
        CHECK(!restoration_owned(f.hand));++cases;
    }
    for(unsigned failure=0;failure<14;++failure){
        Fixture f;setup(f.hand);
        if(failure==0)park_control::enabled_value=false;
        if(failure==1)stage=Stage::parked;
        if(failure==2)parked_plan.valid=false;
        if(failure==3)geometry_value=false;
        if(failure==4)eligible_value=false;
        if(failure==5)nativeequipment::current_value=false;
        if(failure==6)++opening.identity.world;
        if(failure==7)weaponhand::clock_value=now+101000000;
        if(failure==8)current_token=0;
        if(failure==9)f.hand.controllers.sync_ns=now-100000001;
        if(failure==10)f.hand.now_ns=now-100000001;
        if(failure==11)f.hand.stereo.focused=0;
        if(failure==12)f.hand.controllers.hands[1].grip_active=0;
        if(failure==13)f.hand.controllers.hands[1].grip_flags=0;
        restoration_admission::Scope scope(f.hand);CHECK(!bool(scope)&&!restoration_owned(f.hand));++cases;
    }
    {
        Fixture f;setup(f.hand);CHECK(transaction.stow_original(1));CHECK(transaction.reserve_magazine_reusing_loaded(2));
        restoration_admission::Scope scope(f.hand);CHECK(!bool(scope));++cases;
    }
    {
        Fixture f;setup(f.hand);inv::Transaction idle;CHECK(idle.bind(opening));transaction=idle;
        restoration_admission::Scope scope(f.hand);CHECK(!bool(scope));++cases;
    }
    {
        Fixture f;setup(f.hand);opening.loaded=78;opening.reserve=0;transaction={};CHECK(transaction.bind(opening));
        CHECK(transaction.begin(opening,1,gears_weapon::ReloadIntentSource::mechanism_open,0,gears_weapon::ReloadTxnMode::magazine,78,0,1));
        CHECK(transaction.stow_original(1));CHECK(transaction.reserve_magazine_reusing_loaded(2));
        CHECK(transaction.reserved()==0);parked_plan.transaction=transaction;
        restoration_admission::Scope scope(f.hand);CHECK(!bool(scope));++cases;
    }
    {
        Fixture f;setup(f.hand);restoration_admission::Scope scope(f.hand);CHECK(bool(scope));
        restoration_admission::cancel();CHECK(!restoration_owned(f.hand)&&!bool(scope));++cases;
    }
    for(unsigned changed=0;changed<3;++changed){
        Fixture f;setup(f.hand);restoration_admission::Scope scope(f.hand);CHECK(bool(scope));
        if(changed==0)angle=.6;if(changed==1)nativeequipment::current_value=false;if(changed==2)geometry_value=false;
        CHECK(!restoration_owned(f.hand));angle=.5;nativeequipment::current_value=geometry_value=true;
        CHECK(!restoration_owned(f.hand)&&!bool(scope));++cases;
    }
    {
        Fixture f;setup(f.hand);f.hand.left_grip_down=f.hand.left_tracked=f.hand.left_grip_known=0;
        CHECK(wh::frame_active(f.hand,now));restoration_admission::Scope scope(f.hand);CHECK(bool(scope));
        CHECK(transaction.inventory().loaded==70&&transaction.inventory().reserve==3&&!transaction.pending());++cases;
    }
    {
        Fixture f;setup(f.hand);weaponhand::clock_value=now+96000000;
        restoration_admission::Scope scope(f.hand);CHECK(bool(scope));
        ++weaponhand::clock_value;CHECK(wh::frame_active(f.hand,weaponhand::clock_value));
        CHECK(!restoration_owned(f.hand)&&!bool(scope));++cases;
    }
    std::printf("HammerburstRestoreAdmissionTest: %u cases, %u checks; exact cosmetic scope, real Frame freshness and curve\n",cases,checks);return 0;
}
