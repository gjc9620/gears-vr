#pragma once
#include "ngx_typed_resources.h"
#include <array>
#include <cstdio>

// One diagnostic capture, never an admission/producer/state certificate. No
// SDK calls, added GPU commands, waits, fences or typed casts of NGX integers.
namespace gears_ngx_epoch {
struct List {uintptr_t raw{},canonical{};uint64_t generation{};};
inline bool same(List a,List b){return a.raw==b.raw&&a.canonical==b.canonical&&a.generation==b.generation;}
// CPU identity evidence only. Candidate eviction drops evidence, never GPU work.
// The two accepted Eval leases cannot be evicted or retagged by later Reset.
class TypedLists {
public:
    static constexpr unsigned capacity=64,pin_capacity=2;
    enum Reason:unsigned {ok=0,miss=1,closed=2,boundary_failed=3,generation_missing=4,
        world_mismatch=5,generation_changed=6,pin_limit=7};
    struct Entry {List list{};uint64_t world{},age{};unsigned reason=miss;
        Microsoft::WRL::ComPtr<ID3D12CommandList> lease;};
    struct Lookup {List saved{},current{};unsigned reason=miss;};
    std::array<Entry,capacity> candidates{};std::array<Entry,pin_capacity> pins{};
    uint64_t sequence{},evicted{},create_failures{};unsigned pinned{};
    unsigned held()const{unsigned n=pinned;for(auto const& e:candidates)if(e.lease)++n;return n;}
    void release(){for(auto& e:candidates)e={};for(auto& e:pins)e={};pinned=0;}
    void boundary(ID3D12CommandList* p,List ref,uint64_t world,unsigned reason){
        if(!p)return;const auto raw=reinterpret_cast<uintptr_t>(p);
        // A successful API gives a new recording; failure also revokes lookup.
        // Pinned snapshots retain the old lease and exact generation for audit.
        for(auto& e:pins)if(e.lease&&(e.list.raw==raw||(ref.canonical&&e.list.canonical==ref.canonical)))e.reason=boundary_failed;
        Entry* slot{};for(auto& e:candidates)if(e.list.raw==raw||(ref.canonical&&e.list.canonical==ref.canonical)){slot=&e;break;}
        if(!slot)for(auto& e:candidates)if(!e.lease){slot=&e;break;}
        if(!slot){slot=&candidates[0];for(auto& e:candidates)if(e.age<slot->age)slot=&e;++evicted;}
        Entry next;next.list=ref;next.list.raw=raw;next.world=world;next.age=++sequence;next.reason=reason;next.lease=p;
        if(!next.reason&&(!ref.canonical||!ref.generation))next.reason=generation_missing;
        *slot=std::move(next);
    }
    template<class Current> Lookup lookup(uintptr_t raw,uint64_t world,Current current)const{
        Lookup result;
        for(auto const& e:candidates)if(raw&&e.list.raw==raw&&e.lease){
            result.saved=e.list;result.current=current(e.list);result.reason=e.reason;
            if(result.reason)return result;
            if(e.world!=world){result.reason=world_mismatch;return result;}
            if(!result.current.canonical||!result.current.generation){result.reason=generation_missing;return result;}
            if(!same(result.saved,result.current)){result.reason=generation_changed;return result;}
            return result;
        }
        return result;
    }
    bool pin(Lookup const& found){
        if(found.reason||pinned==pins.size())return false;
        for(auto& e:candidates)if(e.lease&&!e.reason&&same(e.list,found.saved)){
            e.age=++sequence;pins[pinned++]=e;return true;}
        return false;
    }
};

struct Geometry {
    uint64_t world{},ticket{};uintptr_t renderer{};unsigned valid{},constructor_valid{};
    std::array<uint32_t,2> extent{},buffer{};
    std::array<uintptr_t,2> views{};
    std::array<std::array<float,4>,2> source{},sampling{},constructor{};
    std::array<std::array<int32_t,4>,2> target{};
};
struct EvalInput {
    uint64_t world{},ticket{};uintptr_t renderer{},scope{},handle{},parameters{};List command{};
    std::array<uintptr_t,4> resources{}; // Native order C,O,D,M.
    std::array<uint32_t,15> values{}; // ngx_route value_offsets, raw float bits.
    Geometry geometry{};unsigned thread{};bool associated{},readable{};
};
enum class Kind:unsigned {transition=1,uav,alias,eval_begin,eval_dispatch,eval_return,copy_begin,copy_return,execute_begin,execute_return,reset};
struct Event {
    uint64_t serial{};Kind kind{};List list{};uintptr_t resource{},other{},queue{},caller{};
    uint64_t order{};unsigned before{},after{},subresource{},flags{},member{},count{};bool returned{},current{};
};
struct Eval {
    EvalInput input{};uint64_t begin{},end{},result{};unsigned dispatches{},copies{};uint32_t dispatch_result{};
    bool returned{},dispatch_returned{},dispatch_match{},copy_returned{};List copy_list{};
    std::array<unsigned,3> destination{};bool boxed{};D3D12_BOX box{};
    unsigned source_type{},target_type{},source_subresource{},target_subresource{};
    unsigned typed_mask{},eval_executes{},copy_executes{};bool eval_generation{},copy_generation{};
    uint64_t eval_order{},copy_order{};uintptr_t eval_queue{},copy_queue{};unsigned eval_member{},copy_member{};
};
class Probe {
public:
    static constexpr unsigned resource_capacity=128,event_capacity=1024;
    struct EmptyEpoch {uint64_t revision{},world{};uintptr_t handle{},parameters{};std::array<uint32_t,7> fields{};unsigned reason{},skipped{};};
    struct Typed {gears_ngx_stereo::TypedResourceLease lease{};uint64_t first{},last{};unsigned observations{};};
    bool armed{},selected{},finished{};unsigned stop{},dropped{},unknown{},skipped{},eval_count{},resource_count{},event_count{};
    uint64_t excluded_execute_members{},excluded_resets{};
    unsigned empty_epoch_count{},end_reason{};std::array<EmptyEpoch,3> empty_epochs{};
    uint64_t revision{},world{},ticket{},serial{},execution_order{};uintptr_t handle{},parameters{},renderer{};
    std::array<uint32_t,7> creation{};std::array<Geometry,3> geometry{};
    std::array<Typed,resource_capacity> resources{};std::array<Event,event_capacity> events{};std::array<Eval,2> evals{};
    enum Reject:unsigned {concurrent_input=1,unassociated=2,unreadable=4,handle_changed=8,parameters_changed=16,
        raw_missing=32,canonical_missing=64,generation_absent=128,scope_missing=256,typed_lookup_failed=512,pinning_failed=1024};
    struct Rejection {bool present{};unsigned mask{},lookup_reason{};EvalInput input{};List saved{},current{};uint64_t tracker_overflow{};bool tracker_capacity_bad{};} rejection;
    static unsigned input_rejection(EvalInput const& in,bool concurrent){
        return (concurrent?concurrent_input:0u)|(!in.associated?unassociated:0u)|(!in.readable?unreadable:0u)|
            (!in.command.raw?raw_missing:0u)|(!in.command.canonical?canonical_missing:0u)|
            (!in.command.generation?generation_absent:0u)|(!in.scope?scope_missing:0u);
    }
    void clear(){*this={};}
    bool observing()const{return armed&&selected&&!finished;}
    bool matches(uint64_t w,uint64_t t,uintptr_t r)const{return observing()&&world==w&&ticket==t&&renderer==r;}
    void gap(){if(observing())++dropped;}
    bool empty_epoch()const{return !selected&&!renderer&&!ticket&&!resource_count&&!eval_count&&!event_count&&!execution_order;}
    void changed(unsigned reason=4){
        if(armed&&!finished){end_reason=reason;stop=empty_epoch()&&empty_epoch_count==empty_epochs.size()?8u:2u;finished=true;}
    }
    void created(uint64_t r,uint64_t w,uintptr_t h,uintptr_t p,std::array<uint32_t,7> const& fields){
        if(!r||!w||!h||!p)return;
        if(armed){
            if(!finished)changed(3);
            // Only a pure empty epoch can be summarized and rearmed. Any
            // selected constructor or retained pair/Execute evidence freezes.
            if(stop!=2||!empty_epoch())return;
            if(empty_epoch_count==empty_epochs.size()){stop=8;return;}
            empty_epochs[empty_epoch_count++]={revision,world,handle,parameters,creation,end_reason,skipped};
            armed=finished=false;stop=end_reason=skipped=0;
        }
        armed=true;revision=r;world=w;handle=h;parameters=p;creation=fields;
    }
    void built(Geometry const& g,bool verified){
        if(!armed||selected||finished||g.world!=world||!verified||g.valid!=31||!g.ticket||!g.renderer||!g.views[0]||!g.views[1]||g.views[0]==g.views[1])return;
        selected=true;ticket=g.ticket;renderer=g.renderer;geometry[0]=g;
    }
    void prepared(unsigned stage,Geometry const& g){
        if(stage<1||stage>2||!matches(g.world,g.ticket,g.renderer))return;
        if(geometry[stage].ticket){gap();return;}geometry[stage]=g;
    }
    uint64_t event(Event e){
        if(!observing())return 0;e.serial=++serial;
        if(event_count==events.size()){++dropped;finished=true;stop=6;return 0;}events[event_count++]=e;return e.serial;
    }
    void typed(ID3D12Resource* value){
        if(!observing()||!value)return;const auto raw=reinterpret_cast<uintptr_t>(value);
        for(unsigned i=0;i<resource_count;++i)if(resources[i].lease.raw==raw){resources[i].last=serial;++resources[i].observations;return;}
        if(resource_count==resources.size()){gap();return;}
        // The caller supplies a real typed D3D API argument. Retention prevents
        // pointer reuse while this diagnostic capture joins later NGX integers.
        auto lease=gears_ngx_stereo::TypedResourceLease::observe(value);
        if(!lease.retained()){gap();return;}resources[resource_count++]={std::move(lease),serial,serial,1};
    }
    void barrier(List list,D3D12_RESOURCE_BARRIER const& b,uintptr_t caller){
        if(!observing())return;Event e;e.list=list;e.caller=caller;e.flags=unsigned(b.Flags);
        if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_TRANSITION){e.kind=Kind::transition;e.resource=reinterpret_cast<uintptr_t>(b.Transition.pResource);
            e.before=unsigned(b.Transition.StateBefore);e.after=unsigned(b.Transition.StateAfter);e.subresource=b.Transition.Subresource;
            event(e);typed(b.Transition.pResource);
        }else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_UAV){e.kind=Kind::uav;e.resource=reinterpret_cast<uintptr_t>(b.UAV.pResource);event(e);typed(b.UAV.pResource);++unknown;}
        else if(b.Type==D3D12_RESOURCE_BARRIER_TYPE_ALIASING){e.kind=Kind::alias;e.resource=reinterpret_cast<uintptr_t>(b.Aliasing.pResourceBefore);e.other=reinterpret_cast<uintptr_t>(b.Aliasing.pResourceAfter);
            event(e);typed(b.Aliasing.pResourceBefore);typed(b.Aliasing.pResourceAfter);++unknown;}
        else {++unknown;gap();}
    }
    void barrier_return(uint64_t from,uint64_t to,List list){
        for(unsigned i=0;i<event_count;++i){auto& e=events[i];if(e.serial>from&&e.serial<=to&&unsigned(e.kind)<=unsigned(Kind::alias)){e.returned=true;e.current=same(e.list,list);}}
    }
    unsigned mask(std::array<uintptr_t,4> const& raw)const{
        unsigned out{};for(unsigned j=0;j<4;++j)for(unsigned i=0;raw[j]&&i<resource_count;++i)
            if(resources[i].lease.raw==raw[j]&&resources[i].lease.retained())out|=1u<<j;
        return out;
    }
    int enter(EvalInput const& in,bool concurrent,unsigned extra=0){
        if(!armed||finished)return -1;if(!selected){++skipped;return -1;}
        if(!matches(in.world,in.ticket,in.renderer))return -1;
        const auto rejected=input_rejection(in,concurrent)|extra|(in.handle!=handle?handle_changed:0u)|(in.parameters!=parameters?parameters_changed:0u);
        if(rejected){if(!rejection.present){rejection.present=true;rejection.mask=rejected;rejection.input=in;}gap();stop=3;finished=true;return -1;}
        if(eval_count==2){gap();stop=4;finished=true;return -1;}
        const auto index=eval_count++;auto& e=evals[index];e.input=in;e.typed_mask=mask(in.resources);
        Event ev;ev.kind=Kind::eval_begin;ev.list=in.command;ev.member=index;e.begin=event(ev);return int(index);
    }
    int active(uintptr_t scope)const{for(unsigned i=0;i<eval_count;++i)if(evals[i].input.scope==scope&&!evals[i].end)return int(i);return -1;}
    int dispatch(uintptr_t command,uintptr_t h,uintptr_t p,uintptr_t callback,unsigned thread){
        if(!observing())return -1;int index=-1;for(unsigned i=0;i<eval_count;++i)if(!evals[i].end){if(index>=0){gap();return -1;}index=int(i);}
        if(index<0)return -1;auto& e=evals[unsigned(index)];++e.dispatches;
        e.dispatch_match=e.dispatches==1&&e.input.command.raw==command&&h==handle&&p==parameters&&!callback&&e.input.thread==thread;
        Event ev;ev.kind=Kind::eval_dispatch;ev.list=e.input.command;ev.member=unsigned(index);event(ev);return index;
    }
    void dispatched(int index,uint32_t result){if(index<0||index>=2)return;auto& e=evals[unsigned(index)];e.dispatch_returned=true;e.dispatch_result=result;}
    void leave(uintptr_t scope,bool returned,uint64_t result){
        const int index=active(scope);if(index<0||!observing())return;auto& e=evals[unsigned(index)];
        e.returned=returned;e.result=result;Event ev;ev.kind=Kind::eval_return;ev.list=e.input.command;ev.member=unsigned(index);ev.returned=returned;e.end=event(ev);
        if(!returned){gap();stop=5;finished=true;}
    }
    int copy(List list,D3D12_TEXTURE_COPY_LOCATION const& dst,std::array<unsigned,3> at,D3D12_TEXTURE_COPY_LOCATION const& src,D3D12_BOX const* box,uintptr_t caller){
        if(!observing()||!eval_count)return -1;const unsigned index=eval_count-1;auto& e=evals[index];
        if(!e.end||!e.returned||reinterpret_cast<uintptr_t>(src.pResource)!=e.input.resources[1]||reinterpret_cast<uintptr_t>(dst.pResource)!=e.input.resources[0])return -1;
        ++e.copies;e.copy_list=list;e.destination=at;e.boxed=box!=nullptr;if(box)e.box=*box;
        e.source_type=unsigned(src.Type);e.target_type=unsigned(dst.Type);
        if(src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)e.source_subresource=src.SubresourceIndex;
        if(dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)e.target_subresource=dst.SubresourceIndex;
        Event ev;ev.kind=Kind::copy_begin;ev.list=list;ev.resource=reinterpret_cast<uintptr_t>(src.pResource);ev.other=reinterpret_cast<uintptr_t>(dst.pResource);ev.caller=caller;ev.member=index;event(ev);
        typed(src.pResource);typed(dst.pResource);return int(index);
    }
    void copied(int index,List current,bool unchanged){
        if(index<0||index>=2||!observing())return;auto& e=evals[unsigned(index)];e.copy_returned=unchanged&&same(e.copy_list,current);
        Event ev;ev.kind=Kind::copy_return;ev.list=current;ev.member=unsigned(index);ev.returned=e.copy_returned;event(ev);
    }
    // Only selected-ticket Barrier/Eval/Copy evidence introduces a recording.
    // Reset/Execute never seed relevance. Pending uses also retain wrong-generation
    // observations of the same raw list, so filtering cannot hide a violation.
    bool watches(List list)const{
        if(!list.raw)return false;
        for(unsigned i=0;i<event_count;++i){auto const& e=events[i];
            if((e.kind==Kind::transition||e.kind==Kind::uav||e.kind==Kind::alias||e.kind==Kind::eval_begin||e.kind==Kind::copy_begin)&&same(e.list,list))return true;}
        for(unsigned i=0;i<eval_count;++i){auto const& e=evals[i];
            if((!e.eval_executes&&e.input.command.raw==list.raw)||(!e.copy_executes&&e.copy_list.raw==list.raw))return true;}
        return false;
    }
    bool executing(uint64_t order,unsigned member,List list)const{
        for(unsigned i=0;i<event_count;++i){auto const& e=events[i];
            if(e.kind==Kind::execute_begin&&e.order==order&&e.member==member&&same(e.list,list))return true;}
        return false;
    }
    uint64_t execute_begin(uintptr_t queue,List const* lists,unsigned count){
        if(!observing())return 0;const auto order=++execution_order;
        for(unsigned i=0;i<count;++i){if(!watches(lists[i])){++excluded_execute_members;continue;}Event ev;ev.kind=Kind::execute_begin;ev.list=lists[i];ev.queue=queue;ev.order=order;ev.member=i;ev.count=count;event(ev);}return order;
    }
    void execute_end(uint64_t order,uintptr_t queue,List const* before,List const* after,unsigned count){
        if(!order||!observing())return;
        for(unsigned i=0;i<count;++i){if(!executing(order,i,before[i]))continue;Event ev;ev.kind=Kind::execute_return;ev.list=before[i];ev.queue=queue;ev.order=order;ev.member=i;ev.count=count;ev.returned=true;ev.current=same(before[i],after[i]);event(ev);
            for(unsigned j=0;j<eval_count;++j){auto& e=evals[j];
                if(same(e.input.command,before[i])||(e.input.command.raw==before[i].raw&&!e.eval_executes)){++e.eval_executes;e.eval_generation=e.eval_executes==1&&ev.current&&same(before[i],e.input.command);e.eval_order=order;e.eval_queue=queue;e.eval_member=i;}
                if(e.copy_list.raw&&(same(e.copy_list,before[i])||(e.copy_list.raw==before[i].raw&&!e.copy_executes))){++e.copy_executes;e.copy_generation=e.copy_executes==1&&ev.current&&same(before[i],e.copy_list);e.copy_order=order;e.copy_queue=queue;e.copy_member=i;}
            }
        }
        if(!finished&&eval_count==2&&evals[0].copy_executes&&evals[1].copy_executes&&evals[0].eval_executes&&evals[1].eval_executes){finished=true;stop=1;}
    }
    void reset(List before,List after,bool returned){if(!observing())return;if(!watches(before)){++excluded_resets;return;}Event ev;ev.kind=Kind::reset;ev.list=before;ev.order=after.generation;ev.returned=returned;ev.current=before.canonical==after.canonical;event(ev);}
    void release_leases(){for(auto& r:resources)r.lease.release();}
    bool complete()const{
        if(!finished||stop!=1||dropped||eval_count!=2||geometry[0].valid!=31||geometry[0].constructor_valid!=3||geometry[1].valid!=31||geometry[2].valid!=31)return false;
        if(evals[0].input.values[7]==evals[1].input.values[7]&&evals[0].input.values[8]==evals[1].input.values[8])return false;
        for(auto const& e:evals)if(!e.returned||!e.dispatch_returned||!e.dispatch_match||e.dispatches!=1||e.copies!=1||!e.copy_returned||
            !e.eval_generation||!e.copy_generation||e.typed_mask!=15||mask(e.input.resources)!=15||e.input.geometry.valid!=31||(e.dispatch_result&0xfff00000u)==0xbad00000u||(uint32_t(e.result)&0xfff00000u)==0xbad00000u)return false;
        return true;
    }
};
}
