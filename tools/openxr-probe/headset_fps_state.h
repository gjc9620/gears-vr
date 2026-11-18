#pragma once
#include <cstdint>
#include <cstdio>
#include <limits>

// Accepted NEW game images in (now-1 second, now]. No extrapolation.
namespace gears_fps_display {
inline constexpr std::int64_t window_ns=1000000000;
inline constexpr unsigned capacity=1024;
struct Rate {
    std::uint64_t session{},generation{},last_delivery{};
    std::int64_t samples[capacity]{};
    unsigned head{},count{};
    std::int64_t measurement_started_ns{},last_new_frame_ns{},last_tick_ns{};
    bool visible{},available{},text_valid{true},overflow{};
    double fps{};
    char text[24]="--";
    void unavailable() noexcept {
        available=false;fps=0;text_valid=true;std::snprintf(text,sizeof text,"--");
    }
    void reset() noexcept {
        session=generation=last_delivery=0;head=count=0;
        measurement_started_ns=last_new_frame_ns=last_tick_ns=0;
        visible=overflow=false;unavailable();
    }
    void expire(std::int64_t now) noexcept {
        while(count&&now-samples[(head+capacity-count)%capacity]>=window_ns)--count;
    }
    bool observe(std::uint64_t timestamp,std::uint64_t session_id,std::uint64_t resource_generation,
                 std::uint64_t distinct) noexcept {
        if(!session_id||!resource_generation||!timestamp||timestamp>std::uint64_t((std::numeric_limits<std::int64_t>::max)()))return false;
        if(session!=session_id||generation!=resource_generation){reset();session=session_id;generation=resource_generation;}
        if(!distinct||distinct<=last_delivery)return false;
        const auto now=static_cast<std::int64_t>(timestamp);
        if(now<last_tick_ns){unavailable();return false;}
        // Missing counter increments have unknown event times: rewarm, never invent.
        if(visible&&distinct!=last_delivery+1){reset();session=session_id;generation=resource_generation;}
        if(!visible){visible=true;measurement_started_ns=now;}
        expire(now);
        if(count==capacity){overflow=true;unavailable();last_delivery=distinct;last_tick_ns=now;return false;}
        samples[head]=now;head=(head+1)%capacity;++count;
        last_delivery=distinct;last_new_frame_ns=last_tick_ns=now;
        render_text(timestamp,distinct);return true;
    }
    void render_text(std::uint64_t timestamp,std::uint64_t distinct) noexcept {
        if(!visible||!timestamp||timestamp>std::uint64_t((std::numeric_limits<std::int64_t>::max)())){unavailable();return;}
        const auto now=static_cast<std::int64_t>(timestamp);
        if(now<last_tick_ns||distinct!=last_delivery||overflow){unavailable();return;}
        last_tick_ns=now;expire(now);
        if(now-measurement_started_ns<window_ns){unavailable();return;}
        available=text_valid=true;fps=double(count);
        std::snprintf(text,sizeof text,"GAME %.1f FPS",fps);
    }
};
}
