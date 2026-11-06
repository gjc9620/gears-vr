// Rejection fence only. A quiet domain is not an engine/writer permit; the
// adapter still requires current Native publication and original game policy.
namespace nativeequipment {
// Owned rollback must remain possible while the Native registration drains.
// This is not acquisition/action admission; the caller still authenticates
// its exact journal and refuses foreign state before any original helper.
inline bool restoration_quiet() noexcept {
    if(weaponshot::stack.depth())return false;
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(weaponaction::depth)return false;
#endif
    return true;
}
inline bool domain_ready() noexcept {
    if(!weaponcall::ready()||weaponshot::stack.depth())return false;
#if defined(GEARS_PLAYGROUND_WEAPON_ACTION_GATE_TRIAL)
    if(!weaponaction::ready()||weaponaction::depth)return false;
#endif
    return true;
}
}
