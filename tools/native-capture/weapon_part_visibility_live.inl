#include "../vr-common/weapon_part_visibility.h"
#include "native_atom_overwrite_receipt.h"
#include "fresh_pose_producer_certificate.h"
namespace native_atom_overwrite_live {bool code_current();}
// Only Native writes this scale; reuse its already-certified decoder hook.
// No second hook, animation writer, mesh clone, engine call or tracking source.
namespace weapon_part_visibility_live {
namespace api=gears_weapon_part_visibility;namespace hand=gears_weapon_hand;
namespace binding=weapon_hand_scene_binding;namespace atom=gears_atom_overwrite;
std::atomic<unsigned> calls{},retained{};std::atomic<bool> drain_requested{};
bool pending() noexcept {return calls.load(std::memory_order_acquire)||retained.load(std::memory_order_acquire);}
void request_drain() noexcept {drain_requested.store(true,std::memory_order_release);}
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
struct Bound {
    hand::FrameKey key{};hand::ObjectIdentity world{},pawn{},weapon{},component{},asset{},node{};
    binding::Proof proof{};binding::Property pawn_weapon{},mesh{},animations{},instigator{};
    std::uint64_t handle{},transaction{};DWORD thread{};api::ScaleLease scale;
    api::Source initial_source{};
};
Bound bound;std::uint64_t next_handle{1},last_retired_handle{},last_retired_cookie{};std::atomic<DWORD> owner_thread{};
api::Receipt retired_receipt{};
struct Active {Active(){++calls;}~Active(){--calls;}};
bool code_current() {
    if(!native_atom_overwrite_live::code_current())return false;
    // Cached-atom layout/copy and direct sequence evaluator are certified
    // separately from the decoder. These entries remain unhooked.
    for(auto const& chunk:gears_fresh_pose_certificate::sequence_pose)
        if(!cover_pose_phase_live::digest(chunk))return false;
    return true;
}
template<class T>bool read(std::uintptr_t at,T& out) noexcept {
    return at&&at<=UINTPTR_MAX-sizeof(T)&&safe_copy(&out,reinterpret_cast<void const*>(at),sizeof(T));
}
bool write_scale(std::uintptr_t at,float value) noexcept {
    if(!at||(at&3)||at>UINTPTR_MAX-4)return false;
    __try{std::memcpy(reinterpret_cast<void*>(at),&value,4);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool same(hand::ObjectIdentity const& a,gears_anchor::Identity const& b) noexcept {
    return hand::identity_valid(a)&&a.address==b.address&&a.outer==b.outer&&a.klass==b.cls&&
        a.registry_index==b.index&&a.name_index==b.name_index&&a.name_number==b.number;
}
bool object(hand::ObjectIdentity const& wanted) {
    gears_anchor::Identity current{};return binding::identity(wanted.address,current,true)&&same(wanted,current);
}
bool derives(hand::ObjectIdentity const& object_id,const wchar_t* type,const wchar_t* package) {
    binding::ClassPath path{};
    if(!binding::derives(object_id.klass,type,0,path))return false;
    for(unsigned i=0;i<path.count;++i)if(binding::named(path.ids[i],type)){
        gears_anchor::Identity outer{};
        return binding::identity(path.ids[i].outer,outer)&&!outer.outer&&binding::named(outer,package);
    }
    return false;
}
bool named_bone(std::uintptr_t record,const wchar_t* wanted) {
    struct Name {unsigned index{},number{};};Name name{};gears_anchor::Registry names{},again{};
    if(!read(record,name)||name.number||!read(binding::metadata.names_header,names)||
       !binding::cohort(names,names)||name.index>=unsigned(names.count))return false;
    std::uintptr_t entry{},repeat{};unsigned index{};std::array<wchar_t,32> text{};
    return read(names.data+std::size_t(name.index)*8,entry)&&entry&&read(entry,index)&&index==name.index&&
        read(entry+20,text)&&std::wmemchr(text.data(),0,text.size())&&!std::wcscmp(text.data(),wanted)&&
        read(names.data+std::size_t(name.index)*8,repeat)&&repeat==entry&&
        read(binding::metadata.names_header,again)&&binding::cohort(names,again)&&names.count==again.count;
}
bool properties(Bound& b) {
    const auto p=[&](std::uintptr_t cls,const wchar_t* label,const wchar_t* kind,const wchar_t* reference,binding::Property& out){
        return binding::property(b.proof,cls,label,kind,8,reference,out);};
    binding::Property asset_property{},local{},cache{},space{},desired{};
    const auto layout=[&](std::uintptr_t cls,const wchar_t* label,const wchar_t* kind,unsigned bytes,unsigned offset,binding::Property& out){
        return binding::property(b.proof,cls,label,kind,bytes,nullptr,out)&&out.offset==offset;};
    const bool fields=p(b.weapon.klass,L"Mesh",L"MeshPointer",nullptr,b.mesh)&&
        p(b.component.klass,L"Animations",L"ObjectProperty",L"AnimNode",b.animations)&&
        p(b.pawn.klass,L"Weapon",L"ObjectProperty",nullptr,b.pawn_weapon)&&
        p(b.weapon.klass,L"Instigator",L"ObjectProperty",nullptr,b.instigator)&&
        layout(b.component.klass,L"SkeletalMesh",L"ObjectProperty",8,0x218,asset_property)&&
        layout(b.component.klass,L"LocalAtoms",L"ArrayProperty",16,0x27c,local)&&
        layout(b.component.klass,L"SpaceBases",L"ArrayProperty",16,0x26c,space)&&
        layout(b.component.klass,L"RequiredBones",L"ArrayProperty",16,0x28c,desired)&&
        layout(b.node.klass,L"CachedBoneAtoms",L"ArrayProperty",16,0x80,cache);
    if(!fields)return false;
    // SpaceBases' inner reflected type is Matrix/64, unlike LocalAtoms and
    // CachedBoneAtoms. Pin this through the same immutable metadata proof.
    std::uintptr_t inner{},structure{};unsigned width{},dimension{},extent{};
    gears_anchor::Identity property{},matrix{};
    return binding::source_field(b.proof,inner,space.id.address,0xd0)&&
        binding::certified(b.proof,inner,property)&&property.outer==space.id.address&&
        binding::kind(b.proof,property,L"StructProperty")&&
        binding::source_field(b.proof,width,inner,0x74)&&width==sizeof(api::SpaceMatrix)&&
        binding::source_field(b.proof,dimension,inner,0x70)&&dimension==1&&
        binding::source_field(b.proof,structure,inner,0xd0)&&binding::certified(b.proof,structure,matrix)&&
        binding::named(matrix,L"Matrix")&&binding::source_field(b.proof,extent,structure,0x88)&&extent==sizeof(api::SpaceMatrix);
}
bool snapshot_unlocked(Bound const& b,api::Source& out,std::uintptr_t* selected=nullptr,bool restoration=false) {
    out={};
    if(GetCurrentThreadId()!=b.thread||!binding::metadata_current()||!binding::current(b.proof))return false;
    for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(!object(id))return false;
    const auto mesh=b.component.address,node=b.node.address;
    std::uintptr_t asset{},linked_mesh{},linked_node{},node_mesh{},sequence{},pawn{},table{},method{};
    api::Array reference{},desired{};
    if((!restoration&&(!read(b.weapon.address+b.mesh.offset,linked_mesh)||linked_mesh!=mesh||
       !read(mesh+b.animations.offset,linked_node)||linked_node!=node||
       !read(b.weapon.address+b.instigator.offset,pawn)||pawn!=b.pawn.address))||
       !read(node+0x60,node_mesh)||node_mesh!=mesh||!read(node+0xfc,sequence)||(!restoration&&!sequence)||
       !read(node,table)||!table||!read(table+0x270,method)||method!=base+0xf8af4||
       !read(mesh+0x218,asset)||asset!=b.asset.address||!read(asset+0xa4,reference)||
       reference.count!=5||reference.capacity<5||reference.capacity>512||!reference.data||
       reference.data>UINTPTR_MAX-std::size_t(reference.capacity)*68)return false;
    const wchar_t* names[]={L"b_HG_Root",L"b_HG_Slide",L"b_HG_Wing_L",L"b_HG_Wing_R",L"b_HG_Magazine"};
    for(unsigned i=0;i<5;++i){int parent{};
        if(!named_bone(reference.data+i*68,names[i])||!read(reference.data+i*68+0x38,parent)||parent)return false;}
    if(!read(mesh+0x27c,out.local)||!read(node+0x80,out.cache)||!read(mesh+0x26c,out.space)||
       !read(mesh+0x28c,desired)||!read(mesh+0x338,out.epoch)||!out.epoch||
       !api::valid(out.local)||!api::valid(out.cache,true)||!api::valid_space(out.space)||
       desired.count<1||desired.count>5||desired.capacity<desired.count||desired.capacity>512||!desired.data||
       desired.data>UINTPTR_MAX-std::size_t(desired.capacity))return false;
    bool seen[5]{};for(int i=0;i<desired.count;++i){unsigned char bone{};
        if(!read(desired.data+std::size_t(i),bone)||bone>=5||seen[bone])return false;seen[bone]=true;}
    if(!seen[4])return false;
    const auto alias=[](api::Array a,unsigned aw,api::Array c,unsigned cw){return a.capacity&&c.capacity&&
        a.data<c.data+std::size_t(c.capacity)*cw&&c.data<a.data+std::size_t(a.capacity)*aw;};
    if(alias(out.local,32,out.cache,32)||alias(out.local,32,out.space,64)||alias(out.cache,32,out.space,64))return false;
    if(selected)*selected=sequence;
    out.component=mesh;out.asset=asset;out.node=node;out.identity_generation=b.handle;out.verified=true;
    return binding::current(b.proof)&&binding::metadata_current();
}
bool snapshot(api::Source& out,std::uintptr_t* sequence=nullptr) noexcept {
    try {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        return lock.owns_lock()&&snapshot_unlocked(bound,out,sequence,bound.scale.restoring());
    }catch(...){return false;}
}
bool held_current() noexcept {
    std::uintptr_t held{};
    return recording.load()&&bound.key.world_generation==committed_world_generation.load()&&
        read(bound.pawn.address+bound.pawn_weapon.offset,held)&&held==bound.weapon.address;
}
bool positively_inactive() noexcept {
    const auto world=committed_world_generation.load();
    if(world&&bound.key.world_generation&&world!=bound.key.world_generation)return true;
    std::uintptr_t a{},b{};
    return read(bound.pawn.address+bound.pawn_weapon.offset,a)&&read(bound.pawn.address+bound.pawn_weapon.offset,b)&&
        a==b&&a!=bound.weapon.address;
}
// Positive slot replacement only. A failed read is not an object-destruction
// receipt and may not erase a pending restoration.
bool original_gone() noexcept {
    try {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current())return false;
        gears_anchor::Registry objects{},again{};std::uintptr_t value{},repeat{};
        if(!read(binding::metadata.objects_header,objects)||!binding::cohort(objects,objects)||
           bound.component.registry_index>=unsigned(objects.count))return false;
        const auto at=objects.data+std::size_t(bound.component.registry_index)*8;
        return read(at,value)&&read(at,repeat)&&value==repeat&&value!=bound.component.address&&
            read(binding::metadata.objects_header,again)&&binding::cohort(objects,again)&&objects.count==again.count;
    }catch(...){return false;}
}
void retire(bool inactive=false,bool destroyed=false) noexcept {last_retired_handle=bound.handle;last_retired_cookie=bound.key.registration_cookie;
    retired_receipt={};retired_receipt.handle=bound.handle;retired_receipt.writes=bound.scale.writes();
    retired_receipt.cpu_restored=!destroyed;retired_receipt.original_inactive=inactive;retired_receipt.original_destroyed=destroyed;
    retired_receipt.result=inactive?api::Result::inactive_restored:api::Result::restored;
    bound={};retained.store(0,std::memory_order_release);}
void drain_on_gt() noexcept {
    if(!retained.load(std::memory_order_acquire)||GetCurrentThreadId()!=owner_thread.load())return;
    if(drain_requested.load(std::memory_order_acquire)||!held_current())bound.scale.request_restore();
    api::Source source{};
    if(!snapshot(source)){if(original_gone()){retire(true,true);}return;}
    bound.scale.observe_cache(source,[](auto at,auto& value){return read(at,value);});
    const bool inactive=positively_inactive();
    if(bound.scale.restoring()&&bound.scale.restore(source,[](auto at,auto& value){return read(at,value);},write_scale,inactive))retire(inactive);
}
struct DecoderTicket {std::uint64_t handle{};api::Source source{};atom::Arguments arguments{};atom::Cohort cohort{};bool admitted{};};
void decoder_begin(atom::Arguments const& a,DecoderTicket& ticket) noexcept {
    ticket={};
    if(!retained.load(std::memory_order_acquire)||GetCurrentThreadId()!=owner_thread.load())return;
    if(a.out!=bound.initial_source.local.data+4*32){
        if(drain_requested.load()||bound.scale.restoring()||!held_current())drain_on_gt();
        return;
    }
    drain_on_gt();
    if(!retained.load()||bound.scale.restoring()||!held_current()||a.caller!=base+0xe0a73||a.track!=4)return;
    try {
        std::uintptr_t sequence{};api::Source source{};
        if(!code_current()||!snapshot(source,&sequence)||sequence!=a.sequence||
           !bound.scale.matches(source,a.out,a.track)||!atom::cohort(safe_copy,a,ticket.cohort))return;
        // Five tracks under the actual original AnimSet, not the character or
        // another component which happens to reuse this shared sequence.
        if(ticket.cohort.offsets.count!=20)return;
        ticket.handle=bound.handle;ticket.source=source;ticket.arguments=a;ticket.admitted=true;
    }catch(...){ticket={};}
}
void decoder_return(DecoderTicket const& ticket,bool normal_return) noexcept {
    if(!ticket.admitted||!normal_return||!retained.load()||ticket.handle!=bound.handle||
       GetCurrentThreadId()!=owner_thread.load()||bound.scale.restoring())return;
    try {api::Source after{};std::uintptr_t sequence{};atom::Cohort cohort{};
        if(!held_current()||!code_current()||!snapshot(after,&sequence)||
           sequence!=ticket.arguments.sequence||!api::identity(ticket.source,after)||
           !api::same(ticket.source.local,after.local)||!atom::cohort(safe_copy,ticket.arguments,cohort)||
           !atom::equal(cohort,ticket.cohort))return;
        bound.scale.decoder_return(after,ticket.arguments.out,ticket.arguments.track,
            [](auto at,auto& value){return read(at,value);},write_scale);
    }catch(...){bound.scale.request_restore();}
}
api::Receipt receipt() noexcept {
    api::Receipt value{};value.handle=bound.handle;value.writes=bound.scale.writes();
    if(!retained.load())return retired_receipt;
    api::Source source{};
    if(snapshot(source)){value.pose_epoch=source.epoch;value.original_space_hidden=bound.scale.hidden(source,
            [](auto at,auto& out){return read(at,out);});}
    value.cpu_restored=bound.scale.cpu_restored();
    value.result=bound.scale.restoring()?api::Result::restoring:(value.original_space_hidden?api::Result::hidden:api::Result::pending);
    return value;
}
api::Result begin(api::Request const* input,api::Receipt* output) noexcept {
    Active active_call;api::Request value{};api::Receipt result{};
    try {
        auto broker=weapon_hand_runtime::current();
        if(!input||!output||!safe_copy(&value,input,sizeof(value))||value.size!=sizeof(value)||value.version!=1||
           !value.transaction||!broker||drain_requested.load()||!broker->admit_part_visibility(value.source)||
           !code_current()||retained.load()||!next_handle||next_handle==UINT64_MAX)return api::Result::rejected;
        Bound b{};b.key=value.source.key;b.world=value.source.world;b.pawn=value.source.pawn;b.weapon=value.source.weapon;
        b.component=value.component;b.asset=value.asset;b.node=value.node;b.thread=GetCurrentThreadId();
        b.handle=next_handle++;b.transaction=value.transaction;
        api::Source source{};
        {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
            if(!lock.owns_lock()||!binding::metadata_current())return api::Result::rejected;
            for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(!object(id))return api::Result::rejected;
            gears_anchor::Identity asset{};
            if(!derives(b.weapon,L"Weap_COGPistol",L"WarfareGame")||
               !derives(b.component,L"SkeletalMeshComponent",L"Engine")||
               !derives(b.asset,L"SkeletalMesh",L"Engine")||!derives(b.node,L"AnimNodeSequence",L"Engine")||
               !binding::identity(b.asset.address,asset,true)||!binding::named(asset,L"COG_Pistol")||
               !properties(b)||!snapshot_unlocked(b,source)||!b.scale.begin(source,[](auto at,auto& out){return read(at,out);}))return api::Result::rejected;
        }
        if(!broker->admit_part_visibility(value.source))return api::Result::rejected;
        b.initial_source=source;bound=b;owner_thread.store(b.thread);retained.store(1,std::memory_order_release);
        result=receipt();result.result=api::Result::pending;
        if(!safe_copy(output,&result,sizeof(result))){bound.scale.request_restore();return api::Result::restoring;}
        return result.result;
    }catch(...){if(retained.load())bound.scale.request_restore();return api::Result::rejected;}
}
api::Result update(api::Control const* input,api::Receipt* output,bool restore) noexcept {
    Active active_call;api::Control value{};
    if(!input||!output||!safe_copy(&value,input,sizeof(value))||value.size!=sizeof(value)||value.version!=1||
       !value.handle||!value.cookie||GetCurrentThreadId()!=owner_thread.load())return api::Result::rejected;
    if(retained.load()&&(value.handle!=bound.handle||value.cookie!=bound.key.registration_cookie))return api::Result::rejected;
    // Once retired, a stale caller cannot obtain or control a different mask.
    if(!retained.load()){if(value.handle!=last_retired_handle||value.cookie!=last_retired_cookie)return api::Result::rejected;
        const auto result=retired_receipt;
        return safe_copy(output,&result,sizeof(result))?result.result:api::Result::rejected;}
    if(restore)bound.scale.request_restore();drain_on_gt();auto result=receipt();
    if(!result.handle)result.handle=value.handle;
    return safe_copy(output,&result,sizeof(result))?result.result:api::Result::rejected;
}
#else
struct DecoderTicket {};
void decoder_begin(atom::Arguments const&,DecoderTicket&) noexcept {}
void decoder_return(DecoderTicket const&,bool) noexcept {}
api::Result begin(api::Request const*,api::Receipt*) noexcept {return api::Result::unavailable;}
api::Result update(api::Control const*,api::Receipt*,bool) noexcept {return api::Result::unavailable;}
#endif
} // namespace weapon_part_visibility_live
