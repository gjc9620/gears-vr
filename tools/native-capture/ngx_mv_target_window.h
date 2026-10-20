#pragma once
#include <array>
#include <cstdint>
#include <limits>

// Pure CPU scheduling evidence for one targeted consumption window. The caller
// serializes access and supplies already trusted identities and monotonic ticks.
// No pointer is dereferenced, resource retained, SDK called, or GPU state proved.
namespace gears_ngx_mv_target {
enum class Phase { idle,waiting,capturing,frozen,failed };
enum class Reason {
    none,invalid_selection,reselected,clock_regressed,deadline,
    disabled,not_current,nested,revision_changed,world_changed,handle_changed,
    parameters_changed,mv_changed,old_ticket,missing_renderer,ticket_changed,
    renderer_changed,consume_not_open,eval_outside_consume,missing_eval_scope,
    eval_nested,eval_capacity,eval_not_open,eval_scope_changed,eval_not_returned,
    eval_failed,eval_unfinished,missing_eval
};
struct Selection {
    std::uint64_t observation_id{},revision{},world{},ticket{};
    std::uintptr_t handle{},parameters{},mv{};
};
struct Context {
    std::uint64_t revision{},world{},ticket{};
    std::uintptr_t handle{},parameters{},mv{},renderer{};
    bool enabled{},current{},nested{};
};
struct Eval {
    std::uintptr_t scope{};
    std::uint64_t entered_at{},returned_at{};
    bool returned{},succeeded{};
};
struct Snapshot {
    Phase phase=Phase::idle;Reason reason=Reason::none;
    Selection selection{};Context consume{};
    std::uint64_t selected_at{},deadline{},last_now{};
    std::array<Eval,2> evals{};
    unsigned eval_count{},completed_evals{};bool eval_open{};
};
class Window {
    Snapshot value_{};
    bool active()const{return value_.phase==Phase::waiting||value_.phase==Phase::capturing;}
    bool fail(Reason reason){
        if(active()||value_.phase==Phase::idle){value_.phase=Phase::failed;value_.reason=reason;}
        return false;
    }
    bool identity(Context const& c){
        auto const& s=value_.selection;
        if(c.revision!=s.revision)return fail(Reason::revision_changed);
        if(c.world!=s.world)return fail(Reason::world_changed);
        if(c.handle!=s.handle)return fail(Reason::handle_changed);
        if(c.parameters!=s.parameters)return fail(Reason::parameters_changed);
        if(c.mv!=s.mv)return fail(Reason::mv_changed);
        if(c.nested)return fail(Reason::nested);
        if(!c.enabled)return fail(Reason::disabled);
        if(!c.current)return fail(Reason::not_current);
        return true;
    }
    bool matching_consume(Context const& c){
        if(c.ticket!=value_.consume.ticket)return fail(Reason::ticket_changed);
        if(c.renderer!=value_.consume.renderer)return fail(Reason::renderer_changed);
        return true;
    }
    bool eval_context(Context const& c,std::uint64_t now){
        if(!poll(now)||!identity(c))return false;
        // Selection can occur inside its Eval. Neither that ticket's Eval nor
        // its trailing consume_end belongs to the subsequent target window.
        if(value_.phase==Phase::waiting){
            if(c.ticket==value_.selection.ticket)return false;
            return fail(Reason::eval_outside_consume);
        }
        return matching_consume(c);
    }
public:
    Snapshot const& snapshot()const{return value_;}
    bool select(Selection const& s,std::uint64_t now,std::uint64_t budget){
        if(value_.phase==Phase::frozen||value_.phase==Phase::failed)return false;
        if(value_.phase!=Phase::idle)return fail(Reason::reselected);
        if(!s.observation_id||!s.revision||!s.world||!s.ticket||!s.handle||!s.parameters||!s.mv||
           !budget||budget>(std::numeric_limits<std::uint64_t>::max)()-now)
            return fail(Reason::invalid_selection);
        value_.selection=s;value_.selected_at=value_.last_now=now;
        value_.deadline=now+budget;value_.phase=Phase::waiting;return true;
    }
    bool poll(std::uint64_t now){
        if(!active())return false;
        if(now<value_.last_now)return fail(Reason::clock_regressed);
        value_.last_now=now;
        if(now>=value_.deadline)return fail(Reason::deadline);
        return true;
    }
    // May also be called at an observed Create/role boundary before another
    // consume arrives. This checks identity without selecting a ticket/renderer.
    bool observe_identity(Context const& c,std::uint64_t now){
        return poll(now)&&identity(c);
    }
    bool consume_begin(Context const& c,std::uint64_t now){
        if(!poll(now)||!identity(c))return false;
        if(value_.phase==Phase::capturing)return fail(Reason::nested);
        if(c.ticket==value_.selection.ticket)return false;
        if(c.ticket<value_.selection.ticket)return fail(Reason::old_ticket);
        if(!c.renderer)return fail(Reason::missing_renderer);
        value_.consume=c;value_.phase=Phase::capturing;return true;
    }
    bool eval_enter(Context const& c,std::uintptr_t scope,std::uint64_t now){
        if(!eval_context(c,now))return false;
        if(!scope)return fail(Reason::missing_eval_scope);
        if(value_.eval_open)return fail(Reason::eval_nested);
        if(value_.eval_count==value_.evals.size())return fail(Reason::eval_capacity);
        value_.evals[value_.eval_count++]={scope,now,0,false,false};
        value_.eval_open=true;return true;
    }
    // Success is an independent observed SDK return supplied by the caller.
    // A consume boundary alone cannot manufacture an Eval success receipt.
    bool eval_leave(Context const& c,std::uintptr_t scope,bool returned,bool succeeded,std::uint64_t now){
        if(!eval_context(c,now))return false;
        if(!value_.eval_open)return fail(Reason::eval_not_open);
        auto& e=value_.evals[value_.eval_count-1];
        if(scope!=e.scope)return fail(Reason::eval_scope_changed);
        e.returned_at=now;e.returned=returned;e.succeeded=returned&&succeeded;
        value_.eval_open=false;
        if(!returned)return fail(Reason::eval_not_returned);
        if(!succeeded)return fail(Reason::eval_failed);
        ++value_.completed_evals;return true;
    }
    bool consume_end(Context const& c,std::uint64_t now){
        if(!poll(now)||!identity(c))return false;
        if(value_.phase==Phase::waiting){
            if(c.ticket==value_.selection.ticket)return false;
            return fail(Reason::consume_not_open);
        }
        if(!matching_consume(c))return false;
        if(value_.eval_open)return fail(Reason::eval_unfinished);
        if(!value_.completed_evals)return fail(Reason::missing_eval);
        value_.phase=Phase::frozen;return true;
    }
};
}
