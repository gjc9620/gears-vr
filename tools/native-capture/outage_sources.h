#pragma once
#include <array>
#include <cstdint>

namespace gears_outage_sources {
struct Overlay {std::uint32_t flags{},rejection{};std::uint64_t ticket{};}; // flags: queried=1, linked=2, plan=4, chain=8
struct Draw {
    std::int64_t qpc{};std::uintptr_t list{},target{},caller{},material_payload{},material_command{},material_proxy{};
    std::uint64_t generation{},material_sequence{},material_invocation{};
    std::uint32_t kind{},elements{},instances{},width{},height{},material_kind{};
    Overlay overlay{};
};
struct Interval {
    std::int64_t qpc{},end_qpc{},published_qpc{};std::uintptr_t target{};
    std::array<std::uint64_t,2> eye_ticket{};
    std::uint64_t ticket{},unknowns{};std::uint32_t rejection{};bool published{};
    Draw first{},material{},overlay{};
};
struct Gap {
    std::int64_t previous_publish{},trigger_qpc{};bool recovered{};
    std::uint64_t intervals{};Interval first_unknown{},trigger{},last{};
    std::array<Interval,16> recent{};unsigned recent_count{};
};
// All calls use the caller's existing frame ordering lock. No locks, allocation,
// I/O or renderer pointers are owned here. Recent intervals and gap slots wrap;
// one long gap consumes one slot. No capture before the first publication.
class Recorder {
    std::array<Interval,16> recent_{};unsigned recent_next_{},recent_count_{};
    std::array<Gap,4> gaps_{};Gap longest_{};unsigned next_{},count_{};std::uint64_t total_{};
    std::int64_t frequency_{},last_publish_{};bool active_{};
    Interval pending_{},first_unknown_{};
    static std::int64_t duration(Gap const& gap){
        const auto end=gap.last.published?gap.last.published_qpc:gap.last.end_qpc;
        return end>gap.previous_publish?end-gap.previous_publish:0;
    }
public:
    void reset(std::int64_t frequency){*this=Recorder{};frequency_=frequency;}
    void unknown(Draw const& draw){
        ++pending_.unknowns;
        if(!pending_.first.qpc)pending_.first=draw;
        if(draw.material_sequence&&!pending_.material.qpc)pending_.material=draw;
        if((draw.overlay.flags&1)&&!pending_.overlay.qpc)pending_.overlay=draw;
    }
    Interval begin(std::int64_t qpc,std::array<std::uint64_t,2> eyes){
        auto out=pending_;pending_={};out.qpc=qpc;out.eye_ticket=eyes;return out;
    }
    void observe(Interval const& row){
        if(frequency_<=0)return;
        if(row.unknowns&&!first_unknown_.unknowns)first_unknown_=row;
        const auto end=row.published?row.published_qpc:row.end_qpc;
        const bool gap=last_publish_>0&&end>=last_publish_&&end-last_publish_>=frequency_/2;
        if(!active_&&gap){
            auto& out=gaps_[next_];out={};out.previous_publish=last_publish_;out.trigger_qpc=end;
            out.trigger=row;
            out.first_unknown=first_unknown_;out.recent_count=recent_count_;
            for(unsigned i=0;i<recent_count_;++i)out.recent[i]=recent_[(recent_next_+recent_.size()-recent_count_+i)%recent_.size()];
            active_=true;++total_;if(count_<gaps_.size())++count_;
        }
        if(active_){auto& out=gaps_[next_];++out.intervals;out.last=row;
            if(!out.first_unknown.unknowns&&row.unknowns)out.first_unknown=row;
            if(row.published){out.recovered=true;
                if(duration(out)>duration(longest_))longest_=out;
                active_=false;next_=(next_+1)%unsigned(gaps_.size());}}
        recent_[recent_next_]=row;recent_next_=(recent_next_+1)%unsigned(recent_.size());
        if(recent_count_<recent_.size())++recent_count_;
        if(row.published){last_publish_=row.published_qpc;first_unknown_={};}
    }
    unsigned count()const{return count_;}std::uint64_t total()const{return total_;}
    // Preserve the largest stall independently of later short stalls. Copy only
    // at recovery; an active long stall can be inspected directly after stop.
    bool has_longest()const{return total_!=0;}
    Gap const& longest()const{
        return active_&&duration(gaps_[next_])>duration(longest_)?gaps_[next_]:longest_;
    }
    Gap const& at(unsigned index)const{
        const auto oldest=count_<gaps_.size()?0u:(active_?(next_+1)%unsigned(gaps_.size()):next_);
        return gaps_[(oldest+index)%gaps_.size()];
    }
};
}
