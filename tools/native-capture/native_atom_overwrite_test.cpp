// Actual receipt live/Registry/Backend/Kernel/RawInvoke6 and Win32 memory.
// Decoder body, actor context and entry attestation are explicit Host seams.
#pragma push_macro("main")
#undef main
#define main PriorBackendTestMain
#include "full_body_c0_native_backend_test.cpp"
#undef main
#pragma pop_macro("main")
#include "world_epoch_gate.h"
#include "MinHook.h"
#include "native_atom_overwrite_live.h"
#include <atomic>
#define GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT 1
namespace cover_pose_phase_live {}
uintptr_t base{};std::atomic<unsigned> active{};
gears_world::EpochGate gate;thread_local gears_world::ThreadState thread_state;
unsigned scope_entries{},observer_reads{},entry_checks{},expected_scope_depth{1};
struct Scope {gears_world::RecursiveAdmission admission{thread_state,gate};Scope(){++scope_entries;}bool world()const{return bool(admission);}};
p::CallPacket* mutate_packet{};bool after_original{},mutated{},bad_entry{};
void (*read_seam)(void const*,size_t){}; // bounded Host initialization race seam
bool safe_copy(void*out,void const*in,size_t count){
    ++observer_reads;
    if(read_seam)read_seam(in,count);
    if(after_original&&mutate_packet&&!mutated){mutate_packet->gpr[0]^=0x123456;mutated=true;}
    return n::read_memory(out,reinterpret_cast<uintptr_t>(in),count);
}
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "native_atom_overwrite_live.inl"
namespace live=native_atom_overwrite_live;namespace a=gears_atom_overwrite;
namespace native_atom_overwrite_live {bool own_entry(){++entry_checks;return !bad_entry;}}
tx::Kernel* observed_kernel{};n::Backend* observed_backend{};bool retired{};unsigned original_calls{},mode{};
namespace full_body_c0_native_live {void atom_overwrite_completed(uint64_t event)noexcept{
    const auto version=live::journal_version();const auto r=observed_kernel->retire_fresh(*observed_backend,{event});
    if(r.code==tx::Code::fresh_retired){retired=true;live::retired(version,event);}
}}
bool load_code(std::string const&path){std::ifstream f(path,std::ios::binary);char magic[8]{};uint32_t count{};
    f.read(magic,8);f.read(reinterpret_cast<char*>(&count),4);if(!f||std::memcmp(magic,"ATOMC001",8)||count!=live::cert::chunks.size())return false;
    for(unsigned i=0;i<count;++i){uint64_t rva{};uint32_t size{};f.read(reinterpret_cast<char*>(&rva),8);f.read(reinterpret_cast<char*>(&size),4);
        auto const&c=live::cert::chunks[i];if(rva!=c.rva||size!=c.size)return false;f.read(reinterpret_cast<char*>(base+rva),size);if(!f)return false;}return f.peek()==EOF;
}
struct Context final:a::Context {n::Backend&backend;uint64_t qpc=1000;bool valid=true,bad_version{};
    explicit Context(n::Backend&b):backend(b){}uint64_t thread_token()const noexcept override{return GetCurrentThreadId();}
    bool current(tx::Stamp const&,a::Observation&o)noexcept override{
        if(!valid||!live::world_current()||!backend.retirement_metadata(o.metadata))return false;
        o.journal_version=live::journal_version()+unsigned(bad_version);o.qpc=++qpc;return true;}
};
body::Atom returned_atom{};uintptr_t fixture_mesh{};
extern "C" __declspec(noinline) void HostDecoder(void*,void*out,uint64_t,float,uint64_t fifth,uint64_t sixth)noexcept(false){
    ++original_calls;C(fifth==0x1234567800000001ull&&sixth==0xfedcba9800000000ull);
    if(mode==0||mode==12)C(thread_state.depth==expected_scope_depth); // independent World Scope, no Root scope/nonce
    std::memcpy(out,&returned_atom,32);after_original=true;
    if(mode==6)RaiseException(0xe0424343,0,0,nullptr);if(mode==7)throw 19;
    if(mode==9){p::ArrayHeader bad{};std::memcpy(&bad,reinterpret_cast<void const*>(fixture_mesh+0x27c),16);--bad.capacity;std::memcpy(reinterpret_cast<void*>(fixture_mesh+0x27c),&bad,16);}
}
bool seh_call(p::CallPacket*pkt,uintptr_t ra,uintptr_t rsp,uintptr_t const*args){__try{live::GearsAtomOverwriteProtected(pkt,ra,rsp,args);return false;}
    __except(GetExceptionCode()==0xe0424343?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}}
