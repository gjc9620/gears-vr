#include "native_atom_overwrite_certificate.h"
namespace native_atom_overwrite_live {
namespace atom=gears_atom_overwrite;namespace pose=gears_cover_pose_phase;namespace phase=cover_pose_phase_live;
namespace cert=gears_atom_overwrite_certificate;
bool enabled{};void*target{};bool own_entry();
extern "C" {void* GearsAtomOriginalTarget{};uintptr_t GearsAtomExpectedCaller{};}
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
extern "C" void GearsAtomOverwriteHook();extern "C" void GearsAtomRawInvoke6(pose::CallPacket*,void*,uintptr_t,uintptr_t)noexcept(false);
atom::Registry registry;std::atomic<DWORD> owner_thread{};std::atomic<unsigned> callbacks{},protocol{};
std::atomic<uint64_t> blend_started{},blend_qualified{};
std::atomic<uint64_t> observed{},qualified{},rejected{};std::atomic<bool> stopping{};bool reserved{},certified{};
struct Call {Call*self{},*previous{};DWORD thread{};bool counted{},linked{},returned{},packet_ok{};
    atom::Arguments arguments{};atom::Cohort before{},after{};atom::Ticket ticket{};
    weapon_part_visibility_live::DecoderTicket part{};
    lancer_original_handle_live::DecoderTicket lancer_handle{};
    alignas(::Scope) unsigned char scope_bytes[sizeof(::Scope)]{};::Scope* scope{};};
thread_local Call* current_call{};
void debt()noexcept{auto x=protocol.load();while(x!=UINT_MAX&&!protocol.compare_exchange_weak(x,x+1)){};stopping=true;}
unsigned pending_count(){const auto n=uint64_t(callbacks.load())+registry.pending()+protocol.load();return n>UINT_MAX?UINT_MAX:unsigned(n);}
bool world_current()noexcept{return fresh_pose_producer_live::blend_world_current()||(current_call&&current_call->self==current_call&&current_call->thread==GetCurrentThreadId()&&current_call->scope&&current_call->scope->world());}
constexpr size_t source_image_size(){size_t n{};for(auto const&c:cert::chunks)n+=c.size;return n;}
static_assert(source_image_size()==4891);
// Runtime-only original bytes, acquired and SHA-certified before hook install.
// They are never serialized or embedded in the source. This is an immutable
// comparison reference, not a permission: every current check reads ALL chunks
// again and authenticates the currently installed hook entry and caller.
std::array<unsigned char,source_image_size()> source_image{};uintptr_t source_image_base{};
std::atomic<unsigned> source_image_state{}; // 0 unavailable, 1 building, 2 immutable
bool compare_source_image(bool installed){
    if(source_image_base!=base)return false;size_t offset{};std::array<unsigned char,1024> bytes{};
    for(auto const&c:cert::chunks){
        for(unsigned at=0;at<c.size;){unsigned n=(std::min)(unsigned(bytes.size()),c.size-at);
            if(installed&&c.rva==cert::entry&&at==0){n=32;
                if(!own_entry()||std::memcmp(source_image.data()+offset,cert::prefix.data(),32))return false;}
            else if(!safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva+at),n)||
                    std::memcmp(bytes.data(),source_image.data()+offset+at,n))return false;
            at+=n;}
        offset+=c.size;
    }return offset==source_image.size();
}
bool freeze_source_image(){
    unsigned empty{};if(reserved||certified||!base||!source_image_state.compare_exchange_strong(empty,1))return false;
    source_image_base=base;size_t offset{};bool valid=true;
    for(auto const&c:cert::chunks){BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};unsigned char digest[32]{};
        // Hash exactly the owned bytes that will become the reference; a later
        // unrelated re-read cannot silently supply the cached comparison image.
        bool ok=safe_copy(source_image.data()+offset,reinterpret_cast<void const*>(base+c.rva),c.size);
        if(ok)ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
        if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
        if(ok)ok=BCryptHashData(h,source_image.data()+offset,c.size,0)>=0;
        if(ok)ok=BCryptFinishHash(h,digest,32,0)>=0;if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);
        constexpr char hex[]="0123456789abcdef";if(ok)for(unsigned i=0;i<32;++i)if(c.sha256[i*2]!=hex[digest[i]>>4]||c.sha256[i*2+1]!=hex[digest[i]&15])ok=false;
        if(!ok){valid=false;break;}offset+=c.size;
    }
    valid=valid&&offset==source_image.size()&&compare_source_image(false);
    source_image_state.store(valid?2u:0u,std::memory_order_release);return valid;
}
bool source_bytes(bool installed){return source_image_state.load(std::memory_order_acquire)==2&&compare_source_image(installed);}
bool code_current(){std::array<unsigned char,5>b{};return certified&&own_entry()&&source_bytes(true)&&
    safe_copy(b.data(),reinterpret_cast<void const*>(base+cert::call_rva),5)&&b==cert::call;}
