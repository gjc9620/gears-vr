// Consume cosmetic receipts from the sole mechanical owner. Neither a target
// position, animation seek nor inventory q1 alone may create these results.
namespace boomshot_endpoint {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
using Owner=boomshot_reload_owner::Owner;using Kind=Owner::EndpointKind;using Receipt=Owner::Endpoint;
struct Display {Receipt receipt{};std::int64_t until_ns{};bool known{};}display;
std::uint64_t consumed_ordinal{};unsigned rows{};
void lost() noexcept {display={};boomshot_reload_owner::owner.discard_endpoints();}
bool scope(hand::Frame const& f,Receipt const& value) noexcept {
    auto a=f.key,b=value.key;a.frame=b.frame=0;
    return value.identity.valid()&&value.identity==inv::identity(f)&&hand::same(a,b)&&
        value.key.frame<=f.key.frame&&value.reference==f.reference_generation&&value.profile==f.controllers.profile_generation&&
        value.source==unsigned(f.stereo.source)&&value.raw_sequence&&value.raw_sequence<=f.controllers.sequence&&
        value.raw_sync_ns>0&&value.raw_sync_ns<=f.controllers.sync_ns;
}
bool current(hand::Frame const& f) noexcept {
    return weaponhand::current_gt(nullptr)&&f.left_tracked==1&&f.left_grip_known==1&&
        gears_vr::controller_pose_usable(f.controllers.hands[0].grip,f.controllers.hands[0].grip_flags,f.controllers.hands[0].grip_active)&&
        gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,f.controllers.hands[1].grip_active)&&
        hand::frame_active(f,weaponhand::clock_ns(nullptr))&&
        nativeequipment::current(nullptr,f);
}
void sample(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr))return;
    Receipt value{};
    while(boomshot_reload_owner::owner.take_endpoint(value)){
        // Destructive take + high-water happen BEFORE any cosmetic consumer.
        // Invalid/dropped effects cannot be queued for later recovery/replay.
        if(!value.ordinal||value.ordinal<=consumed_ordinal)continue;consumed_ordinal=value.ordinal;
        const auto observed_now=weaponhand::clock_ns(nullptr);
        if((value.kind!=Kind::opened&&value.kind!=Kind::seated&&value.kind!=Kind::closed)||!value.serial||
           value.observed_ns<=0||value.observed_ns<value.raw_sync_ns||observed_now<value.observed_ns||observed_now-value.observed_ns>100000000||
           !scope(f,value)||!current(f)){display={};continue;}
        display={value,value.observed_ns>INT64_MAX-700000000?INT64_MAX:value.observed_ns+700000000,true};
        boomshot_endpoint_sound::play(f,value.kind==Kind::opened?boomshot_endpoint_sound::Cue::opened:
            value.kind==Kind::seated?boomshot_endpoint_sound::Cue::seated:boomshot_endpoint_sound::Cue::closed,value.ordinal,value.observed_ns);
        if(log_file&&rows<24){++rows;
            fwprintf(log_file,L"boomshot_endpoint kind=%u ordinal=%llu serial=%llu receipt_frame=%llu receipt_sequence=%llu receipt_sync_ns=%lld observed_ns=%lld current_frame=%llu cosmetic_only=1\n",
                unsigned(value.kind),value.ordinal,value.serial,value.key.frame,value.raw_sequence,value.raw_sync_ns,value.observed_ns,f.key.frame);fflush(log_file);
        }
    }
    if(display.known&&(!scope(f,display.receipt)||weaponhand::clock_ns(nullptr)>=display.until_ns||!current(f)))display={};
}
bool view(hand::Frame const& f,Kind& out) noexcept {
    out=Kind::none;if(!display.known||!scope(f,display.receipt)||!current(f)||
        weaponhand::clock_ns(nullptr)>=display.until_ns)return false;
    // This short cosmetic duration does not extend input/receipt admission.
    out=display.receipt.kind;return true;
}
}
