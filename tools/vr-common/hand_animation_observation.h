#pragma once
// Bookkeeping only. Observing an event/poll never grants animation ownership.
#include <array>
#include <cstdint>

namespace gears_hand_animation {
enum class Source : unsigned { hook, bridge_invoke, call_function };
enum class Kind : unsigned {
    use_request, grab_pickup, dictate_special, do_special, end_special,
    special_start, special_stop, body_end, on_anim_end, body_play, body_stop,
    melee_request, melee_stop_request, melee_begin, melee_end,
    melee_state_begin, melee_state_end, slot_play, slot_stop, count
};
constexpr unsigned max_calls=256, max_depth=16, max_frames=1500;
constexpr std::uint64_t duration_ms=30000, sample_interval_ms=20;
struct Token {
    unsigned call{},parent{},depth{},pickup_request{},melee_request{};
    Kind kind{};Source source{};
    explicit operator bool()const{return call!=0;}
};
struct State {
    bool special_known{},melee_known{};
    std::uint8_t special{};bool melee{};
};
struct Transition {
    unsigned mask{},pickup_episode{},melee_episode{};
    State before{},after{};
};
class Ledger {
public:
    Token enter(Kind kind,Source source){
        if(unsigned(kind)>=unsigned(Kind::count)||unsigned(source)>unsigned(Source::call_function)||
           calls_>=max_calls||depth_>=max_depth){overflow_=true;return {};}
        const auto bit=std::uint64_t{1}<<unsigned(kind);
        (source==Source::hook?hook_mask_:source==Source::bridge_invoke?bridge_mask_:call_mask_)|=bit;
        if(kind==Kind::use_request)++pickup_request_;
        if(kind==Kind::melee_request)++melee_request_;
        Token token{++calls_,depth_?stack_[depth_-1]:0,depth_,
            pickup_request_,melee_request_,kind,source};
        stack_[depth_++]=token.call;
        return token;
    }
    bool leave(Token const& token){
        if(!token)return true;
        if(!depth_||stack_[depth_-1]!=token.call){overflow_=true;return false;}
        --depth_;return true;
    }
    Transition poll(State now){
        Transition result{};result.before=state_;result.after=now;
        if(now.special_known&&(!have_state_||!state_.special_known||state_.special!=now.special)){
            result.mask|=1;
            if(now.special==42||now.special==53)++pickup_episode_;
            ++special_changes_;
        }
        if(now.melee_known&&(!have_state_||!state_.melee_known||state_.melee!=now.melee)){
            result.mask|=2;
            if(now.melee)++melee_episode_;
            ++melee_changes_;
        }
        // Baseline already inside an action cannot prove its missing start.
        if(!have_state_&&((now.special_known&&now.special)||(now.melee_known&&now.melee)))
            started_active_=true;
        result.pickup_episode=pickup_episode_;result.melee_episode=melee_episode_;
        have_state_=true;state_=now;return result;
    }
    unsigned calls()const{return calls_;}
    unsigned depth()const{return depth_;}
    unsigned pickup_request()const{return pickup_request_;}
    unsigned melee_request()const{return melee_request_;}
    unsigned pickup_episode()const{return pickup_episode_;}
    unsigned melee_episode()const{return melee_episode_;}
    unsigned special_changes()const{return special_changes_;}
    unsigned melee_changes()const{return melee_changes_;}
    std::uint64_t hook_mask()const{return hook_mask_;}
    std::uint64_t bridge_mask()const{return bridge_mask_;}
    std::uint64_t call_mask()const{return call_mask_;}
    bool overflow()const{return overflow_;}
    bool started_active()const{return started_active_;}
    static constexpr bool ownership_proved(){return false;}
private:
    std::array<unsigned,max_depth> stack_{};
    unsigned calls_{},depth_{},pickup_request_{},melee_request_{};
    unsigned pickup_episode_{},melee_episode_{},special_changes_{},melee_changes_{};
    std::uint64_t hook_mask_{},bridge_mask_{},call_mask_{};
    State state_{};bool have_state_{},overflow_{},started_active_{};
};
} // namespace gears_hand_animation
