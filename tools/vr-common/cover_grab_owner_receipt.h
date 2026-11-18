#pragma once
#include "cover_input_owner.h"
#include <atomic>
#include <cstdint>

// Data-only bridge on one serialized owner GT. Native supplies its actual
// thread and semantic Scene/actor key; neither this helper nor its key grants
// engine admission. The owner must outlive the bridge. Other threads may only
// attempt read: its atomic thread gate precedes every non-atomic access.
namespace gears_cover_grab_receipt {
namespace vr=gears_vr;
struct Value {
    vr::CoverInputGrabSnapshot snapshot{};
    vr::LeftInteractionControl control{vr::LeftInteractionControl::grip};
};
template<class Key> class Store {
public:
    bool on_thread(std::uint32_t thread) const noexcept {return thread&&thread==thread_.load(std::memory_order_acquire);}
    void clear() noexcept {
        thread_.store(0,std::memory_order_release);
        owner_=nullptr;ticket_={};lifecycle_=frame_=0;
    }
    bool publish(vr::CoverInputOwner const& owner,vr::CoverInputOwnerTicket ticket,
        Key const& key,vr::LeftInteractionControl control,std::uint32_t thread,
        std::uint64_t lifecycle,std::uint64_t frame) noexcept {
        clear();vr::CoverInputGrabSnapshot check{};
        if(!thread||!lifecycle||!frame||
           (control!=vr::LeftInteractionControl::grip&&control!=vr::LeftInteractionControl::pinch)||
           !owner.grab_snapshot(ticket,check)||check.lifecycle!=lifecycle||check.frame!=frame)return false;
        owner_=&owner;ticket_=ticket;key_=key;control_=control;
        lifecycle_=lifecycle;frame_=frame;
        thread_.store(thread,std::memory_order_release);return true;
    }
    // Native may compare its immutable C0 parent source directly with the
    // saved admitted Scene key, without fabricating a new Scene receipt.
    template<class Bound> bool read_if(vr::CoverInputOwner const& owner,
        std::uint32_t thread,Bound bound,Value& out) const noexcept {
        if(!on_thread(thread))return false;
        if(owner_!=&owner||!bound(key_))return false;
        Value next{};
        if(!owner.grab_snapshot(ticket_,next.snapshot)||next.snapshot.lifecycle!=lifecycle_||
           next.snapshot.frame!=frame_)return false;
        next.control=control_;out=next;return true;
    }
    template<class Same> bool read(vr::CoverInputOwner const& owner,Key const& key,
        std::uint32_t thread,Same same,Value& out) const noexcept {
        if(!thread||thread!=thread_.load(std::memory_order_acquire))return false;
        if(owner_!=&owner||!same(key_,key))return false;
        Value next{};
        if(!owner.grab_snapshot(ticket_,next.snapshot)||next.snapshot.lifecycle!=lifecycle_||
           next.snapshot.frame!=frame_)return false;
        next.control=control_;out=next;return true;
    }
private:
    std::atomic<std::uint32_t> thread_{};
    vr::CoverInputOwner const* owner_{};vr::CoverInputOwnerTicket ticket_{};
    Key key_{};vr::LeftInteractionControl control_{};
    std::uint64_t lifecycle_{},frame_{};
};
} // namespace gears_cover_grab_receipt


