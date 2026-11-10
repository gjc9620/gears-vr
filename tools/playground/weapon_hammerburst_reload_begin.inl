// Included inside hammerburst_runtime, after the original state/inventory
// adapters. Capture qualification against pre-insertion counts, exactly once
// for this accepted attempt. A rejected attempt publishes no reward receipt.
namespace reload_begin {
lancerreload_state::BeginResult acquire(hand::Frame const& f,inv::Inventory const& before,
    std::uint64_t& serial,lancerreload_state::RewardReceipt& accepted_reward) noexcept {
    using Result=lancerreload_state::BeginResult;
    serial=0;
    if(!before.valid())return Result::rejected;
    const bool needs_reload=before.loaded<before.capacity&&(before.infinite_reserve||before.reserve);
    lancerreload_state::RewardReceipt candidate{};
    if(needs_reload&&!lancerreload_state::capture_reward_eligibility(before,candidate))return Result::rejected;
    const auto result=needs_reload?lancerreload_state::begin(f,serial):Result::acquired;
    // A recovery-required original acceptance also belongs to this attempt;
    // normal cleanup must retain its receipt rather than manufacture another.
    if(result!=Result::rejected)accepted_reward=candidate;
    return result;
}
}
