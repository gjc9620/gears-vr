// Rejection fence only. A quiet domain is not an engine/writer permit; the
// adapter still requires current Native publication and original game policy.
namespace nativeequipment {
inline bool domain_ready() noexcept {
    if(!weaponcall::ready()||weaponshot::stack.depth())return false;
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(!weaponaction::ready()||weaponaction::depth)return false;
#endif
    return true;
}
}
