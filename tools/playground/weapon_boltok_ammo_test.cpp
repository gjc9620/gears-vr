#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../vr-common/weapon_boltok_visual_geometry.h"
#include "../vr-common/weapon_boltok_perfect_window.h"
#include "../vr-common/weapon_reload_transaction.h"
#include "../vr-common/weapon_hand_participant.h"
#include <cassert>
#include <cstdio>
namespace hand=gears_weapon_hand;
namespace inv {
struct Identity {unsigned value{1};bool valid() const noexcept{return value!=0;}
    bool operator==(Identity const& b)const noexcept{return value==b.value;}};
using Inventory=gears_weapon::ReloadTxnInventory<Identity>;
using Transaction=gears_weapon::ReloadTransaction<Identity>;
using Request=gears_weapon::ReloadTxnRequest<Identity>;
using Receipt=gears_weapon::ReloadTxnReceipt<Identity>;
Inventory stock{};unsigned calls{},mode{};bool readable{true};
bool counts_equal(Inventory const& a,Inventory const& b){return a.identity==b.identity&&a.capacity==b.capacity&&
    a.loaded==b.loaded&&a.reserve==b.reserve&&a.infinite_reserve==b.infinite_reserve;}
struct ObservedInventory {bool sample_owned(Identity const&,Inventory& out){if(!readable)return false;out=stock;return true;}};
enum class CommitResult {rejected,committed,reconciliation_required};
struct InsertionWriter {
    template<class Owns>CommitResult commit(hand::Frame const&,Request const& q,ObservedInventory&,Owns,Receipt& r){
        ++calls;r.token=q.token;
        if(mode==1){r.observed=stock;return CommitResult::rejected;}
        if(mode==2){stock.reserve=q.expected.reserve;++stock.revision;return CommitResult::reconciliation_required;}
        stock=q.expected;stock.revision=q.before.revision+1;r.accepted=true;r.observed=stock;return CommitResult::committed;
    }
};
}
namespace lancerreload_state {
struct RewardReceipt {bool applied{},dispatched{},call_confirmed{},eligible{};int final_bonus{};};
struct Lease {bool acquired{};std::uint64_t serial{};} lease;
unsigned exits{},rewards{},compensations{};
bool clock_readable{true};std::int64_t game_tick_override{-1};
bool owns(hand::Frame const&,std::uint64_t serial){return lease.acquired&&serial==lease.serial;}
bool retains_serial(std::uint64_t serial){return serial&&lease.acquired&&serial==lease.serial;}
bool recover_owned(){++exits;lease.acquired=false;return true;}
bool recover_serial(std::uint64_t serial){return !retains_serial(serial)||recover_owned();}
bool closure_time(hand::Frame const& f,std::uint64_t,std::int64_t& tick){
    if(!clock_readable)return false;tick=game_tick_override>=0?game_tick_override:f.controllers.sync_ns/1000000;return true;}
bool perfect_reward(inv::Request const&,inv::Inventory const&,inv::ObservedInventory&,RewardReceipt& r){
    if(!r.applied){r.applied=true;++rewards;}return true;}
bool restore_partial_insert(inv::Request const& q,inv::ObservedInventory&){
    if(!inv::readable)return false;
    auto half=q.before;half.reserve=q.expected.reserve;
    if(inv::counts_equal(inv::stock,half)){++compensations;inv::stock=q.before;++inv::stock.revision;}
    return inv::counts_equal(inv::stock,q.before)||inv::counts_equal(inv::stock,q.expected);
}
}
namespace boltok_parts {struct Mechanical {gears_vr::Pose root_metres{};
    std::array<gears_vr::Pose,4> in_root{};double units{100};};}
