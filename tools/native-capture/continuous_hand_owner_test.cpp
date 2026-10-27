// Actual candidate Rig publish/latch, Shared/Broker, EpochGate, profile/adapter/math.
// Actor/code-memory/CCTRL original computation are bounded host seams; no engine.
#define GEARS_CONTINUOUS_HAND_OWNER 1
#define GEARS_RIG_TRACKING_NO_ENTRY 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "world_epoch_gate.h"
#include "hand_core_target_adapter.h"
#include "cover_pose_phase.h"
#include "continuous_hand_owner_entry.h"
#include "continuous_hand_caller_certificate.h"
#include "cover_pose_code_certificate.h"
#include <vector>
#include <fstream>
#include <bcrypt.h>
#include <new>
gears_world::EpochGate test_world_gate;
thread_local gears_world::ThreadState test_world_thread;
unsigned constructed{},destroyed{};
struct Scope {gears_world::RecursiveAdmission admission{test_world_thread,test_world_gate};Scope(){++constructed;}~Scope(){++destroyed;}
    bool world()const{return bool(admission);}Scope(Scope const&)=delete;Scope& operator=(Scope const&)=delete;};
#include "rig_tracking_live_test.cpp"
namespace {
namespace p=gears_cover_pose_phase;namespace m=gears_hand_authored_mount;namespace co=gears_cover_control_output_pose;
namespace core=gears_native_rig_tracking;
std::array<unsigned char,0x8a9e74+26> image{};
uintptr_t base=reinterpret_cast<uintptr_t>(image.data());int64_t session_started_qpc=500;wchar_t directory[MAX_PATH]{};
std::array<unsigned char,0x400> mesh_memory{};std::array<unsigned char,0x200> asset_memory{};std::array<uintptr_t,0x268/8> vtable{};
std::array<m::ReferenceRecord68,115> reference{};p::Metadata model_metadata{};
struct Observed {co::Matrix64 limb[3]{},component{},single{};};Observed observed[2]{};
uintptr_t fixture_mesh_at=reinterpret_cast<uintptr_t>(mesh_memory.data()),asset_address=reinterpret_cast<uintptr_t>(asset_memory.data());
bool bad_owner{},bad_metadata{};
std::array<std::vector<unsigned char>,2> caller_fixture{};
unsigned full_reads[2]{},deny_read{},mutate_read{};
bool load_caller(char const* path){std::ifstream file(path,std::ios::binary);char magic[8]{};uint32_t count{};
    file.read(magic,8);file.read(reinterpret_cast<char*>(&count),4);
    if(!file||memcmp(magic,"CCHASH01",8)||count!=2)return false;
    for(unsigned i=0;i<2;++i){uint64_t rva{};uint32_t size{};file.read(reinterpret_cast<char*>(&rva),8);file.read(reinterpret_cast<char*>(&size),4);
        auto const& expected=continuous_hand_caller_certificate::chunks[i];if(!file||rva!=expected.rva||size!=expected.size)return false;
        caller_fixture[i].resize(size);file.read(reinterpret_cast<char*>(caller_fixture[i].data()),size);if(!file)return false;}
    return file.peek()==EOF;
}
bool within(uintptr_t a,size_t n,uintptr_t b,size_t count){return a>=b&&a-b<=count&&n<=count-(a-b);}
bool safe_copy(void* out,void const* in,size_t n){const auto at=reinterpret_cast<uintptr_t>(in);
    const struct R{uintptr_t at;size_t size;} ranges[]={{base,image.size()},{fixture_mesh_at,mesh_memory.size()},{asset_address,asset_memory.size()},
        {reinterpret_cast<uintptr_t>(vtable.data()),sizeof(vtable)},{reinterpret_cast<uintptr_t>(reference.data()),sizeof(reference)}};
    for(auto const& r:ranges)if(within(at,n,r.at,r.size)){
        for(unsigned i=0;i<2;++i){auto const& c=continuous_hand_caller_certificate::chunks[i];
            if(at==base+c.rva&&n==c.size){const auto call=++full_reads[i];if(i==0&&deny_read==call)return false;
                memcpy(out,in,n);if(i==0&&mutate_read==call)image[c.rva+700]^=1;return true;}}
        memcpy(out,in,n);return true;}return false;}
namespace weapon_hand_scene_binding {
std::mutex mutex;gears_anchor::Identity asset{};
bool metadata_current(){return !bad_metadata;}
bool same(gears_anchor::Identity const& a,gears_anchor::Identity const& b){return gears_anchor::equal_identity(a,b);}
bool identity(uintptr_t at,gears_anchor::Identity& out,bool){if(at!=asset.address)return false;out=asset;return true;}
bool current_actor(Binding const& expected){return !bad_owner&&same_key(expected,actor)&&expected.world_generation==committed_world_generation.load();}
}
namespace cover_body_live {bool enabled{};}
namespace weapon_hand_runtime::detail {bool engine_restore_enabled{};}
namespace cover_pose_phase_live {
bool certified=true,enabled=false;std::array<void*,3> targets{};
struct Owner {weapon_hand_scene_binding::Binding binding{};p::OwnerKey key{};unsigned thread{};bool valid{};};
struct Scope {Owner owner{};uint64_t invocation{};uintptr_t matrix{};unsigned kind{},caller{},episode{};bool tick{},matching{},started{},arguments{};};
bool own_entries(){return true;}LONGLONG qpc(){return now();}
namespace certificate=gears_cover_pose_certificate;
// Mechanically identical production digest body; real BCrypt, not a hash stub.
bool digest(certificate::Chunk const& c){
    if(!base||!c.size||c.size>65536||base>UINTPTR_MAX-c.rva||base+c.rva>UINTPTR_MAX-c.size)return false;
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};bool okay=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    std::array<unsigned char,1024> bytes{};unsigned char value[32]{};
    for(unsigned at=0;okay&&at<c.size;){const auto n=(std::min)(unsigned(bytes.size()),c.size-at);
        okay=safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva+at),n)&&BCryptHashData(h,bytes.data(),n,0)>=0;at+=n;}
    if(okay)okay=BCryptFinishHash(h,value,sizeof value,0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);
    constexpr char hex[]="0123456789abcdef";if(okay)for(unsigned i=0;i<32;++i)if(c.sha256[i*2]!=hex[value[i]>>4]||c.sha256[i*2+1]!=hex[value[i]&15])return false;return okay;
}

