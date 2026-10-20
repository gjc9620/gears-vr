#pragma once
#include "ngx_route_observer.h"
#include <string_view>

// CPU-only, bounded one-shot selector. All calls are serialized by the existing
// frame_hook_mutex; that mutex is never held across original Create/Evaluate.
namespace gears_ngx_delayed {
using gears_ngx_route::Sample;
using gears_ngx_route::Ownership;
inline bool owner(Ownership const& a,Ownership const& b){return a.associated&&b.associated&&a.world==b.world&&a.ticket==b.ticket&&a.renderer==b.renderer&&a.context==b.context&&a.view==b.view&&a.views==b.views&&a.count==b.count;}
inline bool valid(Sample const& s){return s.ownership.associated&&s.slots_mask==3&&s.fields_mask==0x7ffff&&s.handle&&s.parameters&&s.values[0]&&s.values[1]&&!s.values[2]&&s.resources[0]&&s.resources[1]&&s.resources[2]&&s.resources[3]&&s.resources[0]!=s.resources[1];}
inline bool signature(Sample const& a,Sample const& b){
    if(a.handle!=b.handle||a.parameters!=b.parameters||a.resources!=b.resources||a.ownership.world!=b.ownership.world)return false;
    for(unsigned i=0;i<a.values.size();++i)if(i!=3&&i!=4&&a.values[i]!=b.values[i])return false;
    return true;
}
struct Request {uint64_t pid{},creation{},started{},world{},epoch{},nonce{};};
inline bool parse(std::string_view text,Request& request){
    Request out;auto literal=[&](std::string_view part){if(text.substr(0,part.size())!=part)return false;text.remove_prefix(part.size());return true;};
    auto number=[&](std::string_view key,uint64_t& value){if(!literal(key))return false;value=0;unsigned digits{};
        while(!text.empty()&&text.front()>='0'&&text.front()<='9'){const auto digit=unsigned(text.front()-'0');if(value>(UINT64_MAX-digit)/10)return false;value=value*10+digit;text.remove_prefix(1);++digits;}
        return digits&&value&&literal("\n");};
    if(text.size()>512||!literal("schema=1\n")||!number("pid=",out.pid)||!number("creation_time=",out.creation)||!number("started_qpc=",out.started)||
       !number("world=",out.world)||!number("epoch=",out.epoch)||!number("nonce=",out.nonce)||!literal("action=resume\n")||!text.empty())return false;
    request=out;return true;
}
enum class Reason:unsigned {none,new_create,invalid_create,bad_request,identity,epoch,duplicate,busy,invalid_eval,overlap,incomplete,drift,timeout,selected_invalid,pair_closed,complete};
enum class Site:unsigned {none,create_entry,eval_overlap,eval_invalid_entry,eval_ticket_change,eval_extra,eval_pair_mismatch,eval_signature,eval_exit,end_ticket,timeout};
inline const char* site_name(Site site){switch(site){
    case Site::create_entry:return "create_entry";case Site::eval_overlap:return "eval_overlap";case Site::eval_invalid_entry:return "eval_invalid_entry";
    case Site::eval_ticket_change:return "eval_ticket_change";case Site::eval_extra:return "eval_extra";case Site::eval_pair_mismatch:return "eval_pair_mismatch";
    case Site::eval_signature:return "eval_signature";case Site::eval_exit:return "eval_exit";case Site::end_ticket:return "end_ticket";case Site::timeout:return "timeout";default:return "none";}}
struct PresentEvidence {bool known{},linked{},contaminated{};unsigned rejection{};uintptr_t buffer{};};
struct Trigger {
    Site site{};uint64_t qpc{};bool incoming_known{};Ownership incoming{};
    bool at_present{},eligible{};uint64_t present_ticket{};PresentEvidence present{};
};
inline Trigger eval_trigger(Site site,uint64_t stamp,Sample const& sample){Trigger out;out.site=site;out.qpc=stamp;out.incoming_known=true;out.incoming=sample.ownership;return out;}
struct Invalidation {
    bool valid{};Reason original_reason{};Trigger trigger{};
    unsigned active{},rows{},stable{},pair_thread{};uint64_t epoch{},world{},last_ticket{},completed_ticket{},selected_ticket{},selected_world{};
};
struct Gate {
    uint64_t epoch{},world{},handle{},parameters{},last_nonce{},accepted{},rejected{},resets{},last_ticket{},completed_ticket{},selected_ticket{},selected_world{};
    uint64_t deadline{};unsigned active{},stable{},rows{},pair_thread{};bool epoch_valid{},armed{},selected{},closed{},poisoned{},blocked{},finished{};
    Reason reason{},rejection{};std::array<uint32_t,7> create{};std::array<Sample,2> pair{},baseline{};
    Invalidation first_invalid{};
    void latch(Reason why,Trigger const& trigger){
        if(!selected||first_invalid.valid)return;
        first_invalid={true,why,trigger,active,rows,stable,pair_thread,epoch,world,last_ticket,completed_ticket,selected_ticket,selected_world};
    }
    void reset(Reason why,Trigger const& trigger){latch(why,trigger);++resets;stable=rows=0;last_ticket=0;blocked=true;reason=why;if(selected){armed=false;reason=Reason::selected_invalid;}}
    void tick(uint64_t stamp){if(armed&&!finished&&stamp>=deadline){Trigger trigger;trigger.site=Site::timeout;trigger.qpc=stamp;latch(Reason::timeout,trigger);armed=false;reason=Reason::timeout;}}
    uint64_t begin_create(uint64_t stamp,Ownership const& incoming){Trigger trigger;trigger.site=Site::create_entry;trigger.qpc=stamp;trigger.incoming_known=true;trigger.incoming=incoming;latch(Reason::new_create,trigger);
        ++epoch;epoch_valid=false;armed=false;stable=rows=0;last_ticket=completed_ticket=0;blocked=false;reason=Reason::new_create;
        if(selected)reason=Reason::selected_invalid;poisoned=active!=0;++active;return epoch;}
    void end_create(uint64_t serial,bool normal,Sample const& before,Sample const& after){
        const bool ok=serial==epoch&&active==1&&!poisoned&&normal&&owner(before.ownership,after.ownership)&&
            before.slots_mask==3&&before.fields_mask==127&&after.slots_mask==3&&after.fields_mask==127&&
            before.create==after.create&&before.parameters==after.parameters&&after.handle&&after.parameters&&
            after.create[0]&&after.create[1]&&after.create[2]&&after.create[3];
        if(active)--active;if(!ok){reason=Reason::invalid_create;return;}
        world=after.ownership.world;handle=after.handle;parameters=after.parameters;create=after.create;epoch_valid=true;
    }
    bool arm(Request const& r,Request const& identity,uint64_t stamp,uint64_t duration){
        auto reject=[&](Reason why){++rejected;rejection=why;return false;};
        if(r.pid!=identity.pid||r.creation!=identity.creation||r.started!=identity.started)return reject(Reason::identity);
        if(!epoch_valid||r.world!=world||r.epoch!=epoch)return reject(Reason::epoch);
        if(r.nonce<=last_nonce)return reject(Reason::duplicate);
        if(active||armed||selected)return reject(Reason::busy);
        if(!duration||stamp>UINT64_MAX-duration)return reject(Reason::bad_request);
        last_nonce=r.nonce;++accepted;armed=true;closed=blocked=false;stable=rows=0;last_ticket=completed_ticket=0;deadline=stamp+duration;reason=Reason::none;return true;
    }
    // Returns a provisional GPU slot only for the NEXT pair after 32 complete
    // stable pairs. A third Eval or any later defect terminally quarantines it.
    int begin_eval(Sample const& s,bool call_allowed,uint64_t stamp,unsigned caller_thread){
        tick(stamp);const bool overlap=active!=0;++active;
        if(overlap){poisoned=true;if(armed)reset(Reason::overlap,eval_trigger(Site::eval_overlap,stamp,s));return -1;}
        poisoned=false;if(!armed||closed||blocked)return -1;
        if(!call_allowed||!valid(s)||!epoch_valid||s.ownership.world!=world||s.handle!=handle||s.parameters!=parameters||s.ownership.ticket<=completed_ticket){reset(Reason::invalid_eval,eval_trigger(Site::eval_invalid_entry,stamp,s));return -1;}
        if(last_ticket&&s.ownership.ticket!=last_ticket){reset(Reason::incomplete,eval_trigger(Site::eval_ticket_change,stamp,s));return -1;}
        if(!last_ticket){last_ticket=s.ownership.ticket;pair_thread=caller_thread;}
        if(rows>=2){reset(Reason::incomplete,eval_trigger(Site::eval_extra,stamp,s));return -1;}
        if(rows&&(pair_thread!=caller_thread||!owner(pair[0].ownership,s.ownership)||pair[0].handle!=s.handle||pair[0].parameters!=s.parameters||pair[0].resources!=s.resources||
           pair[0].values[0]!=s.values[0]||pair[0].values[1]!=s.values[1]||pair[0].values[5]!=s.values[5]||pair[0].values[6]!=s.values[6])){reset(Reason::invalid_eval,eval_trigger(Site::eval_pair_mismatch,stamp,s));return -1;}
        if(stable&& !signature(baseline[rows],s)){reset(Reason::drift,eval_trigger(Site::eval_signature,stamp,s));return -1;}
        if(stable==32&&!rows){selected=true;selected_ticket=s.ownership.ticket;selected_world=s.ownership.world;}
        const auto index=rows;pair[index]=s;++rows;return selected?int(index):-1;
    }
    void end_eval(Sample const& before,Sample const& after,bool normal,uint64_t stamp){
        if(active)--active;if(!armed||closed)return;
        if(poisoned||!normal||!valid(after)||!owner(before.ownership,after.ownership)||before.slots_mask!=after.slots_mask||before.fields_mask!=after.fields_mask||
           before.handle!=after.handle||before.parameters!=after.parameters||before.resources!=after.resources||before.values!=after.values)reset(Reason::invalid_eval,eval_trigger(Site::eval_exit,stamp,after));
    }
    bool end_ticket(uint64_t ticket,bool linked,uint64_t stamp,PresentEvidence const& present={}){
        tick(stamp);if(!armed||closed)return false;
        if(blocked){blocked=false;rows=0;last_ticket=0;return false;}
        if(active||!linked||!last_ticket||ticket!=last_ticket||rows!=2){Trigger trigger;trigger.site=Site::end_ticket;trigger.qpc=stamp;trigger.at_present=true;trigger.eligible=linked;trigger.present_ticket=ticket;trigger.present=present;
            reset(Reason::incomplete,trigger);blocked=false;return false;}
        completed_ticket=ticket;
        if(!stable)baseline=pair;
        if(selected){closed=true;reason=Reason::pair_closed;return true;}
        ++stable;rows=0;last_ticket=0;return false;
    }
};
}
