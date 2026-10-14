#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>

namespace gears_frame_link {
using Identity = uintptr_t;
struct ListRef { Identity identity{}; uint64_t generation{}; };
struct Rect { uint32_t x{}, y{}, width{}, height{}; };
struct Target { Identity resource{}; uint32_t width{}, height{}; };
enum Rejection : uint32_t {
    missing_pair = 1, metadata_lost = 2, capacity = 4, queue_conflict = 8,
    ticket_conflict = 16, target_conflict = 32, duplicate_eye = 64,
    polluted = 128, unknown_list = 256, duplicate_execute = 512,
    bad_rectangle = 1024, stale_ticket = 2048
};
struct Result {
    bool linked{};
    bool contaminated{};
    uint64_t ticket{};
    Target target{};
    std::array<Rect, 2> eyes{};
    uint32_t rejection{};
    bool from_executed{};
    Identity source_queue{};
};
struct OverlayDiagnostic {
    ListRef current{};Target target{};uint64_t maximum_ticket{},retired_ticket{};
    unsigned executed_mask{},other_complete_recorded_lists{};
    uint64_t executed_ticket{},unique_recorded_ticket{};Identity executed_queue{};
    ListRef unique_recorded_ref{};
};
struct Snapshot {
    uint64_t reset_calls{},list_overflow{},event_overflow{},target_overflow{},execute_calls{},unknown_batches{},duplicate_batches{},invalid_batches{},metadata_batches{};
    size_t active_lists{},list_highwater{},event_highwater{};
    bool capacity_bad{};
    uint64_t overlay_recorded{},overlay_executed{},overlay_rejected{};
    uint64_t unknown_coalesced{};
    uint64_t overlay_from_executed_recorded{},overlay_from_executed_executed{};
    uint64_t overlay_local_queries{},overlay_executed_queries{},overlay_no_executed_queries{},
        overlay_invalid_list_queries{},overlay_window_rejected_queries{},overlay_capacity_queries{},
        overlay_polluted_queries{},overlay_ticket_queries{},overlay_target_queries{},overlay_incompatible_queries{};
    size_t overlay_no_executed_snapshot_count{};
    std::array<OverlayDiagnostic,8> overlay_no_executed_snapshots{};
};

// Conditional CPU association only: no pixel or GPU completion proof. Caller must
// supply stable canonical COM identities, actual ExecuteCommandLists members and
// a queue verified to belong to the swapchain. Call reset_success ONLY after a
// successful Reset. Draw rectangles are actual output rectangles, not source UVs.
// Clear/copy/indirect/bundle writes are NOT covered unless caller observes them
// through draw_unknown. Lost metadata must call invalidate(), never be skipped.
// Ticket IDs must be globally unique, monotonically increasing across presents.
// Current single-swapchain policy consumes ALL submissions and seals ALL recorded
// generations at every present attempt (including failed Present / TEST). This
// deliberately rejects other swapchains/in-flight buffers in that window.
// The caller may also seal an explicitly isolated original-Present interval.
// Such a boundary must hold the same ordering lock across original Present so
// work from other threads cannot enter the interval and then be discarded.
// All APIs are synchronized; caller must preserve actual API ordering when hooks
// run concurrently. No COM calls, reference ownership, heap allocation or eviction.
class Tracker {
public:
    static constexpr size_t list_capacity = 512;
    static constexpr size_t events_per_list = 32;
    static constexpr size_t target_capacity = 16;
private:
    struct Event { uint64_t ticket{}; unsigned eye{}; Target target{}; Rect rect{}; bool final{};
        bool overlay{};std::array<Rect,2> overlay_rects{};unsigned completed_mask{};uint64_t multiplicity{1};
        bool from_executed{};Identity expected_queue{}; };
    struct List {
        ListRef ref{};
        bool valid{}, executed{};
        size_t count{};
        std::array<Event, events_per_list> events{};
    };
    struct Candidate {
        Target target{};
        Identity queue{};
        uint64_t ticket{};
        std::array<Rect, 2> eyes{};
        unsigned mask{};
        bool rejected{}, contaminated{};
        uint32_t rejection{};
    };
    std::array<List, list_capacity> lists_{};
    std::array<Candidate, target_capacity> candidates_{};
    std::mutex mutex_;
    uint64_t generation_{}, maximum_ticket_{}, retired_ticket_{};
    bool window_bad_{}, capacity_bad_{};
    uint32_t window_rejection_{};
    Snapshot stats_{};
    List* find(ListRef ref) {
        for (auto& list : lists_)
            if (list.ref.identity == ref.identity && list.ref.generation == ref.generation && list.valid)
                return &list;
        return nullptr;
    }
    bool append(ListRef ref, Event event) {
        auto list = find(ref);
        if (!list || list->executed) { window_bad_ = true; window_rejection_ |= unknown_list; return false; }
        if (list->count == events_per_list) { ++stats_.event_overflow;capacity_bad_ = true; return false; }
        list->events[list->count++] = event;
        if(list->count>stats_.event_highwater)stats_.event_highwater=list->count;
        return true;
    }
    static bool valid_eye(unsigned eye, Target target, Rect rect) {
        if (eye > 1 || !target.resource || !target.width || !target.height || target.width % 2) return false;
        const auto half = target.width / 2;
        return rect.x == eye * half && rect.y == 0 && rect.width == half && rect.height == target.height;
    }
    static bool equal_rect(Rect a,Rect b) {
        return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height;
    }
    static bool same_overlay(Event const& e,uint64_t ticket,Target target,std::array<Rect,2> const& rects,unsigned mask,
        bool from_executed=false,Identity expected_queue=0) {
        return e.overlay&&e.multiplicity&&e.from_executed==from_executed&&e.expected_queue==expected_queue&&
            e.ticket==ticket&&e.target.resource==target.resource&&
            e.target.width==target.width&&e.target.height==target.height&&e.completed_mask==mask&&
            equal_rect(e.overlay_rects[0],rects[0])&&equal_rect(e.overlay_rects[1],rects[1]);
    }
    static bool checked_add(uint64_t& value,uint64_t amount) {
        if(!amount||amount>std::numeric_limits<uint64_t>::max()-value)return false;
        value+=amount;return true;
    }
    Result recorded_pair_locked(ListRef ref,Target target) {
        Result result{};auto list=find(ref);
        if(!list||list->executed||window_bad_||capacity_bad_||
            stats_.overlay_recorded==std::numeric_limits<uint64_t>::max())return result;
        unsigned mask=0;bool bad=false;
        for(unsigned i=0;i<list->count;++i){auto const& e=list->events[i];
            if(e.target.resource!=target.resource)continue;
            if(e.overlay){if(mask!=3||e.ticket!=result.ticket||e.completed_mask!=3||
                e.target.width!=target.width||e.target.height!=target.height||
                !valid_eye(0,target,e.overlay_rects[0])||!valid_eye(1,target,e.overlay_rects[1]))bad=true;}
            else if(!e.final){if(mask)bad=true;}
            else {if(e.target.width!=target.width||e.target.height!=target.height||
                !valid_eye(e.eye,target,e.rect)||!e.ticket||e.ticket<=retired_ticket_)bad=true;
                if(result.ticket&&result.ticket!=e.ticket)bad=true;
                result.ticket=e.ticket;result.target=e.target;
                if(mask&(1u<<e.eye))bad=true;mask|=1u<<e.eye;result.eyes[e.eye]=e.rect;}
        }
        // A ticket cannot own another output target in the same recording.
        for(unsigned i=0;i<list->count;++i){auto const& e=list->events[i];
            if(e.final&&e.ticket==result.ticket&&e.target.resource!=target.resource)bad=true;}
        for(auto const& candidate:candidates_)if(candidate.target.resource==target.resource&&
            (candidate.rejected||candidate.contaminated||candidate.ticket!=result.ticket||
             candidate.target.width!=target.width||candidate.target.height!=target.height))bad=true;
        if(list->count&&same_overlay(list->events[list->count-1],result.ticket,target,result.eyes,3)&&
            list->events[list->count-1].multiplicity==std::numeric_limits<uint64_t>::max())bad=true;
        // A full list admits replay only if the next exact pair can merge into
        // its last event. Never mutate GPU work first and discover capacity later.
        if(list->count==events_per_list&&
            !same_overlay(list->events[list->count-1],result.ticket,target,result.eyes,3))bad=true;
        result.linked=mask==3&&!bad;return result;
    }
    void no_executed_snapshot(ListRef ref,Target target,Candidate const* candidate) {
        if(stats_.overlay_no_executed_snapshot_count>=stats_.overlay_no_executed_snapshots.size())return;
        OverlayDiagnostic value{};value.current=ref;value.target=target;
        value.maximum_ticket=maximum_ticket_;value.retired_ticket=retired_ticket_;
        if(candidate){value.executed_mask=candidate->mask;value.executed_ticket=candidate->ticket;value.executed_queue=candidate->queue;}
        // Diagnostic only, first eight failures. Never select a recorded-only
        // pair as authorization or scan the entire inventory on every UI draw.
        for(auto const& other:lists_)if(other.valid&&!other.executed&&
            (other.ref.identity!=ref.identity||other.ref.generation!=ref.generation)){
            auto pair=recorded_pair_locked(other.ref,target);
            if(pair.linked){++value.other_complete_recorded_lists;
                value.unique_recorded_ref=other.ref;value.unique_recorded_ticket=pair.ticket;}
        }
        if(value.other_complete_recorded_lists!=1){value.unique_recorded_ref={};value.unique_recorded_ticket=0;}
        stats_.overlay_no_executed_snapshots[stats_.overlay_no_executed_snapshot_count++]=value;
    }
    Result overlay_pair_locked(ListRef ref,Target target,bool diagnostic) {
        auto local=recorded_pair_locked(ref,target);
        if(local.linked){if(diagnostic)++stats_.overlay_local_queries;return local;}
        Result result{};auto list=find(ref);
        if(!list||list->executed){if(diagnostic)++stats_.overlay_invalid_list_queries;result.rejection=unknown_list;return result;}
        if(window_bad_){if(diagnostic)++stats_.overlay_window_rejected_queries;result.rejection=metadata_lost|window_rejection_;return result;}
        if(capacity_bad_||stats_.overlay_recorded==std::numeric_limits<uint64_t>::max()||
            stats_.overlay_from_executed_recorded==std::numeric_limits<uint64_t>::max()){
            if(diagnostic)++stats_.overlay_capacity_queries;result.rejection=capacity;return result;}
        Candidate const* candidate{};
        for(auto const& value:candidates_)if(value.target.resource==target.resource){
            if(candidate){if(diagnostic)++stats_.overlay_target_queries;result.rejection=target_conflict;return result;}
            candidate=&value;
        }
        if(!candidate||candidate->mask!=3||!candidate->queue){
            if(diagnostic){++stats_.overlay_no_executed_queries;no_executed_snapshot(ref,target,candidate);}
            result.rejection=missing_pair;return result;
        }
        if(candidate->rejected||candidate->contaminated){if(diagnostic)++stats_.overlay_polluted_queries;
            result.rejection=polluted|candidate->rejection;return result;}
        // Select by exact target, not by a latest-ticket guess. Newer recorded
        // work or retirement disqualifies that executed target's ticket.
        if(!candidate->ticket||candidate->ticket<=retired_ticket_||candidate->ticket!=maximum_ticket_){
            if(diagnostic)++stats_.overlay_ticket_queries;result.rejection=stale_ticket|ticket_conflict;return result;}
        if(candidate->target.width!=target.width||candidate->target.height!=target.height||
            !valid_eye(0,target,candidate->eyes[0])||!valid_eye(1,target,candidate->eyes[1])){
            if(diagnostic)++stats_.overlay_target_queries;result.rejection=target_conflict|bad_rectangle;return result;}
        // An executed-world continuation cannot contain any speculative world
        // writes, unknown writes or another target, even before its first UI draw.
        for(unsigned i=0;i<list->count;++i){auto const& e=list->events[i];
            if(!same_overlay(e,candidate->ticket,target,candidate->eyes,3,true,candidate->queue)){
                if(diagnostic)++stats_.overlay_incompatible_queries;result.rejection=polluted;return result;}
        }
        const bool merge=list->count&&same_overlay(list->events[list->count-1],candidate->ticket,target,candidate->eyes,3,true,candidate->queue);
        if((list->count==events_per_list&&!merge)||(merge&&list->events[list->count-1].multiplicity==std::numeric_limits<uint64_t>::max())){
            if(diagnostic)++stats_.overlay_capacity_queries;result.rejection=capacity;return result;}
        result.linked=true;result.ticket=candidate->ticket;result.target=candidate->target;result.eyes=candidate->eyes;
        result.from_executed=true;result.source_queue=candidate->queue;
        if(diagnostic)++stats_.overlay_executed_queries;return result;
    }
    void submit(Identity queue, const Event& event) {
        if (!event.target.resource) { window_bad_ = true; return; }
        Candidate* candidate{};
        Candidate* empty{};
        for (auto& value : candidates_) {
            if (value.target.resource == event.target.resource) candidate = &value;
            if (!value.target.resource && !empty) empty = &value;
            // One ticket cannot name two actual output resources.
            if (event.final && value.ticket == event.ticket && value.target.resource &&
                value.target.resource != event.target.resource) {window_bad_ = true; window_rejection_ |= target_conflict;}
        }
        if(event.overlay){
            uint32_t rejection=0;
            if(event.from_executed&&(!event.expected_queue||event.expected_queue!=queue))rejection|=queue_conflict;
            if(!candidate||candidate->mask!=3)rejection|=missing_pair;
            if(event.completed_mask!=3||!valid_eye(0,event.target,event.overlay_rects[0])||
                !valid_eye(1,event.target,event.overlay_rects[1]))rejection|=bad_rectangle;
            if(candidate){
                if(candidate->queue!=queue)rejection|=queue_conflict;
                if(candidate->ticket!=event.ticket)rejection|=ticket_conflict;
                if(candidate->target.width!=event.target.width||candidate->target.height!=event.target.height)rejection|=target_conflict;
                if(candidate->contaminated||candidate->rejected)rejection|=polluted;
                candidate->rejection|=rejection;candidate->rejected=candidate->rejected||rejection!=0;
            }
            if(!event.multiplicity||(!rejection&&
                (event.multiplicity>std::numeric_limits<uint64_t>::max()-stats_.overlay_executed||
                (event.from_executed&&event.multiplicity>std::numeric_limits<uint64_t>::max()-stats_.overlay_from_executed_executed))))rejection|=capacity;
            if(!rejection){stats_.overlay_executed+=event.multiplicity;
                if(event.from_executed)stats_.overlay_from_executed_executed+=event.multiplicity;}
            if(rejection){checked_add(stats_.overlay_rejected,event.multiplicity);window_bad_=true;window_rejection_|=rejection;
                if(rejection&capacity)capacity_bad_=true;}
            return;
        }
        if (!event.final) {
            if (candidate && candidate->mask) {
                candidate->contaminated = true;
                candidate->rejected = true;
                candidate->rejection |= polluted;
            }
            return; // Normal writes before the first final eye are permitted.
        }
        if (!candidate) {
            if (!empty) { ++stats_.target_overflow;capacity_bad_ = true; return; }
            candidate = empty;
            candidate->target = event.target;
            candidate->queue = queue;
            candidate->ticket = event.ticket;
        }
        if (candidate->queue != queue) candidate->rejection |= queue_conflict;
        if (candidate->ticket != event.ticket) candidate->rejection |= ticket_conflict;
        if (candidate->target.width != event.target.width || candidate->target.height != event.target.height)
            candidate->rejection |= target_conflict;
        if (candidate->mask & (1u << event.eye)) candidate->rejection |= duplicate_eye;
        candidate->rejected = candidate->rejected || candidate->rejection != 0;
        candidate->eyes[event.eye] = event.rect;
        candidate->mask |= 1u << event.eye;
    }
public:
    Result recorded_pair(ListRef ref,Target target) {
        std::lock_guard<std::mutex> lock(mutex_);return recorded_pair_locked(ref,target);
    }
    // Conditional CPU Execute ordering, not pixel/GPU-completion proof.
    Result overlay_pair(ListRef ref,Target target) {
        std::lock_guard<std::mutex> lock(mutex_);return overlay_pair_locked(ref,target,true);
    }
    bool draw_overlay_pair(ListRef ref,uint64_t ticket,Target target,std::array<Rect,2> rects,unsigned completed_mask) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto list=find(ref);auto pair=overlay_pair_locked(ref,target,false);
        const bool merge=list&&!list->executed&&list->count&&
            same_overlay(list->events[list->count-1],ticket,target,rects,completed_mask,pair.from_executed,pair.source_queue);
        if(list&&list->count==events_per_list&&!merge){checked_add(stats_.overlay_rejected,1);++stats_.event_overflow;capacity_bad_=true;return false;}
        if(!pair.linked||pair.ticket!=ticket||completed_mask!=3||
            !valid_eye(0,target,rects[0])||!valid_eye(1,target,rects[1])){
            checked_add(stats_.overlay_rejected,1);window_bad_=true;window_rejection_|=!pair.linked?missing_pair:pair.ticket!=ticket?ticket_conflict:bad_rectangle;return false;}
        if(stats_.overlay_recorded==std::numeric_limits<uint64_t>::max()||
            (merge&&list->events[list->count-1].multiplicity==std::numeric_limits<uint64_t>::max())){
            checked_add(stats_.overlay_rejected,1);capacity_bad_=true;window_bad_=true;window_rejection_|=capacity;return false;}
        if(merge){++list->events[list->count-1].multiplicity;++stats_.overlay_recorded;
            if(pair.from_executed)++stats_.overlay_from_executed_recorded;return true;}
        Event event{};event.ticket=ticket;event.target=target;event.overlay=true;event.overlay_rects=rects;event.completed_mask=completed_mask;
        event.from_executed=pair.from_executed;event.expected_queue=pair.source_queue;
        const bool ok=append(ref,event);if(ok){++stats_.overlay_recorded;if(pair.from_executed)++stats_.overlay_from_executed_recorded;}
        else checked_add(stats_.overlay_rejected,1);return ok;
    }
    ListRef current(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& list : lists_) if (list.valid && list.ref.identity == identity) return list.ref;
        return {};
    }
    ListRef reset_success(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.reset_calls;
        if (!identity || generation_ == std::numeric_limits<uint64_t>::max()) {
            window_bad_ = true; return {};
        }
        List* slot{};
        for (auto& list : lists_) if (list.ref.identity == identity) { slot = &list; break; }
        if (!slot) for (auto& list : lists_) if (!list.ref.identity) { slot = &list; break; }
        if (!slot) { ++stats_.list_overflow;capacity_bad_ = true; return {}; }
        if(!slot->ref.identity){++stats_.active_lists;if(stats_.active_lists>stats_.list_highwater)stats_.list_highwater=stats_.active_lists;}
        // Reset discards unsubmitted old recording, while already submitted events
        // remain copied in candidates. Generation IDs are never reused.
        *slot = {};
        slot->ref = {identity, ++generation_};
        slot->valid = true;
        return slot->ref;
    }
    bool draw_final(ListRef ref, uint64_t ticket, unsigned eye, Target target, Rect rect) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ticket > maximum_ticket_) maximum_ticket_ = ticket;
        if (!ticket || ticket <= retired_ticket_ || !valid_eye(eye, target, rect)) {
            window_bad_ = true; window_rejection_ |= (!ticket || ticket <= retired_ticket_) ? stale_ticket : bad_rectangle; return false;
        }
        return append(ref, {ticket, eye, target, rect, true});
    }
    bool draw_unknown(ListRef ref, Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!resource) { window_bad_ = true; return false; }
        auto list=find(ref);
        if(list&&!list->executed&&list->count){
            auto& previous=list->events[list->count-1];
            // Repeated unknown writes to the same target have the same rejection
            // effect. Keep their position relative to eyes/overlays/other targets.
            // In particular, never turn an unknown write into an accepted overlay.
            if(!previous.final&&!previous.overlay&&previous.target.resource==resource){
                if(previous.multiplicity==std::numeric_limits<uint64_t>::max()||
                    stats_.unknown_coalesced==std::numeric_limits<uint64_t>::max()){
                    capacity_bad_=true;return false;
                }
                ++previous.multiplicity;++stats_.unknown_coalesced;return true;
            }
        }
        return append(ref, {0, 0, {resource, 0, 0}, {}, false});
    }
    bool execute(Identity queue, const ListRef* members, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.execute_calls;
        if (!queue || !members || !count || count > list_capacity) { ++stats_.invalid_batches;window_bad_ = true; return false; }
        // Validate entire batch before submission, including repeated members.
        for (size_t i = 0; i < count; ++i) {
            auto list = find(members[i]);
            if (!list || list->executed) { if(!list)++stats_.unknown_batches;else ++stats_.duplicate_batches;
                window_bad_ = true; window_rejection_ |= !list ? unknown_list : duplicate_execute; return false; }
            for (size_t j = 0; j < i; ++j)
                if (members[j].identity == members[i].identity) { ++stats_.duplicate_batches;window_bad_ = true; window_rejection_ |= duplicate_execute; return false; }
        }
        for (size_t i = 0; i < count; ++i) {
            auto list = find(members[i]);
            list->executed = true;
            for (size_t e = 0; e < list->count; ++e) submit(queue, list->events[e]);
        }
        if(window_bad_||capacity_bad_)++stats_.metadata_batches;
        return !window_bad_ && !capacity_bad_;
    }
    Result present(Identity backbuffer, Identity verified_swapchain_queue) {
        std::lock_guard<std::mutex> lock(mutex_);
        Result result{};
        result.rejection = window_rejection_ | (window_bad_ ? metadata_lost : 0u) | (capacity_bad_ ? capacity : 0u);
        for (const auto& candidate : candidates_) if (candidate.target.resource == backbuffer && backbuffer) {
            result.ticket = candidate.ticket;
            result.target = candidate.target;
            result.eyes = candidate.eyes;
            result.contaminated = candidate.contaminated;
            result.rejection |= candidate.rejection;
            if (candidate.contaminated) result.rejection |= polluted;
            if (candidate.mask != 3) result.rejection |= missing_pair;
            if (!verified_swapchain_queue || candidate.queue != verified_swapchain_queue) result.rejection |= queue_conflict;
            result.linked = !window_bad_ && !capacity_bad_ && !candidate.rejected && candidate.mask == 3 &&
                verified_swapchain_queue && candidate.queue == verified_swapchain_queue;
        }
        if (!result.ticket) result.rejection |= missing_pair;
        retired_ticket_ = maximum_ticket_;
        candidates_ = {};
        window_bad_ = false;
        window_rejection_ = 0;
        // Explicit consumption retires all generations, including unsubmitted
        // recordings. Reclaim their slots; require fresh Reset next window.
        for(auto& list:lists_)list={};stats_.active_lists=0;
        // Capacity faults remain sticky until explicit invalidate_all recovery.
        return result;
    }
    void invalidate() {
        std::lock_guard<std::mutex> lock(mutex_);
        window_bad_ = true;
    }
    // Discard only work recorded since the pre-Present consume boundary. This
    // preserves sticky capacity faults and ticket retirement; unlike recovery,
    // it permits no old generation or ticket to escape into the next window.
    void retire_present_interval() { (void)present(0,0); }
    void invalidate_capacity() {
        std::lock_guard<std::mutex> lock(mutex_);capacity_bad_=true;
    }
    void invalidate_list(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& list : lists_) if (list.ref.identity == identity) list.valid = false;
        window_bad_ = true;
    }
    // Recovery boundary: discard all tracked work and require fresh successful
    // Resets and newer tickets. The next present is intentionally rejected.
    void invalidate_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        for(auto& list:lists_)list={};
        candidates_ = {};
        retired_ticket_ = maximum_ticket_;
        capacity_bad_ = false;
        window_bad_ = true;
        stats_.active_lists=0;
    }
    Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex_);auto value=stats_;value.capacity_bad=capacity_bad_;return value;}
};
} // namespace gears_frame_link