bool ready_for_commit()noexcept{try{return enabled&&!stopping.load()&&(!owner_thread.load()||owner_thread.load()==GetCurrentThreadId())&&
    registry.can_install(GetCurrentThreadId())&&code_current();}catch(...){return false;}}
void acquire(Call&c){c.self=&c;c.thread=GetCurrentThreadId();c.counted=true;c.previous=current_call;current_call=&c;c.linked=true;++callbacks;++active;++observed;}
void begin(Call&c,pose::CallPacket const&p,uintptr_t ra,uintptr_t rsp,uintptr_t const*args){
    const bool body=enabled&&owner_thread.load()==GetCurrentThreadId()&&registry.may_match(p.gpr[2],ra,GetCurrentThreadId())&&!c.previous;
    const bool part=(weapon_part_visibility_live::pending()||lancer_original_handle_live::pending())&&p.gpr[3]==4&&!c.previous;
    if(!body&&!part)return;
    // Preserve the established body's original World admission point. The
    // independent visibility path neither borrows nor replaces that lease.
    if(body){c.scope=::new(static_cast<void*>(c.scope_bytes))::Scope;if(!c.scope->world())return;}
    uintptr_t actual_ra{},slots[2]{};
    if((rsp&15)!=8||!safe_copy(&actual_ra,reinterpret_cast<void const*>(rsp),8)||actual_ra!=ra||
       !safe_copy(slots,reinterpret_cast<void const*>(rsp+0x28),16)||slots[0]!=args[0]||slots[1]!=args[1]||!code_current())return;
    c.arguments={p.gpr[1],p.gpr[2],ra,p.gpr[3],args[0],args[1],0};std::memcpy(&c.arguments.time_bits,p.xmm[3].data(),4);
    if(part){weapon_part_visibility_live::decoder_begin(c.arguments,c.part);lancer_original_handle_live::decoder_begin(c.arguments,c.lancer_handle);}
    if(!body)return;
    if(!atom::cohort(safe_copy,c.arguments,c.before))return;atom::Cohort again{};
    if(!atom::cohort(safe_copy,c.arguments,again)||!atom::equal(c.before,again))return;
    registry.begin(c.arguments,c.before,c.ticket); // existing registration remains observable after stop
}
void finish(Call&c,pose::CallPacket&p,bool returned){const auto saved=p;c.returned=returned;
    if(c.ticket.admitted&&returned&&world_current()&&code_current()){
        if(!atom::cohort(safe_copy,c.arguments,c.after))c.returned=false;
    }else if(c.ticket.admitted)c.returned=false;
    c.packet_ok=!std::memcmp(&saved,&p,sizeof p);if(!c.packet_ok){p=saved;debt();}
    weapon_part_visibility_live::decoder_return(c.part,returned&&c.packet_ok);
    lancer_original_handle_live::decoder_return(c.lancer_handle,returned&&c.packet_ok);
}
void release(Call&c)noexcept {
    const bool counted=c.self==&c&&c.counted&&c.thread==GetCurrentThreadId();
    const bool known=counted&&c.linked&&current_call==&c;
    if(known){
        if(c.ticket.admitted){if(registry.finish(c.ticket,c.after,c.returned&&!protocol.load(),c.packet_ok)){++qualified;full_body_c0_native_live::atom_overwrite_completed(registry.event());}else ++rejected;}
        current_call=c.previous;c.linked=false;
    }else{debt();if(counted)registry.abandon(c.ticket);}
    // Physical callback ownership is independent of logical registry linkage.
    // Its exact finally must consume its World admission even when linkage is
    // unknown; the separate protocol debt remains and blocks receipt/unload.
    if(counted){if(c.scope==reinterpret_cast<::Scope*>(c.scope_bytes)){c.scope->~Scope();c.scope=nullptr;}
        else if(c.scope)debt();if(current_call==&c)current_call=c.previous;c.counted=false;--callbacks;--active;}
}
void begin_guarded(Call&c,pose::CallPacket const&p,uintptr_t ra,uintptr_t rsp,uintptr_t const*args)noexcept{try{begin(c,p,ra,rsp,args);}catch(...){debt();}}
void finish_guarded(Call&c,pose::CallPacket&p,bool returned)noexcept{const auto saved=p;
    try{finish(c,p,returned);}catch(...){c.returned=false;debt();}
    if(std::memcmp(&saved,&p,sizeof p)){p=saved;c.packet_ok=false;debt();}}
