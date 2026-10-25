// Derived mechanically from Cover's actual UI SEH fixture. This compiles the
// candidate production hook, wrapper, and byte-identical Weapon observer.
// UI geometry and crosshair certificate providers are explicit synthetic host
// seams; actual frame/ancestry helpers and observer guards execute unchanged.
// Runtime transport here is a scope seam; separate actual Broker/runtime tests
// cover real scope/debt semantics. No game, native module or hook installation.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GEARS_UI_CALL_BOUNDARY_OBSERVER 1
#ifndef GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
#define GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER 1
#endif
#include <Windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <type_traits>
#include "ui_call_boundary.h"

using std::uintptr_t;
using std::uint64_t;
unsigned checks{};
void check(bool condition,int line){++checks;if(!condition){std::fprintf(stderr,"check failed line=%d\n",line);std::abort();}}
#define CHECK(condition) check(bool(condition),__LINE__)
constexpr uintptr_t sentinel=0xfedcba9876543210ull;
constexpr DWORD game_exception=0xe1234567ul;
std::atomic<bool> recording{true};
bool ui_observer_enabled=true,world_allowed=true,metadata_current=true,invocation_current=true;
uintptr_t base=0x140000000ull;
long long ticks{};
long long now(){return ++ticks;}
unsigned active{},admissions{},scope_created{},scope_destroyed{},cover_created{},cover_destroyed{},
    cover_returned{},logical_created{},logical_destroyed{},logical_returned{},native_depth{},original_calls{};
unsigned constructor_fault{};
bool observer_fire_active();bool observer_aim_active();
bool transport_admitted=true,transport_stopped=false,expect_observer=false;
bool controller_input_observer=true;
std::atomic<uint64_t> committed_world_generation{1};
std::array<int,128> destruction{};unsigned destruction_count{};
void destroyed(int kind){CHECK(destruction_count<destruction.size());destruction[destruction_count++]=kind;}

