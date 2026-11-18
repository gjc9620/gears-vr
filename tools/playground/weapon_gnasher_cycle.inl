// Receipt-only end of a PHYSICALLY completed Gnasher stroke. The original
// timer may already have ended. Never call FinishedCockingShotgun a second time.
namespace gnasher_cycle {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
struct Reference {
    gears_weapon::GnasherClosedLeverReference closed{};
    hand::ObjectIdentity component{},asset{};
    double units{};
};
struct Owner {
    inv::Identity identity{};Reference reference{};
    gears_weapon::GnasherCycleReadbackGate gate{};
    std::uint64_t profile{},reference_generation{},last_frame{};bool active{};
};
Owner owner;std::uint64_t next_observation{1};
bool begin(hand::Frame const& f,gears_weapon::GnasherMechanicalInteraction const& mechanical,
           Reference const& reference) noexcept {
    if(owner.active||!reference.closed.valid()||!hand::identity_valid(reference.component)||
       !hand::identity_valid(reference.asset)||!std::isfinite(reference.units)||reference.units<10||reference.units>500||
       !gnasher_animation::support_granted(f)||!nativeequipment::current(nullptr,f))return false;
    auto* entry=gnasher_fire::entry(f,false);const auto request=mechanical.cycle_request();
    if(!entry||!(entry->identity==inv::identity(f))||!entry->cycle.can_request_cycle()||
       !(request.identity==gnasher_fire::mechanical_identity(f))||
       request.input_sequence!=f.controllers.sequence||request.physical_completion_ns!=f.controllers.sync_ns)return false;
    // Journal the fresh completed gesture before any engine getter can spend
    // this input's lifetime. Actual bone/asset/readiness ACK follows in tick.
    Owner candidate{};candidate.identity=inv::identity(f);candidate.reference=reference;
    candidate.profile=f.controllers.profile_generation;candidate.reference_generation=f.reference_generation;
    if(!candidate.gate.begin(request,reference.closed,weaponhand::clock_ns(nullptr)))return false;
    candidate.active=true;owner=candidate;return true;
}
bool tick(hand::Frame const& f,gears_weapon::GnasherMechanicalInteraction& mechanical) noexcept {
    if(!owner.active||!(owner.identity==inv::identity(f))||owner.profile!=f.controllers.profile_generation||
       owner.reference_generation!=f.reference_generation||f.key.frame==owner.last_frame||
       !next_observation||next_observation==UINT64_MAX||!nativeequipment::current(nullptr,f))return false;
    auto* entry=gnasher_fire::entry(f,false);
    if(!entry||!(entry->identity==owner.identity))return false;
    bool firing{},reloading{};
    if(!lancerreload_state::bool_call(f.weapon.address,L"IsFiring",firing)||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading))return false;
    gnasher_parts::Measured measured{};
    if(!gnasher_parts::measure(f,owner.reference.units,measured)||
       !hand::same(owner.reference.component,measured.source.component)||
       !hand::same(owner.reference.asset,measured.source.asset))return false;
    inv::Inventory inventory_observed{};
    if(!entry->observer.sample_owned(owner.identity,inventory_observed)||!nativeequipment::current(nullptr,f))return false;
    const auto request=mechanical.cycle_request();
    const gears_weapon::GnasherCycleReadback readback{request.identity,request.token,
        owner.reference.closed.calibration_id,f.controllers.sequence,next_observation++,
        f.controllers.sync_ns,measured.lever_in_main,true,!firing&&!reloading};
    auto candidate=owner.gate;gears_weapon::GnasherCycleReceipt receipt{};
    owner.last_frame=f.key.frame;
    if(firing||!gnasher_state::prepare_cycle(f,request,candidate,readback))return false;
    if(!candidate.observe(readback,weaponhand::clock_ns(nullptr),receipt)){owner.gate=candidate;return false;}
    if(!gears_weapon::gnasher_commit_closed_cycle(mechanical,entry->cycle,receipt,
        gnasher_fire::counts(f,inventory_observed)))return false;
    // Publish both consumer states only after original bone + inventory ACK.
    // This never writes ammo, replays original notifies or produces a shot.
    if(log_file){fwprintf(log_file,L"gnasher_physical_cycle_confirmed token=%llu original_observation=%llu frame=%llu input=%llu original_ready=1 duplicate_cocking_call=0\n",
        receipt.request_token,receipt.original_event_sequence,f.key.frame,f.controllers.sequence);fflush(log_file);}
    gnasher_state::cycle_accepted(f,receipt);owner={};return true;
}
bool pending_work() noexcept{return owner.active;}
} // namespace gnasher_cycle
