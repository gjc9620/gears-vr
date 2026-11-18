// Compatibility alias on the existing sole pad fence. No second admission,
// timestamp, consume, lock, route, worker, hook or publication is introduced.
namespace weapon_button_veto_live {
std::uint32_t publish(gears_weapon_button_veto::Request const* request) noexcept {
 return longshot_scope_input_live::publish(request);
}
}