namespace gears_ui {
struct Identity {uintptr_t address{},outer{};};
struct Registry {uintptr_t data{};int count{},capacity{};};
struct ScopeConfig {unsigned kind{},function{},owner{};uintptr_t script{};unsigned size{};};
struct Config {
    unsigned scope_count{},identity_count{};
    std::array<ScopeConfig,2> scopes{};std::array<Identity,3> identities{};
    uintptr_t objects_header{},names_header{},base{};
};
struct ScopeContext {unsigned kind{},scope_index{};uintptr_t function{},object{},code{};};
constexpr std::array<uintptr_t,1> binding_rvas{{0x1d6ca0}};
template<class Read> struct Reader {
    Read read;
    bool field(Registry& out,uintptr_t,unsigned) const {out={0x8000,32,32};return metadata_current;}
    bool field(uintptr_t& out,uintptr_t,unsigned) const {out=::base+binding_rvas[0];return metadata_current;}
};
template<class R> bool registry(R const&,uintptr_t,Registry& out){out={0x9000,3,3};return metadata_current;}
template<class R> bool fresh_identity(Config const&,R const&,Registry const&,Registry const&,Identity const&){return metadata_current;}
template<class R> bool live_scope_object(Config const&,R const&,unsigned,uintptr_t){return metadata_current;}
}
struct UiInvocation {gears_ui::ScopeContext context;uint64_t sequence{};gears_ui::Config const* frozen{};} ui_invocation;
bool ui_invocation_current(){return invocation_current;}
namespace ui_call_boundary {
struct Row;
extern thread_local Row* current_row;
extern thread_local gears_ui_call_boundary::Nesting nesting;
}
struct Scope;
thread_local Scope* scope_tls{};
struct Scope {
    Scope* previous=scope_tls;bool admitted=world_allowed;
    Scope(){scope_tls=this;++active;++scope_created;if(admitted)++admissions;}
    ~Scope(){CHECK(scope_tls==this);scope_tls=previous;--active;++scope_destroyed;if(admitted)--admissions;destroyed(1);}
    bool world() const{return admitted;}
};
namespace cover_call_live {
struct CallScope;
thread_local CallScope* current{};
struct CallScope {
    CallScope* previous=current;
    explicit CallScope(void*,void*,void*,void*){
        if(constructor_fault==2)RaiseException(game_exception,0,0,nullptr);
        current=this;++cover_created;
    }
    void returned(uintptr_t value){CHECK(value==sentinel);++cover_returned;}
    ~CallScope(){CHECK(current==this);current=previous;++cover_destroyed;destroyed(2);}
};
}
namespace ui_logical_group_live {
struct ChildScope;
thread_local ChildScope* current_child{};
bool boundary_candidate(){return true;}
struct ChildScope {
    ChildScope* previous=current_child;
    ui_call_boundary::Row* saved_row=ui_call_boundary::current_row;
    gears_ui_call_boundary::Child const* saved_child=ui_call_boundary::nesting.current;
    unsigned saved_depth=ui_call_boundary::nesting.depth;
    bool selected{};bool saved_weapon_fire=observer_fire_active(),saved_weapon_aim=observer_aim_active();
    ChildScope(void*,void*,void*,bool candidate):selected(candidate){
        if(constructor_fault==3)RaiseException(game_exception,0,0,nullptr);
        ++logical_created;if(selected)current_child=this;
    }
    template<class Original> uintptr_t invoke(Original original){
        const auto value=original();if(selected)++logical_returned;return value;
    }
    ~ChildScope(){
        CHECK(observer_fire_active()==saved_weapon_fire&&observer_aim_active()==saved_weapon_aim);
        CHECK(ui_call_boundary::current_row==saved_row);
        CHECK(ui_call_boundary::nesting.current==saved_child&&ui_call_boundary::nesting.depth==saved_depth);
        if(selected){CHECK(current_child==this);current_child=previous;}
        ++logical_destroyed;destroyed(3);
    }
};
}
alignas(8) std::array<std::array<unsigned char,0x40>,2> frames{};
bool safe_copy(void* out,void const* in,std::size_t bytes){
    const auto address=reinterpret_cast<uintptr_t>(in);
    for(auto const& frame:frames){const auto start=reinterpret_cast<uintptr_t>(frame.data());
        if(address>=start&&address-start<=frame.size()&&bytes<=frame.size()-(address-start)){
            std::memcpy(out,in,bytes);return true;
        }
    }
    return false;
}
constexpr int MH_OK=0;
int MH_CreateHook(void*,void*,void**){return 1;}
// The production bridge is compiled here too. Its synthetic transport exposes
// the entire begin-original-end gap, including all UI admission branches.
namespace gears_weapon_hand {
struct CallScope {CallScope* previous;uintptr_t args[4];unsigned id;bool active;};
struct ActionScope {bool active;bool revoke_owner_cache;};
}
namespace weapon_hand_runtime {
thread_local gears_weapon_hand::CallScope* current_call{};
unsigned begun{},finished{},returned_calls{},unwound_calls{},pending{},action_begun{},action_finished{},action_pending{};
bool begin_action(uintptr_t,uintptr_t,uintptr_t,uintptr_t,gears_weapon_hand::ActionScope& scope) noexcept {
    if(!transport_admitted||transport_stopped)return false;
    scope.active=true;++action_begun;++action_pending;return true;
}
void finish_action(gears_weapon_hand::ActionScope& scope,bool,uint64_t) noexcept {
    if(!scope.active)return;
    CHECK(action_pending&&pending<action_pending);scope.active=false;--action_pending;++action_finished;
}
bool begin_call(uintptr_t self,uintptr_t caller,uintptr_t result,uintptr_t callee,gears_weapon_hand::CallScope& scope,gears_weapon_hand::ActionScope const* =nullptr) noexcept {
    CHECK(!scope.active);if(!transport_admitted||transport_stopped)return false;
    scope={current_call,{self,caller,result,callee},++begun,true};
    current_call=&scope;++pending;return true;
}
void finish_call(gears_weapon_hand::CallScope& scope,bool returned,uint64_t rax) noexcept {
    if(!scope.active)return;CHECK(current_call==&scope&&pending);
    if(returned){CHECK(rax==sentinel);++returned_calls;}else ++unwound_calls;
    scope.active=false;current_call=scope.previous;--pending;++finished;
}
}
#include "weapon_call_live.inl"

