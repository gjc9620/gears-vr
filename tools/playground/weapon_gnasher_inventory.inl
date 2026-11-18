// One physical round -> existing original inventory writer -> paired private
// consumers. Normal GT only. Never retries a dispatched setter; uncertain or
// partial outcomes keep this exact journal until original counts resolve it.
namespace gnasher_inventory {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
enum class Result {rejected,committed,reconciliation_required,consumer_pending};
class Insertion {
    enum class Stage {idle,dispatching,pending,withdrawing,withdrawal_call,settled};
    Stage stage_{};
    inv::InsertionWriter writer_{};inv::Identity writer_identity_{},identity_{};
    inv::Request original_{};
    gears_weapon::GnasherInventoryRequest physical_{};
    std::uint64_t last_token_{};
    Result settled_{Result::rejected};
    bool expected_permitted_{},compensation_attempted_{};
    bool context(hand::Frame const& f,gnasher_fire::Entry const& entry) const noexcept {
        return weaponhand::current_gt(nullptr)&&entry.identity==identity_&&
            f.key.game_start==identity_.game_start&&f.key.native_started_qpc==identity_.native_start&&
            f.key.registration_cookie==identity_.registration&&f.key.world_generation==identity_.world_generation&&
            hand::same(f.world,identity_.world)&&hand::same(f.pawn,identity_.pawn)&&
            weaponhand::client.registration_alive(identity_.registration,identity_.native_start);
    }
public:
    bool pending() const noexcept {return stage_==Stage::pending||stage_==Stage::dispatching||stage_==Stage::withdrawing||stage_==Stage::withdrawal_call;}
    bool blocks_fire(hand::ObjectIdentity const& weapon) const noexcept {
        return pending()&&hand::same(identity_.weapon,weapon);
    }
    std::uint64_t token() const noexcept {return physical_.token;}
    // A physical proposal may expire BEFORE entering the original writer.
    // Retire only a token newer than every dispatched journal and only after
    // exact before-count observation. This never rejects a partial/unknown
    // original call, renews input, refunds ammo or calls an original setter.
    Result reject_undispatched(hand::Frame const& f,
        gears_weapon::GnasherMechanicalInteraction& mechanical,gnasher_fire::Entry& entry) noexcept {
        const auto request=mechanical.inventory_request();
        if(stage_!=Stage::idle||!request.token||request.token<=last_token_||
           !weaponhand::current_gt(nullptr)||!(request.identity==mechanical.inventory().identity)||
           request.identity.world!=entry.identity.world.address||request.identity.pawn!=entry.identity.pawn.address||
           request.identity.weapon!=entry.identity.weapon.address||request.identity.world_generation!=entry.identity.world_generation||
           !hand::same(f.world,entry.identity.world)||!hand::same(f.pawn,entry.identity.pawn)||
           f.key.game_start!=entry.identity.game_start||f.key.native_started_qpc!=entry.identity.native_start||
           f.key.registration_cookie!=entry.identity.registration||f.key.world_generation!=entry.identity.world_generation||
           !weaponhand::client.registration_alive(entry.identity.registration,entry.identity.native_start))return Result::reconciliation_required;
        inv::Inventory actual{};
        if(!entry.observer.sample_owned(entry.identity,actual)||!actual.valid()||actual.infinite_reserve||
           actual.capacity!=mechanical.capacity()||actual.revision<request.before_revision||
           actual.loaded!=request.before_loaded||actual.reserve!=request.before_reserve)return Result::reconciliation_required;
        const gears_weapon::GnasherInventoryReceipt receipt{request.token,false,
            {request.identity,actual.revision,actual.loaded,actual.reserve}};
        if(!gears_weapon::gnasher_reject_inserted_round(mechanical,entry.cycle,receipt))return Result::consumer_pending;
        last_token_=request.token;return Result::rejected;
    }
    // Receipt reads may finish after the source lifetime or after selecting
    // another weapon. No stored input is renewed and no engine setter runs.
    Result reconcile(hand::Frame const& f,gears_weapon::GnasherMechanicalInteraction& mechanical,
                     gnasher_fire::Entry& entry) noexcept {
        if(stage_==Stage::settled)return settled_;
        if(stage_==Stage::withdrawal_call)return Result::consumer_pending;
        if(stage_==Stage::withdrawing){
            // Withdrawal is the exact OLD resource's cleanup, not another
            // inventory observation or an action on the currently selected
            // world/pawn. Its full original owner and Native registration
            // remain required even after a world or selection change.
            if(!weaponhand::current_gt(nullptr)||!(entry.identity==identity_)||
               !weaponhand::client.registration_alive(identity_.registration,identity_.native_start))return Result::consumer_pending;
            // Counts and BOTH private consumers have already committed. Only
            // retire this exact visual token; never replay the inventory ACK
            // or setter while a failed hide is being retried.
            stage_=Stage::withdrawal_call;
            const bool withdrawn=gnasher_round::release_for_stow(identity_,physical_.round_serial);
            stage_=Stage::withdrawing;
            if(!withdrawn)return Result::consumer_pending;
            stage_=Stage::settled;settled_=Result::committed;return settled_;
        }
        if(stage_!=Stage::pending||!context(f,entry))return Result::reconciliation_required;
        const auto request=mechanical.inventory_request();
        if(request.token!=physical_.token||request.round_serial!=physical_.round_serial||
           !(request.identity==physical_.identity))return Result::consumer_pending;
        inv::Inventory actual{};
        if(!entry.observer.sample_owned(identity_,actual))return Result::reconciliation_required;
        const bool inserted=inv::counts_equal(actual,original_.expected);
        const bool unchanged=inv::counts_equal(actual,original_.before);
        if((!inserted&&!unchanged)||(inserted&&!expected_permitted_))return Result::reconciliation_required;
        const gears_weapon::GnasherInventoryReceipt receipt{physical_.token,inserted,
            {physical_.identity,actual.revision,actual.loaded,actual.reserve}};
        const bool accepted=inserted?
            gears_weapon::gnasher_commit_inserted_round(mechanical,entry.cycle,receipt):
            gears_weapon::gnasher_reject_inserted_round(mechanical,entry.cycle,receipt);
        if(!accepted)return Result::consumer_pending;
        if(inserted){
            stage_=Stage::withdrawing;
            return reconcile(f,mechanical,entry);
        }
        stage_=Stage::settled;settled_=Result::rejected;return settled_;
    }
    Result dispatch(hand::Frame const& f,std::uint64_t reload_serial,
                    gears_weapon::GnasherMechanicalInteraction& mechanical,
                    gnasher_fire::Entry& entry,inv::OwnsReload owns) noexcept {
        if(pending())return Result::reconciliation_required;
        if(stage_!=Stage::idle||!reload_serial||!owns||!owns(f,reload_serial)||
           !nativeequipment::current(nullptr,f)||!(entry.identity==inv::identity(f))||
           !playground::instance(f.weapon.address,L"Weap_Shotgun"))return Result::rejected;
        const auto request=mechanical.inventory_request();
        const auto now=weaponhand::clock_ns(nullptr);
        if(!request.token||request.token<=last_token_||!request.round_serial||
           !gnasher_round::captured_for(f,request.round_serial)||
           !(request.identity==gnasher_fire::mechanical_identity(f))||
           request.input_sequence!=f.controllers.sequence||request.physical_completion_ns!=f.controllers.sync_ns||
           !request.reference_generation||request.reference_generation!=f.reference_generation||
           request.physical_completion_ns<=0||now<request.physical_completion_ns||
           now-request.physical_completion_ns>100000000||entry.cycle.pending()||entry.cycle.faulted())return Result::rejected;
        const auto fire_before=entry.cycle.inventory();
        if(!(fire_before.identity==request.identity)||fire_before.revision!=request.before_revision||
           fire_before.loaded!=request.before_loaded||fire_before.reserve!=request.before_reserve)return Result::rejected;
        inv::Inventory before{};
        if(!entry.observer.sample(f,before)||!before.valid()||before.infinite_reserve||
           before.capacity!=mechanical.capacity()||before.revision!=request.before_revision||
           before.loaded!=request.before_loaded||before.reserve!=request.before_reserve||
           !before.reserve||before.loaded>=before.capacity||request.after_loaded!=before.loaded+1||
           request.after_reserve!=before.reserve-1||!nativeequipment::current(nullptr,f)||!owns(f,reload_serial))return Result::rejected;
        const auto final_now=weaponhand::clock_ns(nullptr);
        if(final_now<request.physical_completion_ns||final_now-request.physical_completion_ns>100000000)return Result::rejected;
        const auto next_identity=inv::identity(f);
        if(writer_identity_.valid()&&!(writer_identity_==next_identity)){
            if(!inv::same_instance_across_hold(writer_identity_,next_identity))return Result::rejected;
            writer_={}; // No journal pending; outer last_token_ stays monotonic.
        }
        inv::Request original_request{};original_request.identity=next_identity;original_request.token=request.token;
        original_request.reload_serial=reload_serial;original_request.item_serial=request.round_serial;
        original_request.call=gears_weapon::ReloadTxnCall::insert;original_request.before=original_request.expected=before;
        original_request.expected.loaded=request.after_loaded;original_request.expected.reserve=request.after_reserve;original_request.expected.revision=0;
        // Journal BEFORE the existing synchronous writer can enter the engine.
        identity_=writer_identity_=next_identity;physical_=request;original_=original_request;
        last_token_=request.token;stage_=Stage::dispatching;expected_permitted_=false;compensation_attempted_=false;
        inv::Receipt receipt{};
        const auto outcome=writer_.commit(f,original_,entry.observer,owns,receipt);
        expected_permitted_=outcome!=inv::CommitResult::rejected;stage_=Stage::pending;
        // Setter outcomes are resolved only from actual owned counts. An
        // error/re-entry never causes a second commit of this physical token.
        return reconcile(f,mechanical,entry);
    }
    // Explicit lifecycle recovery only. The existing common compensator may
    // restore ONLY the exact reserve-first half-write under its original lease.
    // It never retries insertion or changes loaded ammo. Journal before the
    // compensator too: even an uncertain return cannot authorize a second call.
    Result recover(hand::Frame const& f,gears_weapon::GnasherMechanicalInteraction& mechanical,
                   gnasher_fire::Entry& entry) noexcept {
        const auto result=reconcile(f,mechanical,entry);
        if(stage_!=Stage::pending||result!=Result::reconciliation_required||
           compensation_attempted_||!expected_permitted_||!context(f,entry))return result;
        const auto request=mechanical.inventory_request();
        inv::Inventory actual{};
        auto partial=original_.before;partial.reserve=original_.expected.reserve;
        const auto& lease=lancerreload_state::lease;
        if(request.token!=physical_.token||request.round_serial!=physical_.round_serial||
           !(request.identity==physical_.identity)||original_.before.infinite_reserve||
           !entry.observer.sample_owned(identity_,actual)||!inv::counts_equal(actual,partial)||
           !lease.acquired||lease.serial!=original_.reload_serial||!(lease.identity==identity_)||
           !lancerreload_state::owned_state())return result;
        compensation_attempted_=true;stage_=Stage::dispatching;
        (void)lancerreload_state::restore_partial_insert(original_,entry.observer);
        stage_=Stage::pending;return reconcile(f,mechanical,entry);
    }
    bool retire() noexcept {
        if(stage_!=Stage::settled)return false;
        stage_=Stage::idle;identity_={};physical_={};original_={};return true;
    }
};
struct Record {inv::Identity identity{};Insertion insertion{};bool known{};};
std::array<Record,16> records{};
Record* record(hand::Frame const& f,bool create) noexcept {
    const auto identity=inv::identity(f);
    for(auto& r:records)if(r.known&&(r.identity==identity||inv::same_instance_across_hold(r.identity,identity)))return &r;
    if(create)for(auto& r:records)if(!r.known){r.identity=identity;r.known=true;return &r;}
    return nullptr;
}
bool blocks_fire(hand::ObjectIdentity const& weapon) noexcept {
    for(auto const& r:records)if(r.known&&r.insertion.blocks_fire(weapon))return true;return false;
}
bool pending_work() noexcept {
    for(auto const& r:records)if(r.known&&r.insertion.pending())return true;return false;
}
Result commit_round(hand::Frame const& f,std::uint64_t serial,inv::OwnsReload owns) noexcept {
    auto* e=gnasher_fire::entry(f,false);
    if(!e||!(e->identity==inv::identity(f))||!playground::instance(f.weapon.address,L"Weap_Shotgun"))return Result::rejected;
    auto* r=record(f,true);
    if(!r||r->insertion.pending())return Result::reconciliation_required;
    r->identity=e->identity;
    return r->insertion.dispatch(f,serial,e->mechanical,*e,owns);
}
bool retire_settled(hand::Frame const& f) noexcept {
    auto* r=record(f,false);return r&&r->insertion.retire();
}
Result recover_round(hand::Frame const& f) noexcept {
    auto* r=record(f,false);auto* e=gnasher_fire::entry(f,false);
    if(!r||!e)return Result::rejected;
    return r->insertion.recover(f,e->mechanical,*e);
}
Result reject_undispatched_round(hand::Frame const& f) noexcept {
    auto* e=gnasher_fire::entry(f,false);if(!e)return Result::rejected;
    auto* r=record(f,true);if(!r)return Result::reconciliation_required;
    return r->insertion.reject_undispatched(f,e->mechanical,*e);
}
// Existing normal equipment GT boundary. Old-actor receipts retire
// independently while another weapon is selected; no input is dispatched.
void service(hand::Frame const& f) noexcept {
    for(auto& r:records)if(r.known&&r.insertion.pending())
        for(auto& e:gnasher_fire::entries)if(e.known&&e.identity==r.identity){
            (void)r.insertion.reconcile(f,e.mechanical,e);break;
        }
}
}
