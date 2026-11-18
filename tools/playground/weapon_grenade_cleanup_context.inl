#pragma once
// A reservation for this journal's already dispatched original input only.
// It never returns a Frame, renews a pose epoch, or authorizes a positive call.
namespace grenade_cleanup_context {
namespace hand=gears_weapon_hand;
namespace api=::gears_weapon_original_input_cleanup_context;
api::Query reader() noexcept {
    const auto image=weaponhand::native_module;
    const auto address=image?GetProcAddress(image,api::export_name):nullptr;
    return image&&weaponhand::executable(address,image)?reinterpret_cast<api::Query>(address):nullptr;
}
bool available() noexcept {return reader()!=nullptr;}
bool incarnation(hand::EquipmentIdentity const& parent) noexcept {
    const auto read=reader();if(!read)return false;
    api::Request request{};request.parent=parent;
    const auto reply=read(&request);
    static unsigned context_rejection_reports{};
    if(log_file&&reply!=unsigned(api::Result::same_incarnation)&&context_rejection_reports++<8){
        fwprintf(log_file,L"grenade_cleanup_context reply=%u world=%llx pawn=%llx weapon=%llx world_generation=%llu native=%llu cookie=%llu\n",
            reply,parent.world.address,parent.pawn.address,parent.weapon.address,parent.key.world_generation,
            parent.key.native_started_qpc,parent.key.registration_cookie);fflush(log_file);
    }
    return reply==unsigned(api::Result::same_incarnation);
}
class Anchor {
    hand::EquipmentIdentity parent_{};
    std::uint64_t serial_{},hold_{};bool dispatched_{};
public:
    bool capture_before_input(hand::Frame const& source,std::uint64_t serial,std::uint64_t hold) noexcept {
        if(serial_||!serial||!hold||!weaponhand::current_gt(nullptr)||
           !nativeequipment::restoration_quiet()||!nativeequipment::current(nullptr,source))return false;
        hand::ExistingActionLifetime actual{};
        if(!weaponhand::client.capture_existing_action_lifetime_on_gt(source.key,actual)||
           !hand::equipment_matches(source,actual.parent)||!incarnation(actual.parent)||
           !nativeequipment::current(nullptr,source)||!incarnation(actual.parent))return false;
        parent_=actual.parent;serial_=serial;hold_=hold;
        static unsigned anchor_capture_reports{};
        if(log_file&&anchor_capture_reports++<8){
            fwprintf(log_file,L"grenade_cleanup_anchor captured=1 serial=%llu hold=%llu weapon=%llx frame=%llu original_epoch=%llu\n",
                serial_,hold_,parent_.weapon.address,parent_.key.frame,actual.epoch);fflush(log_file);
        }
        return true;
    }
    void record_dispatched(std::uint64_t serial,std::uint64_t hold,bool invoked) noexcept {
        if(serial_&&serial==serial_&&hold==hold_&&invoked)dispatched_=true;
    }
    bool current(hand::Frame const& source,std::uint64_t serial,std::uint64_t hold) const noexcept {
        return serial_&&serial==serial_&&hold==hold_&&dispatched_&&
            hand::equipment_matches(source,parent_)&&weaponhand::current_gt(nullptr)&&
            nativeequipment::restoration_quiet()&&
            weaponhand::client.registration_alive(parent_.key.registration_cookie,parent_.key.native_started_qpc)&&
            incarnation(parent_)&&
            weaponhand::client.registration_alive(parent_.key.registration_cookie,parent_.key.native_started_qpc)&&
            incarnation(parent_);
    }
    bool retire(std::uint64_t serial,std::uint64_t hold,bool proven_neutral) noexcept {
        if(!serial_)return true; // Legacy Native never captured an anchor.
        if(serial!=serial_||hold!=hold_||!proven_neutral)return false;
        *this=Anchor{};return true;
    }
};
}