// The original fixture already models UI identities. These additional inputs
// provide the crosshair certificate/outer-Tick side of the same owned buffer.
// They do not replace the production observer or frame ancestry functions.
namespace gears_crosshair_assignment {
struct Assignment {unsigned controller=1,pawn=2,weapon=3;};
template<class Read> bool validate_bindings(Assignment const&,Read,bool){return metadata_current;}
}
namespace crosshair_live {
struct Identity {uintptr_t address{};};
struct Identities {unsigned identity_count=10;std::array<Identity,10> identities{};};
struct Call {unsigned function{};uintptr_t script{};unsigned size{};};
struct FireTrace {unsigned locals_size=64;std::array<unsigned,3> offsets{0,12,24};};
struct Config {
    bool mounted_suspended{};unsigned weapon_call_count=6;Identities identities{};
    gears_crosshair_assignment::Assignment assignment{};
    std::array<Call,2> tick_roots{};std::array<Call,6> weapon_calls{};FireTrace weapon_fire_trace{};
} config;
struct State {uint64_t token=1;uintptr_t frame{};} state;
std::atomic<uint64_t> config_generation{1};
Config const& current(){return config;}
bool root_binding(unsigned){return metadata_current;}
bool weapon_call_binding(Config const&,unsigned){return metadata_current;}
struct TickScope {bool entered=true;uint64_t token=1,world=1;uintptr_t previous_frame{};};
void reset_fixture(){
    config={};config.identities.identities[0].address=0x5000;
    config.identities.identities[1].address=0x1000;
    config.identities.identities[2].address=0x1100;
    config.identities.identities[3].address=0x1000;
    config.tick_roots[0]={0,0x8000,0x200};
    for(unsigned i=0;i<6;++i){
        config.identities.identities[4+i].address=0x3000+i*16;
        config.weapon_calls[i]={4+i,0x8000+i*0x100,0x200};
    }
    state={1,reinterpret_cast<uintptr_t>(frames[0].data())};
}
}
#include "weapon_call_observer_live.inl"
bool observer_fire_active(){
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    return weapon_call_live::context.fire_active;
#else
    return false;
#endif
}
bool observer_aim_active(){
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    return weapon_call_live::context.aim_active;
#else
    return false;
#endif
}

#include "ui_call_boundary_live.inl"