namespace boltok_visual {
bool ready{true};gears_weapon::BoltokVisualGeometry geometry{1,100,1};unsigned hides{},shows{};
bool hide_held(){++hides;return true;}
bool held(hand::Frame const&,hand::Decision const&,gears_vr::Pose const&){++shows;return true;}
}
namespace nativeequipment {bool current_ok{true};
bool current(void*,hand::Frame const&) noexcept {return current_ok;}}
namespace weaponhand {std::int64_t clock_ns(void*) noexcept {return 1100000000;}}
namespace boltok_runtime {
using P=gears_vr::Pose;using V=gears_vr::Vector;
enum class Stage {idle,opened,completing,recovering};Stage stage{};
inv::Transaction transaction;inv::InsertionWriter inventory_writer;inv::Inventory opening_inventory{};
inv::ObservedInventory inventory_observer;inv::Request completion{};lancerreload_state::RewardReceipt reward{};
std::uint64_t reload_serial{},round_item{},round_sequence{};unsigned pending_slot{6},live_mask{};
bool cycle{},native_exited{};std::int64_t perfect_ms{};hand::Lease held{};
gears_boltok_perfect_window::ClosingWindow perfect_window;
gears_weapon::BoltokChamberInsertion insertion;P actual_left_world{};FILE* log_file{};unsigned recoveries{};
bool granted(hand::Frame const& f,hand::Decision const& d,hand::LeaseKind kind){
    return f.left_tracked&&f.left_grip_known&&f.left_grip_down&&d.lease.kind==kind&&d.lease.id==f.press_token;}
bool restore() noexcept;
namespace boltok_sound {void event(hand::Frame const&,unsigned,std::uint64_t) noexcept {}}
// Mock the geometry/producer boundary with the existing detector, not Game evidence.
namespace feed_motion {void reset() noexcept {}
gears_weapon::FeedResult observe(hand::Frame const&,hand::Decision const&,boltok_parts::Mechanical const&,
    gears_weapon::BoltokReloadStamp const& s,gears_weapon::FeedSample const& sample) noexcept {
    return insertion.observe(s,sample,s.observed_ns);}}
#include "weapon_boltok_ammo.inl"
bool restore() noexcept {++recoveries;stage=Stage::recovering;return recover_ammo();}
}
namespace {
const gears_weapon::BoltokReloadIdentity id{101,102,103,1,1,1,1,1,1};
hand::Frame frame(unsigned sequence,std::uint64_t token=20){hand::Frame f{};
    f.weapon.address=id.weapon;f.world.address=id.world;f.stereo.space_epoch=1;
    f.left_tracked=f.left_grip_known=f.left_grip_down=1;f.press_token=token;
    f.controllers.sequence=sequence;f.controllers.sync_ns=1000000000+sequence*20000000;return f;}
gears_weapon::BoltokReloadStamp stamp(hand::Frame const& f){return {id,f.controllers.sequence,
    f.controllers.sync_ns,1000,true,true,true};}
hand::Decision decision(std::uint64_t token=20){hand::Decision d{};d.new_grant=true;
    d.lease.kind=hand::LeaseKind::ammo;d.lease.id=token;return d;}
void begin(unsigned loaded=2,unsigned reserve=4){using namespace boltok_runtime;
    transaction={};inventory_writer={};opening_inventory={{1},1,6,loaded,reserve,false};
    inv::stock=opening_inventory;inv::calls=inv::mode=0;inv::readable=true;
    completion={};reward={};cycle=true;native_exited=false;reload_serial=reserve&&loaded<6?7:0;
    round_item=round_sequence=0;pending_slot=6;live_mask=(1u<<loaded)-1;perfect_ms=600;
    nativeequipment::current_ok=true;lancerreload_state::clock_readable=true;lancerreload_state::game_tick_override=-1;
    (void)perfect_window.begin(reload_serial,1000,perfect_ms);
    held={};insertion.reset();stage=Stage::opened;recoveries=0;
    lancerreload_state::lease={bool(reload_serial),reload_serial};
    lancerreload_state::exits=lancerreload_state::rewards=lancerreload_state::compensations=0;
    assert(transaction.bind(opening_inventory)&&transaction.begin(opening_inventory,1,
        gears_weapon::ReloadIntentSource::mechanism_open,reload_serial,gears_weapon::ReloadTxnMode::round,6,1000));
}
void reserve_round(){using namespace boltok_runtime;auto f=frame(1);auto s=stamp(f);
    assert(ammo_step(f,decision(),{},s,s.observed_ns));assert(round_item==20&&transaction.reserved()==1&&inv::calls==0);
    assert(inv::stock.loaded==opening_inventory.loaded&&inv::stock.reserve==opening_inventory.reserve);}
void insert(unsigned slot=5){using namespace boltok_runtime;
    const double a=slot*6.283185307179586/6;
    for(unsigned seq=2;seq<=4;++seq){auto f=frame(seq);auto s=stamp(f);const double depth=(4-seq)*.03;
        actual_left_world={{1.92*std::cos(a),gears_weapon::boltok_loading_face_in_bone_raw.y-depth*100-.3,
            gears_weapon::boltok_loading_face_in_bone_raw.z-1.92*std::sin(a)},{}};
        ammo_step(f,decision(),{},s,s.observed_ns);}
}
}
int main(){using namespace boltok_runtime;
    begin();reserve_round();insert();
    std::fprintf(stderr,"calls=%u stock=%u/%u mask=%u item=%llu stage=%u recoveries=%u slot=%u\n",inv::calls,inv::stock.loaded,inv::stock.reserve,live_mask,round_item,unsigned(stage),recoveries,pending_slot);
    assert(inv::calls==1&&inv::stock.loaded==3&&inv::stock.reserve==3&&live_mask==35&&!round_item);
    auto f=frame(5);assert(finish_ammo(f)&&completion.result==gears_weapon::ReloadTxnResult::ordinary);
    complete_ammo();complete_ammo();assert(stage==Stage::idle&&!cycle&&lancerreload_state::exits==1&&lancerreload_state::rewards==0);
    begin(5,1);reserve_round();insert();f=frame(5);assert(!perfect_window.armed()&&arm_perfect_close(f));assert(finish_ammo(f));
    assert(completion.result==gears_weapon::ReloadTxnResult::perfect);assert(recover_ammo());
    assert(recover_ammo()&&lancerreload_state::rewards==1&&inv::stock.loaded==6&&inv::stock.reserve==0);
    // A long physical setup does not consume the closing window: all clocks remain real inputs.
    for(bool fast:{true,false}){begin(5,1);lancerreload_state::game_tick_override=5000;
        reserve_round();insert();assert(!perfect_window.armed());
        lancerreload_state::game_tick_override=9000;assert(arm_perfect_close(frame(5)));
        assert(perfect_window.relative_window_ms()==8600);
        assert(!arm_perfect_close(frame(5)));
        lancerreload_state::game_tick_override=fast?9400:9601;f=frame(5);assert(finish_ammo(f));
        assert(completion.result==(fast?gears_weapon::ReloadTxnResult::perfect:gears_weapon::ReloadTxnResult::ordinary));
        complete_ammo();complete_ammo();assert(!cycle&&!perfect_window.armed());
        assert(inv::stock.loaded==6&&lancerreload_state::rewards==unsigned(fast));}
    for(bool bad_clock:{true,false}){begin(5,1);reserve_round();
        if(bad_clock)lancerreload_state::clock_readable=false;else nativeequipment::current_ok=false;
        insert();assert(inv::stock.loaded==6&&!perfect_window.armed());
        assert(!arm_perfect_close(frame(5)));
        lancerreload_state::clock_readable=true;nativeequipment::current_ok=true;f=frame(5);
        assert(finish_ammo(f)&&completion.result==gears_weapon::ReloadTxnResult::ordinary);
        complete_ammo();complete_ammo();assert(lancerreload_state::rewards==0&&!cycle);}
    // A broken first closing stroke cannot regain its reward by regrasping.
    begin(5,1);reserve_round();insert();assert(arm_perfect_close(frame(5)));
    perfect_window.reset();assert(!arm_perfect_close(frame(6)));
    f=frame(7);assert(finish_ammo(f)&&completion.result==gears_weapon::ReloadTxnResult::ordinary);
    complete_ammo();complete_ammo();assert(!cycle&&lancerreload_state::rewards==0);
    for(unsigned mode:{1u,2u}){begin();reserve_round();inv::mode=mode;insert();
        assert(inv::calls==1&&inv::stock.loaded==2&&inv::stock.reserve==4&&live_mask==3);
        assert(!transaction.pending());if(mode==2)assert(lancerreload_state::compensations==1);}
    begin();reserve_round();f=frame(2);f.left_grip_down=0;auto s=stamp(f);
    assert(ammo_step(f,decision(),{},s,s.observed_ns)&&!round_item&&inv::calls==0&&transaction.reserved()==0);
    for(unsigned loaded:{0u,6u}){begin(loaded,0);f=frame(2);assert(finish_ammo(f));complete_ammo();complete_ammo();
        assert(!cycle&&inv::calls==0&&inv::stock.loaded==loaded&&inv::stock.reserve==0&&lancerreload_state::rewards==0);}
    begin();reserve_round();inv::Request q{};assert(transaction.request_insertion(round_item)&&transaction.take_request(q));
    pending_slot=5;inv::stock=q.expected;inv::stock.revision=q.before.revision+1;inv::readable=false;
    assert(!recover_ammo()&&transaction.pending());inv::readable=true;
    assert(recover_ammo()&&!transaction.pending()&&live_mask==35&&inv::stock.loaded==3&&inv::stock.reserve==3);
    std::puts("Boltok production ammo adapter: reservation, physical insertion, partial/perfect close, rejection, half-write, inspection and retained late commit passed (mock game/geometry boundary, no Native/CF/VR acceptance).");
}
