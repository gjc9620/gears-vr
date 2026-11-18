// Bow-only client of the shared Native R3 veto. No button injection, raw input
// writer or authority to clear a different weapon's current fence exists here.
namespace torquebow_input_fence {
namespace api=gears_weapon_button_veto;namespace hand=gears_weapon_hand;
api::Update bridge() noexcept {
 const auto native_image=weaponhand::native_module;const auto address=native_image?GetProcAddress(native_image,api::export_name):nullptr;
 return native_image&&weaponhand::executable(address,native_image)?reinterpret_cast<api::Update>(address):nullptr;
}
bool available() noexcept {return bridge()!=nullptr;}
bool update(hand::Frame const& f,bool armed) noexcept {
 // Native repeats Broker admission and uses f's original sync_ns. Merely
 // finding an export, holding a module or having a local charge flag is not
 // enough to arm/clear an original-input fence.
 if(!nativeequipment::current(nullptr,f)||!playground::instance(f.weapon.address,L"Weap_Bow_base")||
    !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return false;
 auto call=bridge();if(!call)return false;
 api::Request request{};request.source=f;request.armed=unsigned(armed);
 return call(&request)==unsigned(api::Result::applied);
}
}