int main(int argc,char**argv){if(argc!=4)return 2;mode=unsigned(std::strtoul(argv[3],nullptr,10));std::string directory=argv[1];
    Page image{0xc8b000};base=image.address();C(load_code(argv[2]));live::enabled=true;
    C(!live::code_current());C(live::freeze_source_image());live::certified=live::reserved=true;C(live::source_bytes(false));C(live::ready_for_commit());
    auto frame=std::make_unique<body::Frame>();auto workspace=std::make_unique<body::Workspace>();body::Model model{};body::Output output{};
    C(read_file(directory+"/reference.bin",frame->reference.data(),sizeof frame->reference));C(read_file(directory+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    C(read_file(directory+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));C(body::prepare_model(frame->reference,hash,model));
    for(unsigned i=0;i<16;++i)workspace->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),workspace->pose.local);C(body::d::rebuild(model,*workspace));
    body::Goal goal{};goal.scope=body::Scope::fixed_cover;goal.body_yaw_delta=-.1;goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};goal.head_world=body::d::pose(workspace->pose.world[55]);goal.head_world.position=body::d::add(goal.head_world.position,{3,3,-5});
    for(unsigned i=0;i<2;++i)goal.feet_world[i]=body::d::pose(workspace->pose.world[body::legs[i][2]]);C(body::plan(model,*frame,goal,*workspace,output).status==body::Status::solved);
    Fixture f(*frame);fixture_mesh=f.mesh.address();if(mode==18)f.mesh.put(0x338,UINT32_MAX);C(f.capture());tx::Kernel kernel;tx::Lease lease;const auto stamp=f.backend.stamp();
    C(kernel.prepare(f.backend,stamp,f.captured,output,f.desired_copy,lease).code==tx::Code::prepared);
    if(mode==18){C(kernel.commit(f.backend,lease,stamp).code==tx::Code::guard_rejected);C(!kernel.outstanding_fields());
        C(kernel.finish(f.backend,lease,stamp,tx::End::before_consumer).pending==0);C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));
        printf("atom_receipt mode=18: %u checks %u failures; terminal_epoch_rejects_before_CAS=1\n",checks,failures);return failures?1:0;}
    C(kernel.commit(f.backend,lease,stamp).code==tx::Code::committed);
    Context context(f.backend);C(live::install(context,stamp,f.backend.installation_epoch()));f.backend.receipt_source(live::receipts());
    C(kernel.notify_consumer_entry(f.backend,lease,stamp).code==tx::Code::consumer_marked);C(kernel.finish(f.backend,lease,stamp,tx::End::original_returned).code==tx::Code::scope_closed);
    observed_kernel=&kernel;observed_backend=&f.backend;C(kernel.outstanding_fields()==13);C(!live::ready_for_commit());
    C(kernel.retire_fresh(f.backend,{1}).code==tx::Code::bad_receipt); // value equality has no receipt
    f.context.allowed=false; // original C0 scope closed; no Root scope/ticket exists in this test
    C(thread_state.depth==0); // only each real decoder callback may acquire its independent World Scope
    f.mesh.put(0x338,uint32_t(mode==1?88:mode==2?1:89));
    Page sequence{4096},offsets{4096},keys{8192},stack{4096};const float duration=1;
    sequence.put(0x78,duration);sequence.put(0xc1,uint8_t(mode==5?6:1));sequence.put(0xc4,p::ArrayHeader{keys.address(),4096,8192});sequence.put(0xd4,p::ArrayHeader{offsets.address(),4,1024});
    const std::array<int32_t,4> counts{{0,2,24,2}};offsets.put(0,counts);
    uintptr_t args[2]{0x1234567800000001ull,0xfedcba9800000000ull};const auto rsp=stack.address()+8,ra=base+live::cert::return_rva;stack.put(8,ra);stack.put(0x30,args[0]);stack.put(0x38,args[1]);
    live::GearsAtomOriginalTarget=reinterpret_cast<void*>(HostDecoder);live::GearsAtomExpectedCaller=ra;
    if(mode==8)*(reinterpret_cast<unsigned char*>(base+0xc89a78+100))^=1;
    if(mode==12)live::request_drain();if(mode==14)context.valid=false;if(mode==15)context.bad_version=true;if(mode==16)bad_entry=true;
    auto skipped=[&](uintptr_t out,uintptr_t caller){
        std::array<unsigned char,32> saved{};std::memcpy(saved.data(),reinterpret_cast<void const*>(out),32);
        p::CallPacket packet{};packet.gpr[1]=sequence.address();packet.gpr[2]=out;
        const auto scopes=scope_entries,reads=observer_reads,entries=entry_checks,calls=original_calls;
        const auto observed=live::observed.load(),qualified=live::qualified.load();expected_scope_depth=0;
        live::GearsAtomOverwriteProtected(&packet,caller,rsp,args);expected_scope_depth=1;
        C(scope_entries==scopes&&observer_reads==reads&&entry_checks==entries);
        C(original_calls==calls+1&&live::observed==observed+1&&live::qualified==qualified);
        C(live::callbacks==0&&active==0&&thread_state.depth==0);
        std::memcpy(reinterpret_cast<void*>(out),saved.data(),32); // restore this Host-only decoder fixture
    };
    Page other_actor_local{4096};
    if(mode==0||mode==12){
        C(!live::registry.may_match(f.local.address(),ra,GetCurrentThreadId()+1));
        skipped(other_actor_local.address(),ra); // same GT/caller, another actor's storage
        skipped(f.local.address()+16,ra); // same storage, not a complete aligned Atom
        skipped(f.local.address()+115*32,ra); // first byte after the registered 115 Atoms
        skipped(f.local.address(),ra+1);
    }
    DWORD old{},ignored{};if(mode==13)C(VirtualProtect(sequence.data,4096,PAGE_NOACCESS,&old)!=FALSE);
    if(mode==10||mode==17){p::CallPacket p{};p.gpr[1]=sequence.address();p.gpr[2]=f.local.address();const float time=.5f;std::memcpy(p.xmm[3].data(),&time,4);
        live::Call call{};live::acquire(call);live::begin_guarded(call,p,ra,rsp,args);C(call.ticket.admitted);auto copy=call;
        if(mode==10){live::release(copy);C(live::callbacks==1&&active==1&&live::pending_count()>0);}else live::current_call=nullptr;
        live::release(call);C(live::callbacks==0&&active==0&&live::pending_count()>0&&thread_state.depth==0);
        live::release(call);C(live::callbacks==0&&active==0&&live::pending_count()>0);C(kernel.outstanding_fields()==13);
    }else{
        for(unsigned bone=0;bone<115;++bone){if(!std::memcmp(&frame->local[bone],&output.local[bone],32))continue;
            p::CallPacket p{};p.gpr[1]=sequence.address();p.gpr[2]=f.local.address()+bone*32;p.gpr[3]=0;
            const float time=.5f;std::memcpy(p.xmm[3].data(),&time,4);returned_atom=output.local[bone]; // explicit equal-value stores still count only actual events
            if(mode==4)p.gpr[2]+=16;after_original=false;mutated=false;if(mode==11)mutate_packet=&p;
            if(mode==6)C(seh_call(&p,ra,rsp,args));else if(mode==7){bool caught=false;try{live::GearsAtomOverwriteProtected(&p,ra,rsp,args);}catch(int x){caught=x==19;}C(caught);}
            else live::GearsAtomOverwriteProtected(&p,mode==3?ra+1:ra,rsp,args);
            mutate_packet=nullptr;C(live::callbacks==0&&active==0&&thread_state.depth==0);
            if(mode==6||mode==7||mode==9||mode==11)break;
        }
        const bool good=mode==0||mode==12;C(retired==good);C(kernel.outstanding_fields()==(good?0u:13u));
        if(good){C(kernel.pending()==0&&live::pending_count()==0);C(scope_entries>0&&observer_reads>0&&entry_checks>0);
            skipped(f.local.address(),ra); // owner thread remains registered, but no bound journal
            const auto old_version=live::journal_version();C(live::install(context,stamp,89));C(live::journal_version()==old_version+1&&live::registry.event()==0);
            tx::FreshEvidence evidence{};tx::View v{stamp,f.captured,output,f.desired_copy,nullptr,0};C(live::registry.verify(v,{1},evidence)==tx::Verdict::denied);}
        else C(kernel.pending()>0);
        C(original_calls>0);
    }
    if(mode==13)C(VirtualProtect(sequence.data,4096,old,&ignored)!=FALSE);
    // Mechanical cohort negatives plus all six supported format selectors.
    if(mode==0){a::Arguments x{sequence.address(),f.local.address(),ra,0,args[0],args[1],0};const float time=.5f;std::memcpy(&x.time_bits,&time,4);a::Cohort c{};
        for(unsigned format=0;format<6;++format){sequence.put(0xc1,uint8_t(format));C(a::cohort(safe_copy,x,c)&&c.branch==1&&c.read_mask==3);}
        sequence.put(0xc1,uint8_t(7));C(!a::cohort(safe_copy,x,c));sequence.put(0xc1,uint8_t(1));
        x.track=1;C(!a::cohort(safe_copy,x,c));x.track=0;x.out=sequence.address()+0x80;C(!a::cohort(safe_copy,x,c));x.out=f.local.address();
        offsets.put(4,int32_t(-1));C(!a::cohort(safe_copy,x,c));offsets.put(4,int32_t(0));C(a::cohort(safe_copy,x,c)&&c.branch==3);
        x.sixth|=1;C(a::cohort(safe_copy,x,c)&&c.branch==3);sequence.put(0x88,p::ArrayHeader{keys.address(),1,1});
        keys.put(8,int32_t(1));keys.put(24,int32_t(1));keys.put(40,int32_t(1));C(a::cohort(safe_copy,x,c)&&c.branch==2);
        sequence.put(0x88,p::ArrayHeader{keys.address(),1,65537});C(!a::cohort(safe_copy,x,c));x.sixth&=~uint64_t(1);
        offsets.put(4,int32_t(2));sequence.put(0xc4,p::ArrayHeader{keys.address(),4096,64*1024*1024+1});C(!a::cohort(safe_copy,x,c));
        sequence.put(0xc4,p::ArrayHeader{keys.address(),4096,8192});sequence.put(0xd4,p::ArrayHeader{offsets.address(),4,4*65536+1});C(!a::cohort(safe_copy,x,c));
        sequence.put(0xd4,p::ArrayHeader{UINTPTR_MAX-4,4,1024});C(!a::cohort(safe_copy,x,c));
        sequence.put(0xd4,p::ArrayHeader{f.local.address(),4,1024});C(!a::cohort(safe_copy,x,c));
        sequence.put(0xd4,p::ArrayHeader{offsets.address(),460,1024});offsets.put(114*16,counts);x.track=114;C(a::cohort(safe_copy,x,c));
        offsets.put(114*16+8,int32_t(4096));C(!a::cohort(safe_copy,x,c));
    }
    printf("atom_receipt mode=%u: %u checks %u failures; real_live_registry_backend_CAS=1 real_source_hash=1 Host_decoder_actor_attestation=1\n",mode,checks,failures);return failures?1:0;
}
