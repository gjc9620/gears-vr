#pragma once
#include <array>
#include <atomic>
#include <cstdint>

namespace gears_outage {
struct Activity {std::uint64_t ticket{},count{},sequence{};std::int64_t qpc{},received_ns{};};
// Writer is already serialized by stereo_mutex. All reader-visible storage is
// atomic; read retries zero times and labels an in-progress snapshot unknown.
class ActivitySnapshot {
    std::atomic<std::uint64_t> version_{0},ticket_{0},count_{0},sequence_{0};
    std::atomic<std::int64_t> qpc_{0},received_ns_{0};
public:
    void write(Activity const& a){version_.fetch_add(1);
        ticket_.store(a.ticket);count_.store(a.count);
        sequence_.store(a.sequence);qpc_.store(a.qpc);received_ns_.store(a.received_ns);
        version_.fetch_add(1);}
    bool read(Activity& out) const {auto before=version_.load();if(!before||(before&1))return false;
        // All loads/stores, including both version reads, are seq_cst. In their
        // single total order, equal even versions exclude any intervening writer.
        Activity value{ticket_.load(),count_.load(),sequence_.load(),qpc_.load(),received_ns_.load()};
        if(version_.load()!=before)return false;out=value;return true;}
};
enum Valid:std::uint32_t {association=1,built_before=2,built_after=4,left_draw=8,right_draw=16,
    capture_called=32,image_sample=64,publication=128,present_return=256};
struct Sample {
    std::uint32_t valid{},rejection{},capture_reason{},capture_valid{},capture_state{},consumer{};
    std::int64_t enter_qpc{},original_begin_qpc{},original_end_qpc{},published_qpc{},previous_publish_qpc{};
    std::int32_t present_result{},capture_result{};
    std::uintptr_t buffer{},queue{};
    std::uint64_t ticket{},image_session{},image_sequence{};std::int64_t image_received_ns{};
    bool linked{},contaminated{},published{};
    Activity built_pre{},built_post{};std::array<Activity,2> draw{};
    std::uint64_t pub_before{},pub_after{},serial_before{},serial_after{},ready_completed{},done_completed{};
};
// Bit2 means last-publication -> post-original-Present age exceeds100ms;
// it is not proof that original Present alone took100ms. Use begin/end for that.
enum Trigger:unsigned {none=0,publication_gap=1,original_present_gap=2};
enum End:unsigned {open=0,recovered=1,capacity_full=2};
// Called only under the existing outer-Present ordering lock. One first outage,
// one preceding record, then recovery or capacity. No I/O/heap/waits/locks.
class Window {
    std::array<Sample,256> rows_{};Sample previous_{};bool has_previous_{},armed_{},triggered_{},finished_{};
    std::int64_t frequency_{},last_publish_qpc_{};unsigned count_{},trigger_{},end_{};
    bool over(std::int64_t later,std::int64_t earlier) const {return earlier>0&&later>earlier&&later-earlier>frequency_/10;}
public:
    void reset(std::int64_t frequency){rows_={};previous_={};has_previous_=armed_=triggered_=finished_=false;
        frequency_=frequency;last_publish_qpc_=0;count_=trigger_=end_=0;}
    void observe(Sample sample){
        if(finished_||frequency_<=0)return;
        sample.previous_publish_qpc=last_publish_qpc_;
        // Observe the interval up to capture/Present entry before replacing the
        // previous publication stamp. A successful recovery must not hide it.
        const auto publication_end=sample.published?sample.published_qpc:sample.original_end_qpc;
        const bool before_gap=armed_&&over(publication_end,last_publish_qpc_);
        if(sample.published){last_publish_qpc_=sample.published_qpc;armed_=true;}
        const bool after_gap=armed_&&over(sample.original_end_qpc,last_publish_qpc_);
        if(!triggered_&&(before_gap||after_gap)){
            triggered_=true;trigger_=(before_gap?publication_gap:0u)|(after_gap?original_present_gap:0u);
            if(has_previous_)rows_[count_++]=previous_;
        }
        if(triggered_){rows_[count_++]=sample;
            if(sample.published&&!after_gap){finished_=true;end_=recovered;}
            else if(count_==rows_.size()){finished_=true;end_=capacity_full;}}
        previous_=sample;has_previous_=true;
    }
    bool triggered()const{return triggered_;}bool finished()const{return finished_;}
    unsigned count()const{return count_;}unsigned trigger()const{return trigger_;}unsigned end()const{return end_;}
    std::int64_t frequency()const{return frequency_;}
    Sample const& at(unsigned index)const{return rows_[index];}
};
}