extern "C" __declspec(noinline) void GearsAtomOverwriteProtected(pose::CallPacket*p,uintptr_t ra,uintptr_t rsp,uintptr_t const*args)noexcept(false){
    Call c{};bool returned{};acquire(c);
    __try{begin_guarded(c,*p,ra,rsp,args);GearsAtomRawInvoke6(p,GearsAtomOriginalTarget,args[0],args[1]);returned=true;}
    __finally{__try{finish_guarded(c,*p,returned&&!AbnormalTermination());}__finally{release(c);}}
}
bool install(atom::Context&context,atom::tx::Stamp const&s,std::uint32_t epoch)noexcept {
    // Called only AFTER actual commit installation. A concurrent stop must not
    // prevent registration of fields that already exist and still owe retirement.
    if(!enabled)return false;const DWORD t=GetCurrentThreadId();DWORD empty{};owner_thread.compare_exchange_strong(empty,t);
    return owner_thread.load()==t&&registry.install(context,s,epoch,base+cert::return_rva);
}
std::uintptr_t registered_mesh()noexcept{return enabled&&owner_thread.load()==GetCurrentThreadId()?registry.candidate_mesh(GetCurrentThreadId()):0;}
atom::ReceiptSource*receipts()noexcept{return &registry;}
bool blend_begin(atom::BlendTicket& t,uint32_t epoch)noexcept {try{
    if(!enabled||owner_thread.load()!=GetCurrentThreadId()||!code_current())return false;
    const bool admitted=registry.begin_blend(t,epoch);if(admitted)++blend_started;return admitted;
}catch(...){return false;}}
bool blend_return(atom::BlendTicket& t,bool stores)noexcept {try{
    if(!enabled||owner_thread.load()!=GetCurrentThreadId())return false;
    if(!registry.finish_blend(t,stores&&code_current()))return false;
    ++blend_qualified;full_body_c0_native_live::atom_overwrite_completed(registry.event());return true;
}catch(...){return false;}}
uint64_t journal_version()noexcept{return registry.version();}
void retired(uint64_t version,uint64_t event)noexcept {if(owner_thread.load()!=GetCurrentThreadId()||!registry.retired(version,event))debt();}
bool reserve(){if(!enabled)return true;if(reserved||!freeze_source_image())return false;
    target=reinterpret_cast<void*>(base+cert::entry);GearsAtomExpectedCaller=base+cert::return_rva;
    std::array<unsigned char,32>b{};if(!safe_copy(b.data(),target,32)||b!=cert::prefix||
       MH_CreateHook(target,reinterpret_cast<void*>(GearsAtomOverwriteHook),&GearsAtomOriginalTarget)!=MH_OK||!GearsAtomOriginalTarget)return false;
    reserved=true;certified=true;return safe_copy(b.data(),target,32)&&b==cert::prefix;
}
bool reserved_target(void*p){return reserved&&p==target;}
bool configure(wchar_t const*ini,FILE*f){wchar_t value[8]{};GetPrivateProfileStringW(L"NativeAtomOverwriteReceipt",L"enabled",L"0",value,8,ini);
    if(wcscmp(value,L"0")&&wcscmp(value,L"1"))return false;enabled=!wcscmp(value,L"1");
    if(f)fprintf(f,"atom_overwrite_compiled=1 atom_overwrite_enabled=%u base_clean=0 space_physics_receipt=0\n",unsigned(enabled));return true;}
void request_drain(){stopping=true;} // new commits stop in Kernel; existing overwrite support continues
void summary(FILE*f){if(f)fprintf(f,"atom_blend_exact_failure=%u ",registry.blend_exact_failure());if(f)for(unsigned i=0;i<9;++i)fprintf(f,"atom_blend_result_%u=%llu ",i,registry.blend_result(i));if(f)fprintf(f,"atom_blend_started=%llu atom_blend_qualified=%llu ",blend_started.load(),blend_qualified.load());if(f)fprintf(f,"atom_overwrite_enabled=%u atom_overwrite_calls=%llu atom_overwrite_qualified=%llu atom_overwrite_rejected=%llu atom_overwrite_pending=%u ",unsigned(enabled),observed.load(),qualified.load(),rejected.load(),pending_count());}
#else
bool blend_begin(atom::BlendTicket&,uint32_t)noexcept{return false;}bool blend_return(atom::BlendTicket&,bool)noexcept{return false;}
bool world_current()noexcept{return false;}std::uintptr_t registered_mesh()noexcept{return 0;}bool ready_for_commit()noexcept{return false;}bool install(atom::Context&,atom::tx::Stamp const&,std::uint32_t)noexcept{return false;}
atom::ReceiptSource*receipts()noexcept{return nullptr;}uint64_t journal_version()noexcept{return 0;}void retired(uint64_t,uint64_t)noexcept{}
bool reserve(){return true;}bool reserved_target(void*){return false;}void request_drain(){}unsigned pending_count(){return 0;}void summary(FILE*){}
bool configure(wchar_t const*ini,FILE*){return GetPrivateProfileIntW(L"NativeAtomOverwriteReceipt",L"enabled",0,ini)==0;}
#endif
}
