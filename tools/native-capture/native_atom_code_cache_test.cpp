#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
unsigned provider_calls{};std::uint64_t hashed_bytes{};
NTSTATUS WINAPI counted_open(BCRYPT_ALG_HANDLE*out,LPCWSTR name,LPCWSTR implementation,ULONG flags){++provider_calls;return BCryptOpenAlgorithmProvider(out,name,implementation,flags);}
NTSTATUS WINAPI counted_hash(BCRYPT_HASH_HANDLE hash,PUCHAR data,ULONG size,ULONG flags){hashed_bytes+=size;return BCryptHashData(hash,data,size,flags);}
#define BCryptOpenAlgorithmProvider counted_open
#define BCryptHashData counted_hash
#define main PriorAtomMain
#include "native_atom_overwrite_test.cpp"
#undef main
#undef BCryptOpenAlgorithmProvider
#undef BCryptHashData
#include "full_body_c0_first_failure.h"
#include "legacy_atom_code_check_host.inl"
uint64_t tick(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return uint64_t(q.QuadPart);}
double milliseconds(uint64_t count){LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);return double(count)*1000./double(frequency.QuadPart);}
struct BenchContext final:n::Context {bool legacy{};std::array<unsigned,5> calls{};
    bool current(tx::Stage stage)noexcept override {++calls[unsigned(stage)];
        return stage!=tx::Stage::commit||(legacy?(live::enabled&&!live::stopping&&live::registry.can_install(GetCurrentThreadId())&&legacy_code_current()):live::ready_for_commit());}
};
struct Cycle {uint64_t plan{},prepare{},commit{},consume_end{},retire{};unsigned providers_commit{},units{},current_commit{};};
void run_two_cycles(gears_full_body_c0_first_failure::Record const&snapshot,bool legacy,Cycle(&times)[2]){
    Fixture f(snapshot.source);BenchContext admission;admission.legacy=legacy;auto kernel=std::make_unique<tx::Kernel>();auto lease=std::make_unique<tx::Lease>();
    auto frame=std::make_unique<body::Frame>();auto output=std::make_unique<body::Output>();auto work=std::make_unique<body::Workspace>();body::Model model{};tx::Desired desired{};
    C(body::prepare_model(snapshot.source.reference,hash,model));observed_kernel=kernel.get();observed_backend=&f.backend;fixture_mesh=f.mesh.address();
    Context context(f.backend);Page sequence{4096},offsets{4096},keys{8192},stack{4096};const float duration=1;
    sequence.put(0x78,duration);sequence.put(0xc1,uint8_t(1));sequence.put(0xc4,p::ArrayHeader{keys.address(),4096,8192});sequence.put(0xd4,p::ArrayHeader{offsets.address(),4,1024});
    offsets.put(0,std::array<int32_t,4>{{0,2,24,2}});uintptr_t args[]{0x1234567800000001ull,0xfedcba9800000000ull};
    const auto rsp=stack.address()+8,ra=base+live::cert::return_rva;stack.put(8,ra);stack.put(0x30,args[0]);stack.put(0x38,args[1]);
    live::GearsAtomOriginalTarget=reinterpret_cast<void*>(HostDecoder);
    for(unsigned cycle=0;cycle<2;++cycle){auto&t=times[cycle];retired=false;admission.calls={};
        if(cycle){C(!kernel->pending());f.backend.~Backend();::new(static_cast<void*>(&f.backend))n::Backend;lease=std::make_unique<tx::Lease>();}
        auto seed=f.seed;seed.frame+=cycle*10;seed.producer+=cycle*10;seed.scope+=cycle*10;
        C(f.backend.capture(admission,f.owner,seed,*frame,desired));const auto stamp=f.backend.stamp();
        auto start=tick();const auto report=body::plan(model,*frame,snapshot.goal,*work,*output);t.plan=tick()-start;
        C(report.status==body::Status::solved);C(std::abs(report.head_error-snapshot.report.head_error)<1e-10);
        start=tick();C(kernel->prepare(f.backend,stamp,*frame,*output,desired,*lease).code==tx::Code::prepared);t.prepare=tick()-start;
        const auto before=provider_calls;start=tick();C(kernel->commit(f.backend,*lease,stamp).code==tx::Code::committed);t.commit=tick()-start;
        t.providers_commit=provider_calls-before;t.units=kernel->outstanding_fields();t.current_commit=admission.calls[unsigned(tx::Stage::commit)];
        C(t.units>0&&t.current_commit==3*t.units);C(t.providers_commit==(legacy?7*t.current_commit:0));
        C(live::install(context,stamp,f.backend.installation_epoch()));f.backend.receipt_source(live::receipts());
        start=tick();C(kernel->notify_consumer_entry(f.backend,*lease,stamp).code==tx::Code::consumer_marked);
        C(kernel->finish(f.backend,*lease,stamp,tx::End::original_returned).code==tx::Code::scope_closed);t.consume_end=tick()-start;
        f.mesh.put(0x338,uint32_t(89+cycle));start=tick();
        for(unsigned bone=0;bone<115&&!retired;++bone){if(!std::memcmp(&frame->local[bone],&output->local[bone],32))continue;
            p::CallPacket packet{};packet.gpr[1]=sequence.address();packet.gpr[2]=f.local.address()+bone*32;const float time=.5f;std::memcpy(packet.xmm[3].data(),&time,4);
            returned_atom=snapshot.source.local[bone];after_original=false;live::GearsAtomOverwriteProtected(&packet,ra,rsp,args);}
        t.retire=tick()-start;C(retired&&!kernel->pending()&&!kernel->failed()&&!kernel->protocol_debt());
        C(!std::memcmp(f.local.data,snapshot.source.local.data(),sizeof snapshot.source.local));C(live::ready_for_commit());
        printf("{\"algorithm\":\"%s\",\"cycle\":%u,\"units\":%u,\"commit_context_checks\":%u,\"commit_provider_calls\":%u,\"plan_ms\":%.6f,\"prepare_ms\":%.6f,\"commit_ms\":%.6f,\"consume_end_ms\":%.6f,\"retire_ms\":%.6f,\"registry_version\":%llu}\n",
            legacy?"frozen31_sha":"runtime_full_compare",cycle+1,t.units,t.current_commit,t.providers_commit,milliseconds(t.plan),milliseconds(t.prepare),milliseconds(t.commit),milliseconds(t.consume_end),milliseconds(t.retire),live::journal_version());
    }
}
int main(int argc,char**argv){if(argc!=3)return 2;mode=0;Page image{0xc8b000};base=image.address();C(load_code(argv[1]));live::enabled=true;
    C(!live::source_bytes(false)&&!live::ready_for_commit());auto const&last=live::cert::chunks.back();auto*changed=reinterpret_cast<unsigned char*>(base+last.rva+last.size-1);
    *changed^=1;C(!live::freeze_source_image()&&!live::source_bytes(false)&&!live::ready_for_commit());*changed^=1;
    read_seam=[](void const*at,size_t size){if(reinterpret_cast<uintptr_t>(at)==base+live::cert::entry&&size==1024){auto const&c=live::cert::chunks.back();*reinterpret_cast<unsigned char*>(base+c.rva+c.size-1)^=1;read_seam=nullptr;}};
    C(!live::freeze_source_image()&&!live::source_bytes(false));read_seam=nullptr;*changed^=1;
    C(live::freeze_source_image());C(live::source_bytes(false)&&!live::ready_for_commit());C(!live::freeze_source_image());
    live::reserved=live::certified=true;C(live::ready_for_commit());const auto before=provider_calls;const auto hashed=hashed_bytes;
    for(auto const&c:live::cert::chunks){auto*at=reinterpret_cast<unsigned char*>(base+c.rva+c.size-1);*at^=1;C(!live::code_current()&&!live::ready_for_commit());*at^=1;C(live::code_current());}
    auto*call=reinterpret_cast<unsigned char*>(base+live::cert::call_rva);*call^=1;C(!live::code_current());*call^=1;
    bad_entry=true;C(!live::code_current());bad_entry=false;const auto original_base=base;++base;C(!live::code_current());base=original_base;
    DWORD previous{},ignored{};const auto protected_page=(base+live::cert::chunks[3].rva)&~uintptr_t(4095);
    C(VirtualProtect(reinterpret_cast<void*>(protected_page),4096,PAGE_NOACCESS,&previous)!=FALSE);C(!live::code_current());C(VirtualProtect(reinterpret_cast<void*>(protected_page),4096,previous,&ignored)!=FALSE);
    C(live::code_current()&&provider_calls==before&&hashed_bytes==hashed);
    constexpr unsigned repeats=30;auto start=tick();for(unsigned i=0;i<repeats;++i)C(legacy_code_current());const auto old_time=tick()-start;
    const auto middle=provider_calls;start=tick();for(unsigned i=0;i<repeats;++i)C(live::code_current());const auto new_time=tick()-start;C(provider_calls==middle);
    printf("{\"source_checks\":%u,\"old_total_ms\":%.6f,\"new_total_ms\":%.6f,\"old_provider_calls\":%u,\"new_provider_calls\":%u,\"bytes_per_check\":4891}\n",repeats,milliseconds(old_time),milliseconds(new_time),middle-before,provider_calls-middle);
    auto snapshot=std::make_unique<gears_full_body_c0_first_failure::Record>();C(read_file(argv[2],snapshot.get(),sizeof *snapshot));C(snapshot->header.stage==3&&snapshot->header.detail==12&&snapshot->header.unit==6);
    Cycle old_cycles[2]{},new_cycles[2]{};run_two_cycles(*snapshot,true,old_cycles);run_two_cycles(*snapshot,false,new_cycles);
    C(!live::pending_count()&&!active&&live::journal_version()==4);
    printf("AtomCodeCache: %u checks, %u failures; actual_B2_Frame_Goal296=1 full_current_bytes=1 actual_CAS_Registry=1 Host_context_and_decoder=1\n",checks,failures);return failures?1:0;
}