p::Identity compact(gears_anchor::Identity const& x){return {x.address,x.cls,x.outer,int32_t(x.index),int32_t(x.name_index),x.number};}
}
namespace continuous_hand_owner_live {bool control_current(cover_pose_phase_live::Scope const&) noexcept;}
namespace cover_control_observer_live {
bool certified=true,enabled=false;unsigned row_count=128;uint64_t selected_compose=999;
enum class Failure:unsigned{pending,okay,invalid};
struct Node {p::Metadata metadata{};};
struct Row {p::OwnerKey owner{};uint64_t compose{},outer_tick{},ordinal{},compute_ordinal{};unsigned kind{},thread{};
    uintptr_t node_argument{},raw_rdx{},mesh_argument{},out_argument{},entry_rsp{},caller{};int64_t before_qpc{},after_qpc{};
    Failure failure{};bool before_valid{},after_valid{},returned{};Node before{},after{};std::array<int32_t,3> affected{};std::array<p::Matrix,3> matrices{};};
struct Compose {cover_pose_phase_live::Scope const* address{};cover_pose_phase_live::Scope scope{};uint64_t outer_tick{};bool continuous{},admitted{};};
bool own_entries(){return true;}
template<class T>bool read(T& out,uintptr_t at){return safe_copy(&out,reinterpret_cast<void const*>(at),sizeof(T));}
bool same_metadata(p::Metadata const& a,p::Metadata const& b){return a.asset==b.asset&&a.master==b.master&&
    p::header_equal(a.reference,b.reference)&&p::header_equal(a.local,b.local)&&p::header_equal(a.space,b.space)&&p::header_equal(a.desired,b.desired)&&!memcmp(&a.component,&b.component,64);}
// Same exact-address consumer contract as production CCTRL current().
bool current(Compose const& f){return f.admitted&&own_entries()&&f.address&&continuous_hand_owner_live::control_current(*f.address);}
Failure before(Compose const& f,Row& row){if(!current(f)||row.caller!=base+0x1290e5||row.out_argument!=row.entry_rsp+0x80)return Failure::invalid;
    row.before.metadata=model_metadata;const unsigned side=row.raw_rdx==11?0u:1u;
    row.affected=row.kind?std::array<int32_t,3>{int32_t(row.raw_rdx),0,0}:side?std::array<int32_t,3>{32,34,35}:std::array<int32_t,3>{8,10,11};return Failure::okay;}
Failure after(Compose const& f,Row& row){if(!current(f))return Failure::invalid;
    row.after.metadata=model_metadata;const unsigned side=row.raw_rdx==11?0u:1u;
    memcpy(row.matrices.data(),row.kind?&observed[side].single:observed[side].limb,row.kind?64:192);return Failure::okay;}
}
#include "continuous_hand_owner_live.inl"
namespace live=continuous_hand_owner_live;using Entry=gears_continuous_hand_entry::Entry;
bool fixtures(char const* matrices,char const* refs){std::ifstream f(matrices,std::ios::binary);char magic[8]{};uint32_t count{},stride{};
    f.read(magic,8);f.read(reinterpret_cast<char*>(&count),4);f.read(reinterpret_cast<char*>(&stride),4);f.read(reinterpret_cast<char*>(observed),sizeof(observed));
    std::ifstream r(refs,std::ios::binary);r.read(reinterpret_cast<char*>(reference.data()),sizeof(reference));
    return bool(f)&&bool(r)&&!memcmp(magic,"HCTRO3V1",8)&&count==2&&stride==sizeof(Observed)&&f.peek()==EOF&&r.peek()==EOF;}