enum class Mode {normal,seh,nested,nested_caught_seh,nested_unhandled_seh,stop_original};
Mode mode{};
struct Arguments {void* self;void* caller;void* result;void* callee;};
std::array<Arguments,2> arguments{};
gears_ui::Config config{};
bool expect_selected=true,expect_cover=true,expect_logical=true;
DWORD observed_exception{};
bool call_caught(Arguments const& args,uintptr_t& value){
    __try {value=ui_call_boundary::hook(args.self,args.caller,args.result,args.callee);}
    __except(GetExceptionCode()==game_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){
        observed_exception=GetExceptionCode();return true;
    }
    return false;
}
uintptr_t __fastcall original_call(void* self,void* caller,void* result,void* callee){
    CHECK(native_depth<arguments.size());auto const& expected=arguments[native_depth];
    CHECK(self==expected.self&&caller==expected.caller&&result==expected.result&&callee==expected.callee);
    ++original_calls;++native_depth;
    CHECK(weapon_hand_runtime::pending==(transport_admitted?native_depth:0));
    if(transport_admitted){
        CHECK(weapon_hand_runtime::current_call);
        auto const& call=*weapon_hand_runtime::current_call;
        CHECK(call.args[0]==reinterpret_cast<uintptr_t>(self)&&call.args[1]==reinterpret_cast<uintptr_t>(caller)&&
            call.args[2]==reinterpret_cast<uintptr_t>(result)&&call.args[3]==reinterpret_cast<uintptr_t>(callee));
    }else CHECK(!weapon_hand_runtime::current_call&&!weapon_hand_runtime::action_pending);
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    if(expect_observer)CHECK(weapon_call_live::context.active);
#endif
    CHECK(active==native_depth&&scope_tls!=nullptr);
    CHECK(admissions==(world_allowed?native_depth:0));
    CHECK(bool(cover_call_live::current)==expect_cover);
    CHECK(bool(ui_logical_group_live::current_child)==expect_logical);
    if(expect_selected){
        CHECK(ui_call_boundary::current_row!=nullptr&&ui_call_boundary::nesting.current==&ui_call_boundary::current_row->child);
        CHECK(ui_call_boundary::nesting.depth==native_depth);
        CHECK(ui_call_boundary::current_row->child.self==reinterpret_cast<uintptr_t>(self));
    }
    auto* saved_scope=scope_tls;auto* saved_cover=cover_call_live::current;
    auto* saved_logical=ui_logical_group_live::current_child;
    auto* saved_row=ui_call_boundary::current_row;
    auto const* saved_child=ui_call_boundary::nesting.current;
    const unsigned saved_depth=ui_call_boundary::nesting.depth;
    __try {
        if(mode==Mode::stop_original)transport_stopped=true;
        if(mode==Mode::seh||(native_depth==2&&(mode==Mode::nested_caught_seh||mode==Mode::nested_unhandled_seh)))
            RaiseException(game_exception,0,0,nullptr);
        if(native_depth==1&&(mode==Mode::nested||mode==Mode::nested_caught_seh||mode==Mode::nested_unhandled_seh)){
            uintptr_t value{};auto const& inner=arguments[1];
            if(mode==Mode::nested_caught_seh){CHECK(call_caught(inner,value));CHECK(observed_exception==game_exception);}
            else {value=ui_call_boundary::hook(inner.self,inner.caller,inner.result,inner.callee);CHECK(value==sentinel);}
            CHECK(active==1&&admissions==1&&scope_tls==saved_scope);
            CHECK(cover_call_live::current==saved_cover&&ui_logical_group_live::current_child==saved_logical);
            CHECK(ui_call_boundary::current_row==saved_row&&ui_call_boundary::nesting.current==saved_child&&ui_call_boundary::nesting.depth==saved_depth);
        }
    }__finally {--native_depth;}
    return sentinel;
}
void reset(){
    CHECK(!active&&!admissions&&!scope_tls&&!cover_call_live::current&&!ui_logical_group_live::current_child&&!native_depth);
    CHECK(!weapon_hand_runtime::pending&&!weapon_hand_runtime::current_call);
    weapon_hand_runtime::begun=weapon_hand_runtime::finished=weapon_hand_runtime::returned_calls=weapon_hand_runtime::unwound_calls=0;
    recording=true;ui_observer_enabled=world_allowed=metadata_current=invocation_current=true;
    transport_admitted=true;transport_stopped=false;expect_observer=false;
    weapon_hand_runtime::action_begun=weapon_hand_runtime::action_finished=weapon_hand_runtime::action_pending=0;
    weapon_call_live::reset_tls();
    expect_selected=expect_cover=expect_logical=true;constructor_fault=0;observed_exception=0;mode=Mode::normal;
    scope_created=scope_destroyed=cover_created=cover_destroyed=cover_returned=0;
    logical_created=logical_destroyed=logical_returned=original_calls=destruction_count=0;
    ui_call_boundary::reset_tls();ui_call_boundary::samples={};
    ui_call_boundary::calls=0;ui_call_boundary::candidates=0;ui_call_boundary::rejected=0;
    ui_call_boundary::depth_rejected=0;ui_call_boundary::accepted=0;ui_call_boundary::completed=0;
    config={};config.base=base;config.scope_count=1;config.identity_count=2;
    config.identities[0]={0x3000,0x4000};config.identities[1]={0x4000,0};
    config.scopes[0]={1,0,1,0x8000,32};ui_invocation={{1,0},77,&config};
    for(unsigned i=0;i<frames.size();++i){frames[i]={};const uintptr_t node=0x5000,object=0x1000,code=0x8000;
        std::memcpy(frames[i].data()+0x18,&node,8);std::memcpy(frames[i].data()+0x20,&object,8);std::memcpy(frames[i].data()+0x28,&code,8);
        arguments[i]={reinterpret_cast<void*>(0x1000ull),frames[i].data(),reinterpret_cast<void*>(0x2000ull+i*8),reinterpret_cast<void*>(0x3000)};
    }
    const auto parent=reinterpret_cast<uintptr_t>(frames[0].data());
    std::memcpy(frames[1].data()+0x38,&parent,8);
    crosshair_live::reset_fixture();
    ui_call_boundary::original=original_call;
}
void drained(unsigned scopes,unsigned covers,unsigned logicals){
    CHECK(!active&&!admissions&&!scope_tls&&!cover_call_live::current&&!ui_logical_group_live::current_child&&!native_depth);
    CHECK(!ui_call_boundary::current_row&&!ui_call_boundary::nesting.current&&!ui_call_boundary::nesting.depth);
    CHECK(scope_created==scopes&&scope_destroyed==scopes);
    CHECK(cover_created==covers&&cover_destroyed==covers);
    CHECK(logical_created==logicals&&logical_destroyed==logicals);
    CHECK(!weapon_hand_runtime::pending&&!weapon_hand_runtime::current_call);
    const unsigned notices=transport_admitted?original_calls:0;
    CHECK(weapon_hand_runtime::begun==notices&&weapon_hand_runtime::finished==notices);
    CHECK(weapon_hand_runtime::returned_calls+weapon_hand_runtime::unwound_calls==notices);
    CHECK(!weapon_hand_runtime::action_pending&&weapon_hand_runtime::action_begun==notices&&weapon_hand_runtime::action_finished==notices);
    CHECK(!observer_fire_active()&&!observer_aim_active());
}
void run(bool fault,unsigned scopes=1,unsigned covers=1,unsigned logicals=1){
    uintptr_t value{};
    {
        crosshair_live::TickScope source_tick;
        weapon_call_live::TickScope actual_tick(source_tick);
        CHECK(call_caught(arguments[0],value)==fault);
        if(fault)CHECK(observed_exception==game_exception);else CHECK(value==sentinel);
        CHECK(!observer_fire_active()&&!observer_aim_active());
    }
    drained(scopes,covers,logicals);
}
int main(){
    reset();run(false);CHECK(original_calls==1&&cover_returned==1&&logical_returned==1&&ui_call_boundary::completed==1);
    CHECK(destruction_count==3&&destruction[0]==3&&destruction[1]==2&&destruction[2]==1);
    reset();mode=Mode::seh;run(true);CHECK(original_calls==1&&!cover_returned&&!logical_returned&&!ui_call_boundary::completed);
    CHECK(weapon_hand_runtime::unwound_calls==1&&!weapon_hand_runtime::returned_calls);
    CHECK(destruction_count==3&&destruction[0]==3&&destruction[1]==2&&destruction[2]==1);
    reset();mode=Mode::nested;run(false,2,2,2);CHECK(original_calls==2&&cover_returned==2&&ui_call_boundary::completed==2);
    CHECK(ui_call_boundary::samples.rows[0][0].child.parent_child==ui_call_boundary::samples.rows[0][1].child.id);
    reset();mode=Mode::nested_caught_seh;run(false,2,2,2);CHECK(original_calls==2&&cover_returned==1&&ui_call_boundary::completed==1);
    CHECK(weapon_hand_runtime::returned_calls==1&&weapon_hand_runtime::unwound_calls==1);
    reset();mode=Mode::nested_unhandled_seh;run(true,2,2,2);CHECK(original_calls==2&&!cover_returned&&!ui_call_boundary::completed);
    for(unsigned fault=0;fault<2;++fault){
        reset();ui_observer_enabled=false;expect_selected=expect_logical=false;if(fault)mode=Mode::seh;
        run(fault!=0,1,1,0);CHECK(original_calls==1&&!ui_call_boundary::calls);
        reset();world_allowed=false;expect_selected=expect_cover=expect_logical=false;if(fault)mode=Mode::seh;
        run(fault!=0,1,0,0);CHECK(original_calls==1&&!ui_call_boundary::calls);
        reset();recording=false;expect_selected=expect_cover=expect_logical=false;if(fault)mode=Mode::seh;
        run(fault!=0,1,0,0);CHECK(original_calls==1&&!ui_call_boundary::calls);
        reset();arguments[0].callee=reinterpret_cast<void*>(0x3010);expect_selected=false;if(fault)mode=Mode::seh;
        run(fault!=0);CHECK(original_calls==1&&ui_call_boundary::calls==1&&!ui_call_boundary::candidates);
        reset();metadata_current=false;expect_selected=false;if(fault)mode=Mode::seh;
        run(fault!=0);CHECK(original_calls==1&&ui_call_boundary::rejected==1&&!ui_call_boundary::accepted);
        reset();ui_invocation.context.kind=0;expect_selected=false;if(fault)mode=Mode::seh;
        run(fault!=0);CHECK(original_calls==1&&ui_call_boundary::rejected==1);
        reset();ui_invocation.frozen=nullptr;expect_selected=expect_logical=false;if(fault)mode=Mode::seh;
        run(fault!=0);CHECK(original_calls==1&&!ui_call_boundary::candidates);
        reset();expect_selected=false;ui_call_boundary::nesting.depth=gears_ui_call_boundary::frame_limit;
        gears_ui_call_boundary::Child parent{};ui_call_boundary::Row parent_row{};
        ui_call_boundary::nesting.current=&parent;ui_call_boundary::current_row=&parent_row;if(fault)mode=Mode::seh;
        uintptr_t value{};CHECK(call_caught(arguments[0],value)==bool(fault));
        CHECK(ui_call_boundary::nesting.depth==gears_ui_call_boundary::frame_limit&&ui_call_boundary::nesting.current==&parent);
        CHECK(ui_call_boundary::current_row==&parent_row&&ui_call_boundary::depth_rejected==1&&original_calls==1);
        ui_call_boundary::reset_tls();drained(1,1,1);
    }
    reset();constructor_fault=2;run(true,1,0,0);CHECK(!original_calls);
    reset();constructor_fault=3;run(true,1,1,0);CHECK(!original_calls);
    reset();mode=Mode::stop_original;run(false);CHECK(transport_stopped&&original_calls==1);
    reset();transport_admitted=false;expect_observer=true;
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    const auto before_noauth=weapon_call_live::accepted[0].load();
#endif
    run(false);CHECK(original_calls==1&&!weapon_hand_runtime::begun&&!weapon_hand_runtime::action_begun);
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
    CHECK(weapon_call_live::accepted[0].load()==before_noauth+1); // RO accepted, transport still denied.
#endif
    // Exercise actual fire/aim ownership, including destructor cleanup when the
    // original raises SEH. This catches incorrect reverse slot destruction.
    for(unsigned fault=0;fault<2;++fault){
        reset();expect_selected=false;arguments[0].callee=reinterpret_cast<void*>(0x3010);
        const uintptr_t fire_node=0x3010,fire_pc=0x8101;
        std::memcpy(frames[1].data()+0x18,&fire_node,8);std::memcpy(frames[1].data()+0x28,&fire_pc,8);
        arguments[1].callee=reinterpret_cast<void*>(0x3030);
        mode=fault?Mode::nested_unhandled_seh:Mode::nested;
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
        const auto fire_before=weapon_call_live::accepted[1].load(),aim_before=weapon_call_live::accepted[3].load();
#endif
        run(fault!=0,2,2,2);CHECK(original_calls==2);
#if GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
        CHECK(weapon_call_live::accepted[1].load()==fire_before+1);
        CHECK(weapon_call_live::accepted[3].load()==aim_before+1);
#endif
    }
    std::printf("UiCallMergeTest feature=%u passed: %u checks; actual hook/wrapper/observer; explicit metadata and transport host seams\n",
        unsigned(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER),checks);
    return 0;
}
