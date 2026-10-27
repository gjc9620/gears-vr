// Included after crosshair_live, before the existing ui_call_boundary hook.
// Read-only shot-call metadata; no game call, Result read, or input injection.
#include "weapon_call_trace.h"
namespace weapon_call_live {
#if defined(GEARS_UI_CALL_BOUNDARY_OBSERVER) && GEARS_UI_CALL_BOUNDARY_OBSERVER && defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
struct Row {
    uint64_t id{},fire_id{},parent_aim_id{},tick{},world{};
    unsigned role{},thread{};uintptr_t receiver{},caller{},callee{},result_address{},opaque_rax{};
    gears_ui_call_boundary::Frame before{},after{};
    long long begin{},end{};bool before_valid{},after_valid{},returned{},body_verified{},result_valid{},trace_valid{};
    std::array<std::int32_t,3> result_rotator{};
    std::array<float,3> trace_start{},trace_pushed{},trace_end{};
};
struct Context {
    bool active{},poisoned{},fire_active{},aim_active{};
    uint64_t generation{},world{},tick{},fire_id{},aim_id{};
    unsigned thread{};uintptr_t tick_frame{},controller{},pawn{},weapon{},fire_caller{},aim_caller{};
    std::array<Row,48> staged{};unsigned count{};
};
thread_local Context context;
std::mutex rows_mutex;std::array<Row,96> rows{};unsigned row_count{};
std::atomic<uint64_t> next_call{},tick_seen{},tick_sealed{},tick_rejected{},
    call_rejected{},target_seen_outside_tick{},staged_dropped{},rows_dropped{},
    aim_result_rejected{},fire_trace_rejected{};
std::array<std::atomic<uint64_t>,6> seen{},accepted{};
struct TickScope {
    bool entered{};Context saved{};
    explicit TickScope(crosshair_live::TickScope const& tick){
        if(!tick.entered)return;
        entered=true;saved=context;context={};++tick_seen;
        if(saved.active){saved.poisoned=true;++tick_rejected;return;}
        const auto& c=crosshair_live::current();
        if(!controller_input_observer||c.mounted_suspended||c.weapon_call_count!=6||
           tick.previous_frame||tick.world!=committed_world_generation.load()||
           tick.token!=crosshair_live::state.token||!recording.load()||!crosshair_live::root_binding(0)){
            ++tick_rejected;return;
        }
        gears_ui_call_boundary::Frame root{};
        if(!gears_ui_call_boundary::frame(safe_copy,crosshair_live::state.frame,root)||
           root.node!=c.identities.identities[c.tick_roots[0].function].address||
           root.object!=c.identities.identities[c.assignment.controller].address||
           !gears_weapon_call_trace::script_pc(root.code,c.tick_roots[0].script,c.tick_roots[0].size)){
            ++tick_rejected;return;
        }
        context.active=true;context.generation=crosshair_live::config_generation.load();
        context.world=tick.world;context.tick=tick.token;context.thread=GetCurrentThreadId();
        context.tick_frame=crosshair_live::state.frame;
        context.controller=c.identities.identities[c.assignment.controller].address;
        context.pawn=c.identities.identities[c.assignment.pawn].address;
        context.weapon=c.identities.identities[c.assignment.weapon].address;
    }
    ~TickScope(){
        if(!entered)return;
        bool valid=context.active&&!context.poisoned&&recording.load()&&
            context.world==committed_world_generation.load()&&
            context.generation==crosshair_live::config_generation.load()&&
            context.thread==GetCurrentThreadId()&&!context.fire_active&&!context.aim_active;
        if(valid){const auto& c=crosshair_live::current();
            valid=!c.mounted_suspended&&c.weapon_call_count==6&&
                c.identities.identities[c.assignment.controller].address==context.controller&&
                c.identities.identities[c.assignment.pawn].address==context.pawn&&
                c.identities.identities[c.assignment.weapon].address==context.weapon&&
                gears_crosshair_assignment::validate_bindings(c.assignment,safe_copy,false);
        }
        if(valid){++tick_sealed;std::lock_guard<std::mutex> lock(rows_mutex);
            for(unsigned i=0;i<context.count;++i){
                if(row_count<rows.size())rows[row_count++]=context.staged[i];else ++rows_dropped;
            }
        }else if(context.active)++tick_rejected;
        context=saved;
        if(saved.active)context.poisoned=true;
    }
};
struct CallScope {
    bool admitted{},fire_owned{},aim_owned{};unsigned role{};Row row{};
    CallScope(void* self,void* caller,void* result,void* callee){
        if(!controller_input_observer||!recording.load())return;
        auto const& c=crosshair_live::current();
        if(c.mounted_suspended||c.weapon_call_count!=6||c.identities.identity_count>c.identities.identities.size())return;
        constexpr unsigned none=6;unsigned candidate=none;
        for(unsigned i=0;i<6;++i){auto const& call=c.weapon_calls[i];
            if(call.function<c.identities.identity_count&&
               c.identities.identities[call.function].address==reinterpret_cast<uintptr_t>(callee)){
                candidate=i;break;
            }
        }
        if(candidate==none)return;
        role=candidate;++seen[role];
        if(!context.active){++target_seen_outside_tick;return;}
        auto const& tick_root=c.tick_roots[0];
        const bool current=context.thread==GetCurrentThreadId()&&!context.poisoned&&
            context.world==committed_world_generation.load()&&
            context.generation==crosshair_live::config_generation.load()&&
            context.tick==crosshair_live::state.token&&
            context.tick_frame==crosshair_live::state.frame&&
            reinterpret_cast<uintptr_t>(self)==(role==2?context.pawn:context.weapon)&&
            crosshair_live::weapon_call_binding(c,role);
        if(!current){++call_rejected;return;}
        const auto caller_address=reinterpret_cast<uintptr_t>(caller);
        bool ancestry=false;
        if(role<=1){
            ancestry=gears_weapon_call_trace::tick_ancestry(safe_copy,caller_address,
                context.tick_frame,c.identities.identities[tick_root.function].address,context.controller,
                tick_root.script,tick_root.size);
            if(role==1&&context.fire_active)ancestry=false;
        }else if(context.fire_active){
            auto const& fire=c.weapon_calls[1];
            ancestry=gears_weapon_call_trace::fire_body_ancestry(safe_copy,caller_address,
                context.fire_caller,c.identities.identities[fire.function].address,context.weapon,
                fire.script,fire.size,context.tick_frame,
                c.identities.identities[tick_root.function].address,context.controller,
                tick_root.script,tick_root.size);
        }
        // Only the directly nested aim chain can label an adjusted direction
        // as belonging to this exact shot. A shared fire ancestor alone is
        // insufficient when helpers or recursive calls are active.
        if(ancestry&&role==3){
            auto const& fire=c.weapon_calls[1];
            ancestry=!context.aim_active&&
                gears_weapon_call_trace::direct_caller(safe_copy,caller_address,
                    c.identities.identities[fire.function].address,context.weapon,
                    context.fire_caller,fire.script,fire.size);
        }else if(ancestry&&role==2){
            auto const& aim=c.weapon_calls[3];
            ancestry=context.aim_active&&context.aim_caller&&
                gears_weapon_call_trace::direct_caller(safe_copy,caller_address,
                    c.identities.identities[aim.function].address,context.weapon,
                    context.aim_caller,aim.script,aim.size);
        }
        if(!ancestry){++call_rejected;return;}
        admitted=true;++accepted[role];row.id=++next_call;row.fire_id=context.fire_id;
        if(role==2)row.parent_aim_id=context.aim_id;
        row.tick=context.tick;row.world=context.world;row.role=role;row.thread=context.thread;
        row.receiver=reinterpret_cast<uintptr_t>(self);row.caller=caller_address;
        row.callee=reinterpret_cast<uintptr_t>(callee);row.result_address=reinterpret_cast<uintptr_t>(result);
        row.begin=now();row.before_valid=gears_ui_call_boundary::frame(safe_copy,caller_address,row.before);
        row.body_verified=role>1;
        if(role==4){
            auto const& fire=c.weapon_calls[1];
            auto const& trace=c.weapon_fire_trace;
            gears_weapon_call_trace::FireInput input{};
            row.trace_valid=crosshair_live::weapon_call_binding(c,1)&&
                gears_weapon_call_trace::direct_fire_input(safe_copy,caller_address,
                    c.identities.identities[fire.function].address,context.weapon,
                    context.fire_caller,fire.script,fire.size,
                    trace.locals_size,trace.offsets,input);
            if(row.trace_valid){row.trace_start=input.start;row.trace_end=input.end;
                row.trace_pushed=input.pushed;}
            if(!row.trace_valid)++fire_trace_rejected;
        }
        if(role==1){context.fire_active=true;context.fire_caller=caller_address;
            context.fire_id=row.id;row.fire_id=row.id;fire_owned=true;}
        if(role==3){context.aim_active=true;context.aim_caller=caller_address;
            context.aim_id=row.id;aim_owned=true;}
    }
    void returned(std::uintptr_t value){
        if(!admitted)return;
        row.opaque_rax=value;row.end=now();row.returned=true;
        row.after_valid=gears_ui_call_boundary::frame(safe_copy,row.caller,row.after);
        // Entry R8 is the return expression's destination. It is not a
        // callee-local parameter block and the original RAX remains opaque.
        if(role==2||role==3){
            auto const& c=crosshair_live::current();
            const auto address=row.result_address;
            if(context.active&&!context.poisoned&&
               context.thread==GetCurrentThreadId()&&
               context.world==committed_world_generation.load()&&
               context.generation==crosshair_live::config_generation.load()&&
               address&&!(address&3)&&address<=UINTPTR_MAX-sizeof(row.result_rotator)&&
               crosshair_live::weapon_call_binding(c,role)&&
               safe_copy(row.result_rotator.data(),reinterpret_cast<void const*>(address),
                         sizeof(row.result_rotator)))row.result_valid=true;
            else ++aim_result_rejected;
        }
        if(context.active&&!context.poisoned&&context.count<context.staged.size())
            context.staged[context.count++]=row;
        else ++staged_dropped;
        if(fire_owned){context.fire_active=false;context.fire_id=0;context.fire_caller=0;fire_owned=false;}
        if(aim_owned){context.aim_active=false;context.aim_id=0;context.aim_caller=0;aim_owned=false;}
    }
    ~CallScope(){
        if(fire_owned){context.fire_active=false;context.fire_id=0;context.fire_caller=0;}
        if(aim_owned){context.aim_active=false;context.aim_id=0;context.aim_caller=0;}
    }
};
void reset_tls(){context={};}
void print(FILE* status){
    fprintf(status,"weapon_call_observer compiled=1 observation_only=1 tick_seen=%llu tick_sealed=%llu tick_rejected=%llu call_rejected=%llu outside_tick=%llu staged_dropped=%llu rows=%u rows_dropped=%llu aim_result_rejected=%llu fire_trace_rejected=%llu other_results_unread=1 impact_unread=1 game_actions_called=0\n",
        tick_seen.load(),tick_sealed.load(),tick_rejected.load(),call_rejected.load(),
        target_seen_outside_tick.load(),staged_dropped.load(),row_count,rows_dropped.load(),
        aim_result_rejected.load(),fire_trace_rejected.load());
    for(unsigned i=0;i<seen.size();++i)fprintf(status,
        "weapon_call_role role=%u seen=%llu accepted=%llu\n",i,seen[i].load(),accepted[i].load());
    std::lock_guard<std::mutex> lock(rows_mutex);
    for(unsigned i=0;i<row_count;++i){auto const& r=rows[i];
        fprintf(status,"weapon_call_row id=%llu fire_id=%llu parent_aim_id=%llu role=%u body_verified=%u tick=%llu world=%llu thread=%u receiver=%p caller=%p callee=%p result_address=%p opaque_rax=%p enter=%lld exit=%lld returned=%u caller_before_valid=%u caller_node_before=%p caller_code_before=%p caller_after_valid=%u caller_node_after=%p caller_code_after=%p aim_result_valid=%u aim_pitch=%d aim_yaw=%d aim_roll=%d fire_trace_valid=%u fire_start=%.9g,%.9g,%.9g fire_pushed=%.9g,%.9g,%.9g fire_end=%.9g,%.9g,%.9g\n",
            r.id,r.fire_id,r.parent_aim_id,r.role,r.body_verified?1:0,r.tick,r.world,r.thread,
            reinterpret_cast<void*>(r.receiver),reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.callee),
            reinterpret_cast<void*>(r.result_address),reinterpret_cast<void*>(r.opaque_rax),r.begin,r.end,
            r.returned?1:0,r.before_valid?1:0,reinterpret_cast<void*>(r.before.node),reinterpret_cast<void*>(r.before.code),
            r.after_valid?1:0,reinterpret_cast<void*>(r.after.node),reinterpret_cast<void*>(r.after.code),
            r.result_valid?1:0,r.result_rotator[0],r.result_rotator[1],r.result_rotator[2],
            r.trace_valid?1:0,r.trace_start[0],r.trace_start[1],r.trace_start[2],
            r.trace_pushed[0],r.trace_pushed[1],r.trace_pushed[2],
            r.trace_end[0],r.trace_end[1],r.trace_end[2]);
    }
}
#else
struct TickScope {explicit TickScope(crosshair_live::TickScope const&) {}};
struct CallScope {CallScope(void*,void*,void*,void*){}void returned(std::uintptr_t){}};
void reset_tls(){}
void print(FILE* status){fprintf(status,"weapon_call_observer compiled=0 observation_only=1\n");}
#endif
}
