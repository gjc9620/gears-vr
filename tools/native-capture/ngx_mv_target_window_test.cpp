#include "ngx_mv_target_window.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_ngx_mv_target;

static Selection selected(){return {17,2,3,40,0x100,0x200,0x300};}
static Context context(std::uint64_t ticket=41,std::uintptr_t renderer=0x900){
    auto s=selected();return {s.revision,s.world,ticket,s.handle,s.parameters,s.mv,renderer,true,true,false};
}
static Window waiting(){Window w;assert(w.select(selected(),100,100));return w;}
static Window capturing(){auto w=waiting();assert(w.consume_begin(context(),101));return w;}
static void succeeded(Window& w,Context const& c,std::uintptr_t scope,std::uint64_t now){
    assert(w.eval_enter(c,scope,now));assert(w.eval_leave(c,scope,true,true,now+1));
}
static void failed(Window const& w,Reason why){assert(w.snapshot().phase==Phase::failed&&w.snapshot().reason==why);}
static void complete_window(){
    auto w=waiting();auto excluded=context(40,0x800);
    assert(!w.consume_begin(excluded,101));assert(!w.eval_enter(excluded,1,102));
    assert(!w.eval_leave(excluded,1,true,true,103));assert(!w.consume_end(excluded,104));
    assert(w.snapshot().phase==Phase::waiting&&!w.snapshot().eval_count&&!w.snapshot().consume.renderer);
    // The next ticket can have a new renderer. No selection renderer is pinned.
    auto c=context(43,0xabc);assert(w.consume_begin(c,105));
    succeeded(w,c,0x501,106);succeeded(w,c,0x502,108);
    assert(w.consume_end(c,110));auto const before=w.snapshot();
    assert(before.phase==Phase::frozen&&before.selection.observation_id==17);
    assert(before.consume.ticket==43&&before.consume.renderer==0xabc&&before.completed_evals==2);
    assert(!w.consume_begin(context(44),500));assert(!w.select(selected(),0,10));
    assert(!w.eval_leave(context(),1,false,false,0));assert(!w.poll(0));
    assert(w.snapshot().phase==before.phase&&w.snapshot().last_now==before.last_now);
    assert(w.snapshot().completed_evals==2&&w.snapshot().consume.renderer==0xabc);
    // One successful Eval is sufficient scheduling evidence, not a pair claim.
    auto single=capturing();succeeded(single,context(),1,102);
    assert(single.consume_end(context(),104)&&single.snapshot().completed_evals==1);
}
static void changed_identities(){
    for(unsigned stage=0;stage<3;++stage)for(unsigned change=0;change<8;++change){
        auto w=stage?capturing():waiting();auto c=context();Reason why=Reason::none;
        switch(change){
        case 0:++c.revision;why=Reason::revision_changed;break;
        case 1:++c.world;why=Reason::world_changed;break;
        case 2:++c.handle;why=Reason::handle_changed;break;
        case 3:++c.parameters;why=Reason::parameters_changed;break;
        case 4:++c.mv;why=Reason::mv_changed;break;
        case 5:c.nested=true;why=Reason::nested;break;
        case 6:c.enabled=false;why=Reason::disabled;break;
        case 7:c.current=false;why=Reason::not_current;break;
        }
        if(stage==0)assert(!w.consume_begin(c,102));
        else if(stage==1)assert(!w.eval_enter(c,1,102));
        else {assert(w.eval_enter(context(),1,102));assert(!w.eval_leave(c,1,true,true,103));}
        failed(w,why);
    }
    auto old=waiting();assert(!old.consume_begin(context(39),101));failed(old,Reason::old_ticket);
    auto missing=waiting();assert(!missing.consume_begin(context(41,0),101));failed(missing,Reason::missing_renderer);
    auto newer=capturing();assert(!newer.consume_end(context(42),102));failed(newer,Reason::ticket_changed);
    auto renderer=capturing();assert(!renderer.consume_end(context(41,0x901),102));failed(renderer,Reason::renderer_changed);
    auto nesting=capturing();assert(!nesting.consume_begin(context(),102));failed(nesting,Reason::nested);
    auto creation=waiting();auto changed=context();++changed.revision;
    assert(!creation.observe_identity(changed,101));failed(creation,Reason::revision_changed);
}
static void eval_boundaries(){
    auto absent=capturing();assert(!absent.consume_end(context(),102));failed(absent,Reason::missing_eval);
    auto open=capturing();assert(open.eval_enter(context(),1,102));
    assert(!open.consume_end(context(),103));failed(open,Reason::eval_unfinished);
    auto nested=capturing();assert(nested.eval_enter(context(),1,102));
    assert(!nested.eval_enter(context(),2,103));failed(nested,Reason::eval_nested);
    auto scope=capturing();assert(scope.eval_enter(context(),1,102));
    assert(!scope.eval_leave(context(),2,true,true,103));failed(scope,Reason::eval_scope_changed);
    auto zero=capturing();assert(!zero.eval_enter(context(),0,102));failed(zero,Reason::missing_eval_scope);
    auto unopened=capturing();assert(!unopened.eval_leave(context(),1,true,true,102));failed(unopened,Reason::eval_not_open);
    auto outside=waiting();assert(!outside.eval_enter(context(),1,101));failed(outside,Reason::eval_outside_consume);
    auto end=waiting();assert(!end.consume_end(context(),101));failed(end,Reason::consume_not_open);
    for(unsigned i=0;i<2;++i){
        const bool returned=i!=0;
        auto w=capturing();assert(w.eval_enter(context(),1,102));
        assert(!w.eval_leave(context(),1,returned,false,103));
        failed(w,returned?Reason::eval_failed:Reason::eval_not_returned);
    }
    auto excess=capturing();succeeded(excess,context(),1,102);succeeded(excess,context(),2,104);
    assert(!excess.eval_enter(context(),3,106));failed(excess,Reason::eval_capacity);
}
static void deadlines_and_terminal(){
    auto before=waiting();assert(before.poll(199));assert(!before.poll(200));failed(before,Reason::deadline);
    auto capture=capturing();assert(capture.eval_enter(context(),1,102));
    assert(!capture.eval_leave(context(),1,true,true,200));failed(capture,Reason::deadline);
    auto regressed=waiting();assert(!regressed.poll(99));failed(regressed,Reason::clock_regressed);
    auto end=capturing();succeeded(end,context(),1,102);
    assert(!end.consume_end(context(),200));failed(end,Reason::deadline);
    auto again=waiting();assert(!again.select(selected(),101,100));failed(again,Reason::reselected);
    for(unsigned invalid=0;invalid<9;++invalid){
        Window w;auto s=selected();std::uint64_t now=100,budget=100;
        switch(invalid){
        case 0:s.observation_id=0;break;case 1:s.revision=0;break;case 2:s.world=0;break;
        case 3:s.ticket=0;break;case 4:s.handle=0;break;case 5:s.parameters=0;break;
        case 6:s.mv=0;break;case 7:budget=0;break;
        case 8:now=(std::numeric_limits<std::uint64_t>::max)()-1;budget=2;break;
        }
        assert(!w.select(s,now,budget));failed(w,Reason::invalid_selection);
    }
    auto dead=capturing();assert(!dead.consume_end(context(),102));auto snapshot=dead.snapshot();
    assert(!dead.select(selected(),0,1));assert(!dead.consume_begin(context(99),1));
    assert(!dead.eval_enter(context(),7,300));assert(!dead.eval_leave(context(),7,true,true,300));
    assert(!dead.consume_end(context(),300));assert(!dead.poll(300));
    assert(!dead.observe_identity(context(),300));
    assert(dead.snapshot().reason==snapshot.reason&&dead.snapshot().phase==snapshot.phase);
    assert(dead.snapshot().last_now==snapshot.last_now&&dead.snapshot().eval_count==snapshot.eval_count);
    assert(dead.snapshot().consume.ticket==snapshot.consume.ticket);
    Window idle;assert(!idle.poll(0)&&!idle.consume_begin(context(),0));
    assert(idle.snapshot().phase==Phase::idle);
}
int main(){
    complete_window();changed_identities();eval_boundaries();deadlines_and_terminal();
    std::puts("MV target consumption window CPU boundaries PASS");
}
