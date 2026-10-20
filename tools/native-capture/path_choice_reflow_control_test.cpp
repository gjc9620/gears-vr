#define GEARS_PATH_CHOICE_REFLOW_CONTROL 1
#define main observer_contract_main
#include "path_choice_layout_observer_test.cpp"
#undef main
namespace control=observer::reflow_control;
namespace control_core=gears_path_reflow_control;
static unsigned native_calls{},native_mode{},native_nested{};
static std::vector<std::array<std::uint32_t,4>> native_inputs;
static std::promise<void>* control_entered{},*control_release{};
static position::Request* request_during_original{};
static uintptr_t __fastcall native_child(void* object,float left,float top,float right,float bottom,unsigned char type,int origin,int sort){
    ++native_calls;assert(object==expected_object&&type==1&&origin==0&&sort==0);
    native_inputs.push_back({trial::bits(left),trial::bits(top),trial::bits(right),trial::bits(bottom)});
    assert(trial::bits(left)==child_input[0]&&trial::bits(right)==child_input[2]);
    if(native_mode==8){control_entered->set_value();control_release->get_future().wait();}
    if(native_mode==7&&!native_nested){++native_nested;assert(call_child()==child_sentinel);--native_nested;}
    if(native_mode==7&&native_nested)return child_sentinel;
    auto const& c=position::state.lease.config;
    std::uint32_t t=trial::bits(top/1080.f),b=trial::bits(bottom/1080.f);
    if(native_mode==1){t=control_core::bits(control_core::value(c.top_bits)+trial::delta_for(c))+1;b=control_core::bits(control_core::value(c.bottom_bits)+trial::delta_for(c))+1;}
    if(native_mode==2){t=control_core::bits(control_core::value(c.top_bits)+trial::delta_for(c))+3;b=control_core::bits(control_core::value(c.bottom_bits)+trial::delta_for(c))+3;}
    put(c.panel+0x64,t);if(native_mode==6)throw 93;put(c.panel+0x6c,b);
    if(native_mode==3)put(c.panel+0x60,std::uint32_t(1042069800));
    if(native_mode==4)put(c.panel+0x70,std::uint32_t(0x04040405));
    if(native_mode==5)put(c.panel+0x4f8,std::uint32_t(2));
    // Change authorization after this original has performed its own normal
    // writes, before it returns to the controlled postcondition check.
    if(native_mode==9){assert(request_during_original);request_during_original->expires=0;}
    if(native_mode==10)position::disable();
    return child_sentinel;
}
struct ControlFixture:Fixture {
    std::shared_ptr<position::Request> request;
    ControlFixture(bool prefix_policy=false,unsigned offset=16,bool fit=false){
        control::current=nullptr;control::parent_depth=0;control::attempted=0;control::modified=0;control::adopted=0;control::native_restored=0;
        control::guard_rejected=0;control::failed=0;control::receipt_slots=0;
        for(unsigned i=0;i<4;++i){control::ready[i]=false;control::receipts[i]={};}
        position::first_auth_failure={};position::auth_failure_claimed=false;position::auth_failure_ready=false;
        position::restored=0;position::rejected=0;position::identity_lost=0;position::setter_failed=0;position::restore_retries=0;position::drain_started=0;
        position::unresolved=false;
        native_calls=0;native_mode=0;native_nested=0;native_inputs.clear();
        child_input={1133702550,1109496168,1154172058,1149367157};child_input_type=1;child_origin=0;child_sort=0;
        c.top_bits=1025061054;c.bottom_bits=1064725749;
        c.vertical_offset_reference_pixels=offset;
        if(fit){c.group_fit.enabled=true;const float values[]{.28663f,.06703f,.71394f,.2411013f,.25f,.75f};
            for(unsigned i=0;i<6;++i)c.group_fit.bounds[i]=trial::bits(values[i]);}
        put(c.panel+0x64,c.top_bits);put(c.panel+0x6c,c.bottom_bits);
        put(c.panel+0x60,std::uint32_t(1042069801));put(c.panel+0x68,std::uint32_t(1062785461));
        put(c.panel+0x1dc,c.scene);put(c.panel+0x4e4,static_cast<unsigned char>(0));put(c.panel+0x4f8,std::uint32_t(1));
        put(0x6000,std::uint32_t(1064098280));put(0x6004,std::uint32_t(1064098280));
        if(prefix_policy){c.schema=2;c.registry_header_policy=trial::RegistryHeaderPolicy::fixed_prefix_v1;
            for(unsigned i=0;i<2;++i){trial::RegistryHeader header{0x700000+i*0x100000,100,200};trial::Check check;
                check.address=c.base+trial::registry_header_rvas[i];check.bytes.resize(16);std::memcpy(check.bytes.data(),&header,16);
                c.checks.push_back(check);put(check.address,header);}}
        request=std::make_shared<position::Request>();request->config=c;request->expires=GetTickCount64()+6000;
        std::atomic_store(&position::selected,std::shared_ptr<position::Request const>(request));
        apply();run_child_in_parent=true;observer::child_original=native_child;
    }
    auto setter(){return [&](auto panel,float value,unsigned face,unsigned,unsigned){put(panel+0x60+face*4,trial::bits(value));return true;};}
    auto positions(){std::array<std::uint32_t,2> p{};assert(trial::positions(c,safe_copy,p));return p;}
};
static void six_reflows_and_exact_core_restore(){
    ControlFixture f;const auto initial=position::state.lease.owned;
    for(unsigned i=0;i<6;++i){f.call();assert(control::adopted==i+1&&control::modified==i+1&&position::state.lease.active&&position::state.lease.owned==f.positions());}
    assert(native_calls==6&&original_calls==6&&observer::slots==4&&observer::child_slots==4&&control::receipt_slots==4);
    assert(position::state.lease.owned[0]+1==initial[0]&&position::state.lease.owned[1]==initial[1]);
    for(auto const& input:native_inputs){assert(input[0]==child_input[0]&&input[2]==child_input[2]);
        assert(input[1]==trial::bits(trial::value(child_input[1])+16.f)&&input[3]==trial::bits(trial::value(child_input[3])+16.f));}
    f.request->expires=0;struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
    unsigned originals{};assert(position::invoke_with(candidate,[&]{++originals;return sentinel;},safe_copy,f.setter(),[]{return true;})==sentinel);
    assert(originals==1&&position::restored==1&&!position::pending_restore&&!position::state.lease.active);
    assert((f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits})&&control::modified==6);
}
static void exact_holding_stays_exact(){
    ControlFixture f;f.call();auto owned=position::state.lease.owned;
    struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;unsigned setters{};
    auto setter=[&](auto,float,unsigned,unsigned,int){++setters;return true;};
    position::invoke_with(candidate,[]{return sentinel;},safe_copy,setter,[]{return true;});assert(position::ownership_lost==0&&setters==0);
    put(f.c.panel+0x64,owned[0]+1);
    position::invoke_with(candidate,[]{return sentinel;},safe_copy,setter,[]{return true;});
    assert(position::ownership_lost==1&&position::unresolved&&position::terminal&&setters==0);
}
static void one_ulp_own_adoption_only(){
    ControlFixture f;native_mode=1;f.call();
    assert(control::adopted==1&&position::state.lease.owned==f.positions()&&!position::terminal);
    assert(position::state.lease.owned[0]==trial::bits(trial::value(f.c.top_bits)+trial::delta)+1);
}
static void reject_bad_exit_and_exception(){
    for(unsigned mode_bad=2;mode_bad<=6;++mode_bad){ControlFixture f;auto owned=position::state.lease.owned;native_mode=mode_bad;
        if(mode_bad==6){try{f.call();assert(false);}catch(int n){assert(n==93);}}else f.call();
        assert(native_calls==1&&control::modified==1&&control::adopted==0&&control::failed==1&&position::state.lease.owned==owned);
        assert(position::unresolved&&position::terminal&&!position::state.lease.active&&!position::pending_restore&&control::ready[0]);
        assert(control::receipts[0].status==(mode_bad==6?control::Status::exception:control::Status::post_rejected));
        assert(observer::child_depth==0&&control::parent_depth==0&&!control::current&&!observer::parent_scope&&active==0);
    }
}
static void expired_or_draining_native_restore(){
    for(bool drain:{false,true}){ControlFixture f;f.call();auto owned=position::state.lease.owned;
        if(drain)position::request_drain();else f.request->expires=0;
        f.call();assert(native_calls==2&&native_inputs.back()==child_input&&control::modified==1&&control::adopted==1&&control::native_restored==1);
        assert(position::restored==1&&!position::state.lease.active&&!position::pending_restore&&!position::unresolved&&position::state.lease.owned==owned);
        assert((f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));
        assert(control::receipts[1].status==control::Status::native_restored&&!control::receipts[1].changed);
    }
}
static void entry_guard_and_metadata(){
    for(unsigned bad=0;bad<5;++bad){ControlFixture f;auto owned=position::state.lease.owned;
        if(bad==0)put(f.c.panel+0x64,owned[0]+1);
        if(bad==1)put(f.c.panel+0x4f8,std::uint32_t(2));
        if(bad==2)child_input[1]=trial::bits(100.f);
        if(bad==3)put(f.c.panel+0x4e4,static_cast<unsigned char>(1));
        if(bad==4)put(f.c.panel+0x4f4,int(3));
        f.call();assert(native_calls==1&&native_inputs.back()==child_input&&control::modified==0&&control::adopted==0&&control::guard_rejected==1);
        assert(position::state.lease.owned==owned&&position::terminal&&position::unresolved&&!position::state.lease.active);
    }
}
static void foreign_and_nested_passthrough(){
    {ControlFixture f;auto owned=position::state.lease.owned;
        std::thread foreign([&]{f.call();});foreign.join();assert(control::modified==0&&control::adopted==0&&native_inputs.back()==child_input&&position::state.lease.owned==owned);}
    {ControlFixture f;native_mode=7;f.call();assert(native_calls==2&&control::modified==1&&control::adopted==1);
        assert(native_inputs[1]==child_input&&native_inputs[0][1]!=child_input[1]);}
    {ControlFixture f;mode=2;f.call();assert(native_calls==2&&control::adopted==0&&control::modified==0&&position::terminal);
        assert(native_inputs[0]==child_input&&native_inputs[1]==child_input);}
}
static void pure_helper_bounds(){
    ControlFixture f;control::Frame frame{f.c.panel,f.c.scene,f.c.client,0,f.c.nonce,f.c.world_generation};
    auto before=observer::snapshot(frame,safe_copy);control::Contract contract{f.c.scene,f.c.top_bits,f.c.bottom_bits};
    control_core::Input input{child_input,1,0,0};control_core::Plan plan;
    assert(control_core::prepare(contract,before,input,trial::delta,plan)&&plan.height==1080.f&&plan.offset==16.f);
    for(unsigned bad=0;bad<7;++bad){auto changed=input;
        if(bad==0)changed.faces[1]=0x7fc12345;if(bad==1)changed.faces[3]=0x7f800000;if(bad==2)changed.faces[1]=trial::bits(-1.f);
        if(bad==3)changed.type=0;if(bad==4)changed.origin=1;if(bad==5)changed.sort=1;if(bad==6)changed.faces[3]=changed.faces[1];
        assert(!control_core::prepare(contract,before,changed,trial::delta,plan));}
}
static void restoring_bypasses_control(){
    ControlFixture f;f.call();auto owned=position::state.lease.owned;position::restore_retries=1;f.call();
    assert(control::modified==1&&control::adopted==1&&control::native_restored==0&&native_inputs.back()==child_input&&position::state.lease.owned==owned);
}
static void no_active_lease_or_wrong_object(){
    {ControlFixture f;position::state.lease.touched[1]=false;auto owned=position::state.lease.owned;f.call();
        assert(control::modified==0&&control::adopted==0&&native_inputs.back()==child_input&&position::state.lease.owned==owned);}
    {ControlFixture f;auto owned=position::state.lease.owned;expected_object=reinterpret_cast<void*>(f.c.panel+8);f.call();
        assert(control::modified==0&&control::adopted==0&&native_inputs.back()==child_input&&position::state.lease.owned==owned);}
    {ControlFixture f;auto owned=position::state.lease.owned;++committed_world_generation;f.call();
        assert(control::modified==0&&control::adopted==0&&native_inputs.back()==child_input&&position::state.lease.owned==owned);}
}
static void control_receipt_publication(){
    ControlFixture f;native_mode=8;std::promise<void> start,release;control_entered=&start;control_release=&release;
    std::thread owner([&]{position::owner_thread=GetCurrentThreadId();f.call();});start.get_future().wait();
    auto text=progress();assert(text.find("path_reflow_control_receipts=1")!=std::string::npos&&text.find("path_reflow_control_0=")==std::string::npos);
    release.set_value();owner.join();text=progress();assert(text.find("path_reflow_control_0=")!=std::string::npos&&control::ready[0]);
}
static void authorization_changes_during_original(){
    struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
    for(unsigned change:{9u,10u})for(bool native_restore_before_core:{false,true}){
        ControlFixture f;request_during_original=f.request.get();native_mode=change;f.call();
        assert(native_calls==1&&control::modified==1&&control::adopted==1&&control::failed==0&&position::state.lease.active);
        assert(change==9?f.request->expires==0:!std::atomic_load(&position::selected));
        auto const owned=position::state.lease.owned;assert(owned==f.positions());
        // The already-authorized call still uses only its own <=2 ULP path,
        // including the native rounding difference from initial fractional apply.
        assert(owned[0]+1==trial::bits(trial::value(f.c.top_bits)+trial::delta));
        assert(control_core::within_ulps(owned[0],control::receipts[0].plan.expected[0])&&
               control_core::within_ulps(owned[1],control::receipts[0].plan.expected[1]));
        assert(control::receipts[0].changed&&control::receipts[0].normal&&control::receipts[0].status==control::Status::adopted);
        if(native_restore_before_core){native_mode=0;f.call();
            assert(native_calls==2&&native_inputs.back()==child_input&&control::modified==1&&control::adopted==1&&control::native_restored==1);
            assert(position::state.lease.owned==owned&&!position::state.lease.active&&!position::state.lease.touched[0]&&!position::state.lease.touched[1]);
            assert(!control::receipts[1].changed&&control::receipts[1].status==control::Status::native_restored);
        }
        unsigned setters{},originals{};
        auto setter=[&](auto panel,float value,unsigned face,unsigned input,int origin){++setters;return f.setter()(panel,value,face,input,origin);};
        assert(position::invoke_with(candidate,[&]{++originals;return sentinel;},safe_copy,setter,[]{return true;})==sentinel);
        assert(originals==1&&setters==(native_restore_before_core?0u:2u)&&position::restored==1&&position::ownership_lost==0);
        assert(!position::pending_restore&&!position::state.lease.active&&!position::unresolved&&!position::terminal);
        assert((f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));
        request_during_original=nullptr;
    }
}
static void schema2_suffix_with_control_and_exact_restore(){
    ControlFixture f(true);struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
    for(int num:{101,120,100,110,105,130}){
        for(unsigned i=0;i<2;++i)put(f.c.base+trial::registry_header_rvas[i],trial::RegistryHeader{0x700000+i*0x100000,num,200});
        f.call();assert(position::state.lease.owned==f.positions()&&!position::terminal);
        position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});
        assert(!position::auth_failure_ready&&!position::identity_lost&&!position::ownership_lost);
    }
    assert(control::adopted==6&&control::modified==6&&control::receipt_slots==4);
    assert(position::state.lease.owned[0]+1==trial::bits(trial::value(f.c.top_bits)+trial::delta));
    f.request->expires=0;position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});
    assert(position::restored==1&&!position::unresolved&&!position::state.lease.active&&
        (f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));
}
static void offset374_native_holding_restore(){
    for(unsigned rounding:{0u,1u}){ControlFixture f(true,374);native_mode=rounding;
        auto initial=position::state.lease.owned;assert(initial[0]==trial::bits(trial::value(f.c.top_bits)+374.f/1080.f));
        for(unsigned i=0;i<6;++i){f.call();assert(control::adopted==i+1&&position::state.lease.owned==f.positions()&&!position::terminal);}
        assert(control::receipts[0].plan.height==1080.f&&control::receipts[0].plan.offset==374.f&&control::modified==6);
        for(auto const& input:native_inputs)assert(input[0]==child_input[0]&&input[2]==child_input[2]&&
            input[1]==trial::bits(trial::value(child_input[1])+374.f)&&input[3]==trial::bits(trial::value(child_input[3])+374.f));
        struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
        position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});assert(!position::ownership_lost&&position::state.lease.active);
        f.request->expires=0;position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});
        assert(position::restored==1&&!position::unresolved&&!position::state.lease.active&&
            (f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));
    }
}
static void same_nonce_offset_mismatch(){
    {ControlFixture f(true,374);control::Parent p{{f.c.panel,f.c.scene,f.c.client,0,f.c.nonce,f.c.world_generation},
            {f.c.scene,f.c.top_bits,f.c.bottom_bits,374},trial::delta_for(f.c)};
        assert(control::same_lease(p)&&control::request_current(p));f.request->config.vertical_offset_reference_pixels=16;
        assert(!control::request_current(p));f.call();assert(control::modified==0&&control::adopted==0&&control::native_restored==1);
        assert(native_inputs.back()==child_input&&!position::state.lease.active);}
    {ControlFixture f(true,374);control::Parent p{{f.c.panel,f.c.scene,f.c.client,0,f.c.nonce,f.c.world_generation},
            {f.c.scene,f.c.top_bits,f.c.bottom_bits,16},trial::delta};assert(!control::same_lease(p));
        f.request->config.vertical_offset_reference_pixels=16;struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
        position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});
        assert(position::restored==1&&!position::state.lease.active&&control::modified==0&&
            (f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));}
}
static void fit_native_holding_restore(){
    ControlFixture f(true,16,true);const float fraction=trial::delta_for(f.c);
    assert(fraction>0&&fraction<374.f/1080.f);
    for(unsigned i=0;i<6;++i){f.call();assert(control::adopted==i+1&&position::state.lease.owned==f.positions()&&!position::terminal);}
    assert(control::receipts[0].plan.offset==1080.f*fraction&&control::modified==6);
    control::Parent p{{f.c.panel,f.c.scene,f.c.client,0,f.c.nonce,f.c.world_generation},
        {f.c.scene,f.c.top_bits,f.c.bottom_bits,16,f.c.group_fit},fraction};
    assert(control::same_lease(p)&&control::request_current(p));
    f.request->config.group_fit.bounds[4]++;assert(!control::request_current(p));
    f.request->config.group_fit=f.c.group_fit;f.request->expires=0;
    struct Candidate {uintptr_t client=0x1000,scene=0x2000;} candidate;
    position::invoke_with(candidate,[]{return sentinel;},safe_copy,f.setter(),[]{return true;});
    assert(position::restored==1&&!position::unresolved&&!position::state.lease.active&&
        (f.positions()==std::array<std::uint32_t,2>{f.c.top_bits,f.c.bottom_bits}));
}
int main(){six_reflows_and_exact_core_restore();exact_holding_stays_exact();one_ulp_own_adoption_only();reject_bad_exit_and_exception();expired_or_draining_native_restore();entry_guard_and_metadata();foreign_and_nested_passthrough();pure_helper_bounds();restoring_bypasses_control();no_active_lease_or_wrong_object();control_receipt_publication();authorization_changes_during_original();schema2_suffix_with_control_and_exact_restore();offset374_native_holding_restore();same_nonce_offset_mismatch();fit_native_holding_restore();std::puts("PathChoiceReflowControlTest passed");}
