// Actual cover Goal policy -> Native Backend/Kernel -> original-consumer
// readback -> view publication. Actor/source/receipt admission and original
// Compose math are explicit Host seams; no game/physics proof is claimed.
// Actual Goal live path, adapter/planner/marker, Backend CAS128/Kernel and
// EpochGate. Actor/property/Core lifecycle/receipt admission are Host seams.
#include "host_native_live_prefix.inl"
#include "host_native_goal_seams.inl"
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
#include "cover_physx_query.h"
#include "cover_physics_c0_live.h"
// Query dispatch remains a Host seam in this actual Goal-path test.
namespace cover_physics_c0_live {
void observe_sphere_once(gears_continuous_hand_entry::Entry const&,gears_native_rig_tracking::CoreSnapshot const&,
    gears_cover_physx_query::Vector const&)noexcept{}
}
#include "body_c0_goal_policy_live.inl"
#include "cover_observed_passive_atoms_test.inl"
namespace goal=body_c0_goal_policy_live;namespace neutral=gears_body_c0_neutral;namespace adapter=gears_body_goal_view;
void finish_return(Entry const&e){bool called=false,returned=false;__try{live::before_original(e);called=true;returned=true;}
    __finally{goal::pose_end(e,returned,called);}}
void finish_throw_inner(Entry const&e){bool called=false;__try{live::before_original(e);called=true;RaiseException(0xe0424242,0,0,nullptr);}
    __finally{goal::pose_end(e,false,called);}}
