// All engine objects, script arrays, frames and Result storage below are owned
// artificial memory. Run the actual client/arbiter/entry/writer/hook code; no
// game, Native DLL registration, engine call, device or remote process exists.
#define GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL 1
#define GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE 1
#define GEARS_PLAYGROUND_WEAPON_GRIP_CALL_CONTEXT_PROBE 1
#define GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_WRITE_TRIAL 1
#include <initializer_list>
#include <cstdio>
namespace {FILE* log_file{};}
#include "../vr-common/weapon_shot_scope.h"
#include "../vr-common/weapon_hand_owner_adapter.h"
#include "weapon_hand_offline_fixture.h"
namespace {
std::atomic<unsigned> callbacks{};
namespace playground {bool automatic=true,ready=true;uptr arena_pc{},arena_world{};}
#include "weapon_grip_call_context.inl"
#include "weapon_shot_scope.inl"
namespace gripdamage_write {void begin(unsigned) noexcept;void dump();bool reset_drained() noexcept;}
#include "weapon_grip_damage_result_probe.inl"
#include "weapon_grip_damage_write_trial.inl"
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
#include "weapon_call_subscriber.inl"
using TestOriginal=uptr(__fastcall*)(uptr,uptr,uptr,uptr);
TestOriginal test_original{};
uptr call_boundary(uptr self,uptr caller,uptr output,uptr callee){
    // This is an explicit local Host model, not the actual Native wrapper.
    static std::uint64_t next_call=1;
    gears_weapon_call::Invocation call{};call.hand_cookie=weaponhand::client.cookie();
    call.native_started_qpc=weaponhand::client.native_started_qpc();call.call_id=next_call++;
    call.thread_id=GetCurrentThreadId();call.self=self;call.caller_frame=caller;call.result=output;call.callee=callee;
    const auto before=callbacks.load();gears_weapon_call::State state{};
    weaponcall::begin(&weaponhand::client,&call,&state);
    assert(callbacks.load()==before+1&&weaponcall::transactions.load()==before+1);
    uptr value{};bool returned{};
    __try{value=test_original(self,caller,output,callee);returned=true;}
    __finally{
        const auto end=returned?weaponcall::returned(&weaponhand::client,&call,&state,value):
            weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::original_exception);
        assert(end==gears_weapon_call::End::closed&&callbacks.load()==before&&weaponcall::transactions.load()==before);
        // A repeat cleanup notification cannot re-scale or decrement twice.
        const auto again=returned?weaponcall::returned(&weaponhand::client,&call,&state,value):
            weaponcall::unwind(&weaponhand::client,&call,&state,gears_weapon_call::UnwindReason::original_exception);
        assert(again==gears_weapon_call::End::closed);
        assert(callbacks.load()==before&&weaponcall::transactions.load()==before);
    }
    return value;
}
bool exception_forwarded(uptr self,uptr caller,uptr output,uptr callee){
    __try{call_boundary(self,caller,output,callee);}
    __except(GetExceptionCode()==0xe1234567ul?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
    return false;
}
#else
constexpr int MH_OK=0;
int MH_CreateHook(void*,void*,void**){return 1;}
int MH_RemoveHook(void*){return 1;}
namespace handanim {
enum class Source {call_function};
unsigned enter(uptr,uptr,void*,Source,uptr){return 1;}
void leave(unsigned,uptr,uptr,void*,uptr){}
}
#include "hand_animation_call_probe.inl"
uptr call_boundary(uptr self,uptr caller,uptr output,uptr callee){return handanimcall::hook(self,caller,output,callee);}
#endif
struct alignas(8) Frame {std::array<unsigned char,64> bytes{};};
enum class Scenario {normal,soft_loss,terminal,unknown_nested,soft_unknown_nested,original_exception};
struct Run:Memory {
    gears_vr::LeftHandInteractionOwner owner;abi::Registration registration{};abi::Frame owner_frame{};
    std::array<std::array<unsigned char,128>,5> scripts{};std::array<unsigned char,1421> damage_script{};
    std::array<Frame,4> frames{};alignas(4) std::array<unsigned char,168> locals{};
    uptr function_type{},float_type{},war_fn{},engine_fn{},getter{},pih{},fire_ammo{},unknown{};
    std::uint64_t cookie{},frozen_frame{},frozen_shot{};unsigned originals{},getters{},emitted{};
    Scenario scenario;std::array<float,4> values{};
    static inline std::uint64_t next_cookie=33;
    static inline Run* current{};
    uptr frame_address(unsigned n){return reinterpret_cast<uptr>(frames[n].bytes.data());}
    uptr result(){return reinterpret_cast<uptr>(locals.data())+164;}
    uptr function(wchar_t const* label,uptr declaring,unsigned script_slot){
        const auto p=node(label,function_type,declaring);
        put(p,0x8c,meta::Table{reinterpret_cast<uptr>(scripts[script_slot].data()),128,128});
        put(p,0xf4,0x24102u);put(p,0xf8,std::uint16_t(0));put(p,0x114,base+0x1d6ca0);return p;
    }
    void body(unsigned n,uptr function_address,uptr parent){
        auto const script=get<meta::Table>(function_address,0x8c);const auto address=frame_address(n);
        put(address,0x18,function_address);put(address,0x20,weapon);put(address,0x28,script.data+16);put(address,0x38,parent);
    }
    void scene(){
        owner_frame.now_ns=weaponhand::clock_ns(nullptr);auto& s=owner_frame.stereo;auto& c=owner_frame.controllers;
        s.received_ns=owner_frame.now_ns-1000000;c.received_ns=s.received_ns;c.sync_ns=c.received_ns-1;
        abi::Proposal proposal{};const auto reply=registration.prepare(registration.context,&owner_frame,&proposal);
        if(reply!=unsigned(abi::Reply::accepted)){
            abi::GripGameState game{};const auto observed=weaponhand::observe(nullptr,owner_frame,weaponhand::clock_ns(nullptr),game);
            std::fprintf(stderr,"scene prepare=%u shape=%u active=%u observed=%u known=%u main=%u phase=%u frame=%llu\n",
                reply,unsigned(abi::frame_shape_valid(owner_frame)),unsigned(abi::frame_active(owner_frame,weaponhand::clock_ns(nullptr))),
                unsigned(observed),unsigned(game.known),unsigned(game.main_hand_held),unsigned(weaponhand::client.phase()),owner_frame.key.frame);
        }
        assert(reply==unsigned(abi::Reply::accepted));
        gears_vr::LeftHandInteractionFrame requests{};requests.lifecycle=1;requests.frame=owner_frame.key.frame;
        requests.context_valid=requests.tracked=true;gears_weapon_hand::append_weapon_proposal(proposal,requests);
        auto decision=gears_weapon_hand::owner_decision(owner_frame,proposal,owner.update(requests),weaponhand::clock_ns(nullptr));
        assert(registration.commit(registration.context,&owner_frame,&proposal,&decision)==unsigned(abi::Reply::accepted));
        ++owner_frame.key.frame;++s.sequence;c.sequence=s.sequence;
    }
    void invalidate(abi::InvalidateReason reason){abi::Invalidation message{};
        message.registration_cookie=cookie;message.native_started_qpc=22;message.reason=reason;
        registration.invalidate(registration.context,&message);
    }
    explicit Run(Scenario value):scenario(value){
        function_type=node(L"Function",class_type,core);float_type=node(L"FloatProperty",class_type,core);
        war_fn=function(L"InstantFire",weapon_class,0);
        engine_fn=function(L"InstantFire",cls(L"Weapon",engine),1);
        getter=function(L"GetFireModeDamage",weapon_class,2);
        pih=function(L"ProcessInstantHit_Internal",weapon_class,3);fire_ammo=function(L"FireAmmunition",weapon_class,4);
        unknown=function(L"InstantFire",cls(L"UnverifiedWeapon",warfare),4);
        put(getter,0x88,36u);put(getter,0x106,std::uint16_t(4));put(getter,0x108,std::uint16_t(0));
        const auto returned=property(getter,L"ReturnValue",float_type,0,4);put(returned,0x78,std::uint64_t(0x480));
        put(pih,0x88,168u);put(pih,0x8c,meta::Table{reinterpret_cast<uptr>(damage_script.data()),1421,1421});
        put(pih,0xf4,0xc44103u);property(pih,L"DamageDealt",float_type,164,4);tables();
        playground::arena_world=world;playground::arena_pc=pc;playground::arena_pawn=pawn;
        game_thread.store(0);assert(weaponhand::client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
        cookie=next_cookie++;abi::RegistrationResult receipt{};receipt.cookie=cookie;receipt.native_started_qpc=22;
        assert(weaponhand::client.connected(abi::Reply::accepted,receipt)==abi::Reply::accepted);
        registration=weaponhand::client.registration();game_thread.store(GetCurrentThreadId());owner_frame=frame;
        owner_frame.flags=abi::scene_owned|abi::input_matched|abi::gameplay_interactable;
        owner_frame.key={11,22,cookie,1,1,1,GetCurrentProcessId(),GetCurrentThreadId()};
        owner_frame.reference_generation=1;owner_frame.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
        owner_frame.left_grip_known=owner_frame.left_tracked=1;
        auto& s=owner_frame.stereo;s.session=1;s.sequence=100;s.predicted_time=123;
        s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;s.head={{0,1.6,0},{0,0,0,1}};
        s.eyes[0].pose.position={-.032,1.6,0};s.eyes[1].pose.position={.032,1.6,0};
        for(auto& eye:s.eyes){eye.pose.orientation={0,0,0,1};eye.fov={-.8,.8,.8,-.8};}
        auto& c=owner_frame.controllers;c.session=1;c.sequence=100;c.predicted_time=123;c.space_epoch=s.space_epoch;c.focused=1;
        for(auto& hand:c.hands){hand.grip_active=1;hand.grip_flags=15;hand.grip.position={0,1,0};}
        scene();scene();frozen_frame=owner_frame.key.frame-1;
        assert(gripdamage::begin(world,pc,pawn,GetTickCount64()+5000));
        body(0,fire_ammo,0);body(1,war_fn,frame_address(0));body(2,engine_fn,frame_address(1));body(3,pih,frame_address(2));
        put(frame_address(3),0x30,reinterpret_cast<uptr>(locals.data()));current=this;
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        test_original=original;
        // The local Host's accepted attach is modeled here. It does not prove
        // real DLL exports, loader module pins or the Native registration code.
        weaponcall::attached_native.store(22);weaponcall::attached_cookie.store(cookie);
#else
        handanimcall::original=original;
#endif
    }
    ~Run(){
        assert(!callbacks.load()&&weaponshot::stack.drained());gripdamage::callbacks_drained();assert(gripdamage::reset_drained());
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        assert(weaponcall::local_quiescent()&&weaponcall::detached());
#endif
        game_thread.store(0);assert(weaponhand::client.begin_disconnect()==abi::Reply::draining);
        assert(weaponhand::client.finish_disconnect(cookie)==abi::Reply::quiescent);
        playground::arena_pc=playground::arena_world=playground::arena_pawn=0;current=nullptr;
    }
    uptr emit(){
        put(frame_address(3),0x28,reinterpret_cast<uptr>(damage_script.data())+322);
        return call_boundary(weapon,frame_address(3),result(),getter);
    }
    static uptr __fastcall original(uptr self,uptr caller,uptr output,uptr callee){
        auto& r=*current;++r.originals;assert(self==r.weapon&&callbacks.load());
        if(callee==r.war_fn){
            assert(caller==r.frame_address(0));
            assert(call_boundary(self,r.frame_address(1),output,r.engine_fn)==0x8877665544332211ull);
            return 0x1122334455667788ull;
        }
        if(callee==r.engine_fn){
            assert(caller==r.frame_address(1));
            for(unsigned i=0;i<4;++i){
                if(i==1&&(r.scenario==Scenario::unknown_nested||r.scenario==Scenario::soft_unknown_nested)){
                    assert(call_boundary(self,r.frame_address(2),output,r.unknown)==0xaabbccddull);
                }else assert(r.emit()==0xfedcba9876543210ull);
                r.values[i]=get<float>(r.result(),0);++r.emitted;
                if(!i){
                    if(r.scenario==Scenario::terminal){
                        abi::Proposal rejected{};
                        assert(r.registration.prepare(r.registration.context,&r.owner_frame,&rejected)==unsigned(abi::Reply::unavailable));
                    }else r.scene();
                    if(r.scenario==Scenario::soft_loss||r.scenario==Scenario::soft_unknown_nested)r.invalidate(abi::InvalidateReason::input_lost);
                }
            }
            return 0x8877665544332211ull;
        }
        if(callee==r.unknown){assert(r.emit()==0xfedcba9876543210ull);return 0xaabbccddull;}
        assert(callee==r.getter&&caller==r.frame_address(3)&&output==r.result());++r.getters;
        put(output,0,100.0f);put(caller,0x28,reinterpret_cast<uptr>(r.damage_script.data())+379);
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        if(r.scenario==Scenario::original_exception)RaiseException(0xe1234567ul,0,0,nullptr);
#endif
        if(r.scenario==Scenario::terminal&&r.getters==1)r.invalidate(abi::InvalidateReason::native_stopping);
        return 0xfedcba9876543210ull;
    }
    void run(){
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        if(scenario==Scenario::original_exception){
            assert(exception_forwarded(weapon,frame_address(0),result(),war_fn));
            assert(getters==1&&!callbacks.load()&&weaponshot::stack.drained()&&weaponcall::local_quiescent());
            assert(gripdamage::state.depth==0&&gripdamage::state.rows[0].closed&&gripdamage::state.rows[0].incomplete);
            assert(!gripdamage_write::state.rows[0].attempted&&gripdamage_write::state.rows[0].outcome==gripdamage_write::Outcome::unwind);
            std::puts("subscriber original SEH forwarded; all nested scopes and raw/write tokens closed");return;
        }
#endif
        assert(call_boundary(weapon,frame_address(0),result(),war_fn)==0x1122334455667788ull);
        unsigned expected_rows=4;
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
        // Terminal stop closes new subscriber admission; original getters still
        // run, but only the already-entered first raw observation is recorded.
        if(scenario==Scenario::terminal)expected_rows=1;
#endif
        if(getters!=4||emitted!=4||gripdamage::state.count!=expected_rows||callbacks.load()||!weaponshot::stack.drained())
            std::fprintf(stderr,"scenario=%u getters=%u emitted=%u rows=%u expected=%u callbacks=%u depth=%u\n",
                unsigned(scenario),getters,emitted,gripdamage::state.count,expected_rows,callbacks.load(),weaponshot::stack.depth());
        assert(getters==4&&emitted==4&&gripdamage::state.count==expected_rows&&!callbacks.load()&&weaponshot::stack.drained());
        for(unsigned i=0;i<4;++i){
            const bool scaled=scenario!=Scenario::terminal&&!((scenario==Scenario::unknown_nested||scenario==Scenario::soft_unknown_nested)&&i==1);
            assert(values[i]==(scaled?70.0f:100.0f));auto const& entry=gripdamage_write::state.rows[i];
            if(scaled){
                assert(entry.attempted&&entry.exchanged&&entry.outcome==gripdamage_write::Outcome::applied);
                assert(entry.packet.grip.key.frame==frozen_frame&&entry.packet.grip.grip==abi::Grip::single);
                if(!frozen_shot)frozen_shot=entry.packet.shot;assert(frozen_shot==entry.packet.shot);
            }else assert(!entry.attempted);
        }
        if(scenario==Scenario::soft_loss||scenario==Scenario::soft_unknown_nested)assert(!weaponhand::client.armed()&&weaponhand::client.registration_alive(cookie,22));
        if(scenario==Scenario::terminal)assert(weaponhand::client.phase()==abi::ClientPhase::active&&!weaponhand::client.registration_alive(cookie,22));
        assert(emit()==0xfedcba9876543210ull&&get<float>(result(),0)==100.0f); // No old-shot borrowing after return.
        assert(!gripdamage_write::state.rows[4].attempted);
        std::printf("integrated scenario=%u passed: getters=5, same Result storage, fixture two ignored\n",unsigned(scenario));
    }
};
}
int main(){
    for(auto key:{L"GEARSVR_TEST_TINY_OUTPUT_OBSERVE_ONLY",L"GEARSVR_TEST_WINDOWS_BEHIND",L"GEARSVR_TEST_MUTE_GAME",L"GEARSVR_TEST_GRIP_DAMAGE_WRITE"})
        assert(SetEnvironmentVariableW(key,L"1"));
    assert(SetEnvironmentVariableW(L"GEARSVR_TEST_GRIP_DAMAGE_FIXTURE",L"two")); // Must be ignored by the live scope.
    assert(tmpfile_s(&log_file)==0&&log_file);
    for(auto scenario:{Scenario::normal,Scenario::soft_loss,Scenario::terminal,Scenario::unknown_nested}){
        auto run=std::make_unique<Run>(scenario);run->run();
    }
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    for(auto scenario:{Scenario::soft_unknown_nested,Scenario::original_exception}){
        auto run=std::make_unique<Run>(scenario);run->run();
    }
#endif
    std::fclose(log_file);log_file=nullptr;
#if defined(GEARS_PLAYGROUND_WEAPON_CALL_SUBSCRIBER_TRIAL)
    std::puts("WeaponCallSubscriberOfflineTest passed: 6 scenarios; actual PG subscriber/client/owner/entry/writer with a local Native Host model and artificial memory only");
#else
    std::puts("WeaponShotScopeOfflineTest passed: 4 integrated scenarios; actual client/owner/entry/hook/writer on artificial memory only");
#endif
}
