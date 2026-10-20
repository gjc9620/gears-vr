#pragma once
#include "stereo_frame.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <new>

// Pure passive storage. Callers serialize native access with stereo_mutex;
// XR access stays on its render thread. No hooks, engine writes or pose edits.
namespace gears_sixdof {
struct Key {
    std::uint64_t pid{},start{},session{},world{},ticket{};
};
inline bool same(Key const& a,Key const& b) {
    return a.pid==b.pid&&a.start==b.start&&a.session==b.session&&a.world==b.world&&a.ticket==b.ticket;
}
inline bool valid(Key const& k){return k.pid&&k.start&&k.session&&k.world&&k.ticket;}
struct Stage {
    std::int64_t qpc{};unsigned reads{},count{};
    std::array<gears_vr::StereoMatrix,2> actual{};
};
struct NativeRow {
    Key key{};std::uint64_t reference_generation{},space_epoch{},sequence{};
    double units_per_metre{};
    gears_vr::Pose reference{},head{};
    std::array<gears_vr::Pose,2> eyes{};
    gears_vr::StereoMatrix source_view{};
    std::array<gears_vr::StereoMatrix,2> expected{};
    std::array<Stage,4> stages{};
    unsigned mask{},snapshot_index{};std::uint64_t source_serial{};
};
inline void print_key(FILE* f,Key const& k){
    fprintf(f,"\"game_pid\":%llu,\"game_start\":%llu,\"session\":%llu,\"world\":%llu,\"ticket\":%llu",
        (unsigned long long)k.pid,(unsigned long long)k.start,(unsigned long long)k.session,(unsigned long long)k.world,(unsigned long long)k.ticket);
}
inline void print_pose(FILE* f,gears_vr::Pose const& p){
    fprintf(f,"{\"position\":[%.17g,%.17g,%.17g],\"orientation\":[%.17g,%.17g,%.17g,%.17g]}",
        p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w);
}
inline void print_matrix(FILE* f,gears_vr::StereoMatrix const& m){
    fprintf(f,"[");for(unsigned i=0;i<16;++i){if(i)fprintf(f,",");
        if(std::isfinite(m[i]))fprintf(f,"%.9g",double(m[i]));else fprintf(f,"null");}fprintf(f,"]");
}
class NativeRecorder {
public:
    static constexpr unsigned recent_capacity=1024,pin_capacity=8;
private:
    std::unique_ptr<std::array<NativeRow,recent_capacity>> recent_;
    std::array<NativeRow,pin_capacity> pinned_{};
    std::array<Key,pin_capacity> requested_{};
    std::array<unsigned,pin_capacity> outcomes_{};
public:
    std::uint64_t evicted{},invalid{},duplicates{},allocation_failed{};
    bool observe(NativeRow const& row,unsigned stage,Stage const& observation) noexcept {
        if(stage>=4||!valid(row.key)){++invalid;return false;}
        if(!recent_){recent_.reset(new(std::nothrow) std::array<NativeRow,recent_capacity>{});if(!recent_){++allocation_failed;return false;}}
        auto& target=(*recent_)[row.key.ticket%recent_capacity];
        if(!same(target.key,row.key)){if(target.key.ticket)++evicted;target=row;target.mask=0;target.stages={};}
        auto update=[&](NativeRow& r){if(r.mask&(1u<<stage)){++duplicates;return;}
            r.stages[stage]=observation;r.mask|=1u<<stage;};
        update(target);
        for(auto& p:pinned_)if(same(p.key,row.key))update(p);
        return true;
    }
    // Slot ownership is append-only. No foreign/mismatched request can replace
    // a retained row. outcome: 1=exact pin, 2=not in recent window, 3=invalid.
    unsigned pin(unsigned slot,Key const& key,unsigned index,std::uint64_t serial) noexcept {
        if(slot>=pin_capacity||!valid(key)||!serial)return 3;
        if(outcomes_[slot])return same(requested_[slot],key)?outcomes_[slot]:3;
        requested_[slot]=key;
        if(!recent_){outcomes_[slot]=2;return 2;}
        auto const& row=(*recent_)[key.ticket%recent_capacity];
        if(!same(row.key,key)){outcomes_[slot]=2;return 2;}
        pinned_[slot]=row;pinned_[slot].snapshot_index=index;pinned_[slot].source_serial=serial;
        outcomes_[slot]=1;return 1;
    }
    NativeRow const* pinned(unsigned slot) const noexcept {return slot<pin_capacity&&outcomes_[slot]==1?&pinned_[slot]:nullptr;}
    void dump(FILE* f) const noexcept {
        fprintf(f,"{\"sixdof_native_summary\":true,\"schema\":1,\"recent_capacity\":%u,\"pin_capacity\":%u,\"evicted\":%llu,\"invalid\":%llu,\"duplicates\":%llu,\"allocation_failed\":%llu}\n",
            recent_capacity,pin_capacity,(unsigned long long)evicted,(unsigned long long)invalid,(unsigned long long)duplicates,(unsigned long long)allocation_failed);
        const char* names[]={"built","consume_before","prepare_before","prepare_after"};
        for(unsigned i=0;i<pin_capacity;++i){
            if(!outcomes_[i])continue;
            fprintf(f,"{\"sixdof_native_pin\":true,\"slot\":%u,\"outcome\":%u,",i,outcomes_[i]);print_key(f,requested_[i]);fprintf(f,"}\n");
            if(outcomes_[i]!=1)continue;auto const& r=pinned_[i];
            fprintf(f,"{\"sixdof_native_ticket\":true,");print_key(f,r.key);
            fprintf(f,",\"snapshot_index\":%u,\"source_serial\":%llu,\"reference_generation\":%llu,\"space_epoch\":%llu,\"sequence\":%llu,\"units_per_metre\":%.17g,\"stage_mask\":%u,\"reference\":",
                r.snapshot_index,(unsigned long long)r.source_serial,(unsigned long long)r.reference_generation,(unsigned long long)r.space_epoch,(unsigned long long)r.sequence,r.units_per_metre,r.mask);
            print_pose(f,r.reference);fprintf(f,",\"head\":");print_pose(f,r.head);
            fprintf(f,",\"source_view\":");print_matrix(f,r.source_view);fprintf(f,",\"eyes\":[");
            for(unsigned e=0;e<2;++e){if(e)fprintf(f,",");print_pose(f,r.eyes[e]);}fprintf(f,"]}\n");
            for(unsigned s=0;s<4;++s)if(r.mask&(1u<<s))for(unsigned e=0;e<2;++e){auto const& a=r.stages[s];
                fprintf(f,"{\"sixdof_native_stage\":true,");print_key(f,r.key);
                fprintf(f,",\"stage\":\"%s\",\"eye\":%u,\"qpc\":%lld,\"view_reads\":%u,\"family_count\":%u,\"expected\":",names[s],e,(long long)a.qpc,a.reads,a.count);
                print_matrix(f,r.expected[e]);fprintf(f,",\"actual\":");print_matrix(f,a.actual[e]);fprintf(f,"}\n");}
        }
    }
};
struct SubmittedRow {
    Key key{};unsigned snapshot_index{};std::uint64_t source_serial{},space_epoch{};
    std::int64_t ns{},display_time{};
    bool success{},local{};
    std::array<gears_vr::Pose,2> eyes{};
    std::array<gears_vr::StereoFov,2> fov{};
};
class SubmittedRecorder {
    std::array<SubmittedRow,8> rows_{};unsigned count_{};
public:
    unsigned overflow{};
    bool request(SubmittedRow const& r) noexcept {
        if(!valid(r.key)||!r.source_serial)return false;
        for(unsigned i=0;i<count_;++i)if(same(rows_[i].key,r.key)&&rows_[i].source_serial==r.source_serial)return true;
        if(count_==rows_.size()){++overflow;return false;}rows_[count_++]=r;return true;
    }
    void observe(SubmittedRow const& r) noexcept {
        if(!r.success)return;
        for(unsigned i=0;i<count_;++i)if(same(rows_[i].key,r.key)&&rows_[i].source_serial==r.source_serial&&!rows_[i].success){
            auto index=rows_[i].snapshot_index;rows_[i]=r;rows_[i].snapshot_index=index;}
    }
    SubmittedRow const* row(unsigned i) const noexcept{return i<count_?&rows_[i]:nullptr;}
    void dump(FILE* f) const noexcept {
        fprintf(f,"{\"sixdof_xr_summary\":true,\"schema\":1,\"capacity\":8,\"retained\":%u,\"overflow\":%u,\"semantics\":\"successful_xrEndFrame_not_display_confirmation\"}\n",count_,overflow);
        for(unsigned i=0;i<count_;++i){auto const& r=rows_[i];fprintf(f,"{\"sixdof_xr_submission\":true,");print_key(f,r.key);
            fprintf(f,",\"snapshot_index\":%u,\"source_serial\":%llu,\"submitted\":%s,\"local_space\":%s,\"space_epoch\":%llu,\"qpc_ns\":%lld,\"display_time\":%lld,\"eyes\":[",
                r.snapshot_index,(unsigned long long)r.source_serial,r.success?"true":"false",r.local?"true":"false",(unsigned long long)r.space_epoch,(long long)r.ns,(long long)r.display_time);
            for(unsigned e=0;e<2;++e){if(e)fprintf(f,",");fprintf(f,"{\"pose\":");print_pose(f,r.eyes[e]);auto const& v=r.fov[e];
                fprintf(f,",\"fov\":[%.17g,%.17g,%.17g,%.17g]}",v.left,v.right,v.up,v.down);}fprintf(f,"]}\n");}
    }
};
}
