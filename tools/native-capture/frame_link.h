#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include "recording_capacity.h"

namespace gears_frame_link {
using Identity = uintptr_t;
struct ListRef { Identity identity{}; uint64_t generation{}; };
struct Rect { uint32_t x{}, y{}, width{}, height{}; };
struct Target { Identity resource{}; uint32_t width{}, height{}; };
enum Rejection : uint32_t {
    missing_pair = 1, metadata_lost = 2, capacity = 4, queue_conflict = 8,
    ticket_conflict = 16, target_conflict = 32, duplicate_eye = 64,
    polluted = 128, unknown_list = 256, duplicate_execute = 512,
    bad_rectangle = 1024, stale_ticket = 2048, owned_state_lost = 4096
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
    uint64_t owned_generation{};uint32_t final_state{};bool state_certified{};
};
struct ExecutionToken {uint64_t value{};};
struct OwnedStatus {bool enabled{},certified{};Target target{};Identity queue{};uint64_t generation{};uint32_t state{};};
struct OwnedRetirement {
    // revision covers owned-source mutations, not unrelated desktop work.
    uint64_t world_epoch{},revision{},ticket_cutoff{};
    OwnedStatus output{};
};
struct OverlayDiagnostic {
    ListRef current{};Target target{};uint64_t maximum_ticket{},retired_ticket{};
    unsigned executed_mask{},other_complete_recorded_lists{};
    uint64_t executed_ticket{},unique_recorded_ticket{};Identity executed_queue{};
    ListRef unique_recorded_ref{};
};
struct Snapshot {
    uint64_t world_epoch{1};
    uint64_t reset_calls{},list_creates{},list_overflow{},event_overflow{},target_overflow{},execute_calls{},unknown_batches{},duplicate_batches{},invalid_batches{},metadata_batches{};
    size_t active_lists{},list_highwater{},event_highwater{};
    bool capacity_bad{};
    uint64_t overlay_recorded{},overlay_executed{},overlay_rejected{};
    uint64_t unknown_coalesced{};
    uint64_t overlay_from_executed_recorded{},overlay_from_executed_executed{};
    uint64_t owned_births{},owned_birth_discarded_events{},owned_birth_discarded_candidates{},owned_consumed_frames{};
    uint64_t overlay_local_queries{},overlay_executed_queries{},overlay_no_executed_queries{},
        overlay_invalid_list_queries{},overlay_window_rejected_queries{},overlay_capacity_queries{},
        overlay_polluted_queries{},overlay_ticket_queries{},overlay_target_queries{},overlay_incompatible_queries{};
    size_t overlay_no_executed_snapshot_count{};
    std::array<OverlayDiagnostic,8> overlay_no_executed_snapshots{};
};

// Conditional CPU association only: no pixel or GPU completion proof. Caller must
// supply stable canonical COM identities, actual ExecuteCommandLists members and
// a queue verified to belong to the swapchain or configured owned output.
// Call reset_success ONLY after a
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
// Owned mode tracks explicit legacy barriers for exactly one normal texture.
// Caller owns native placement lifetime and certifies creation state, direct
// queue, complete barrier/write coverage and absence of implicit-state ambiguity.
// It must record_owned_barrier_fault for unsupported alias/UAV/enhanced coverage.
class Tracker {
public:
    static constexpr size_t list_capacity = gears_recording_capacity::command_lists;
    static constexpr size_t events_per_list = 32;
    static constexpr size_t target_capacity = 16;
private:
    struct Event { uint64_t ticket{}; unsigned eye{}; Target target{}; Rect rect{}; bool final{};
        bool overlay{};std::array<Rect,2> overlay_rects{};unsigned completed_mask{};uint64_t multiplicity{1};
        bool from_executed{};Identity expected_queue{};
        bool transfer{};Target transfer_source{};
        unsigned state_kind{};uint64_t output_generation{};uint32_t before{},after{},subresource{},flags{}; };
    struct List {
        ListRef ref{};
        uint64_t world_epoch{};
        bool valid{}, executed{};
        size_t count{};
        std::array<Event, events_per_list> events{};
    };
    // Events are read only below count, and append writes a complete Event.
    // Retiring a recording only needs to clear the live header.
    static void clear_list(List& list) {
        list.ref={};list.world_epoch=0;
        list.valid=false;list.executed=false;list.count=0;
    }
    struct Candidate {
        Target target{};
        Identity queue{};
        uint64_t ticket{};
        std::array<Rect, 2> eyes{};
        unsigned mask{};
        bool rejected{}, contaminated{};
        uint32_t rejection{};
        Identity transfer_source{};
    };
    std::array<List, list_capacity> lists_{};
    // Only the prefix is logically live. Seal/recovery drops it in O(1);
    // slots appended into a later window are reset before any field is read.
    // Holes inside the prefix retain the original reuse order.
    size_t used_prefix_{};
    template<class T> struct ListRange {T* first;T* last;T* begin() const{return first;}T* end() const{return last;}};
    ListRange<List> used_lists(){return {lists_.data(),lists_.data()+used_prefix_};}
    ListRange<const List> used_lists() const{return {lists_.data(),lists_.data()+used_prefix_};}
    // Advisory identity index. Every hit checks the live slot; reset, owned
    // consumption and world sealing may reuse slots without cache maintenance.
    static constexpr size_t list_cache_capacity=2048;
    struct ListCacheEntry {Identity identity{};size_t index{};};
    std::array<ListCacheEntry,list_cache_capacity> list_cache_{};
    static size_t list_cache_bucket(Identity identity){
        const auto value=static_cast<uint64_t>(identity)>>4; // COM pointers are aligned.
        return static_cast<size_t>((value*11400714819323198485ull)>>(64-11));
    }
    List* cached_list(Identity identity){
        if(!identity)return nullptr;
        const auto& entry=list_cache_[list_cache_bucket(identity)];
        return entry.identity==identity&&entry.index<used_prefix_&&lists_[entry.index].ref.identity==identity
            ?&lists_[entry.index]:nullptr;
    }
    void cache_list(List* list){
        if(list&&list->ref.identity)list_cache_[list_cache_bucket(list->ref.identity)]={list->ref.identity,
            static_cast<size_t>(list-lists_.data())};
    }
    List* find_identity(Identity identity){
        if(auto* list=cached_list(identity))return list;
        for(auto& list:used_lists())if(list.ref.identity==identity){cache_list(&list);return &list;}
        return nullptr;
    }
    std::array<Candidate, target_capacity> candidates_{};
    std::mutex mutex_;
    uint64_t generation_{}, maximum_ticket_{}, retired_ticket_{};
    uint64_t world_epoch_{1};
    bool window_bad_{}, capacity_bad_{};
    uint32_t window_rejection_{};
    Snapshot stats_{};
    OwnedStatus owned_{};
    uint64_t retired_owned_epoch_{},retired_owned_generation_{},owned_revision_{};
    uint64_t revision_{},execution_sequence_{};
    struct Pending {bool active{};uint64_t token{},revision{};Identity queue{};size_t count{};uint32_t state{};
        std::array<ListRef,list_capacity> members{};} pending_{};
    void change_owned(){if(owned_.enabled){if(owned_revision_==UINT64_MAX){capacity_bad_=true;owned_.certified=false;}else ++owned_revision_;}}
    bool owned_event(Event const& event) const {
        return owned_.enabled&&(event.state_kind||event.target.resource==owned_.target.resource||event.transfer_source.resource==owned_.target.resource);
    }
    bool owned_events(List const& list) const {
        for(size_t i=0;i<list.count;++i)if(owned_event(list.events[i]))return true;
        return false;
    }
    void lose_owned(){if(owned_.enabled){change_owned();owned_.certified=false;window_bad_=true;window_rejection_|=owned_state_lost;}pending_.active=false;}
    void discard_recording(List const& list){
        if(!owned_.enabled||list.executed)return;
        if(owned_events(list))change_owned();
        for(size_t i=0;i<list.count;++i)
            if(list.events[i].overlay&&list.events[i].target.resource==owned_.target.resource){lose_owned();return;}
    }
    bool owned_retirement_ready_locked() const {
        if(!owned_.enabled||!owned_.certified||pending_.active||window_bad_||capacity_bad_||revision_==UINT64_MAX||owned_revision_==UINT64_MAX)return false;
        for(auto const& list:used_lists())if(!list.executed)
            for(size_t i=0;i<list.count;++i){auto const& event=list.events[i];
                if(event.target.resource==owned_.target.resource||event.transfer_source.resource==owned_.target.resource)return false;}
        return true;
    }
    bool owned_retirement_current_locked(OwnedRetirement const& receipt) const {
        auto const& expected=receipt.output;
        return owned_retirement_ready_locked()&&receipt.world_epoch==world_epoch_&&receipt.revision==owned_revision_&&
            receipt.ticket_cutoff==maximum_ticket_&&expected.enabled&&expected.certified&&
            expected.target.resource==owned_.target.resource&&expected.target.width==owned_.target.width&&
            expected.target.height==owned_.target.height&&expected.queue==owned_.queue&&
            expected.generation==owned_.generation&&expected.state==owned_.state;
    }
    static bool color_state(uint32_t state){return state==0||state==4||state==128||state==1024||state==2048||state==4096||state==8192;}
    bool transition_supported(Event const& event) const {
        return event.state_kind==1&&event.target.resource==owned_.target.resource&&event.output_generation==owned_.generation&&
            !event.flags&&(event.subresource==0||event.subresource==UINT32_MAX)&&color_state(event.before)&&color_state(event.after);
    }
    bool validate_members(Identity queue,const ListRef* members,size_t count) {
        if(!queue||!members||!count||count>list_capacity){++stats_.invalid_batches;window_bad_=true;return false;}
        for(size_t i=0;i<count;++i){auto list=find(members[i]);
            if(!list||list->executed){if(!list)++stats_.unknown_batches;else ++stats_.duplicate_batches;
                window_bad_=true;window_rejection_|=!list?unknown_list:duplicate_execute;return false;}
            for(size_t j=0;j<i;++j)if(members[j].identity==members[i].identity){++stats_.duplicate_batches;
                window_bad_=true;window_rejection_|=duplicate_execute;return false;}}
        return true;
    }
    bool apply_state(Event const& event,uint32_t& state) const {
        if(!event.state_kind){
            if(event.target.resource!=owned_.target.resource||(!event.final&&!event.overlay))return true;
            return state==4&&event.target.width==owned_.target.width&&event.target.height==owned_.target.height;
        }
        if(!transition_supported(event)||event.before!=state)return false;
        state=event.after;return true;
    }
    List* find(ListRef ref) {
        if(auto* list=cached_list(ref.identity))
            if(list->ref.generation==ref.generation&&list->valid&&list->world_epoch==world_epoch_)return list;
        for (auto& list : used_lists())
            if (list.ref.identity == ref.identity && list.ref.generation == ref.generation && list.valid && list.world_epoch==world_epoch_){
                cache_list(&list);
                return &list;
            }
        return nullptr;
    }
    bool append(ListRef ref, Event event) {
        auto list = find(ref);
        if (!list || list->executed) { window_bad_ = true; window_rejection_ |= unknown_list; return false; }
        if (list->count == events_per_list) { ++stats_.event_overflow;capacity_bad_ = true; return false; }
        list->events[list->count++] = event;
        ++revision_;
        if(owned_event(event))change_owned();
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
    Result recorded_pair_locked(ListRef ref,Target target,size_t end=events_per_list) {
        Result result{};auto list=find(ref);
        if(!list||list->executed||window_bad_||capacity_bad_||(owned_.enabled&&!owned_.certified)||
            stats_.overlay_recorded==std::numeric_limits<uint64_t>::max())return result;
        const size_t count=list->count<end?list->count:end;
        // Exactly one explicit transfer per recording. Its source must have a
        // complete local pair BEFORE the transfer, never an executed fallback.
        for(size_t i=0;i<count;++i)if(list->events[i].transfer){
            auto const& transfer=list->events[i];
            if(target.resource==transfer.transfer_source.resource)return result;
            if(target.resource!=transfer.target.resource)continue;
            auto source=recorded_pair_locked(ref,transfer.transfer_source,i);
            if(!source.linked||source.ticket!=transfer.ticket||target.width!=transfer.target.width||
               target.height!=transfer.target.height)return result;
            for(size_t j=0;j<count;++j){auto const& e=list->events[j];
                if(j==i)continue;
                if(j<i&&e.target.resource==target.resource)return result;
                if(j>i&&(e.target.resource==transfer.transfer_source.resource||e.transfer))return result;
                if(j>i&&e.target.resource==target.resource&&
                   !same_overlay(e,transfer.ticket,target,transfer.overlay_rects,3))return result;
                if(e.final&&e.ticket==transfer.ticket&&e.target.resource!=transfer.transfer_source.resource)return result;
            }
            for(auto const& candidate:candidates_)if(candidate.target.resource==target.resource)return result;
            const bool merge=count&&same_overlay(list->events[count-1],transfer.ticket,target,transfer.overlay_rects,3);
            if((count==events_per_list&&!merge)||(merge&&list->events[count-1].multiplicity==UINT64_MAX))return result;
            result.linked=true;result.ticket=transfer.ticket;result.target=target;result.eyes=transfer.overlay_rects;return result;
        }
        unsigned mask=0;bool bad=false;
        for(unsigned i=0;i<count;++i){auto const& e=list->events[i];
            if(e.target.resource!=target.resource)continue;
            if(e.state_kind){if(!transition_supported(e))bad=true;continue;}
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
        for(unsigned i=0;i<count;++i){auto const& e=list->events[i];
            if(e.final&&e.ticket==result.ticket&&e.target.resource!=target.resource)bad=true;}
        for(auto const& candidate:candidates_)if(candidate.target.resource==target.resource&&
            (candidate.rejected||candidate.contaminated||candidate.ticket!=result.ticket||
             candidate.target.width!=target.width||candidate.target.height!=target.height))bad=true;
        if(count&&same_overlay(list->events[count-1],result.ticket,target,result.eyes,3)&&
            list->events[count-1].multiplicity==std::numeric_limits<uint64_t>::max())bad=true;
        // A full list admits replay only if the next exact pair can merge into
        // its last event. Never mutate GPU work first and discover capacity later.
        if(count==events_per_list&&
            !same_overlay(list->events[count-1],result.ticket,target,result.eyes,3))bad=true;
        result.linked=mask==3&&!bad;return result;
    }
    void no_executed_snapshot(ListRef ref,Target target,Candidate const* candidate) {
        if(stats_.overlay_no_executed_snapshot_count>=stats_.overlay_no_executed_snapshots.size())return;
        OverlayDiagnostic value{};value.current=ref;value.target=target;
        value.maximum_ticket=maximum_ticket_;value.retired_ticket=retired_ticket_;
        if(candidate){value.executed_mask=candidate->mask;value.executed_ticket=candidate->ticket;value.executed_queue=candidate->queue;}
        // Diagnostic only, first eight failures. Never select a recorded-only
        // pair as authorization or scan the entire inventory on every UI draw.
        for(auto const& other:used_lists())if(other.valid&&!other.executed&&
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
            if(e.state_kind&&transition_supported(e))continue;
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
        if(event.state_kind)return; // State is staged/committed separately in this same event order.
        if (!event.target.resource) { window_bad_ = true; return; }
        if(event.transfer){
            Candidate* source{};bool destination_used=false;
            for(auto& value:candidates_){
                if(value.target.resource==event.transfer_source.resource)source=&value;
                if(value.target.resource==event.target.resource)destination_used=true;
            }
            uint32_t rejection=0;
            if(owned_.enabled||destination_used)rejection|=target_conflict;
            if(!source||source->mask!=3)rejection|=missing_pair;
            if(source){
                if(source->queue!=queue)rejection|=queue_conflict;
                if(source->ticket!=event.ticket||event.ticket<=retired_ticket_)rejection|=ticket_conflict;
                if(source->transfer_source||source->target.width!=event.transfer_source.width||
                   source->target.height!=event.transfer_source.height)rejection|=target_conflict;
                if(source->rejected||source->contaminated)rejection|=polluted;
            }
            // Unknown destination writes are never silently reclassified as
            // this transfer, even when recorded before its full-image draw.
            for(auto const& list:used_lists())if(list.executed)for(size_t i=0;i<list.count;++i){auto const& e=list.events[i];
                if(e.target.resource==event.target.resource&&!e.final&&!e.overlay&&!e.transfer&&!e.state_kind)rejection|=polluted;
            }
            if(rejection){window_bad_=true;window_rejection_|=rejection;return;}
            source->transfer_source=source->target.resource;source->target=event.target;source->eyes=event.overlay_rects;
            return;
        }
        Candidate* candidate{};
        Candidate* empty{};
        for (auto& value : candidates_) {
            if(value.transfer_source==event.target.resource){value.rejected=value.contaminated=true;value.rejection|=polluted;}
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
    // Caller supplies canonical identities, DIRECT queue proof and a certified
    // creation state for one normal single-subresource color texture. Never seed
    // from an engine cache. Configure BEFORE tracking is enabled / any Reset or
    // Execute is observed. Actual RHI allocation/activation ordering belongs to
    // integration, not this helper. One generation; no recovery/replacement API.
    bool configure_owned_output(Target target,Identity queue,uint64_t generation,uint32_t initial_state,bool certified_single_normal_color) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(owned_.enabled||retired_owned_epoch_||!certified_single_normal_color||!target.resource||!queue||!generation||
           !target.width||target.width%2||!target.height||target.width>8192||target.height>8192||
           (initial_state!=4&&initial_state!=128)||stats_.active_lists||stats_.reset_calls||stats_.list_creates||stats_.execute_calls||
           maximum_ticket_||window_bad_||capacity_bad_||pending_.active)return false;
        owned_={true,true,target,queue,generation,initial_state};++revision_;change_owned();return true;
    }
    // Explicit resource-creation boundary for a tracker with real Reset history.
    // Caller must hold the SAME API-order lock used across Reset, recording,
    // original Execute + commit and Present. No original Execute may be in
    // flight (legacy execute() cannot itself detect that external interval).
    // The newly allocated native owner/resource has NEVER been referenced by
    // recorded/submitted GPU commands. CPU-only adapter metadata queries may
    // precede birth, provided the caller observes and rejects every GPU use.
    // Its canonical identity, native
    // placement lifetime, DIRECT queue, descriptor and RT/PSR creation state are
    // externally certified; a reused address alone proves none of these facts.
    // Both proof flags are mandatory. Replacement requires explicit certified
    // retirement, a later world epoch and a strictly newer resource generation;
    // birth alone cannot recover or replace a still-enabled output.
    // Discard only association metadata predating birth, not actual GPU work.
    // Preserve every list's real generation, validity and executed marker:
    // existing unsubmitted lists may continue without inventing a Reset; already
    // executed lists still require a real Reset. Old tickets are retired and the
    // caller must begin a strictly newer frozen ticket for this output.
    bool birth_owned_output(Target target,Identity queue,uint64_t generation,uint32_t initial_state,
                            bool certified_single_normal_color,bool certified_unexposed_ordered_birth) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(owned_.enabled||(retired_owned_epoch_&&world_epoch_<=retired_owned_epoch_)||generation<=retired_owned_generation_||
           !certified_single_normal_color||!certified_unexposed_ordered_birth||
           !target.resource||!queue||!generation||!target.width||target.width%2||!target.height||
           target.width>8192||target.height>8192||(initial_state!=4&&initial_state!=128)||
           window_bad_||capacity_bad_||pending_.active||revision_==UINT64_MAX)return false;
        // Detect contradictions to the birth proof before modifying anything.
        uint64_t discarded_events{},discarded_candidates{};
        for(auto const& list:used_lists()){
            for(size_t i=0;i<list.count;++i){
                if(list.events[i].target.resource==target.resource)return false;
                ++discarded_events;
            }
        }
        for(auto const& candidate:candidates_)if(candidate.target.resource){
            if(candidate.target.resource==target.resource)return false;
            ++discarded_candidates;
        }
        for(auto& list:used_lists())list.count=0;
        candidates_={};retired_ticket_=maximum_ticket_;
        stats_.owned_births=1;
        stats_.owned_birth_discarded_events=discarded_events; // Event records, not multiplicity/GPU draws.
        stats_.owned_birth_discarded_candidates=discarded_candidates;
        owned_={true,true,target,queue,generation,initial_state};++revision_;change_owned();return true;
    }
    OwnedStatus owned_status(){std::lock_guard<std::mutex> lock(mutex_);return owned_;}
    // Capture BEFORE the caller's independent CPU/GPU retirement procedure,
    // under the same API-order lock. This snapshot certifies no GPU completion.
    // Failure leaves out untouched. Pending Execute and even barrier-only owned
    // recordings block retirement; unrelated recordings need not be submitted.
    bool owned_retirement_snapshot(OwnedRetirement& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!owned_retirement_ready_locked())return false;
        out={world_epoch_,owned_revision_,maximum_ticket_,owned_};return true;
    }
    // Recheck the ORIGINAL pre-fence snapshot before irreversible owner release.
    // Does not refresh its revision or substitute a new snapshot for an old fence.
    // Unrelated Reset/Execute may continue; source writes, state observations,
    // discarded unsubmitted source recordings and consumption invalidate it.
    bool owned_retirement_current(OwnedRetirement const& receipt) {
        std::lock_guard<std::mutex> lock(mutex_);return owned_retirement_current_locked(receipt);
    }
    // Caller has frozen old-world producers, drained CPU payload consumers,
    // prevented any future use of this owner/resource and verified completion
    // of ALL GPU users with an actual ordered fence (including external copies).
    // Keep that admission freeze and API-order lock through this call and world
    // commit. A boolean alone is not evidence: the host owns the full receipt.
    // Refuse changed observations instead of relabeling them as already drained.
    // This is a memory-only transition, not a resource release or state recovery.
    bool retire_owned_output(OwnedRetirement const& receipt,bool certified_cpu_gpu_retired) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!certified_cpu_gpu_retired||!owned_retirement_current_locked(receipt))return false;
        retired_owned_epoch_=world_epoch_;retired_owned_generation_=owned_.generation;
        owned_.enabled=false;owned_.certified=false;
        retired_ticket_=maximum_ticket_;candidates_={};++revision_;
        // Seal every old recording immediately, without manufacturing Reset or
        // Execute. Keep canonical identities/generations/validity/executed bits;
        // epoch zero is never admitted by find(). Only real Reset/Create can
        // restore admission. Clear old associations so a genuinely new resource
        // may reuse an address after the separately certified destruction.
        for(auto& list:used_lists()){list.count=0;list.world_epoch=0;}
        return true;
    }
    // Waiting hint only; never consumes evidence or changes certification.
    // Any active submission prevents consumption, even with unrelated members.
    // Caller must retain the API-order lock through its later consume decision.
    bool has_unsubmitted_owned_events(Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!resource||!owned_.enabled||resource!=owned_.target.resource)return false;
        if(pending_.active)return true;
        for(auto const& list:used_lists())if(!list.executed)
            for(size_t i=0;i<list.count;++i)if(list.events[i].target.resource==resource)return true;
        return false;
    }
    // Record, never advance submitted state. Invalid split/subresource/state
    // events become certification failures only if this recording is submitted.
    bool record_transition(ListRef ref,Identity resource,uint64_t generation,uint32_t before,uint32_t after,
        uint32_t subresource=UINT32_MAX,uint32_t flags=0) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!owned_.enabled)return false;
        Event event{};event.target.resource=resource;event.state_kind=1;event.output_generation=generation;
        event.before=before;event.after=after;event.subresource=subresource;event.flags=flags;
        return append(ref,event);
    }
    // Alias (including wildcard), UAV, enhanced/unknown coverage: no guessing.
    bool record_owned_barrier_fault(ListRef ref,uint64_t generation) {
        std::lock_guard<std::mutex> lock(mutex_);if(!owned_.enabled)return false;
        Event event{};event.target.resource=owned_.target.resource;event.state_kind=2;event.output_generation=generation;
        return append(ref,event);
    }
    // Under the caller's existing API-order lock: prepare, forward the ORIGINAL
    // ExecuteCommandLists exactly once, then commit before unlocking. This is
    // submitted queue order, not GPU completion. No GPU waits are performed.
    bool prepare_execute(Identity queue,const ListRef* members,size_t count,ExecutionToken& out) {
        std::lock_guard<std::mutex> lock(mutex_);++stats_.execute_calls;
        if(!owned_.enabled)return false;
        if(pending_.active||!owned_.certified||queue!=owned_.queue||window_bad_||capacity_bad_||
           execution_sequence_==UINT64_MAX||!validate_members(queue,members,count)){lose_owned();return false;}
        auto state=owned_.state;
        for(size_t i=0;i<count;++i){auto list=find(members[i]);
            for(size_t j=0;j<list->count;++j)if(!apply_state(list->events[j],state)){lose_owned();return false;}}
        pending_={};pending_.active=true;pending_.token=++execution_sequence_;pending_.revision=revision_;
        pending_.queue=queue;pending_.count=count;pending_.state=state;
        for(size_t i=0;i<count;++i)pending_.members[i]=members[i];
        out.value=pending_.token;return true;
    }
    bool commit_execute(ExecutionToken token) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!owned_.enabled||!owned_.certified||!pending_.active||token.value!=pending_.token||
           revision_!=pending_.revision||window_bad_||capacity_bad_){lose_owned();return false;}
        // Revalidate membership against Reset/reentrant changes, and apply the
        // same retained event values after the real queue submission returned.
        if(!validate_members(pending_.queue,pending_.members.data(),pending_.count)){lose_owned();return false;}
        for(size_t i=0;i<pending_.count;++i){auto list=find(pending_.members[i]);if(owned_events(*list))change_owned();list->executed=true;
            for(size_t j=0;j<list->count;++j)submit(pending_.queue,list->events[j]);}
        owned_.state=pending_.state;pending_.active=false;++revision_;
        if(window_bad_||capacity_bad_){++stats_.metadata_batches;lose_owned();return false;}return true;
    }
    Result recorded_pair(ListRef ref,Target target) {
        std::lock_guard<std::mutex> lock(mutex_);return recorded_pair_locked(ref,target);
    }
    // Before a material write to an intermediate transfer source, require the
    // current ticket in this recording. Never borrow an executed continuation.
    Result current_recorded_pair(ListRef ref,Target target) {
        std::lock_guard<std::mutex> lock(mutex_);auto result=recorded_pair_locked(ref,target);
        if(!result.ticket||result.ticket!=maximum_ticket_){result.linked=false;result.rejection|=ticket_conflict;}
        return result;
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
        if(merge){++revision_;if(owned_event(list->events[list->count-1]))change_owned();++list->events[list->count-1].multiplicity;++stats_.overlay_recorded;
            if(pair.from_executed)++stats_.overlay_from_executed_recorded;return true;}
        Event event{};event.ticket=ticket;event.target=target;event.overlay=true;event.overlay_rects=rects;event.completed_mask=completed_mask;
        event.from_executed=pair.from_executed;event.expected_queue=pair.source_queue;
        const bool ok=append(ref,event);if(ok){++stats_.overlay_recorded;if(pair.from_executed)++stats_.overlay_from_executed_recorded;}
        else checked_add(stats_.overlay_rejected,1);return ok;
    }
    ListRef current(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(auto* list=cached_list(identity))if(list->valid)return list->ref;
        for (auto& list : used_lists()) if (list.valid && list.ref.identity == identity){cache_list(&list);return list.ref;}
        return {};
    }
    uint64_t world_epoch(){std::lock_guard<std::mutex> lock(mutex_);return world_epoch_;}
    bool world_handoff_ready(uint64_t expected,uint64_t next){
        std::lock_guard<std::mutex> lock(mutex_);
        return expected==world_epoch_&&expected!=UINT64_MAX&&next==expected+1&&
            !owned_.enabled&&!pending_.active&&revision_!=UINT64_MAX;
    }
    // CPU association retirement only, never a GPU drain certificate. Caller
    // orders this boundary against actual recording/submission. No waits for
    // engine work: the mutex protects only bounded in-memory bookkeeping.
    // Keep real list identities/generations, including unsubmitted recordings;
    // find() excludes their old epoch until a genuine Reset/Create is observed.
    bool advance_world_epoch(uint64_t expected,uint64_t next){
        std::lock_guard<std::mutex> lock(mutex_);
        if(expected!=world_epoch_||expected==UINT64_MAX||next!=expected+1||
           owned_.enabled||pending_.active||revision_==UINT64_MAX)return false;
        ++revision_;world_epoch_=next;retired_ticket_=maximum_ticket_;
        candidates_={};window_bad_=false;window_rejection_=0;
        // capacity_bad_ is deliberately sticky across world transitions.
        return true;
    }
    ListRef reset_success(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.reset_calls;
        ++revision_;
        if (!identity || generation_ == std::numeric_limits<uint64_t>::max()) {
            window_bad_ = true; return {};
        }
        List* slot{};
        slot=find_identity(identity);
        if (!slot) for (auto& list : used_lists()) if (!list.ref.identity) { slot = &list; break; }
        if (!slot && used_prefix_ < list_capacity) {slot = &lists_[used_prefix_++];clear_list(*slot);}
        if (!slot) { ++stats_.list_overflow;capacity_bad_ = true; return {}; }
        if(!slot->ref.identity){++stats_.active_lists;if(stats_.active_lists>stats_.list_highwater)stats_.list_highwater=stats_.active_lists;}
        discard_recording(*slot);
        // Reset discards unsubmitted old recording, while already submitted events
        // remain copied in candidates. Generation IDs are never reused.
        clear_list(*slot);
        slot->ref = {identity, ++generation_};
        slot->world_epoch=world_epoch_;
        slot->valid = true;
        cache_list(slot);
        return slot->ref;
    }
    // Actual successful API creation, not a synthetic Reset. CreateCommandList
    // returns recording/open; CreateCommandList1 returns closed and requires a
    // subsequent observed Reset before recording/submission may be associated.
    // Caller holds the canonical object lifetime against address reuse.
    ListRef create_success(Identity identity,bool initially_open) {
        std::lock_guard<std::mutex> lock(mutex_);++stats_.list_creates;++revision_;
        if(!identity||generation_==UINT64_MAX){window_bad_=true;lose_owned();return {};}
        List* slot{};
        slot=find_identity(identity);
        if(!slot)for(auto& list:used_lists())if(!list.ref.identity){slot=&list;break;}
        if(!slot&&used_prefix_<list_capacity){slot=&lists_[used_prefix_++];clear_list(*slot);}
        if(!slot){++stats_.list_overflow;capacity_bad_=true;lose_owned();return {};}
        if(!slot->ref.identity){++stats_.active_lists;if(stats_.active_lists>stats_.list_highwater)stats_.list_highwater=stats_.active_lists;}
        discard_recording(*slot);
        clear_list(*slot);slot->ref={identity,++generation_};slot->valid=initially_open;slot->world_epoch=world_epoch_;
        cache_list(slot);
        return slot->ref;
    }
    bool draw_final(ListRef ref, uint64_t ticket, unsigned eye, Target target, Rect rect) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++revision_;
        if (ticket > maximum_ticket_) maximum_ticket_ = ticket;
        if (!ticket || ticket <= retired_ticket_ || !valid_eye(eye, target, rect)) {
            window_bad_ = true; window_rejection_ |= (!ticket || ticket <= retired_ticket_) ? stale_ticket : bad_rectangle; return false;
        }
        return append(ref, {ticket, eye, target, rect, true});
    }
    // Recording-only query for selectively observing later writes. A partial
    // pair counts; this is not permission to publish or replay an overlay.
    bool has_recorded_target(ListRef ref,Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);auto list=find(ref);
        if(!resource||!list||list->executed)return false;
        for(size_t i=0;i<list->count;++i){auto const& e=list->events[i];
            if((e.final&&e.target.resource==resource)||(e.transfer&&
                (e.target.resource==resource||e.transfer_source.resource==resource)))return true;
        }
        return false;
    }
    // Executed transfers remain relevant to writes recorded on another list
    // until this association window is retired. No resource pointer is borrowed.
    bool has_transfer_endpoint(Identity resource) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!resource||owned_.enabled)return false;
        for(auto const& candidate:candidates_)if(candidate.transfer_source&&
            (candidate.transfer_source==resource||candidate.target.resource==resource))return true;
        return false;
    }
    // Opt-in CPU provenance edge. Caller certifies the native pass samples this
    // exact source and maps its full stereo image to these destination halves.
    // No shader, sampling, resource state or GPU completion proof is inferred.
    // Both source eyes must precede this draw in the SAME recording generation.
    bool draw_transfer(ListRef ref,Target source,Target destination) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto reject=[&](uint32_t reason){window_bad_=true;window_rejection_|=reason;return false;};
        if(owned_.enabled)return reject(owned_state_lost);
        auto list=find(ref);
        if(!list||list->executed)return reject(unknown_list);
        if(source.resource==destination.resource||!valid_eye(0,source,{0,0,source.width/2,source.height})||
           !valid_eye(0,destination,{0,0,destination.width/2,destination.height}))return reject(bad_rectangle);
        for(size_t i=0;i<list->count;++i){auto const& e=list->events[i];
            if(e.transfer||e.target.resource==destination.resource)return reject(target_conflict);
        }
        auto pair=recorded_pair_locked(ref,source);
        if(!pair.linked||!pair.ticket||pair.ticket!=maximum_ticket_)return reject(missing_pair|ticket_conflict);
        Event event{};event.transfer=true;event.ticket=pair.ticket;event.target=destination;event.transfer_source=source;
        event.overlay_rects={Rect{0,0,destination.width/2,destination.height},
            Rect{destination.width/2,0,destination.width/2,destination.height}};
        return append(ref,event);
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
            if(!previous.final&&!previous.overlay&&!previous.transfer&&!previous.state_kind&&previous.target.resource==resource){
                if(previous.multiplicity==std::numeric_limits<uint64_t>::max()||
                    stats_.unknown_coalesced==std::numeric_limits<uint64_t>::max()){
                    capacity_bad_=true;return false;
                }
                ++revision_;if(owned_event(previous))change_owned();++previous.multiplicity;++stats_.unknown_coalesced;return true;
            }
        }
        return append(ref, {0, 0, {resource, 0, 0}, {}, false});
    }
    // Call only after the original pre-birth Execute returned, under the API
    // order lock. Even a rejected mixed batch really submitted its known lists.
    // Preserve that fact without admitting unknown or replacing Reset generations.
    bool submitted_before_owned_birth(const ListRef* members,size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(owned_.enabled||pending_.active||!members||!count||count>list_capacity||revision_==UINT64_MAX)return false;
        ++revision_;
        for(size_t i=0;i<count;++i)if(auto list=find(members[i]))list->executed=true;
        return true;
    }
    bool execute(Identity queue, const ListRef* members, size_t count) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.execute_calls;
        if(owned_.enabled){lose_owned();return false;} // Owned integration must use prepare/original/commit.
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
private:
    Result result_locked(Identity backbuffer, Identity verified_swapchain_queue,bool owned_seal) const {
        Result result{};
        result.rejection = window_rejection_ | (window_bad_ ? metadata_lost : 0u) | (capacity_bad_ ? capacity : 0u);
        for (const auto& candidate : candidates_) if (candidate.target.resource == backbuffer && backbuffer) {
            result.ticket = candidate.ticket;
            result.target = candidate.target;
            result.source_queue=candidate.queue;
            result.eyes = candidate.eyes;
            result.contaminated = candidate.contaminated;
            result.rejection |= candidate.rejection;
            if (candidate.contaminated) result.rejection |= polluted;
            if (candidate.mask != 3) result.rejection |= missing_pair;
            if (!verified_swapchain_queue || candidate.queue != verified_swapchain_queue) result.rejection |= queue_conflict;
            result.linked = !window_bad_ && !capacity_bad_ && !candidate.rejected && candidate.mask == 3 &&
                verified_swapchain_queue && candidate.queue == verified_swapchain_queue;
        }
        if(owned_.enabled){
            result.owned_generation=owned_.generation;result.final_state=owned_.state;
            if(!owned_seal||!owned_.certified||backbuffer!=owned_.target.resource||verified_swapchain_queue!=owned_.queue||
               (owned_.state!=0&&owned_.state!=4&&owned_.state!=128)){
                result.linked=false;result.rejection|=owned_state_lost;
            }
            result.state_certified=result.linked&&owned_.certified;
        }
        if (!result.ticket) result.rejection |= missing_pair;
        return result;
    }
    Result seal_locked(Identity backbuffer, Identity verified_swapchain_queue,bool owned_seal=false) {
        if(pending_.active)lose_owned();
        change_owned();
        ++revision_;
        auto result=result_locked(backbuffer,verified_swapchain_queue,owned_seal);
        retired_ticket_ = maximum_ticket_;
        candidates_ = {};
        window_bad_ = false;
        window_rejection_ = 0;
        // Explicit consumption retires all generations, including unsubmitted
        // recordings. Reclaim their slots; require fresh Reset next window.
        used_prefix_=0;stats_.active_lists=0; // No list owns COM state; old bytes are unreachable.
        // Capacity faults remain sticky until explicit invalidate_all recovery.
        return result;
    }
