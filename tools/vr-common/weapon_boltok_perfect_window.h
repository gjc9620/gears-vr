#pragma once
#include <cstdint>
#include <limits>

// Pure relative perfect-window metadata. No Frame, pointer, currentness claim,
// substituted clock or action permission is stored here.
namespace gears_boltok_perfect_window {

class ClosingWindow {
public:
    bool begin(std::uint64_t serial,std::int64_t opening_ms,
        std::int64_t native_window_ms) noexcept {
        reset();
        if(!serial||opening_ms<0||native_window_ms<=0)return false;
        serial_=serial;opening_ms_=opening_ms;native_window_ms_=native_window_ms;
        return true;
    }

    // The first authenticated alignment with the feed port opens one fixed,
    // nonrenewable native-duration window. Later alignment cannot move it.
    bool arm_first_alignment(std::uint64_t serial,std::int64_t aligned_ms) noexcept {
        if(!serial_||serial!=serial_||seen_||aligned_ms<opening_ms_||aligned_ms<0)
            return false;
        if(aligned_ms>(std::numeric_limits<std::int64_t>::max)()-native_window_ms_)
            return false;
        anchor_ms_=aligned_ms;
        relative_window_ms_=(aligned_ms-opening_ms_)+native_window_ms_;
        seen_=armed_=true;return true;
    }
    bool alignment_lost() noexcept {if(!armed_||inserted_)return false;armed_=false;return true;}
    bool seen_alignment() const noexcept {return seen_;}
    bool confirm_insertion(std::uint64_t serial,std::int64_t tick) noexcept {
        if(inserted_||serial!=serial_||tick<opening_ms_)return false;
        eligible_=valid_insert(serial,tick);inserted_=true;inserted_ms_=tick;return true;
    }
    bool valid_insert(std::uint64_t serial,std::int64_t insert_ms) const noexcept {
        return armed_&&serial==serial_&&insert_ms>=anchor_ms_&&
            insert_ms<=anchor_ms_+native_window_ms_;
    }

    // Supplies a relative duration to the existing classifier only after an
    // actual close at/after the first authenticated close-grab anchor. It awards nothing itself.
    std::int64_t window_for_close(std::uint64_t serial,
        std::int64_t actual_close_ms) const noexcept {
        if(!inserted_||!eligible_||serial!=serial_||actual_close_ms<inserted_ms_)return 0;
        // Insertion decides eligibility. Automatic motor/ACK latency must not
        // change a measured valid insertion into an ordinary completion.
        return actual_close_ms-opening_ms_;
    }

    void reset() noexcept {
        serial_=0;opening_ms_=0;native_window_ms_=0;
        anchor_ms_=0;relative_window_ms_=0;inserted_ms_=0;armed_=seen_=inserted_=eligible_=false;
    }

    bool armed() const noexcept {return armed_;}
    std::uint64_t serial() const noexcept {return serial_;}
    std::int64_t anchor_ms() const noexcept {return anchor_ms_;}
    std::int64_t relative_window_ms() const noexcept {return relative_window_ms_;}

private:
    std::uint64_t serial_{};
    std::int64_t opening_ms_{},native_window_ms_{};
    std::int64_t anchor_ms_{},relative_window_ms_{},inserted_ms_{};
    bool armed_{},seen_{},inserted_{},eligible_{};
};

} // namespace gears_boltok_perfect_window
