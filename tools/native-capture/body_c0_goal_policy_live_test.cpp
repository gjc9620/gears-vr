// Actual Goal live path, adapter/planner/marker, Backend CAS128/Kernel and
// EpochGate. Actor/property/Core lifecycle/receipt admission are Host seams.
#include "host_native_live_prefix.inl"
#include "body_c0_neutral.h"
bool simulator_operator=true;
bool safe_copy(void* out,void const* in,size_t size){return n::read_memory(out,reinterpret_cast<uintptr_t>(in),size);}
unsigned host_cover{},host_crouched{},host_wants{};bool host_property=true;
int64_t host_now{};gears_native_rig_tracking::CoreSnapshot host_core{};
weapon_hand_scene_binding::Binding host_actor{};gears_anchor::Identity host_asset{};
namespace weapon_hand_scene_binding {
std::mutex mutex;uint64_t metadata_generation=444;std::array<unsigned char,sizeof(gears_anchor::Config)>metadata_bytes{};
struct Property {gears_anchor::Identity id{};unsigned offset{};};
struct Proof {Property fields[5]{};bool ready{};};
bool metadata_current(){return metadata_valid;}bool current_actor(Binding const& b){return actor_valid&&recording&&same_key(b,host_actor)&&b.special_move==host_actor.special_move;}
bool identity(uintptr_t address,gears_anchor::Identity& out,bool){out=host_asset;if(address!=host_asset.address){out={};out.address=address;}return metadata_valid;}
bool current_actor_storage(Binding const& b){return actor_valid&&gears_anchor::equal_identity(b.world,host_actor.world)&&
    gears_anchor::equal_identity(b.controller,host_actor.controller)&&gears_anchor::equal_identity(b.pawn,host_actor.pawn)&&
    gears_anchor::equal_identity(b.mesh,host_actor.mesh)&&b.world_generation==host_actor.world_generation;}
void keys(Proof&,gears_anchor::Identity const&){}bool begin_prepare(Proof&){return host_property;}bool current(Proof const&){return host_property;}
bool property(Proof&,uintptr_t,wchar_t const* name,wchar_t const*,unsigned,wchar_t const*,Property& p){
    p.offset=!wcscmp(name,L"CoverType")?1u:!wcscmp(name,L"bIsCrouched")?2u:3u;p.id.address=p.offset;return host_property;}
bool source_field(Proof&,unsigned& out,uintptr_t,unsigned){out=1;return host_property;}
template<class T>bool field(T& out,uintptr_t,unsigned off){out=T(off==1?host_cover:off==2?host_crouched:host_wants);return host_property;}
bool sample(gears_anchor::Receipt const*,Binding& out){out=host_actor;return actor_valid;}
}
namespace cover_pose_phase_live {p::Identity compact(gears_anchor::Identity const& i){p::Identity out{};out.address=i.address;return out;}}
namespace weapon_hand_runtime {struct Session {uint32_t game_pid{};uint64_t game_start{},native_started_qpc{};};
Session session(){return {host_core.native.game_pid,host_core.native.game_start,host_core.native.native_started_qpc};}void* current(){return reinterpret_cast<void*>(1);}}
namespace rig_tracking_live {
int64_t clock_ns(){return host_now;}
bool source_fresh(int64_t stamp,int64_t now,uint64_t age){return stamp>0&&now>=stamp&&uint64_t(now-stamp)<=age;}
bool native_session_current(void*,weapon_hand_runtime::Session const&){return actor_valid;}
bool reference_snapshot(gears_rig_tracking::NativeReference& out,uint64_t& revision){out=host_core.native_reference;revision=host_core.native_reference_revision;return metadata_valid;}
}
// Test-only receipt surface. The actual Registry/ReceiptSource is retained;
// certification/entry admission is a Host seam. No synthetic coverage is made
// and no receipt ever retires body debt in this Goal integration test.
namespace native_atom_overwrite_live {
bool available{};unsigned ready_calls{},install_calls{},retire_calls{};gears_atom_overwrite::Registry registry;
bool ready_for_commit()noexcept{++ready_calls;return available&&registry.can_install(GetCurrentThreadId());}
bool world_current()noexcept{return actor_valid;}
bool install(gears_atom_overwrite::Context& context,gears_atom_overwrite::tx::Stamp const& stamp,uint32_t epoch)noexcept {
    ++install_calls;return registry.install(context,stamp,epoch,base+0x129d09);}
gears_atom_overwrite::ReceiptSource* receipts()noexcept{return &registry;}
uint64_t journal_version()noexcept{return registry.version();}
void retired(uint64_t version,uint64_t event)noexcept{++retire_calls;registry.retired(version,event);}
}
#define GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT 1
#define GEARS_BODY_C0_GOAL_POLICY 1
#include "body_c0_goal_policy_live.inl"
namespace goal=body_c0_goal_policy_live;namespace neutral=gears_body_c0_neutral;namespace adapter=gears_body_goal_view;
void finish_return(Entry const&e){bool called=false,returned=false;__try{live::before_original(e);called=true;returned=true;}
    __finally{goal::pose_end(e,returned,called);}}
