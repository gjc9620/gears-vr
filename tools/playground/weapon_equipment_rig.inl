// Current prepared tracking data only. No wrist/weapon pose cache, setter,
// ownership grant, engine call or physical calibration is created here.
namespace nativeequipment_rig {
namespace rig=::gears_rig_tracking;
namespace hand=::gears_weapon_hand;
namespace binding=::gears_equipment_rig;
enum class Status:unsigned {source_unavailable,provider_unavailable,binding_rejected,source_changed,ready};
struct Result {
    Status status{Status::source_unavailable};
    rig::ReadResult read{};binding::Status agreement{binding::Status::inactive_hand};
    std::int64_t observed_ns{};
};
// A repeated read of the same publication is data-only, not an action replay.
// Keep minimum_sequence zero here so a diagnostic read cannot consume the
// publication needed by another caller in this same authenticated GT frame.
Result acquire(hand::Frame const& source,rig::Frame& out) noexcept {
    out={};Result result{};
    if(!nativeequipment::current(nullptr,source))return result;
    rig::Frame candidate{};
    const auto expected=static_cast<std::uint32_t>(source.stereo.source);
    result.read=handrig::read(candidate,0,expected);
    if(result.read.reply!=rig::Reply::accepted){result.status=Status::provider_unavailable;return result;}
    result.observed_ns=nativeequipment::clock_ns(nullptr);
    result.agreement=binding::match(source,candidate,result.observed_ns,expected);
    if(result.agreement!=binding::Status::matched){result.status=Status::binding_rejected;return result;}
    // source_current re-observes the exact committed actors and their links;
    // model held/busy may have changed in the preceding equipment operation.
    if(!nativeequipment::current(nullptr,source)){result.status=Status::source_changed;return result;}
    result.observed_ns=nativeequipment::clock_ns(nullptr);
    result.agreement=binding::match(source,candidate,result.observed_ns,expected);
    if(result.agreement!=binding::Status::matched){result.status=Status::binding_rejected;return result;}
    out=candidate;result.status=Status::ready;return result;
}
const char* status_name(Status value) noexcept {
    switch(value){
    case Status::source_unavailable:return "source_unavailable";
    case Status::provider_unavailable:return "provider_unavailable";
    case Status::binding_rejected:return "binding_rejected";
    case Status::source_changed:return "source_changed";
    case Status::ready:return "ready";
    }
    return "unknown";
}
std::uint64_t ready_count{},rejected_count{},last_publication{};
Status last_status{};bool have_status{};unsigned logs{};
void tick(hand::Frame const& source) noexcept {
    rig::Frame prepared{};const auto result=acquire(source,prepared);
    const bool ready=result.status==Status::ready;
    const bool held=vrequip::model.held_for(source.world.address,source.pawn.address,source.weapon.address);
    if(ready)++ready_count;else ++rejected_count;
    if(log_file&&logs<12&&(!have_status||last_status!=result.status||
        (ready&&held&&last_publication!=prepared.publication_sequence))){
        const auto& p=prepared.world_grip[1];
        fwprintf(log_file,L"native_equipment_rig status=%hs agreement=%u reply=%u failure=%u held=%u source=%u frame=%llu cookie=%llu pid=%u game_start=%llu native_started=%llu world_generation=%llu publication=%llu scene=%llu source_sequence=%llu observed_ns=%lld input_received_ns=%lld input_sync_ns=%lld published_ns=%llu units=%.9g right_grip=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
            status_name(result.status),unsigned(result.agreement),unsigned(result.read.reply),unsigned(result.read.failure),
            unsigned(held),unsigned(source.stereo.source),source.key.frame,source.key.registration_cookie,source.key.game_pid,
            source.key.game_start,source.key.native_started_qpc,source.key.world_generation,
            prepared.publication_sequence,prepared.scene_invocation,source.controllers.sequence,result.observed_ns,
            source.controllers.received_ns,source.controllers.sync_ns,prepared.published_ns,prepared.units_per_metre,
            p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w);
        fflush(log_file);++logs;
    }
    last_status=result.status;have_status=true;last_publication=prepared.publication_sequence;
}
void reset_after_drain() noexcept {
    if(log_file&&(ready_count||rejected_count)){
        fwprintf(log_file,L"native_equipment_rig_summary ready=%llu rejected=%llu\n",ready_count,rejected_count);fflush(log_file);
    }
    ready_count=rejected_count=last_publication=0;last_status={};have_status=false;logs=0;
}
} // namespace nativeequipment_rig
