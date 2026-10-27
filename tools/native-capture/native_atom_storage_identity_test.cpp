// Actual actor storage reader and actual Native Backend/Kernel retirement.
// Existing synthetic actor registry + isolated pose pages; no game calls.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "full_body_c0_native_backend.h"
#define main PriorActorTestMain
#include "weapon_hand_scene_binding_test.cpp"
#undef main
#undef C
#define checks backend_checks
#define failures backend_failures
#define main PriorPoseTestMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
#undef checks
#undef failures
namespace atom=gears_atom_overwrite;
struct StorageContext final:atom::Context {
    h::Binding actor;Fixture& fixture;atom::Registry& registry;uint64_t time=100;
    StorageContext(h::Binding const&a,Fixture&f,atom::Registry&r):actor(a),fixture(f),registry(r){}
    uint64_t thread_token()const noexcept override{return GetCurrentThreadId();}
    bool current(tx::Stamp const&,atom::Observation&out)noexcept override {
        if(!h::current_actor_storage(actor)||!fixture.backend.retirement_metadata(out.metadata))return false;
        out.qpc=++time;out.journal_version=registry.version();return h::current_actor_storage(actor);}
};
int main(int argc,char**argv){if(argc!=2)return 2;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    setup();h::Binding owner{};C(take(owner));C(h::current_actor(owner)&&h::current_actor_retained(owner)&&h::current_actor_storage(owner));
    std::string directory=argv[1];auto frame=std::make_unique<body::Frame>();auto workspace=std::make_unique<body::Workspace>();body::Model model{};body::Output output{};
    C(read_file(directory+"/reference.bin",frame->reference.data(),sizeof frame->reference));C(read_file(directory+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read_file(directory+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));C(body::prepare_model(frame->reference,hash,model));
    for(unsigned i=0;i<16;++i)workspace->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),workspace->pose.local);C(body::d::rebuild(model,*workspace));
    body::Goal goal{};goal.scope=body::Scope::fixed_cover;goal.body_yaw_delta=-.1;goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};goal.head_world=body::d::pose(workspace->pose.world[55]);goal.head_world.position=body::d::add(goal.head_world.position,{3,3,-5});
    for(unsigned i=0;i<2;++i)goal.feet_world[i]=body::d::pose(workspace->pose.world[body::legs[i][2]]);C(body::plan(model,*frame,goal,*workspace,output).status==body::Status::solved);
    Fixture f(*frame);C(f.capture());tx::Kernel kernel;tx::Lease lease;auto const stamp=f.backend.stamp();
    C(kernel.prepare(f.backend,stamp,f.captured,output,f.desired_copy,lease).code==tx::Code::prepared);C(kernel.commit(f.backend,lease,stamp).code==tx::Code::committed);
    atom::Registry registry;StorageContext context(owner,f,registry);constexpr uintptr_t caller=0x1400e0a73;
    C(registry.install(context,stamp,88,caller));f.backend.receipt_source(&registry);
    C(kernel.notify_consumer_entry(f.backend,lease,stamp).code==tx::Code::consumer_marked);C(kernel.finish(f.backend,lease,stamp,tx::End::original_returned).code==tx::Code::scope_closed);C(kernel.outstanding_fields()==13);
    put(fixture_pawn+0x191,static_cast<unsigned char>(4));C(!h::current_actor(owner)&&!h::current_actor_retained(owner)&&h::current_actor_storage(owner));
    put(fixture_pawn+0x1d0,fixture_weapon2);put(fixture_weapon+8,0u);C(!h::current_actor(owner)&&h::current_actor_storage(owner));
    f.context.allowed=h::current_actor(owner);tx::Kernel new_work;tx::Lease blocked_lease;
    C(new_work.prepare(f.backend,stamp,f.captured,output,f.desired_copy,blocked_lease).code==tx::Code::guard_rejected&&new_work.pending()==0);
    recording=false;C(!h::current_actor(owner)&&h::current_actor_storage(owner));f.mesh.put(0x338,uint32_t(89));
    atom::Cohort cohort{};cohort.format=1;cohort.branch=1;cohort.read_mask=3;
    for(unsigned bone=0;bone<115;++bone)if(std::memcmp(&frame->local[bone],&output.local[bone],32)){
        atom::Arguments args{};args.caller=caller;args.out=f.local.address()+bone*32;atom::Ticket ticket{};
        C(registry.begin(args,cohort,ticket));std::memcpy(reinterpret_cast<void*>(args.out),&output.local[bone],32);
        C(registry.finish(ticket,cohort,true,true));kernel.retire_fresh(f.backend,{registry.event()});}
    C(kernel.outstanding_fields()==0&&kernel.pending()==0);C(!h::current_actor(owner)&&h::current_actor_storage(owner));
    // Changed real storage identity remains a rejection, irrespective of policy.
    put(fixture_pawn+0x188,fixture_weapon2);C(!h::current_actor_storage(owner));
    setup();C(take(owner));recording=false;put(fixture_mesh+76,3u);C(!h::current_actor_storage(owner));
    setup();C(take(owner));recording=false;committed_world_generation=2;C(!h::current_actor_storage(owner));
    C(native_calls==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=FALSE);
    printf("atom_storage_identity: %u checks %u failures; actual_actor_reader_backend_kernel=1 synthetic_registry_pose_mapping=1 SM_weapon_exit_retires13=1 new_admission_denied=1\n",checks+backend_checks,failures+backend_failures);return (failures||backend_failures)?1:0;
}