void finish_throw_inner(Entry const&e){bool called=false;__try{live::before_original(e);called=true;RaiseException(0xe0424242,0,0,nullptr);}
    __finally{goal::pose_end(e,false,called);}}
bool finish_throw(Entry const&e){__try{finish_throw_inner(e);return false;}__except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
int main(int argc,char**argv){if(argc!=4)return 2;unsigned mode=unsigned(std::strtoul(argv[3],nullptr,10));
    auto frame=std::make_unique<body::Frame>();C(read_file(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read_file(std::string(argv[1])+"/C0-local.bin",frame->local.data(),sizeof frame->local));C(read_file(std::string(argv[1])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(read_file(argv[2],&host_core,sizeof host_core));host_now=int64_t(host_core.published_ns);host_core.world_generation=444;
    host_core.native.game_thread=GetCurrentThreadId();host_core.native.native_started_qpc=333;host_core.flags&=~gears_rig_tracking::cover_key_known;
    host_core.cover_type=0;host_core.cover_link={};host_core.cover_slot=-1;host_core.stereo.head=host_core.reference;
    host_core.stereo.head.position.y-=.03; // Real-shape Host HMD physical squat, no game crouch flag.
    C(gears_native_rig_tracking::core_snapshot_valid(host_core,host_now,50000000,true));
    Fixture fixture(*frame);Scope lease;Entry tick{},entry{};tick.thread=GetCurrentThreadId();tick.invocation=10;tick.kind=2;tick.depth=1;tick.tick_depth=tick.selected=true;
    entry.thread=tick.thread;entry.invocation=11;entry.kind=0;entry.depth=1;entry.compose_depth=entry.selected=true;
    entry.ra=base+p::compose_caller;entry.rcx=fixture.mesh.address();entry.rdx=entry.rcx+0x27c;entry.r8=entry.rcx+0x28c;
    auto&s=hand::slot;s.tick_address=&tick;s.compose_address=&entry;s.tick=tick.invocation;s.tick_depth=s.compose_depth=1;s.lease=&lease;s.active=true;
    s.key.request.native.game_thread=tick.thread;s.key.outer_tick=tick.invocation;s.compose.admitted=true;s.compose.outer_tick=tick.invocation;s.compose.scope.invocation=entry.invocation;s.compose.scope.arguments=true;
    s.data.owner.key=fixture.owner;s.data.owner.thread=tick.thread;s.data.owner.valid=true;s.data.owner.binding.context_valid=s.data.owner.binding.weapon_known=true;
    s.data.owner.binding.world_generation=444;s.data.owner.binding.mesh.address=fixture.mesh.address();s.data.owner.binding.pawn.address=222;s.data.owner.binding.pawn.cls=777;
    s.data.asset.address=fixture.asset.address();s.data.signature=body::rig::marcus115_signature;s.data.record.source=host_core;
    s.data.record.tick=tick.invocation;s.data.record.compose=entry.invocation;host_actor=s.data.owner.binding;host_asset=s.data.asset;
    live::enabled=goal::enabled=true;native_atom_overwrite_live::available=mode!=0&&mode!=10&&mode!=11;
    if(mode==5)host_cover=2;if(mode==6)host_crouched=1;if(mode==7)host_wants=1;if(mode==8)host_now+=50000001;
    const auto entry_before=entry;live::pose_begin(entry);C(live::state.captured);goal::pose_begin(entry);C(!std::memcmp(&entry,&entry_before,sizeof entry));
    if(mode==0||mode>=5&&mode<=8||mode==10||mode==11){
        C(goal::applied==0&&!goal::view_valid&&!live::state.prepared);C(!std::memcmp(fixture.local.data,frame->local.data(),sizeof frame->local));
        if(mode==0){adapter::Neutral moved{};adapter::Output prepared{};adapter::Policy policy{host_now,true,body::Scope::stationary};
            C(neutral::move(goal::origin.stored,live::state.source.component_to_world,moved));
            C(adapter::prepare(moved,host_core,live::state.model,live::state.source,goal::origin.marker,{},policy,prepared)==adapter::Status::okay);
            const auto before=native_atom_overwrite_live::ready_calls;
            C(!live::apply(entry,prepared.goal,s.data.record.source)); // actual apply's independent receipt gate
            C(native_atom_overwrite_live::ready_calls>before&&!live::state.prepared&&!live::state.kernel.pending());
            C(!std::memcmp(fixture.local.data,frame->local.data(),sizeof frame->local));}
        finish_return(entry);C(!goal::pending_count()&&!live::pending_count()&&!goal::view_valid);
        if(mode==0){C(goal::origin.initialized&&goal::plans==1);adapter::Neutral base_neutral{},rotated{};auto const& stored=goal::origin.stored;
            C(neutral::move(stored,frame->component_to_world,base_neutral));auto original_ctw=neutral::matrix(frame->component_to_world);
            for(double yaw:{-3.141592653589793,-1.5707963267948966,1.5707963267948966,3.141592653589793}){
                body::Atom atom{};body::d::set_quat(atom,body::d::yaw(yaw));atom.scale=1;atom.translation[0]=7;atom.translation[1]=-4;
                auto transform=body::d::atom_matrix(atom),matrix=body::d::multiply(original_ctw,transform);body::Matrix64 ctw{};
                for(unsigned i=0;i<16;++i)ctw.values[i]=float(matrix.m[i]);C(neutral::move(stored,ctw,rotated));
                const auto expected=neutral::world_point(transform,base_neutral.eye_anchor.neutral_eye_world);
                C(body::d::length(body::d::sub(expected,rotated.eye_anchor.neutral_eye_world))<.002);
                C(body::d::length(body::d::sub(neutral::world_point(transform,base_neutral.pelvis_world),rotated.pelvis_world))<.002);
                for(unsigned i=0;i<2;++i)C(body::d::length(body::d::sub(neutral::world_point(transform,base_neutral.feet_world[i].position),rotated.feet_world[i].position))<.002);
            }
            const auto before=rotated;auto invalid=frame->component_to_world;invalid.values[0]*=2;
            C(!neutral::move(stored,invalid,rotated)&&!std::memcmp(&before,&rotated,sizeof before));}
        if(mode==10||mode==11){C(goal::origin.initialized);const auto stored=goal::origin.stored;
            if(mode==10)fixture.atoms()[0].translation[0]+=100;
            else {auto moved=frame->component_to_world;moved.values[12]+=7;fixture.mesh.put(0xa0,moved);}
            ++tick.invocation;++entry.invocation;s.tick=tick.invocation;s.key.outer_tick=tick.invocation;s.compose.outer_tick=tick.invocation;s.data.record.tick=tick.invocation;
            s.compose.scope.invocation=entry.invocation;s.data.record.compose=entry.invocation;
            live::pose_begin(entry);C(live::state.captured);goal::pose_begin(entry);C(!std::memcmp(&stored,&goal::origin.stored,sizeof stored)&&goal::plans==2);
            adapter::Neutral old{},moved{};C(neutral::move(stored,frame->component_to_world,old));C(neutral::move(stored,live::state.source.component_to_world,moved));
            C(std::abs(moved.eye_anchor.neutral_eye_world.x-old.eye_anchor.neutral_eye_world.x-(mode==11?7.:0.))<1e-6);
            finish_return(entry);C(!live::pending_count()&&!goal::pending_count());}
    }else{
        if(!goal::pending.committed){adapter::Neutral moved{};adapter::Output planned{};adapter::Policy policy{host_now,true,body::Scope::stationary};
            C(neutral::move(goal::origin.stored,frame->component_to_world,moved));auto status=adapter::prepare(moved,host_core,live::state.model,live::state.source,goal::origin.marker,{},policy,planned);
            auto report=body::plan(live::state.model,live::state.source,planned.goal,live::state.workspace,live::state.output);
            printf("diagnose adapter=%u planner=%u bone=%u distance=%g reach=%g\n",unsigned(status),unsigned(report.status),report.failed_bone,report.requested_distance,report.available_reach);}
        C(goal::pending.committed&&goal::applied==1&&live::state.prepared&&live::state.kernel.outstanding_fields()>0);
        C(live::state.atom_installed&&native_atom_overwrite_live::install_calls==1&&native_atom_overwrite_live::journal_version()==1);
        C(native_atom_overwrite_live::ready_calls>live::state.kernel.outstanding_fields()&&!native_atom_overwrite_live::ready_for_commit());
        const auto debt=live::state.kernel.outstanding_fields();
        if(mode==2)C(finish_throw(entry));else if(mode==3){live::before_original(entry);goal::request_drain();goal::pose_end(entry,true,true);}
        else if(mode==4){live::before_original(entry);Entry copy=entry;goal::pose_end(copy,true,true);C(goal::pending_count()>0);goal::pose_end(entry,true,true);}
        else finish_return(entry);
        C(!goal::pending_count()&&live::state.kernel.outstanding_fields()==debt);
        if(mode==2||mode==3||mode==4)C(!goal::view_valid&&goal::published==0);
        else{C(goal::view_valid&&goal::published==1);Entry other=entry;other.selected=false;++other.invocation;goal::pose_begin(other);C(goal::view_valid);
            gears_anchor::Receipt scene{};scene.invocation=999;scene.thread=GetCurrentThreadId();
            gears_vr::StereoResult stereo{};stereo.status=gears_vr::StereoStatus::accepted;stereo.sample=host_core.stereo;stereo.reference=host_core.reference;stereo.reference_generation=host_core.reference_generation;
            gears_vr::CoverViewResult output{};C(goal::read_view(&scene,stereo,host_core.units_per_metre,output));C(output.base_view==goal::view.render.base_view);
            if(mode==15){
                const auto stored=goal::origin.stored;const auto base_view=output;const auto published=goal::published.load();
                goal::pose_begin(entry); // Selected attempt with no captured C0: no body publication.
                C(goal::view_valid&&goal::published==published&&goal::applied==1);
                host_now+=100000000;stereo.sample.received_ns=host_now;
                C(goal::read_view(&scene,stereo,host_core.units_per_metre,output));
                C(output.base_view==base_view.base_view&&goal::view_held==1&&goal::published==published);
                auto moved=frame->component_to_world;moved.values[12]+=7;fixture.mesh.put(0xa0,moved);
                C(goal::read_view(&scene,stereo,host_core.units_per_metre,output));
                C(std::abs(output.base_eye_world.x-base_view.base_eye_world.x-7)<.002);
                C(std::abs(output.base_eye_world.y-base_view.base_eye_world.y)<.002&&std::abs(output.base_eye_world.z-base_view.base_eye_world.z)<.002);
                C(!std::memcmp(&stored,&goal::origin.stored,sizeof stored)&&goal::applied==1&&goal::published==published);
                // A stale CURRENT tracking sample is still refused, without destroying the neutral anchor.
                const auto before=output;stereo.sample.received_ns=host_now-50000001;
                C(!goal::read_view(&scene,stereo,host_core.units_per_metre,output)&&goal::view_valid&&!std::memcmp(&before,&output,sizeof output));
                stereo.sample.received_ns=host_now;C(goal::read_view(&scene,stereo,host_core.units_per_metre,output));
            }
            const auto before=output;const auto retained=live::pending_count();
            if(mode==9)fixture.reference.data[12]^=1;else if(mode==12)host_cover=2;else if(mode==13)host_crouched=1;
            else if(mode==14)host_core.native_reference_revision+=2;else host_wants=1;
            C(!goal::read_view(&scene,stereo,host_core.units_per_metre,output)&&!std::memcmp(&before,&output,sizeof output));
            C(!goal::view_valid&&live::pending_count()==retained);}
    }
    C(native_atom_overwrite_live::retire_calls==0);C(lease.world());printf("BodyGoalLive mode=%u: %u checks, %u failures; actual_atom_Native_apply_Kernel_Registry_CAS=1 Host_receipt_admission_actor_property_Core=1 no_coverage_fabricated=1\n",mode,checks,failures);return failures?1:0;
}
