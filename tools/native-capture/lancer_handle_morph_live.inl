#include "../vr-common/weapon_lancer_handle_morph.h"
#include "lancer_handle_morph_certificate.h"
#include "lancer_handle_factory_certificate.h"
#include "../vr-common/weapon_lancer_instance_factory.h"
// Existing GPU-skin Update/Upload hooks are the only render entry points.
// Original instance ActiveMorphs and shared SkeletalMesh are never modified.
namespace lancer_handle_morph_live {
namespace api=gears_lancer_handle_morph;namespace hand=gears_weapon_hand;
namespace binding=weapon_hand_scene_binding;namespace phase=cover_pose_phase_live;
std::atomic<unsigned> retained{},calls{};std::atomic<bool> stop_requested{};
std::atomic<std::uintptr_t> target0{},target1{};
std::atomic<std::uintptr_t> routed_gpu{};
std::atomic<bool> factory_reserved{};
std::atomic<std::uintptr_t> primary_instance_hook{},secondary_instance_hook{};
std::atomic<bool> actual_payload_live{};
std::atomic<bool> neutral_upload_ready{};
std::atomic<std::uint64_t> actual_version{};
std::atomic<double> actual_distance{};
std::atomic<std::int64_t> actual_at{};
std::atomic<std::uintptr_t> actual_gpu{},actual_data{};
std::atomic<std::uint64_t> regular_factory_routes{},regular_secondary_routes{};
std::atomic<std::uint64_t> submitted{},rendered{};
bool pending() noexcept {return retained.load(std::memory_order_acquire)||calls.load();}
void request_drain() noexcept {stop_requested.store(true,std::memory_order_release);}
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
struct Bound {
    api::Request request{};binding::Proof proof{};binding::Property pawn_weapon{},weapon_mesh{},instigator{};
    binding::Property asset_materials{},component_materials{},material_parent{},main_morph{},blade_morph{};
    std::array<gears_anchor::Identity,3> materials{};unsigned main_morph_mask{},blade_morph_mask{};
    std::array<api::Lod,2> lod{};std::uint64_t handle{};DWORD thread{};
    std::uintptr_t gpu{};bool revoking{},last_had_morph{},fence_started{};
    std::uint64_t matched_updates{},angle_rejections{},admission_rejections{};
    alignas(4) volatile LONG fence{};double last_distance_raw{},wanted_distance_raw{};
    hand::Frame render_source{};
};
Bound bound{};api::Receipt retired{};std::uint64_t next_handle{1},retired_cookie{};DWORD retired_thread{};
// Stable storage: never copied with Bound or freed while a GPU can reference
// it. A failed/ambiguous pointer write remains an owned cleanup obligation.
gears_lancer_instance_factory::Lease instance_factory;
struct Call {Call(){++calls;}~Call(){--calls;}};
template<class T>bool read(std::uintptr_t at,T& value) noexcept {
    return at&&at<=UINTPTR_MAX-sizeof(T)&&safe_copy(&value,reinterpret_cast<void const*>(at),sizeof(T));
}
bool compare_table(std::uintptr_t at,std::uintptr_t before,std::uintptr_t after,bool& changed) noexcept {
    changed=false;if(!at||(at&7))return false;
    __try{changed=reinterpret_cast<std::uintptr_t>(InterlockedCompareExchangePointer(
        reinterpret_cast<void* volatile*>(at),reinterpret_cast<void*>(after),reinterpret_cast<void*>(before)))==before;return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool install_instance_factory(std::uintptr_t gpu) noexcept {
    const auto reader=[](auto at,auto& value){return read(at,value);};
    return instance_factory.install(base,gpu,primary_instance_hook.load(),secondary_instance_hook.load(),reader,compare_table);
}
bool object(hand::ObjectIdentity const& id) {return weapon_part_visibility_live::object(id);}
bool certificate() {
    for(const auto& c:gears_lancer_handle_morph_certificate::chunks){
        std::array<unsigned char,4096> bytes{};std::array<unsigned char,32> hash{};
        if(c.size>bytes.size()||!safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva),c.size)||
           !first_person_live::authored_digest(bytes.data(),c.size,hash)||hash!=c.sha256)return false;
    }return true;
}
bool same_array(api::Array a,api::Array b) noexcept {return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
bool array(api::Array a,unsigned count,unsigned bytes) noexcept {
    return a.count==int(count)&&a.capacity>=a.count&&a.capacity<=65536&&a.data&&!(a.data&3)&&
        a.data<=UINTPTR_MAX-std::size_t(a.capacity)*bytes;
}
bool target(hand::ObjectIdentity const& id,api::Lod& out,unsigned which,bool hash_data) {
    static constexpr const char* hashes[]={gears_weapon::lancer_front::delta_sha256,gears_weapon::lancer_front::delta_sha256};
    api::Array lods{},again{};std::uint64_t flags{};
    if(!object(id)||!weapon_part_visibility_live::derives(id,L"MorphTarget",L"Engine")||
       !read(id.address+12,flags)||!(flags&0x4000)||!read(id.address+96,lods)||!array(lods,1,sizeof(api::Lod))||
       !read(lods.data,out)||out.base_vertices!=12721||!array(out.vertices,api::vertices,sizeof(api::Delta)))return false;
    if(hash_data){
        std::array<api::Delta,api::vertices> deltas{};std::array<unsigned char,32> digest{};
        if(!safe_copy(deltas.data(),reinterpret_cast<void const*>(out.vertices.data),sizeof deltas)||
           !first_person_live::authored_digest(deltas.data(),sizeof deltas,digest))return false;
        const auto nib=[](char c){return c<='9'?c-'0':c-'a'+10;};
        for(unsigned i=0;i<digest.size();++i)if(digest[i]!=((nib(hashes[which][i*2])<<4)|nib(hashes[which][i*2+1])))return false;
        for(unsigned i=0;i<deltas.size();++i)if(deltas[i].vertex!=api::first_vertex+i||deltas[i].padding)return false;
    }
    api::Lod closing{};
    return read(id.address+96,again)&&same_array(lods,again)&&read(lods.data,closing)&&
        same_array(out.vertices,closing.vertices)&&out.base_vertices==closing.base_vertices&&object(id);
}
bool original_material_instance(Bound const& b,std::uintptr_t selected,unsigned slot) {
    if(!selected)return true; // Original MeshComponent null override semantics.
    std::array<gears_anchor::Identity,4> chain{};std::array<std::uintptr_t,4> parents{};
    unsigned count{};bool reached{};
    for(auto at=selected;at&&count<chain.size();at=parents[count-1]){
        if(at==b.materials[slot].address){reached=true;break;}
        for(unsigned i=0;i<count;++i)if(chain[i].address==at)return false;
        if(!binding::identity(at,chain[count],true)||chain[count].cls!=b.materials[0].cls||
           !read(at+b.material_parent.offset,parents[count])||!parents[count])return false;
        ++count;
    }
    if(!reached)return false;
    // Stock weapon tint/reload effects wrap both slots in dynamic MICs. Only
    // a freshly registered chain back to this exact original slot is accepted;
    // an unrelated MaterialInterface or a shared base alone is insufficient.
    for(unsigned i=0;i<count;++i){gears_anchor::Identity closing{};std::uintptr_t parent{};
        if(!binding::identity(chain[i].address,closing,true)||!binding::same(chain[i],closing)||
           !read(chain[i].address+b.material_parent.offset,parent)||parent!=parents[i])return false;}
    return true;
}
bool material_pair(Bound const& b) {
    api::Array asset{},overrides{},closing{};std::array<std::uintptr_t,2> values{},actual{};std::uintptr_t parent{};
    unsigned main_flags{},blade_flags{};
    if(!read(b.request.asset.address+b.asset_materials.offset,asset)||!array(asset,2,8)||
       !safe_copy(values.data(),reinterpret_cast<void const*>(asset.data),sizeof values)||
       !read(b.request.component.address+b.component_materials.offset,overrides)||
       overrides.count<0||overrides.count>2||overrides.capacity<overrides.count||overrides.capacity>32||
       (overrides.count&&(!overrides.data||!safe_copy(actual.data(),reinterpret_cast<void const*>(overrides.data),std::size_t(overrides.count)*8))))return false;
    for(unsigned i=0;i<2;++i)if(values[i]!=b.materials[i].address||
        (i<unsigned(overrides.count)&&!original_material_instance(b,actual[i],i)))return false;
    if(!read(b.materials[0].address+b.material_parent.offset,parent)||parent!=b.materials[2].address||
       !read(parent+b.main_morph.offset,main_flags)||!(main_flags&b.main_morph_mask)||
       !read(b.materials[1].address+b.blade_morph.offset,blade_flags)||(blade_flags&b.blade_morph_mask))return false;
    for(auto const& id:b.materials){gears_anchor::Identity again{};
        if(!binding::identity(id.address,again,true)||!binding::same(id,again))return false;}
    std::array<std::uintptr_t,2> repeated{};
    if(!read(b.request.asset.address+b.asset_materials.offset,closing)||!same_array(asset,closing)||
       !safe_copy(repeated.data(),reinterpret_cast<void const*>(asset.data),sizeof repeated)||repeated!=values||
       !read(b.request.component.address+b.component_materials.offset,closing)||!same_array(overrides,closing))return false;
    if(overrides.count){repeated={};if(!safe_copy(repeated.data(),reinterpret_cast<void const*>(overrides.data),std::size_t(overrides.count)*8)||repeated!=actual)return false;}
    return true;
}
bool bind_materials(Bound& b,unsigned& gate) {
    gate=40;if(!binding::property(b.proof,b.request.asset.klass,L"Materials",L"ArrayProperty",16,nullptr,b.asset_materials))return false;
    gate=41;if(!binding::property(b.proof,b.request.component.klass,L"Materials",L"ArrayProperty",16,nullptr,b.component_materials))return false;
    api::Array values{};std::array<std::uintptr_t,2> addresses{};
    gate=42;
    if(!read(b.request.asset.address+b.asset_materials.offset,values)||!array(values,2,8)||
       !safe_copy(addresses.data(),reinterpret_cast<void const*>(values.data),sizeof addresses))return false;
    gate=43;for(unsigned i=0;i<2;++i)if(!binding::certified(b.proof,addresses[i],b.materials[i]))return false;
    const auto named_path=[&](gears_anchor::Identity const& id,const wchar_t* name,const wchar_t* cls,const wchar_t* outer){
        gears_anchor::Identity type{},package{};
        return binding::named(id,name)&&binding::certified(b.proof,id.cls,type)&&binding::named(type,cls)&&
            binding::certified(b.proof,id.outer,package)&&binding::named(package,outer);};
    gate=44;if(!named_path(b.materials[0],L"COG_AssaultRifle_Delta_MAT_MIC",L"MaterialInstanceConstant",L"Materials"))return false;
    gate=45;if(!named_path(b.materials[1],L"BlurringBlades_Delta_MAT",L"Material",L"COG_AssaultRifle"))return false;
    gate=46;if(!binding::property(b.proof,b.materials[0].cls,L"Parent",L"ObjectProperty",8,L"MaterialInterface",b.material_parent))return false;
    gate=47;std::uintptr_t parent{};if(!read(b.materials[0].address+b.material_parent.offset,parent)||
       !binding::certified(b.proof,parent,b.materials[2])||
       !named_path(b.materials[2],L"BaseMaterial",L"Material",L"EnvironmentBaseMaterials"))return false;
    const auto usage=[&](gears_anchor::Identity const& material,binding::Property& property,unsigned& mask){
        return binding::property(b.proof,material.cls,L"bUsedWithMorphTargets",L"BoolProperty",4,nullptr,property)&&
            binding::source_field(b.proof,mask,property.id.address,0xd0)&&mask&&!(mask&(mask-1));};
    gate=48;if(!usage(b.materials[2],b.main_morph,b.main_morph_mask))return false;
    gate=49;if(!usage(b.materials[1],b.blade_morph,b.blade_morph_mask))return false;
    gate=50;return material_pair(b);
}
bool relationships(Bound const& b) {
    const auto& q=b.request;std::uintptr_t weapon{},component{},asset{},pawn{};
    return GetCurrentThreadId()==b.thread&&q.source.key.world_generation==committed_world_generation.load()&&
        object(q.source.world)&&object(q.source.pawn)&&object(q.source.weapon)&&object(q.component)&&object(q.asset)&&
        binding::current(b.proof)&&read(q.source.pawn.address+b.pawn_weapon.offset,weapon)&&weapon==q.source.weapon.address&&
        read(weapon+b.instigator.offset,pawn)&&pawn==q.source.pawn.address&&
        read(weapon+b.weapon_mesh.offset,component)&&component==q.component.address&&
        read(component+0x218,asset)&&asset==q.asset.address&&material_pair(b);
}
bool grasp_owner_current(hand::Frame const& f) noexcept try {
    if(!retained.load(std::memory_order_acquire)||bound.revoking||!bound.handle||
       GetCurrentThreadId()!=bound.thread)return false;
    auto const& original=bound.request.source;
    if(f.key.registration_cookie!=original.key.registration_cookie||
       f.key.game_start!=original.key.game_start||f.key.native_started_qpc!=original.key.native_started_qpc||
       f.key.world_generation!=original.key.world_generation||
       !hand::same(f.world,original.world)||!hand::same(f.pawn,original.pawn)||
       !hand::same(f.weapon,original.weapon))return false;
    // A retained asset binding is not an old input or pose basis. Recenter or
    // a new input epoch may keep this same asset; the caller authenticates
    // TODAY's full broker frame, never the bind request's old input clock.
    // Re-read only
    // this retained asset's actor/component relationship; do not repeat the
    // unrelated material/Morph payload walk on every cosmetic hand request.
    std::uintptr_t weapon{},pawn{},mesh{},asset{},closing_mesh{},closing_asset{};
    return object(bound.request.component)&&object(bound.request.asset)&&binding::current(bound.proof)&&
        read(f.pawn.address+bound.pawn_weapon.offset,weapon)&&weapon==f.weapon.address&&
        read(weapon+bound.instigator.offset,pawn)&&pawn==f.pawn.address&&
        read(weapon+bound.weapon_mesh.offset,mesh)&&mesh==bound.request.component.address&&
        read(mesh+0x218,asset)&&asset==bound.request.asset.address&&
        read(weapon+bound.weapon_mesh.offset,closing_mesh)&&closing_mesh==mesh&&
        read(mesh+0x218,closing_asset)&&closing_asset==asset;
}catch(...){return false;}
bool private_targets_current(Bound const& b) {
    for(unsigned i=0;i<2;++i){api::Lod lod{};
        if(!target(b.request.targets[i],lod,i,false)||!same_array(lod.vertices,b.lod[i].vertices))return false;
    }return true;
}
void begin_fence() noexcept {
    if(bound.fence_started||bound.last_had_morph||actual_payload_live.load(std::memory_order_acquire))return;
    // Native FRenderCommandFence is a4-byte atomic counter; its original FIFO
    // command decrements it after all previously queued morph reads finish.
    // The storage stays in this retained owner until that actual decrement.
    bound.fence_started=true;
    using F=void(__fastcall*)(volatile LONG*);
    __try {reinterpret_cast<F>(base+0xca87c)(&bound.fence);}
    __except(EXCEPTION_EXECUTE_HANDLER){bound.fence=1;}
}
bool drain() noexcept {
    if(!retained.load()||GetCurrentThreadId()!=bound.thread)return !retained.load();
    if(stop_requested.load())bound.revoking=true;
    if(!bound.revoking)return false;
    begin_fence();
    if(!bound.fence_started||InterlockedCompareExchange(&bound.fence,0,0)!=0)return false;
    if(actual_payload_live.load(std::memory_order_acquire)||
       !instance_factory.restore([](auto at,auto& value){return read(at,value);},compare_table))return false;
    retired={};retired.result=api::Result::retired;retired.handle=bound.handle;
    retired.submitted=submitted.load();retired.rendered=rendered.load();retired.fenced=true;
    retired.matched_updates=bound.matched_updates;retired.angle_rejections=bound.angle_rejections;
    retired.admission_rejections=bound.admission_rejections;
    retired_cookie=bound.request.source.key.registration_cookie;retired_thread=bound.thread;
    retired.regular_factory_routes=regular_factory_routes.load();retired.regular_secondary_routes=regular_secondary_routes.load();
    routed_gpu=0;target0=target1=0;bound={};actual_payload_live=false;retained.store(0,std::memory_order_release);return true;
}
api::Receipt receipt() noexcept {
    if(!retained.load())return retired;
    api::Receipt r{};r.result=bound.revoking?api::Result::revoking:api::Result::ready;r.handle=bound.handle;
    r.submitted=submitted.load();r.rendered=rendered.load();r.last_distance_raw=bound.last_distance_raw;
    r.neutral_prepared=neutral_upload_ready.load(std::memory_order_acquire);
    for(unsigned attempt=0;attempt<2;++attempt){
        const auto version=actual_version.load(std::memory_order_acquire);if(!version||(version&1))continue;
        const auto distance=actual_distance.load(std::memory_order_relaxed);
        const auto at=actual_at.load(std::memory_order_relaxed);
        const auto gpu=actual_gpu.load(std::memory_order_relaxed),data=actual_data.load(std::memory_order_relaxed);
        if(actual_version.load(std::memory_order_acquire)!=version)continue;
        r.actual_upload_serial=version/2;
        double installed{};
        const auto reader=[](std::uintptr_t p,void* out,std::size_t n){return p&&n&&p<=UINTPTR_MAX-n&&safe_copy(out,reinterpret_cast<void const*>(p),n);};
        // A current read is not a new upload. Keep the REAL upload serial so
        // repeated polls cannot acknowledge another return frame. Refresh
        // only the geometry observation clock after bracketing that exact
        // still-installed, known payload; controller clocks remain untouched.
        if(at>0&&gpu==routed_gpu.load(std::memory_order_acquire)&&
           api::installed_distance(reader,gpu,data,target0.load(),target1.load(),installed)&&
           std::abs(installed-distance)<.00001&&actual_version.load(std::memory_order_acquire)==version){
            r.actual_distance_raw=installed;r.actual_at_ns=rig_tracking_live::clock_ns();r.actual_known=r.actual_at_ns>0;
        }
        break;
    }
    r.regular_factory_routes=regular_factory_routes.load();r.regular_secondary_routes=regular_secondary_routes.load();
    r.fenced=bound.fence_started&&InterlockedCompareExchange(&bound.fence,0,0)==0;return r;
}
bool original_lod(hand::ObjectIdentity const& asset) noexcept {
    api::Array lods{},closing{};std::uintptr_t lod{},again{};unsigned vertices{};
    return read(asset.address+0xd4,lods)&&lods.count>0&&lods.count<=8&&lods.capacity>=lods.count&&
        lods.capacity<=32&&lods.data&&read(lods.data,lod)&&lod&&read(lod+0x88,vertices)&&vertices==12721&&
        read(asset.address+0xd4,closing)&&same_array(lods,closing)&&read(lods.data,again)&&again==lod;
}
api::Result begin(api::Request const* input,api::Receipt* output) noexcept {
    if(!api::retail_material_route_available||!factory_reserved.load()||
       !primary_instance_hook.load()||!secondary_instance_hook.load())return api::Result::unavailable;
    struct Attempt {api::Receipt* output{};unsigned gate{1};bool ready{};
        ~Attempt(){if(output&&!ready){api::Receipt r{};r.binding_gate=gate;safe_copy(output,&r,sizeof r);}}} attempt{output};
    Call call;api::Request q{};auto* broker=weapon_hand_runtime::current();
    if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||retained.load()||
       stop_requested.load()||!broker||!broker->admit_weapon_render(q.source)||!certificate()||
       !next_handle||next_handle==UINT64_MAX)return api::Result::rejected;
    try {attempt.gate=10;Bound b{};b.request=q;b.render_source=q.source;b.thread=GetCurrentThreadId();b.handle=next_handle++;
        std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        gears_anchor::Identity asset{};
        if(!lock.owns_lock()||!binding::metadata_current()||
           !weapon_part_visibility_live::derives(q.source.weapon,L"Weap_AssaultRifle",L"WarfareGame")||
           !binding::identity(q.asset.address,asset,true)||!binding::named(asset,L"COGAssaultRifle")||
           !binding::property(b.proof,q.source.pawn.klass,L"Weapon",L"ObjectProperty",8,nullptr,b.pawn_weapon)||
           !binding::property(b.proof,q.source.weapon.klass,L"Mesh",L"MeshPointer",8,nullptr,b.weapon_mesh)||
           !binding::property(b.proof,q.source.weapon.klass,L"Instigator",L"ObjectProperty",8,nullptr,b.instigator))return api::Result::rejected;
        if(!bind_materials(b,attempt.gate))return api::Result::rejected;
        attempt.gate=60;
        if(q.targets[0].address==q.targets[1].address||!relationships(b)||!original_lod(q.asset))return api::Result::rejected;
        attempt.gate=70;
        for(unsigned i=0;i<2;++i)if(!target(q.targets[i],b.lod[i],i,true))return api::Result::rejected;
        lock.unlock();attempt.gate=80;if(!broker->admit_weapon_render(q.source))return api::Result::rejected;
        if(instance_factory.active())return api::Result::rejected;
        bound=b;submitted=rendered=0;regular_factory_routes=regular_secondary_routes=0;routed_gpu=0;actual_payload_live=false;neutral_upload_ready=false;
        actual_version=0;actual_at=0;actual_distance=0;actual_gpu=actual_data=0;
        target0=q.targets[0].address;target1=q.targets[1].address;
        retained.store(1,std::memory_order_release);auto r=receipt();
        if(!safe_copy(output,&r,sizeof r)){bound.revoking=true;return api::Result::revoking;}
        attempt.ready=true;return r.result;
    }catch(...){return api::Result::rejected;}
}
api::Result update(api::Control const* input,api::Receipt* output) noexcept {
    Call call;api::Control q{};auto* broker=weapon_hand_runtime::current();
    if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||
       !retained.load()||GetCurrentThreadId()!=bound.thread||bound.revoking||
       q.handle!=bound.handle||q.cookie!=bound.request.source.key.registration_cookie||
       !std::isfinite(q.distance_raw)||q.distance_raw<0||q.distance_raw>gears_weapon::lancer_front::travel_raw||
       !hand::same(q.source.world,bound.request.source.world)||!hand::same(q.source.pawn,bound.request.source.pawn)||
       !hand::same(q.source.weapon,bound.request.source.weapon)||!broker||!broker->admit_weapon_render(q.source)||
       !relationships(bound))return api::Result::rejected;
    bound.render_source=q.source;bound.wanted_distance_raw=q.distance_raw;
    const auto r=receipt();return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
}
api::Result revoke(api::Control const* input,api::Receipt* output,bool detach=true) noexcept {
    Call call;api::Control q{};
    if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||!q.handle)return api::Result::rejected;
    if(retained.load()){
        if(GetCurrentThreadId()!=bound.thread||q.handle!=bound.handle||q.cookie!=bound.request.source.key.registration_cookie)return api::Result::rejected;
        if(detach){bound.revoking=true;drain();}
    }else if(q.handle!=retired.handle||q.cookie!=retired_cookie||GetCurrentThreadId()!=retired_thread)return api::Result::rejected;
    auto r=receipt();
    if(retained.load()){r.matched_updates=bound.matched_updates;r.angle_rejections=bound.angle_rejections;r.admission_rejections=bound.admission_rejections;}
    return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
}
struct Input {std::array<api::Active,34> values{};api::Array header{};bool matching{},injected{};};
std::uintptr_t prepare(std::uintptr_t gpu,int lod,std::uintptr_t mesh,std::uintptr_t original,Input& out) noexcept {
    if(!retained.load()||GetCurrentThreadId()!=bound.thread||mesh!=bound.request.component.address)return original;
    out.matching=true;
    ++bound.matched_updates;
    if(stop_requested.load())bound.revoking=true;
    if(bound.revoking||lod!=0)return original;
    try {const auto start=rig_tracking_live::clock_ns();const double a=bound.wanted_distance_raw;
        // Neutral preparation belongs to ONE actual GPU instance, not merely
        // the same weapon UObject. Reload/reattach can recreate that instance.
        // Check it BEFORE the neutral fast path; otherwise the old prepared
        // flag suppresses every update of its replacement and leaves a stale
        // render receipt forever. Withdraw through the existing FIFO owner;
        // never move the old private factory table to another GPU object.
        if(instance_factory.active()){
            std::uintptr_t table{};
            if(instance_factory.gpu()!=gpu||!read(gpu,table)||table!=instance_factory.table()){
                bound.revoking=true;return original;
            }
        }
        // Neutral stock input is a no-op, not a tracked hand action. After
        // the one actual zero-weight preparation, don't walk action/asset
        // metadata again on every ordinary gun frame. Nonzero displacement
        // below still requires its original fresh frame and every guard.
        if(a<.00001&&neutral_upload_ready.load(std::memory_order_acquire))return original;
        auto* broker=weapon_hand_runtime::current();
        if(!broker||!broker->admit_weapon_render(bound.render_source)){++bound.angle_rejections;return original;}
        if(instance_factory.active()&&instance_factory.gpu()!=gpu){bound.revoking=true;return original;}
        bound.gpu=gpu;routed_gpu.store(gpu,std::memory_order_release);bound.last_distance_raw=a;
        // First-use morph resource/shader preparation must happen while the
        // gun is neutral, before a player's grasp. Submit two ZERO-weight
        // targets once through the same certified material/factory route.
        // This changes no vertex, grants no gesture and cannot charge a gun.
        // Once the actual RT upload is seen, neutral frames use stock input.
        const auto owner=phase::owner_copy();
        if(!owner.valid||owner.thread!=bound.thread||!weapon_part_visibility_live::same(bound.request.source.weapon,owner.binding.weapon)||
           !binding::current_actor(owner.binding)){++bound.admission_rejections;return original;}
        std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);api::Array input{};
        if(!lock.owns_lock()||!relationships(bound)||!private_targets_current(bound)||!read(original,input)||
           input.count!=0||input.capacity<input.count||input.capacity>65536||
           (input.count&&(!input.data||!safe_copy(out.values.data(),reinterpret_cast<void const*>(input.data),std::size_t(input.count)*12))))return original;
        std::array<float,2> weight{};if(!api::weights(a,weight))return original;
        for(unsigned i=0;i<2;++i)out.values[unsigned(input.count)+i]={bound.request.targets[i].address,weight[i]};
        const auto now=rig_tracking_live::clock_ns();if(start<=0||now<start||now-start>100000000||!relationships(bound))return original;
        // Install BEFORE queuing any Morph payload. Otherwise the compiler's
        // inlined regular-table path can choose a Morph VF for blade chunk1,
        // whose actual material has no Morph depth shader (67404 failure).
        if(instance_factory.active()&&instance_factory.gpu()!=gpu){bound.revoking=true;return original;}
        bound.gpu=gpu;routed_gpu.store(gpu,std::memory_order_release);
        if(!install_instance_factory(gpu)){bound.revoking=true;return original;}
        out.header={reinterpret_cast<std::uintptr_t>(out.values.data()),input.count+2,input.count+2};
        out.injected=true;bound.last_distance_raw=a;++submitted;return reinterpret_cast<std::uintptr_t>(&out.header);
    }catch(...){return original;}
}
void after_update(Input const& input) noexcept {
    if(!input.matching||!retained.load())return;
    bound.last_had_morph=input.injected;
    if(bound.revoking)drain();
}
void after_upload(std::uintptr_t gpu,std::uintptr_t data) noexcept {
    if(!retained.load()||gpu!=routed_gpu.load(std::memory_order_acquire))return;api::Array a{};
    if(!read(data+44,a)||a.count<0||a.count>34||a.capacity<a.count)return;
    std::uintptr_t installed{};
    if(!read(gpu+80,installed)||installed!=data)return;
    bool ours{},known=a.count==0;double distance{};
    std::array<api::Active,2> pair{};
    for(int i=0;i<a.count;++i){api::Active x{};
        if(!read(a.data+std::uintptr_t(i)*12,x))return;
        if(x.target==target0.load()||x.target==target1.load())ours=true;
        if(i<2)pair[unsigned(i)]=x;
    }
    if(a.count==2&&pair[0].target==target0.load()&&pair[1].target==target1.load()&&
       std::isfinite(pair[0].weight)&&std::isfinite(pair[1].weight)&&
       pair[0].weight>=0&&pair[0].weight<=.5f&&pair[1].weight>=0&&pair[1].weight<=.5f&&
       std::abs(pair[0].weight-pair[1].weight)<.00001f){
        distance=(double(pair[0].weight)+double(pair[1].weight))*gears_weapon::lancer_front::travel_raw;known=true;
        neutral_upload_ready.store(true,std::memory_order_release);
    }
    if(!read(gpu+80,installed)||installed!=data)return;
    actual_payload_live.store(ours,std::memory_order_release);
    // Observe the installed original upload. Duplicate polls cannot supply
    // a second closed frame, and requested weights are not an observation.
    actual_version.fetch_add(1,std::memory_order_acq_rel);
    actual_distance.store(distance,std::memory_order_relaxed);
    actual_at.store(known?rig_tracking_live::clock_ns():0,std::memory_order_relaxed);
    actual_gpu.store(gpu,std::memory_order_relaxed);actual_data.store(data,std::memory_order_relaxed);
    actual_version.fetch_add(1,std::memory_order_release);
    if(ours)++rendered;
}
using Factory=std::uintptr_t(__fastcall*)(std::uintptr_t,int,int);
std::uintptr_t factory(Factory original,std::uintptr_t gpu,int lod,int chunk,bool secondary) noexcept {
    Call call;api::RegularLookup local{};
    const auto reader=[](std::uintptr_t at,void* to,std::size_t bytes){return at&&bytes&&at<=UINTPTR_MAX-bytes&&safe_copy(to,reinterpret_cast<void const*>(at),bytes);};
    if(retained.load(std::memory_order_acquire)&&api::regular_blade_lookup(reader,gpu,routed_gpu.load(std::memory_order_acquire),
        lod,chunk,target0.load(),target1.load(),local)){
        if(secondary)++regular_secondary_routes;else ++regular_factory_routes;
        return original(reinterpret_cast<std::uintptr_t>(local.gpu.data()),lod,chunk);
    }
    return original(gpu,lod,chunk);
}
bool factory_certificate() noexcept {
    for(auto const& c:gears_lancer_handle_factory_certificate::chunks){
        std::array<unsigned char,128> bytes{};std::array<unsigned char,32> hash{};
        if(c.size>bytes.size()||!safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva),c.size)||
           !first_person_live::authored_digest(bytes.data(),c.size,hash)||hash!=c.sha256)return false;
    }return true;
}
#else
bool grasp_owner_current(hand::Frame const&) noexcept{return false;}
struct Input {};
std::uintptr_t prepare(std::uintptr_t,int,std::uintptr_t,std::uintptr_t original,Input&) noexcept {return original;}
void after_update(Input const&) noexcept {} void after_upload(std::uintptr_t,std::uintptr_t) noexcept {}
api::Result begin(api::Request const*,api::Receipt*) noexcept {return api::Result::unavailable;}
api::Result update(api::Control const*,api::Receipt*) noexcept {return api::Result::unavailable;}
api::Result revoke(api::Control const*,api::Receipt*,bool=true) noexcept {return api::Result::unavailable;}
#endif
}
