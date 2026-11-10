// Observe the already computed physical insertion result. No new source,
// metadata, clock, callback, engine call, geometry limit or inventory permit.
unsigned feed_move_gate{};
#if defined(GEARS_PLAYGROUND_HB_FEED_OBSERVE) && GEARS_PLAYGROUND_HB_FEED_OBSERVE
struct FeedObservationState {
    unsigned rows[2]{},last_result[2]{999,999},rejects{},reject_rows{},moves{},move_rows{};
    int bucket[2]{-1000,-1000};
} feed_observation;
void observe_feed_result(hand::Frame const& f,std::uint64_t item,gears_weapon::FeedResult result,
    std::int64_t gap,bool rearmed,double depth,double lateral,double aligned) noexcept {
    if(!log_file||stage!=Stage::new_magazine||!std::isfinite(depth)||!std::isfinite(lateral)||
       lateral>=.05||depth<=-.02||depth>=.12)return;
    const unsigned region=depth<=.015?1u:0u;const int bucket=int(std::floor(depth*200));
    auto& s=feed_observation;
    const bool changed=bucket!=s.bucket[region]||unsigned(result)!=s.last_result[region]||rearmed;
    s.bucket[region]=bucket;s.last_result[region]=unsigned(result);
    if(!changed||s.rows[region]>=24)return;
    ++s.rows[region];
    fwprintf(log_file,L"hammerburst_feed_outcome region=%u frame=%llu sequence=%llu item=%llu result=%u gap_ns=%lld rearmed=%u depth=%.9g lateral=%.9g orientation_dot=%.9g sync_ns=%lld frame_now_ns=%lld\n",
        region,f.key.frame,f.controllers.sequence,item,unsigned(result),gap,unsigned(rearmed),depth,lateral,aligned,
        f.controllers.sync_ns,f.now_ns);
    fflush(log_file);
}
void observe_feed_rejection(hand::Frame const& f,unsigned gate) noexcept {
    if(stage!=Stage::new_magazine)return;
    auto& s=feed_observation;++s.rejects;
    if(!log_file||s.reject_rows>=12||(s.rejects>2&&s.rejects%32))return;
    ++s.reject_rows;
    fwprintf(log_file,L"hammerburst_feed_rejected gate=%u frame=%llu sequence=%llu sync_ns=%lld previous_sequence=%llu previous_sync_ns=%lld count=%u\n",
        gate,f.key.frame,f.controllers.sequence,f.controllers.sync_ns,feed_sequence,feed_ns,s.rejects);fflush(log_file);
}
void observe_move_rejection(hand::Frame const& f,hand::Decision const& decision,bool granted) noexcept {
    if(stage!=Stage::new_magazine)return;
    auto& s=feed_observation;++s.moves;
    if(!log_file||s.move_rows>=16||(s.moves>2&&s.moves%16))return;
    ++s.move_rows;
    fwprintf(log_file,L"hammerburst_feed_move_rejected gate=%u granted=%u frame=%llu sequence=%llu decision=%u lease_kind=%u lease_id=%llu token=%llu count=%u\n",
        granted?feed_move_gate:0,unsigned(granted),f.key.frame,f.controllers.sequence,unsigned(decision.status),
        unsigned(decision.lease.kind),decision.lease.id,f.press_token,s.moves);fflush(log_file);
}
#else
void observe_feed_result(hand::Frame const&,std::uint64_t,gears_weapon::FeedResult,std::int64_t,bool,double,double,double) noexcept {}
void observe_feed_rejection(hand::Frame const&,unsigned) noexcept {}
void observe_move_rejection(hand::Frame const&,hand::Decision const&,bool) noexcept {}
#endif