bool finish_throw(Entry const&e){__try{finish_throw_inner(e);return false;}__except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
int main(int argc,char** argv){
    if(argc!=4)return 2;const unsigned scenario=unsigned(std::strtoul(argv[3],nullptr,10));if(scenario>11)return 2;
    const unsigned mode=scenario==8?1:16;const bool cover_case=scenario!=8;
    auto frame=std::make_unique<body::Frame>();C(read_file(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(read_file(std::string(argv[1])+"/C0-local.bin",frame->local.data(),sizeof frame->local));C(read_file(std::string(argv[1])+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(read_file(argv[2],&host_core,sizeof host_core));if(failures)return 1;host_now=int64_t(host_core.published_ns);host_core.world_generation=444;
    // Explicit coherent Host clock; the input file remains unchanged and is not live evidence.
    C(host_now>0&&host_core.qpc_frequency>0&&host_core.qpc_frequency<=1000000000);
    if(failures)return 1;
    host_core.scene_qpc=(host_now/1000000000)*host_core.qpc_frequency+
        (host_now%1000000000)*host_core.qpc_frequency/1000000000;
    host_core.native.game_thread=GetCurrentThreadId();host_core.native.native_started_qpc=333;host_core.flags&=~gears_rig_tracking::cover_key_known;
    host_core.cover_type=0;host_core.cover_link={};host_core.cover_slot=-1;host_core.stereo.head=host_core.reference;
    host_core.stereo.head.position.y-=.03; // Real-shape Host HMD physical squat, no game crouch flag.
    C(gears_native_rig_tracking::core_snapshot_valid(host_core,host_now,50000000,true));
    if(cover_case){host_core.flags|=gears_rig_tracking::cover_key_known;host_core.cover_type=2;
        host_core.cover_link=host_core.world;host_core.cover_slot=0;host_cover=2;}
    auto standing_frame=std::make_unique<body::Frame>(*frame);
    if(scenario==11)recorded_passive_atoms(*frame);
    Fixture fixture(*frame);Scope lease;Entry tick{},entry{};tick.thread=GetCurrentThreadId();tick.invocation=10;tick.kind=2;tick.depth=1;tick.tick_depth=tick.selected=true;
    entry.thread=tick.thread;entry.invocation=11;entry.kind=0;entry.depth=1;entry.compose_depth=entry.selected=true;
    entry.ra=base+p::compose_caller;entry.rcx=fixture.mesh.address();entry.rdx=entry.rcx+0x27c;entry.r8=entry.rcx+0x28c;
    auto&s=hand::slot;s.tick_address=&tick;s.compose_address=&entry;s.tick=tick.invocation;s.tick_depth=s.compose_depth=1;s.lease=&lease;s.active=true;
    s.key.request.native.game_thread=tick.thread;s.key.outer_tick=tick.invocation;s.compose.admitted=true;s.compose.outer_tick=tick.invocation;s.compose.scope.invocation=entry.invocation;s.compose.scope.arguments=true;
    s.data.owner.key=fixture.owner;s.data.owner.thread=tick.thread;s.data.owner.valid=true;s.data.owner.binding.context_valid=s.data.owner.binding.weapon_known=true;
    s.data.owner.binding.world_generation=444;s.data.owner.binding.mesh.address=fixture.mesh.address();s.data.owner.binding.pawn.address=222;s.data.owner.binding.pawn.cls=777;
    s.data.asset.address=fixture.asset.address();s.data.signature=body::rig::marcus115_signature;s.data.record.source=host_core;
    s.data.record.tick=tick.invocation;s.data.record.compose=entry.invocation;host_actor=s.data.owner.binding;host_asset=s.data.asset;
    weapon_hand_scene_binding::metadata_generation=444;cover_shared_input_live::goal_absent_epoch=1;live::diagnostic_first_failure=true;live::enabled=goal::enabled=true;native_atom_overwrite_live::available=mode!=0&&mode!=10&&mode!=11;
    if(mode==5)host_cover=2;if(mode==6)host_crouched=1;if(mode==7)host_wants=1;if(mode==8)host_now+=50000001;
    if(mode==17)cover_context_live::host_change_closing=true;
    if(mode==18)cover_context_live::host_current=false;
    if(mode==19)cover_context_live::host_link_delta=8;
    if(mode==20)cover_context_live::host_slot_delta=1;
    if(mode==21)cover_context_live::host_key_hook=[](unsigned n){if(n==2)goal::stopping=true;};
    if(mode==22)host_now+=50000001;
    if(mode==23)cover_context_live::host_key_hook=[](unsigned n){if(n==2)++goal::revision;};
    if(cover_case){goal::current_cover_pose.binding.publication=987;goal::current_cover_pose.binding.slot=123;}
    const auto entry_before=entry;live::pose_begin(entry);C(live::state.captured);

    goal::cover_enabled=true;
    if(scenario==8){
        goal::pose_begin(entry);C(goal::pending.committed&&!goal::pending.next.cover&&goal::applied==1);
        live::before_original(entry);fixture.captured=live::state.source;fixture.original_space();
        goal::pose_end(entry,true,true);
        C(goal::view_valid&&goal::published==1&&goal::cover_applied==0&&goal::cover_completed==0);
        C(!goal::pending_count()&&lease.world());
        printf("CoverGoalLive scenario=8 standing regression checks=%u failures=%u; Game=0\n",checks,failures);
        return failures?1:0;
    }
    if(scenario==6){host_core.cover_type=1;host_cover=1;s.data.record.source=host_core;
        live::state.context.source=host_core;}
    auto& current=live::state;adapter::marker::Workspace marker_work{};adapter::marker::Marker marker{};
    C(adapter::marker::prepare(current.model,current.source,adapter::marker::Basis::marcus_mesh_face_forward_y,
        marker_work,marker)==adapter::marker::Status::okay);
    auto math_work=std::make_unique<body::Workspace>();neutral::Stored calibration{};adapter::Neutral calibrated{};
    C(neutral::capture(current.model,*standing_frame,marker,*math_work,calibration));
    C(neutral::move(calibration,current.source.component_to_world,calibrated));
    adapter::Policy policy{};policy.now_ns=host_now;policy.allow_sim=true;policy.scope=body::Scope::fixed_cover;
    adapter::CoverPoseMeasurement measured{};
    C(adapter::measure_cover_pose(host_core,current.model,current.source,marker,policy,*math_work,measured)==adapter::Status::okay);
    auto& drag=cover_full_goal_live::host_drag;drag={};auto& key=drag.key;
    key.epochs={1,2,3,4,5,6,7};key.world=host_core.world.address;key.pawn=host_core.pawn.address;
    key.mesh=host_core.mesh.address;key.link=host_core.cover_link.address;key.world_generation=host_core.world_generation;
    key.slot=host_core.cover_slot;key.thread=host_core.native.game_thread;key.scene=host_core.scene_invocation;
    key.xr_session=host_core.stereo.session;key.xr_space=host_core.stereo.space_epoch;
    key.reference=host_core.reference_generation;key.sequence=host_core.stereo.sequence;
    drag.pawn=host_core.pawn;drag.mesh=host_core.mesh;drag.link=host_core.cover_link;drag.reference=host_core.reference;
    drag.anchor={measured.eye.position,calibrated.eye_anchor.forward_world,calibrated.eye_anchor.right_world};drag.view_anchor=drag.anchor;
    drag.units_per_metre=host_core.units_per_metre;drag.type=host_core.cover_type;drag.tracking_source=host_core.stereo.source;
    drag.range={-.7,.7,0,scenario==6?0.:1.};drag.offset={.02,scenario==6?0.:.01};drag.observed_ns=host_now;
    cover_full_goal_live::host_available=true;
    if(scenario==1)native_atom_overwrite_live::available=false;
    if(scenario==2)++drag.pawn.name_index;
    if(scenario==3)cover_context_live::host_change_closing=true;
    if(scenario==9)cover_context_live::host_action=3;
    if(scenario==11){
        goal::cover_cache.calibrated=true;goal::cover_cache.calibration=calibration;
        goal::cover_cache.calibration_key=goal::cover_current_key();
    }
    if(scenario==10||scenario==11)live::hands_enabled=true; // Actual shared-hand configuration must not veto cover.
    goal::pose_begin(entry);
    C(!std::memcmp(&entry,&entry_before,sizeof entry));
    if((scenario>=1&&scenario<=3)||scenario==9){
        C(goal::applied==0&&!goal::pending.entry&&!current.prepared&&!goal::view_valid);
        C(!std::memcmp(fixture.local.data,frame->local.data(),sizeof frame->local));
        goal::pose_end(entry,true,true);C(!goal::pending_count());
    }else{
        C(goal::pending.committed&&goal::pending.next.cover&&goal::cover_applied==1);
        if(!goal::pending.committed){printf("cover reason=%u apply=%llu\n",goal::cover_reason,goal::applied.load());return 1;}
        C(!goal::origin.initialized&&goal::cover_cache.calibrated&&goal::cover_fits==1);
        C(current.cover_plan&&current.kernel.outstanding_fields()>0);
        if(scenario==11)for(unsigned i:{65u,67u,106u}){
            C(!std::memcmp(&current.output.local[i],&frame->local[i],sizeof(body::Atom)));
            C((current.output.changed_mask[i/64]&(uint64_t(1)<<(i%64)))==0);
        }
        if(scenario!=4){live::before_original(entry);fixture.captured=current.source;fixture.original_space();}
        if(scenario==5)reinterpret_cast<body::Matrix64*>(fixture.space.data)[body::head].values[12]+=1;
        goal::pose_end(entry,scenario!=7,scenario!=4);
        C(!goal::pending_count()&&!current.captured&&!current.prepared);
        if(scenario==4||scenario==5||scenario==7){C(!goal::view_valid&&goal::published==0&&goal::cover_completed==0);}
        else{
            C(goal::view_valid&&goal::published==1&&goal::cover_completed==1);
            if(scenario==11){namespace obs=live::cover_consumption_observation;
                C(obs::ready.load()&&obs::first.read_code==unsigned(n::ConsumedPoseCode::verified));
                C(obs::first.finish_code==unsigned(tx::Code::scope_closed)&&obs::first.returned&&obs::first.original_called&&obs::first.same_entry);
                C(obs::first.notified&&obs::first.consumed&&obs::first.atom_installed);
                const auto code=obs::first.finish_code;obs::capture(99,false,false,false);
                C(obs::first.finish_code==code&&obs::first.returned); // Immutable first record.
                obs::export_record();C(!obs::export_attempted.load()); // Never GT file IO.
                wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};
                C(GetTempPathW(MAX_PATH,temp)>0&&GetTempFileNameW(temp,L"cco",0,path)>0);
                C(DeleteFileW(path)&&CreateDirectoryW(path,nullptr));C(wcscpy_s(directory,path)==0);
                std::thread worker([]{obs::export_record();});worker.join();
                C(obs::export_written.load()&&!obs::export_error.load());
                const wchar_t* files[]={L"source",L"output",L"observed",L"core",L"stamp"};
                const void* values[]={&obs::first.source,&obs::first.planned,&obs::first.observed,&obs::first.core,&obs::first.stamp};
                const size_t sizes[]={sizeof obs::first.source,sizeof obs::first.planned,sizeof obs::first.observed,sizeof obs::first.core,sizeof obs::first.stamp};
                for(unsigned i=0;i<5;++i){wchar_t file[MAX_PATH]{};C(swprintf_s(file,L"%s\\cover-first-consumption-%s.bin",path,files[i])>0);
                    std::vector<unsigned char> read(sizes[i]);FILE* fp{};C(_wfopen_s(&fp,file,L"rb")==0&&fp);
                    if(fp){C(fread(read.data(),1,read.size(),fp)==read.size()&&fgetc(fp)==EOF);C(fclose(fp)==0);C(!std::memcmp(read.data(),values[i],sizes[i]));}
                    C(DeleteFileW(file));
                }
                directory[0]=0;C(RemoveDirectoryW(path));
            }
            gears_anchor::Receipt scene{};scene.invocation=999;scene.thread=GetCurrentThreadId();
            gears_vr::StereoResult stereo{};stereo.status=gears_vr::StereoStatus::accepted;
            stereo.sample=host_core.stereo;stereo.reference=host_core.reference;stereo.reference_generation=host_core.reference_generation;
            gears_vr::CoverViewResult view{};C(goal::read_view(&scene,stereo,host_core.units_per_metre,view));
            C(view.base_view==goal::view.render.base_view);
            auto untouched=view;drag.offset.right+=.01;
            C(goal::read_view(&scene,stereo,host_core.units_per_metre,view));
            C(view.base_view==untouched.base_view); // New intent cannot move the camera before C0.
            if(scenario==0){
                const auto frozen=goal::cover_cache.frozen;const auto saved_calibration=goal::cover_cache.calibration;
                // A raw animation overwrite without the real overwrite
                // receipt cannot erase the previous transaction's debt.
                const auto debt=current.kernel.pending();C(debt>0);
                std::memcpy(fixture.local.data,frame->local.data(),sizeof frame->local);
                fixture.captured=*frame;fixture.original_space();host_now+=2000000;
                ++tick.invocation;++entry.invocation;s.tick=tick.invocation;s.key.outer_tick=tick.invocation;
                s.compose.outer_tick=tick.invocation;s.data.record.tick=tick.invocation;
                s.compose.scope.invocation=entry.invocation;s.data.record.compose=entry.invocation;
                live::pose_begin(entry);C(!current.captured&&current.kernel.pending()==debt);
                goal::pose_begin(entry);C(!goal::pending.entry&&!goal::view_valid&&goal::cover_applied==1);
                C(!std::memcmp(&frozen,&goal::cover_cache.frozen,sizeof frozen)&&
                    !std::memcmp(&saved_calibration,&goal::cover_cache.calibration,sizeof saved_calibration));
                C(!goal::read_view(&scene,stereo,host_core.units_per_metre,view));
                C(view.base_view==untouched.base_view);
                goal::pose_end(entry,true,true);C(current.kernel.pending()==debt);
                untouched=view;
            }
            ++drag.key.epochs.cover;
            C(!goal::read_view(&scene,stereo,host_core.units_per_metre,view));
            C(!std::memcmp(&view,&untouched,sizeof view));
        }
    }
    C(native_atom_overwrite_live::retire_calls==0&&lease.world());
    printf("CoverGoalLive scenario=%u checks=%u failures=%u; actual Native apply/Kernel/consumed-pose; Host admissions and original math; Game=0\n",scenario,checks,failures);
    return failures?1:0;
}