void memory(){auto put=[](auto& a,size_t offset,auto const& v){memcpy(a.data()+offset,&v,sizeof(v));};
    model_metadata.asset=asset_address;model_metadata.reference={reinterpret_cast<uintptr_t>(reference.data()),115,115};
    model_metadata.local={0x230000,115,115};model_metadata.space={0x240000,115,115};model_metadata.desired={0x250000,115,115};
    memcpy(&model_metadata.component,&observed[0].component,64);put(mesh_memory,0x218,model_metadata.asset);put(mesh_memory,0x29c,model_metadata.master);
    put(mesh_memory,0x27c,model_metadata.local);put(mesh_memory,0x26c,model_metadata.space);put(mesh_memory,0x28c,model_metadata.desired);
    put(mesh_memory,0xa0,model_metadata.component);put(asset_memory,0xa4,model_metadata.reference);
    const uintptr_t table=reinterpret_cast<uintptr_t>(vtable.data());put(mesh_memory,0,table);vtable[0x260/8]=base+0xfa730;
    cover_pose_phase_live::targets[2]=reinterpret_cast<void*>(base+0xfa730);
    namespace c=continuous_hand_caller_certificate;for(unsigned i=0;i<2;++i){C(caller_fixture[i].size()==c::chunks[i].size);
        memcpy(image.data()+c::chunks[i].rva,caller_fixture[i].data(),caller_fixture[i].size());}
    native::asset=id(asset_address,10);
}
void start(){setup(false,true);input::enabled=false;native::actor.mesh.address=fixture_mesh_at;sync_neutral();memory();
    LARGE_INTEGER f{};C(QueryPerformanceFrequency(&f)!=0);qpc_frequency=f.QuadPart;rig::enabled=true;rig::clock_ns=rig_clock;rig::reference_changed(frozen_reference);
    shared::continuity_enabled=true;selected();C(publish());live::requested=live::enabled=true;live::caller_certified=live::certify_caller();live::length_mode=gears_hand_two_bone::LengthMode::adaptive;
    C(live::caller_certified&&live::tick_target(fixture_mesh_at));C(!cover_pose_phase_live::enabled&&!cover_control_observer_live::enabled);
}
Entry tick_entry(){Entry e{};e.kind=2;e.rcx=fixture_mesh_at;e.ra=base+0x12e537;return e;}
Entry compose_entry(){Entry e{};e.kind=0;e.rcx=fixture_mesh_at;e.rdx=fixture_mesh_at+0x27c;e.r8=fixture_mesh_at+0x28c;e.ra=base+0xf948c;return e;}
void compute(unsigned side,unsigned kind){Entry e{};e.kind=kind;e.rcx=0x500000+side*0x100+kind*0x10;e.rdx=side?35:11;e.r8=fixture_mesh_at;e.rsp=0x600008;e.r9=e.rsp+0x80;e.ra=base+0x1290e5;
    live::control_begin(e);live::control_end(e,true);}
