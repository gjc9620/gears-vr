namespace torquebow_cleanup_context {
namespace api=gears_weapon_original_input_cleanup_context;
bool current(gears_weapon_hand::EquipmentIdentity const& parent) noexcept {
    const auto cleanup_image=weaponhand::native_module;
    const auto address=cleanup_image?GetProcAddress(cleanup_image,api::export_name):nullptr;
    if(!cleanup_image||!weaponhand::executable(address,cleanup_image))return false;
    api::Request request{};request.parent=parent;
    return reinterpret_cast<api::Query>(address)(&request)==unsigned(api::Result::same_incarnation);
}
}
