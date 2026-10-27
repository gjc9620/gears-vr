#pragma once
#include <openxr/openxr.h>
#include <cstdint>
#include <cstdio>
#include "../vr-common/support_diagnostics.h"

// Render-thread-only, bounded observation. Presence and focus are independent;
// an unsupported extension or missing event is unknown, never "headset off".
namespace gears_focus {
struct Diagnostics {
    FILE* output{};
    std::uint64_t events{},written{},omitted{},frames{},focused_frames{};
    std::uint64_t state_frames[10]{};
    std::int64_t next_snapshot{},started_ns{};
    int state{},presence{-1};
    bool extension_enabled{},system_supported{},last_render{},final_written{};
    explicit Diagnostics(FILE* stream):output(stream){}
    void capabilities(XrResult result,bool advertised,bool enabled,bool supported,
                      const char* stage) {
        extension_enabled=enabled;system_supported=supported;
        gears_support::begin(output,"xr","xr_focus_capability");
        fprintf(output,"\"stage\":\"%s\",\"error_code\":%d,\"extension_advertised\":%s,"
            "\"presence_extension_enabled\":%s,\"presence_system_supported\":%s,"
            "\"presence_state\":%d,\"available\":%s}\n",stage,int(result),
            advertised?"true":"false",enabled?"true":"false",supported?"true":"false",
            presence,supported?"true":"false");fflush(output);
    }
    void event(XrStructureType type,bool matches,int next_state=-1,
               std::int64_t xr_time=0,int user_present=-1,unsigned lost=0) {
        ++events;const int previous=state;
        if(matches&&next_state>=0)state=next_state;
        if(matches&&extension_enabled&&system_supported&&user_present>=0)presence=user_present;
        if(written>=256){++omitted;return;}
        ++written;
        gears_support::begin(output,"xr","xr_focus_event");
        fprintf(output,"\"sequence\":%llu,\"event_type\":%d,\"session_matches\":%s,"
            "\"session_state\":%d,\"previous_session_state\":%d,\"reported_session_state\":%d,\"xr_event_time\":%lld,\"presence_state\":%d,"
            "\"events_lost\":%u,\"omitted\":%llu}\n",events,int(type),matches?"true":"false",
            state,previous,next_state,xr_time,presence,lost,omitted);fflush(output);
    }
    void call(const char* action,XrResult result) {
        if(written>=256){++omitted;return;}++written;
        gears_support::begin(output,"xr","xr_focus_event");
        fprintf(output,"\"action\":\"%s\",\"error_code\":%d,\"session_state\":%d,"
            "\"presence_state\":%d}\n",action,int(result),state,presence);fflush(output);
    }
    void snapshot(std::int64_t now,bool should_render,bool final=false) {
        if(final&&final_written)return;
        if(final)final_written=true;else last_render=should_render;
        if(!final){++frames;if(state==XR_SESSION_STATE_FOCUSED)++focused_frames;
            if(state>=0&&state<10)++state_frames[state];}
        if(!started_ns)started_ns=now;
        if(!final&&(now<next_snapshot||now-started_ns>120000000000LL))return;
        next_snapshot=now+1000000000LL;
        gears_support::begin(output,"xr","xr_focus_snapshot");
        fprintf(output,"\"final\":%s,\"session_state\":%d,\"should_render\":%s,"
            "\"presence_state\":%d,\"presence_extension_enabled\":%s,"
            "\"presence_system_supported\":%s,\"frames\":%llu,\"focused_frames\":%llu,"
            "\"visible_frames\":%llu,\"events_observed\":%llu,\"omitted\":%llu}\n",
            final?"true":"false",state,last_render?"true":"false",presence,
            extension_enabled?"true":"false",system_supported?"true":"false",frames,
            focused_frames,state_frames[XR_SESSION_STATE_VISIBLE],events,omitted);fflush(output);
    }
};
}