void four(){compute(0,0);compute(0,1);compute(1,0);compute(1,1);}
void drained(){C(live::pending_count()==0&&constructed==destroyed&&!live::slot.active&&test_world_thread.depth==0);
    gears_world::EpochGate::CloseToken token{};C(test_world_gate.try_close(token));C(test_world_gate.open(token));}
void record_checks(live::TickRecord const& r){C(r.complete&&r.planned==15&&r.seen==15&&r.returned);
    for(unsigned side=0;side<2;++side){auto const& limb=r.plans[side*2];auto const& single=r.plans[side*2+1];C(!memcmp(&limb.wrist,&single.wrist,sizeof(limb.wrist)));
        for(unsigned index:{3u,7u,11u,12u,13u,14u,15u})C(!memcmp(single.original[0].values+index,single.planned[0].values+index,4));}}
__declspec(noinline) void original_exception(Entry* e){__try{live::pose_begin(*e);RaiseException(0xe0421111,0,0,nullptr);}__finally{live::pose_end(*e,!AbnormalTermination());}}
void catch_original(Entry* e){__try{original_exception(e);}__except(EXCEPTION_EXECUTE_HANDLER){}}
int exercise(char const* mode){start();
    if(!strcmp(mode,"configure")){
        wchar_t folder[MAX_PATH]{},path[MAX_PATH]{};C(GetCurrentDirectoryW(MAX_PATH,folder)>0);C(swprintf_s(path,L"%s\\owner-config.ini",folder)>0);
        live::enabled=false;live::requested=false;live::caller_certified=false;
        C(live::preconfigure(path)&&!live::requested&&live::configure(path,nullptr)&&!live::enabled);
        C(WritePrivateProfileStringW(L"ContinuousHandOwner",L"enabled",L"1",path)!=0);C(live::preconfigure(path)&&live::requested);
        image[continuous_hand_caller_certificate::chunks[1].rva]^=1;C(!live::configure(path,nullptr)&&!live::enabled&&!live::caller_certified);
        image[continuous_hand_caller_certificate::chunks[1].rva]^=1;C(live::configure(path,nullptr)&&live::enabled&&live::caller_certified);
        C(!live::configure(path,nullptr));drained();
    }else if(!strcmp(mode,"compose-wrong-end")){
        Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);live::pose_begin(compose);C(compose.selected&&live::slot.compose_address==&compose);
        Entry wrong=compose;live::pose_end(wrong,true);C(live::slot.compose_depth==1&&live::slot.compose_address==&compose&&live::slot.active);
        gears_world::EpochGate::CloseToken token{};C(!test_world_gate.try_close(token));live::pose_end(tick,true);C(live::slot.active&&live::pending_count()!=0);
        Entry nested=compose_entry();live::pose_begin(nested);C(!nested.selected&&live::slot.compose_depth==2);live::pose_end(nested,true);
        C(live::slot.compose_depth==1&&live::slot.compose_address==&compose);
        live::pose_end(compose,true);C(live::slot.compose_depth==0&&!live::slot.compose_address&&live::slot.active);
        live::pose_end(tick,true);drained();C(live::completed==0&&live::record_count==1);
    }else if(!strcmp(mode,"control-wrong-end")){
        Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);live::pose_begin(compose);
        Entry control{};control.kind=0;control.rcx=0x500000;control.rdx=11;control.r8=fixture_mesh_at;control.rsp=0x600008;control.r9=control.rsp+0x80;control.ra=base+0x1290e5;
        live::control_begin(control);C(control.selected&&live::slot.control_address==&control);
        Entry wrong=control;live::control_end(wrong,true);C(live::slot.control_depth==1&&live::slot.control_address==&control&&live::slot.active);
        gears_world::EpochGate::CloseToken token{};C(!test_world_gate.try_close(token));live::pose_end(tick,true);C(live::slot.active&&live::pending_count()!=0);
        Entry nested=control;nested.selected=nested.control_depth=false;live::control_begin(nested);C(!nested.selected&&live::slot.control_depth==2);live::control_end(nested,true);
        C(live::slot.control_depth==1&&live::slot.control_address==&control);
        live::control_end(control,true);C(live::slot.control_depth==0&&!live::slot.control_address&&live::slot.active);
        live::pose_end(compose,true);live::pose_end(tick,true);drained();C(live::completed==0&&live::record_count==1);
    }else if(!strncmp(mode,"typed-age",9)){
        Entry tick=tick_entry();live::pose_begin(tick);C(tick.selected&&!live::slot.sticky);
        rig::TickFailure why=rig::TickFailure::hard;C(live::same_source(&why)&&why==rig::TickFailure::none);
        const auto source=live::slot.data.record.source;fake_clock=source.stereo.received_ns+50000001;
        if(!strcmp(mode,"typed-age-owner"))bad_owner=true;
        if(!strcmp(mode,"typed-age-cohort"))++live::slot.data.record.source.controllers.sequence;
        const bool hard=strcmp(mode,"typed-age")!=0;
        C(!live::same_source(&why)&&why==(hard?rig::TickFailure::hard:rig::TickFailure::source_age_only));
        if(!hard)C(!memcmp(&live::slot.verify,&source,sizeof(source)));
        else C(live::slot.verify.status==api::Status::unavailable&&!live::slot.verify.publication_sequence);
        C(!live::same_source(&why)&&why==rig::TickFailure::hard);
        live::pose_end(tick,true);drained();C(live::completed==0);
    }else if(!strcmp(mode,"address-contract")){
        Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);live::pose_begin(compose);
        C(live::slot.compose.address==&live::slot.compose.scope);C(cover_control_observer_live::current(live::slot.compose));
        const auto old_scope=live::slot.compose.scope;const auto old_invocation=old_scope.invocation;
        auto wrong_scope=old_scope;const auto canonical=live::slot.compose.address;
        live::slot.compose.address=&wrong_scope;C(!cover_control_observer_live::current(live::slot.compose));
        live::slot.compose.address=nullptr;C(!cover_control_observer_live::current(live::slot.compose));live::slot.compose.address=canonical;
        four();printf("address-contract producer_address=%p canonical=%p sticky=%u failure=%u seen=%u planned=%u\n",static_cast<void const*>(live::slot.compose.address),static_cast<void const*>(&live::slot.compose.scope),unsigned(live::slot.sticky),unsigned(live::slot.data.record.failure),live::slot.data.record.seen,live::slot.data.record.planned);
        C(!live::slot.sticky&&live::slot.data.record.seen==15&&live::slot.data.record.planned==15);
        live::pose_end(compose,true);C(!live::slot.compose.address&&!live::slot.compose.admitted&&!cover_control_observer_live::current(live::slot.compose));
        C(!live::control_current(old_scope));live::pose_end(tick,true);drained();C(live::completed==1);
        selected();C(publish());Entry next_tick=tick_entry(),next_compose=compose_entry();live::pose_begin(next_tick);live::pose_begin(next_compose);
        C(live::slot.compose.address==&live::slot.compose.scope&&live::slot.compose.scope.invocation!=old_invocation&&cover_control_observer_live::current(live::slot.compose));
        C(!live::control_current(old_scope));auto stale=live::slot.compose;stale.address=&old_scope;C(!cover_control_observer_live::current(stale));
        live::request_drain();C(!cover_control_observer_live::current(live::slot.compose));live::pose_end(next_compose,true);live::pose_end(next_tick,true);drained();
        C(!live::slot.compose.address&&!live::slot.compose.admitted);
    }else if(!strcmp(mode,"continuous")){
        for(unsigned i=0;i<40;++i){selected();C(publish());Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);
            gears_world::EpochGate::CloseToken token{};C(!test_world_gate.try_close(token));const auto source=live::slot.data.record.source;
            live::pose_begin(compose);four();C(!memcmp(&source,&live::slot.data.record.source,sizeof(source)));live::pose_end(compose,true);live::pose_end(tick,true);drained();}
        C(live::ticks==40&&live::completed==40&&live::record_count==32&&live::record_sequence==40&&live::preparations[0]==40&&live::preparations[1]==40&&rig::core_reads==40);
        for(auto const& r:live::records)record_checks(r);C(cover_control_observer_live::row_count==128&&cover_control_observer_live::selected_compose==999);
        C(GetCurrentDirectoryW(MAX_PATH,directory)>0);live::request_drain();live::print(nullptr,true);C(live::file_written&&live::file_error==0);
    }else if(!strcmp(mode,"exception")){auto e=tick_entry();catch_original(&e);drained();C(live::record_count==1&&!live::records[0].returned&&live::records[0].failure==live::Failure::exception);
    }else if(!strcmp(mode,"caller")){auto e=tick_entry();e.ra++;live::pose_begin(e);C(!e.selected);live::pose_end(e,true);image[0x12e531]^=1;C(!live::certify_caller());
        e=tick_entry();live::pose_begin(e);C(!e.selected||live::slot.sticky);live::pose_end(e,true);image[0x12e531]^=1;vtable[0x260/8]++;e=tick_entry();live::pose_begin(e);C(!e.selected||live::slot.sticky);live::pose_end(e,true);drained();C(live::completed==0);
    }else{Entry tick=tick_entry(),compose=compose_entry();live::pose_begin(tick);C(tick.selected);
        if(!strcmp(mode,"wrong-end")){Entry wrong=tick;live::pose_end(wrong,true);C(live::pending_count()!=0&&constructed!=destroyed);gears_world::EpochGate::CloseToken t{};C(!test_world_gate.try_close(t));}
        else if(!strcmp(mode,"nested")){Entry inner=tick_entry();live::pose_begin(inner);live::pose_end(inner,true);C(live::slot.sticky);}
        else{live::pose_begin(compose);compute(0,0);
            if(!strcmp(mode,"revoke")){rig::reference_changed({2,false,true,false});compute(0,1);C(live::slot.sticky);}
            else if(!strcmp(mode,"new-scene")){const auto old=live::slot.data.record.source.publication_sequence;selected();C(publish());compute(0,1);C(live::slot.sticky&&live::slot.data.record.source.publication_sequence==old);}
            else if(!strcmp(mode,"stop")){live::request_drain();rig::stop();}
            else if(!strcmp(mode,"metadata")){bad_metadata=true;compute(0,1);C(live::slot.sticky);}
            else C(false);live::pose_end(compose,true);}
        live::pose_end(tick,true);drained();C(!live::records[0].complete&&live::completed==0);
        if(!strcmp(mode,"revoke")){native::Binding a{};rig::CoreReadRequest r{};C(!rig::copy_scene_owner(a,r));}
    }
    C(pg.prepares==0&&pg.commits==0);rig::stop();finish();printf("continuous_owner %s: %u checks %u failures; real provider/world/math; host actor/control seams\n",mode,checks,failures);return failures?1:0;
}
}
void abi(){
    printf("{\"Header\":%zu,\"TickRecord\":%zu,\"Plan\":%zu,\"CoreSnapshot\":%zu,\"header\":{",sizeof(live::Header),sizeof(live::TickRecord),sizeof(live::Plan),sizeof(core::CoreSnapshot));
#define H(x) printf("\"" #x "\":%zu,",offsetof(live::Header,x))
    H(version);H(header_size);H(record_size);H(count);H(capacity_value);H(read_only);H(writer);H(length_mode_value);H(ticks);H(completed);H(rejected);
#undef H
    printf("\"preparations\":%zu},\"record\":{",offsetof(live::Header,preparations));
#define R(x) printf("\"" #x "\":%zu,",offsetof(live::TickRecord,x))
    R(tick);R(compose);R(begin_qpc);R(end_qpc);R(thread);R(seen);R(planned);R(failure);R(returned);R(complete);R(asset);R(geometry_signature);R(reference);R(source);
#undef R
    printf("\"plans\":%zu},\"plan\":{",offsetof(live::TickRecord,plans));
#define P(x) printf("\"" #x "\":%zu,",offsetof(live::Plan,x))
    P(node);P(ordinal);P(side);P(kind);P(count);P(prepare);P(solve);P(wrist);P(original);P(planned);
#undef P
    printf("\"component\":%zu},\"source\":{",offsetof(live::Plan,component));
#define S(x) printf("\"" #x "\":%zu,",offsetof(core::CoreSnapshot,x))
    S(publication_sequence);S(scene_invocation);S(world_generation);S(scene_qpc);S(source_ticket);S(native_reference_revision);
#undef S
    printf("\"binding_generation\":%zu}}\n",offsetof(core::CoreSnapshot,binding_generation));
}
int caller_auth(){namespace c=continuous_hand_caller_certificate;
    auto reset=[](){for(unsigned i=0;i<2;++i){auto const& chunk=c::chunks[i];memcpy(image.data()+chunk.rva,caller_fixture[i].data(),chunk.size);full_reads[i]=0;}deny_read=mutate_read=0;};
    reset();C(live::certify_caller());C(full_reads[0]==2&&full_reads[1]==2);
    reset();image[c::chunks[0].rva+5]^=1;C(!live::certify_caller());
    reset();image[c::chunks[0].rva+c::chunks[0].size-1]^=1;C(!live::certify_caller());
    reset();image[c::chunks[1].rva]^=1;C(!live::certify_caller());
    reset();deny_read=1;C(!live::certify_caller());C(full_reads[0]==1);
    reset();deny_read=2;C(!live::certify_caller());C(full_reads[0]==2);
    reset();mutate_read=1;C(!live::certify_caller());C(full_reads[0]==2);
    reset();C(live::certify_caller());image[0x12e531]^=1;C(!live::code_bytes(0x12e527,c::callsite));
    printf("caller_auth_sha256: %u checks %u failures; real BCrypt; private fixture; no Game\n",checks,failures);return failures?1:0;
}
int main(int argc,char** argv){
    if(argc==3&&!strcmp(argv[1],"caller-auth"))return load_caller(argv[2])?caller_auth():2;
    if(argc!=5||!fixtures(argv[2],argv[3])||!load_caller(argv[4]))return 2;
    if(!strcmp(argv[1],"abi")){abi();return 0;}return exercise(argv[1]);}