public:
    Result present(Identity backbuffer,Identity verified_swapchain_queue) {
        std::lock_guard<std::mutex> lock(mutex_);return seal_locked(backbuffer,verified_swapchain_queue);
    }
    Result seal_owned_output(Identity resource,uint64_t generation,Identity verified_queue) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!owned_.enabled||resource!=owned_.target.resource||generation!=owned_.generation||verified_queue!=owned_.queue){
            lose_owned();return seal_locked(0,0,true);}
        return seal_locked(resource,verified_queue,true);
    }
    // Continuous owned output: caller has finished ALL intended contributors to
    // this ticket (including queued UI when enabled), under the same API-order
    // lock. It must enqueue publication/copy before admitting the next writer.
    // This proves CPU submission order, not GPU completion or borrower lifetime.
    // Unlike Present/seal, retain actual Reset generations, executed markers and
    // the certified submitted state. Never re-seed state from creation each frame.
    // Reject unfinished recordings to this resource; do not discard them to make
    // a frame appear complete. Failure is terminal for this output generation.
    Result consume_owned_output(Identity resource,uint64_t generation,Identity verified_queue,uint64_t ticket,
                                Identity* retired_executed=nullptr,size_t retired_capacity=0,size_t* retired_count=nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(retired_count)*retired_count=0;
        auto result=result_locked(resource,verified_queue,true);
        if(!owned_.enabled||!owned_.certified||resource!=owned_.target.resource||
           generation!=owned_.generation||verified_queue!=owned_.queue||pending_.active)
            result.rejection|=owned_state_lost;
        if(!ticket||ticket<=retired_ticket_||ticket!=result.ticket||ticket!=maximum_ticket_)
            result.rejection|=ticket_conflict;
        for(auto const& candidate:candidates_)if(candidate.target.resource&&candidate.target.resource!=resource)
            result.rejection|=target_conflict;
        for(auto const& list:used_lists())if(list.count&&!list.executed){
            for(size_t i=0;i<list.count;++i)if(list.events[i].target.resource==resource)
                result.rejection|=metadata_lost;
        }
        // A caller requesting reclamation must be able to receive every retired
        // identity. Never drop a lease while the tracker still holds its slot.
        if(revision_==UINT64_MAX||stats_.owned_consumed_frames==UINT64_MAX||
           (retired_executed&&(!retired_count||retired_capacity<stats_.active_lists)))result.rejection|=capacity;
        if(!result.linked||result.rejection){
            result.linked=false;result.state_certified=false;lose_owned();return result;
        }
        // Executed recordings cannot be submitted again without a real Reset.
        // A caller with canonical identity leases may retire these slots and
        // release matching leases after this successful consume. Re-execution
        // without Reset remains a proof failure. Unsubmitted recordings stay.
        for(auto& list:used_lists())if(list.executed){
            if(retired_executed){retired_executed[(*retired_count)++]=list.ref.identity;clear_list(list);--stats_.active_lists;}
            else list.count=0;
        }
        candidates_={};retired_ticket_=ticket;
        ++revision_;change_owned();++stats_.owned_consumed_frames;
        return result;
    }
    void invalidate() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++revision_;
        window_bad_ = true;
        lose_owned();
    }
    // Discard only work recorded since the pre-Present consume boundary. This
    // preserves sticky capacity faults and ticket retirement; unlike recovery,
    // it permits no old generation or ticket to escape into the next window.
    void retire_present_interval() { (void)present(0,0); }
    // Pre-birth observation may span many Presents before the independent
    // allocation exists. Retire association payload, but preserve real Reset
    // history (including failed/invalid and already-executed generations).
    // This does not certify unknown lists, recover capacity faults, or alter an
    // enabled owned output. The integration separately rejects all pre-birth
    // references to the new allocation; those observations cannot live here.
    bool retire_before_owned_birth() {
        std::lock_guard<std::mutex> lock(mutex_);
        if(owned_.enabled||pending_.active||revision_==UINT64_MAX)return false;
        ++revision_;retired_ticket_=maximum_ticket_;candidates_={};
        for(auto& list:used_lists())list.count=0;
        window_bad_=false;window_rejection_=0;
        return true;
    }
    void invalidate_capacity() {
        std::lock_guard<std::mutex> lock(mutex_);++revision_;capacity_bad_=true;lose_owned();
    }
    void invalidate_list(Identity identity) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++revision_;lose_owned();
        for (auto& list : used_lists()) if (list.ref.identity == identity) list.valid = false;
        window_bad_ = true;
    }
    // Recovery boundary: discard all tracked work and require fresh successful
    // Resets and newer tickets. The next present is intentionally rejected.
    void invalidate_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++revision_;lose_owned();
        used_prefix_=0; // Reused slots are reset before their old bytes can be read.
        candidates_ = {};
        retired_ticket_ = maximum_ticket_;
        capacity_bad_ = false;
        window_bad_ = true;
        stats_.active_lists=0;
    }
    Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex_);auto value=stats_;value.capacity_bad=capacity_bad_;value.world_epoch=world_epoch_;return value;}
};
} // namespace gears_frame_link
