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
#include "body_c0_goal_policy_live.inl"
namespace goal=body_c0_goal_policy_live;namespace neutral=gears_body_c0_neutral;namespace adapter=gears_body_goal_view;
void finish_return(Entry const&e){bool called=false,returned=false;__try{live::before_original(e);called=true;returned=true;}
    __finally{goal::pose_end(e,returned,called);}}
void finish_throw_inner(Entry const&e){bool called=false;__try{live::before_original(e);called=true;RaiseException(0xe0424242,0,0,nullptr);}
    __finally{goal::pose_end(e,false,called);}}
bool finish_throw(Entry const&e){__try{finish_throw_inner(e);return false;}__except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
int main(int argc,char**argv){if(argc!=4)return 2;unsigned mode=unsigned(std::strtoul(argv[3],nullptr,10));if(mode>23)return 2;const bool cover_case=mode>=16;
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
    const auto measurement_before=goal::current_cover_pose;
    const auto entry_before=entry;live::pose_begin(entry);C(live::state.captured);goal::pose_begin(entry);C(!std::memcmp(&entry,&entry_before,sizeof entry));
    if(mode==0||mode>=5&&mode<=8||mode==10||mode==11||cover_case){
        C(goal::applied==0&&!goal::view_valid&&!live::state.prepared);C(!std::memcmp(fixture.local.data,frame->local.data(),sizeof frame->local));
        if(mode==16){C(goal::current_cover_pose_valid);
            C(goal::current_cover_pose.binding.publication==host_core.publication_sequence&&
              goal::current_cover_pose.binding.slot==host_core.cover_slot);
            C(!goal::origin.initialized&&!native_atom_overwrite_live::ready_calls);}
        if(cover_case&&mode!=16){C(!goal::current_cover_pose_valid);
            C(!std::memcmp(&measurement_before,&goal::current_cover_pose,sizeof measurement_before));
            C(!goal::origin.initialized&&!native_atom_overwrite_live::ready_calls);}
        if(cover_case)C(native_atom_overwrite_live::install_calls==0&&!live::state.kernel.outstanding_fields());
        if(mode==0){adapter::Neutral moved{};adapter::Output prepared{};adapter::Policy policy{host_now,true,body::Scope::stationary};
            C(neutral::move(goal::origin.stored,live::state.source.component_to_world,moved));
            C(adapter::prepare(moved,host_core,live::state.model,live::state.source,goal::origin.marker,{},policy,prepared)==adapter::Status::okay);
            const auto before=native_atom_overwrite_live::ready_calls;
            C(!live::apply(entry,prepared.goal,s.data.record.source)); // actual apply's independent receipt gate
            C(native_atom_overwrite_live::ready_calls>before&&!live::state.prepared&&!live::state.kernel.pending());
            C(!std::memcmp(fixture.local.data,frame->local.data(),sizeof frame->local));}
        finish_return(entry);C(!goal::pending_count()&&!live::pending_count()&&!goal::view_valid);
        if(cover_case)C(!goal::current_cover_pose_valid);
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
            const auto before=rotated;auto invalid=frame->component_to_world;for(unsigned i=0;i<3;++i)invalid.values[i]*=2;
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
            printf("diagnose adapter=%u planner=%u bone=%u distance=%g reach=%g goal_reject=%u admission=%u now=%lld scene_ns=%lld source_ns=%llu head_ns=%lld\n",unsigned(status),unsigned(report.status),report.failed_bone,report.requested_distance,report.available_reach,goal::first_reject.reason,unsigned(live::admission_budget()),host_now,live::failure::ns(uint64_t(host_core.scene_qpc),host_core.qpc_frequency),host_core.published_ns,host_core.stereo.received_ns);
            if(auto* failure=live::first_failure.published())printf("native first stage=%u detail=%u\n",failure->header.stage,failure->header.detail);}
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
