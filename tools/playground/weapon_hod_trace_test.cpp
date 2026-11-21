// Runs production HOD trace callbacks with owned memory and synthetic schema/
// source witnesses. Real Invocation and FFrame ancestry helpers are retained.
// This is not proof of retail metadata, GT cost, engine calls or pixels.
#include "../vr-common/weapon_call_boundary.h"
#include "../native-capture/weapon_call_trace.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <map>
#include <string>
using uptr=std::uintptr_t;namespace hand=gears_weapon_hand;
constexpr uptr base=0x100000,query_fn=0x80,fx_fn=0x90,owner_address=0x200,package_address=0x210;
constexpr uptr function_type=0x220,class_type=0x230,weapon=0x4000,pawn=0x4010;
constexpr uptr aim_fn=0x7000,origin_fn=0x7010,pushed_fn=0x7020,result_address=0xa000;
constexpr uptr parent_frame=0xb000,root_frame=0xb100,wrapper_frame=0xb200;
std::map<uptr,unsigned char> memory;
bool source_current=true,armed=true,metadata_current=true,signature_current=true,gt=true;
bool fail_rollback{},interfere{};unsigned cas_calls{},fail_cas{},invalidate_after{};
std::uint64_t source_epoch=9,script_hash=100;
std::int64_t now=1000000000;
unsigned long GetCurrentThreadId() noexcept {return 7;}
template<class T>void put(uptr at,T const& value){
    const auto* bytes=reinterpret_cast<unsigned char const*>(&value);
    for(std::size_t i=0;i<sizeof value;++i)memory[at+i]=bytes[i];
}
bool copy(void* out,uptr at,std::size_t size) noexcept {
    for(std::size_t i=0;i<size;++i){auto found=memory.find(at+i);if(found==memory.end())return false;
        static_cast<unsigned char*>(out)[i]=found->second;}
    return true;
}
hand::ObjectIdentity object(uptr at){return {at,0,16,unsigned(at/16),unsigned(at/16),0,1};}
namespace gripdamage_metadata {
enum class Stage {function_path,final_check};
struct RawName {unsigned index{},number{};};
struct Identity {uptr address{},outer{},klass{};unsigned index{};RawName name{};};
struct Table {uptr data{};int count{},capacity{};};
struct Snapshot {uptr module_base{};Table objects{},names{};std::array<Identity,64> identities{};};
bool tables(uptr,Table&,Table&){return metadata_current;}
struct Reader {
    Snapshot& result;Stage stage;
    bool path(uptr address,unsigned& slot){
        if(!address||address%16||address/16>=result.identities.size())return false;
        slot=unsigned(address/16);auto& id=result.identities[slot];id={address,0,0,slot,{slot,0}};
        if(address==query_fn||address==fx_fn){id.outer=owner_address;id.klass=function_type;}
        if(address==owner_address){id.outer=package_address;id.klass=class_type;}
        return true;
    }
    template<std::size_t N>bool named(unsigned slot,wchar_t const(&name)[N]) const {
        const auto at=result.identities[slot].address;
        const wchar_t* actual=at==query_fn?L"GetTraceImpact":at==fx_fn?L"UpdateFX":
            at==owner_address?L"WEAP_HammerOfDawn":at==package_address?L"warfaregamecontentweapons":
            at==function_type?L"Function":at==class_type?L"Class":L"unknown";
        return std::wcscmp(actual,name)==0;
    }
    template<class T>bool at(uptr address,unsigned offset,T& out){
        const auto assign=[&](auto value){if constexpr(sizeof(T)==sizeof(value)){
            std::memcpy(&out,&value,sizeof out);return true;}else return false;};
        if(address!=query_fn&&address!=fx_fn)return false;
        if(offset==0x8c)return assign(Table{0x10000+address*16,64,64});
        if(offset==0xf4)return assign(unsigned(0));
        if(offset==0xf8)return assign(std::uint16_t(0));
        if(offset==0x114)return assign(uptr(base+0x1d6ca0));
        return false;
    }
};
}
namespace weaponhand {
bool current_gt(void*){return gt;}
std::int64_t clock_ns(void*){return now;}
bool object(gripdamage_metadata::Reader&,hand::ObjectIdentity const& id){return metadata_current&&id.address==weapon;}
template<std::size_t N,std::size_t M>bool derives(gripdamage_metadata::Reader&,uptr at,wchar_t const(&)[N],wchar_t const(&)[M]){return at==weapon&&metadata_current;}
template<std::size_t N>bool core_type(gripdamage_metadata::Reader& r,unsigned slot,wchar_t const(&name)[N]){return r.named(slot,name);}
struct Client {bool armed(){return ::armed;}}client;
}
namespace weaponshot {
bool hash_script(uptr,unsigned,std::uint64_t& out){out=script_hash;return metadata_current;}
hand::ObjectIdentity identity(gripdamage_metadata::Identity const& id){return {id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};}
bool read(void* out,void const* at,std::size_t size){return copy(out,reinterpret_cast<uptr>(at),size);}
}
namespace playground {
struct Request {uptr function{};};
struct Args {Request r{};bool valid{};Args(uptr at,const wchar_t* name){
    r.function=!std::wcscmp(name,L"GetTraceImpact")?query_fn:!std::wcscmp(name,L"UpdateFX")?fx_fn:0;
    valid=at==weapon&&r.function!=0;}};
bool named(uptr at,const wchar_t* name){return
    (at==aim_fn&&!std::wcscmp(name,L"GetAdjustedAimFor"))||
    (at==origin_fn&&!std::wcscmp(name,L"GetWeaponStartTraceLocation"))||
    (at==pushed_fn&&!std::wcscmp(name,L"GetPushedForwardStartTraceLocation"));}
}
namespace unified_weapon_fire {
struct MuzzleSample {hand::Frame source{};std::array<float,3> origin{};std::array<int,3> aim{};
    std::uint64_t epoch{},hold_generation{};std::int64_t sampled_ns{};bool valid{};};
enum class ResultKind:unsigned {none,base_aim,trace_origin,pushed_origin};
struct ReturnSignature {ResultKind kind{};hand::ObjectIdentity function{},result_property{};std::uint64_t fingerprint{};};
bool live_sample(MuzzleSample const& s,std::int64_t at){return source_current&&s.valid&&s.epoch==source_epoch&&at>=s.sampled_ns&&at-s.sampled_ns<=100000000;}
bool retained_metadata(gripdamage_metadata::Reader&){return metadata_current;}
struct Fingerprint {std::uint64_t value=1469598103934665603ull;void word(std::uint64_t v){value^=v;value*=1099511628211ull;}};
void schema_fingerprint(gripdamage_metadata::Snapshot const&,Fingerprint&){}
bool preflight_returns(MuzzleSample const&){return signature_current;}
bool typed_result(gears_weapon_call::Invocation const& call,MuzzleSample const&,ReturnSignature& out){
    if(!signature_current||call.result!=result_address)return false;
    const auto kind=call.callee==aim_fn?ResultKind::base_aim:call.callee==origin_fn?ResultKind::trace_origin:
        call.callee==pushed_fn?ResultKind::pushed_origin:ResultKind::none;
    if(kind==ResultKind::none)return false;out={kind,object(call.callee),object(0x7100),44};return true;
}
bool same_signature(ReturnSignature const& a,ReturnSignature const& b){return a.kind==b.kind&&hand::same(a.function,b.function)&&a.fingerprint==b.fingerprint;}
bool compare_word(uptr at,unsigned expected,unsigned desired,bool& exchanged){
    exchanged=false;++cas_calls;
    if(cas_calls==fail_cas){if(interfere)put(result_address,unsigned(777));return false;}
    if(fail_rollback&&cas_calls>fail_cas)return false;
    unsigned before{};if(!copy(&before,at,4))return false;
    if(before==expected){put(at,desired);exchanged=true;}
    if(exchanged&&invalidate_after&&cas_calls==invalidate_after)source_current=false;
    return true;
}
}
#include "weapon_hod_trace.inl"
namespace trace=hod_trace;namespace abi=gears_weapon_call;
void frame(uptr at,uptr node,uptr self,uptr code,uptr previous){
    put(at+0x18,node);put(at+0x20,self);put(at+0x28,code);put(at+0x38,previous);
}
abi::Invocation invocation(std::uint64_t id,uptr fn,uptr self,uptr caller,uptr result=0){
    abi::Invocation c{};c.hand_cookie=1;c.native_started_qpc=2;c.call_id=id;c.thread_id=7;
    c.self=self;c.caller_frame=caller;c.result=result;c.callee=fn;return c;
}
struct Fixture {
    unified_weapon_fire::MuzzleSample sample{};abi::Invocation root{},child{};
    std::array<unsigned,3> before{{11,22,33}};
    explicit Fixture(bool fx=false){
        assert(trace::active_scopes==0);trace::prepared={};trace::roots={};trace::scopes={};trace::pending={};trace::depth=0;trace::faulted=false;
        trace::origin_writes=trace::aim_writes=trace::query_returns=trace::fx_returns=0;
        trace::query_ray_commits=trace::fx_ray_commits=0;trace::use_observation={};
        source_current=armed=metadata_current=signature_current=gt=true;source_epoch=9;script_hash=100;
        cas_calls=fail_cas=invalidate_after=0;fail_rollback=interfere=false;memory.clear();
        sample.source.weapon=object(weapon);sample.origin={2.f,3.f,4.f};sample.aim={100,200,300};
        sample.valid=true;sample.epoch=9;sample.hold_generation=12;sample.sampled_ns=now;
        for(unsigned i=0;i<3;++i)put(result_address+4*i,before[i]);
        root=invocation(1,fx?fx_fn:query_fn,weapon,parent_frame);
        frame(root_frame,root.callee,weapon,0x10000+root.callee*16+4,parent_frame);
        frame(wrapper_frame,0x5000,weapon,0x5100,root_frame);
        child=invocation(2,origin_fn,pawn,wrapper_frame,result_address);
        assert(trace::prepare(sample));
    }
    void begin(){trace::begin(root);assert(trace::depth==1&&trace::active_scopes==1);}
    std::array<unsigned,3> result(){std::array<unsigned,3> out{};assert(copy(out.data(),result_address,12));return out;}
    void close(){assert(trace::cleanup(child));assert(trace::returned(root));assert(trace::cleanup(root));assert(trace::active_scopes==0);}
};
int main(int argc,char**){
    if(argc>1){
        // The query returned without replacing either result. Only UpdateFX
        // consumed the ray. Aggregate totals must not certify the query.
        Fixture f;f.begin();assert(trace::returned(f.root));assert(trace::cleanup(f.root));
        f.root.call_id=3;f.root.callee=fx_fn;
        frame(root_frame,fx_fn,weapon,0x10000+fx_fn*16+4,parent_frame);
        f.begin();f.child.call_id=4;trace::begin(f.child);assert(trace::returned(f.child)&&trace::cleanup(f.child));
        f.child.call_id=5;f.child.callee=aim_fn;trace::begin(f.child);assert(trace::returned(f.child)&&trace::cleanup(f.child));
        assert(trace::returned(f.root)&&trace::cleanup(f.root));
        assert(trace::query_ray_commits==0&&trace::fx_ray_commits==1);
        std::printf("{\"origin_writes\":\"%llu\",\"aim_writes\":\"%llu\",\"query_returns\":\"%llu\",\"fx_returns\":\"%llu\",\"query_ray_commits\":\"%llu\",\"fx_ray_commits\":\"%llu\"}\n",
            trace::origin_writes,trace::aim_writes,trace::query_returns,trace::fx_returns,trace::query_ray_commits,trace::fx_ray_commits);
        return 0;
    }
    // Actual body ancestry is necessary; an argument-expression call must not write.
    {Fixture f;trace::begin(f.child);assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);}
    for(bool fx:{false,true}){
        Fixture f(fx);trace::begin_use_observation(f.sample,71);trace::UseCounters observed{};
        assert(trace::use_counters(71,observed)&&!observed.query_returns&&!observed.fx_returns);
        f.begin();trace::begin(f.child);assert(trace::returned(f.child));
        std::array<unsigned,3> expected{};std::memcpy(expected.data(),f.sample.origin.data(),12);
        assert(f.result()==expected&&trace::origin_writes==1);assert(trace::cleanup(f.child));
        f.child.call_id=3;f.child.callee=aim_fn;for(unsigned i=0;i<3;++i)put(result_address+4*i,f.before[i]);
        trace::begin(f.child);assert(trace::returned(f.child));std::memcpy(expected.data(),f.sample.aim.data(),12);
        assert(f.result()==expected&&trace::aim_writes==1);f.close();
        assert(fx?trace::fx_returns==1:trace::query_returns==1);
        assert(fx?(trace::fx_ray_commits==1&&trace::query_ray_commits==0):(trace::query_ray_commits==1&&trace::fx_ray_commits==0));
        assert(trace::use_counters(71,observed));
        assert(fx?(observed.fx_returns==1&&observed.fx_origin_bodies==1&&observed.fx_aim_bodies==1&&observed.fx_paired_bodies==1&&!observed.query_ray_commits):
            (observed.query_returns==1&&observed.query_ray_commits==1&&!observed.fx_returns));
        trace::end_use_observation(70);assert(trace::use_counters(71,observed));
        trace::begin_use_observation(f.sample,72);assert(!trace::use_counters(72,observed));
        trace::end_use_observation(71);assert(!trace::use_counters(71,observed));
    }
    // FX may consume an origin without an aim pair; do not manufacture query
    // or paired-FX evidence. The phase observation does not change writes.
    {Fixture f(true);trace::begin_use_observation(f.sample,71);f.begin();
        trace::begin(f.child);assert(trace::returned(f.child));f.close();trace::UseCounters observed{};
        assert(trace::use_counters(71,observed)&&observed.fx_returns==1&&observed.fx_origin_bodies==1&&
            !observed.fx_aim_bodies&&!observed.fx_paired_bodies&&!observed.query_ray_commits);}
    // Same logical use may span fresh frames/timestamps, but never another
    // owner, hold, world, Native lifecycle, registration, rig/input epoch.
    {Fixture f;trace::begin_use_observation(f.sample,71);auto next=f.sample;
        ++next.source.key.frame;++next.sampled_ns;assert(trace::observed_use(next));
        for(unsigned field=0;field<21;++field){next=f.sample;
            switch(field){
            case 0:++next.hold_generation;break;case 1:++next.epoch;break;
            case 2:++next.source.key.game_start;break;case 3:++next.source.key.game_pid;break;
            case 4:++next.source.key.native_started_qpc;break;case 5:++next.source.key.registration_cookie;break;
            case 6:++next.source.key.lifecycle;break;case 7:++next.source.key.world_generation;break;
            case 8:++next.source.key.thread_id;break;case 9:++next.source.world.registry_index;break;
            case 10:++next.source.controller.address;break;case 11:++next.source.pawn.address;break;
            case 12:++next.source.weapon.registry_index;break;case 13:++next.source.mesh.address;break;
            case 14:++next.source.reference_generation;break;case 15:++next.source.controllers.session;break;
            case 16:++next.source.controllers.space_epoch;break;case 17:++next.source.controllers.input_epoch;break;
            case 18:++next.source.stereo.session;break;case 19:++next.source.stereo.space_epoch;break;
            default:++next.source.weapon.klass;break;}
            assert(!trace::observed_use(next));
        }
        ++trace::prepared.source.key.world_generation;f.begin();trace::begin(f.child);
        assert(trace::returned(f.child));f.close();trace::UseCounters observed{};
        assert(trace::origin_writes==1&&trace::use_counters(71,observed)&&!observed.query_returns);}
    // Separate query bodies cannot combine one origin with another aim.
    {Fixture f;f.begin();trace::begin(f.child);assert(trace::returned(f.child)&&trace::cleanup(f.child));
        assert(trace::returned(f.root)&&trace::cleanup(f.root));assert(trace::query_ray_commits==0);
        f.root.call_id=3;f.begin();f.child.call_id=4;f.child.callee=aim_fn;
        trace::begin(f.child);assert(trace::returned(f.child));f.close();
        assert(trace::origin_writes==1&&trace::aim_writes==1&&trace::query_returns==2&&trace::query_ray_commits==0);}
    // A later declined occurrence invalidates only this body's evidence.
    {Fixture f;f.begin();trace::begin(f.child);assert(trace::returned(f.child)&&trace::cleanup(f.child));
        f.child.call_id=3;f.child.callee=aim_fn;trace::begin(f.child);assert(trace::returned(f.child)&&trace::cleanup(f.child));
        f.child.call_id=4;signature_current=false;trace::begin(f.child);assert(trace::returned(f.child));signature_current=true;
        f.close();assert(trace::query_ray_commits==0);}
    {Fixture f;f.begin();f.child.caller_frame=parent_frame;trace::begin(f.child);assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();frame(root_frame,query_fn,weapon+16,0x10804,parent_frame);trace::begin(f.child);assert(trace::returned(f.child)&&cas_calls==0);f.close();}
    {Fixture f;f.begin();f.child.callee=pushed_fn;trace::begin(f.child);assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);++source_epoch;assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();++f.child.thread_id;trace::begin(f.child);assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();++f.child.hand_cookie;trace::begin(f.child);assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);++script_hash;assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);signature_current=false;assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==0);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);fail_cas=2;assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==3&&!trace::faulted);f.close();}
    for(unsigned boundary:{1u,3u}){Fixture f;f.begin();trace::begin(f.child);invalidate_after=boundary;
        assert(trace::returned(f.child)&&f.result()==f.before&&cas_calls==2*boundary&&!trace::faulted);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);auto other=f.child;other.self+=16;
        assert(!trace::returned(other)&&!trace::cleanup(other)&&f.result()==f.before);f.close();}
    {Fixture f;f.begin();trace::begin(f.child);fail_cas=2;interfere=true;
        assert(!trace::returned(f.child)&&trace::faulted&&!trace::cleanup(f.child));
        assert(f.result()[0]==777&&trace::origin_writes==0);
        assert(trace::cleanup(f.root)&&trace::active_scopes==0);}
    std::puts("HOD production trace: query/FX paired ray, body/identity/source/schema rejection, pushed-start preservation and transactional rollback passed (owned-memory only)");
}
