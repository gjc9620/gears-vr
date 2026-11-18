// Borrow only original Lancer bone4 inside the existing certified Compose owner.
// No extra hook, input source, shared-asset write or engine dispatch.
namespace lancer_original_handle_live {
namespace api=gears_lancer_original_handle;namespace hand=gears_weapon_hand;
namespace existing=weapon_part_visibility_live;namespace binding=weapon_hand_scene_binding;
namespace atom=gears_atom_overwrite;
std::atomic<unsigned> calls{},retained{};std::atomic<bool> drain_requested{};
bool pending() noexcept {return calls.load(std::memory_order_acquire)||retained.load(std::memory_order_acquire);}
void request_drain() noexcept {drain_requested.store(true,std::memory_order_release);}
#if defined(GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT) && GEARS_NATIVE_ATOM_OVERWRITE_RECEIPT
struct Bound {
    hand::FrameKey key{};hand::ObjectIdentity world{},pawn{},weapon{},component{},asset{},node{};
    binding::Proof proof{};binding::Property pawn_weapon{},mesh{},instigator{};
    std::uint64_t handle{};DWORD thread{};api::ComposeLease pose;api::Source initial{};unsigned restore_gate{};
};
Bound bound;std::atomic<DWORD> owner_thread{};std::uint64_t next_handle{1},retired_cookie{};
// Preserve the FIRST existing restore cause in the existing receipt, not a new stream.
void request_restore(unsigned gate) noexcept {
    if(!bound.restore_gate)bound.restore_gate=gate;bound.pose.request_restore();
}
api::Receipt retired{};unsigned last_gate{};
struct Call {Call(){++calls;}~Call(){--calls;}};
template<class T>bool read(std::uintptr_t at,T& out) noexcept{return existing::read(at,out);}
bool write(std::uintptr_t at,api::Atom const& value) noexcept {
    if(!at||(at&3)||at>UINTPTR_MAX-sizeof value)return false;
    __try{std::memcpy(reinterpret_cast<void*>(at),&value,sizeof value);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool properties(Bound& b) {
    const auto p=[&](std::uintptr_t cls,const wchar_t* label,const wchar_t* kind,const wchar_t* ref,binding::Property& out){
        return binding::property(b.proof,cls,label,kind,8,ref,out);};
    binding::Property asset{},local{},cache{},space{},required{};
    const auto layout=[&](std::uintptr_t cls,const wchar_t* label,const wchar_t* kind,unsigned bytes,unsigned offset,binding::Property& out){
        return binding::property(b.proof,cls,label,kind,bytes,nullptr,out)&&out.offset==offset;};
    if(!p(b.weapon.klass,L"Mesh",L"MeshPointer",nullptr,b.mesh)||
       !p(b.pawn.klass,L"Weapon",L"ObjectProperty",nullptr,b.pawn_weapon)||
       !p(b.weapon.klass,L"Instigator",L"ObjectProperty",nullptr,b.instigator)||
       !layout(b.component.klass,L"SkeletalMesh",L"ObjectProperty",8,0x218,asset)||
       !layout(b.component.klass,L"LocalAtoms",L"ArrayProperty",16,0x27c,local)||
       !layout(b.component.klass,L"SpaceBases",L"ArrayProperty",16,0x26c,space)||
       !layout(b.component.klass,L"RequiredBones",L"ArrayProperty",16,0x28c,required)||
       !layout(b.node.klass,L"CachedBoneAtoms",L"ArrayProperty",16,0x80,cache))return false;
    std::uintptr_t inner{},structure{};unsigned width{},dimension{},extent{};
    gears_anchor::Identity property{},matrix{};
    return binding::source_field(b.proof,inner,space.id.address,0xd0)&&
        binding::certified(b.proof,inner,property)&&property.outer==space.id.address&&
        binding::kind(b.proof,property,L"StructProperty")&&
        binding::source_field(b.proof,width,inner,0x74)&&width==64&&
        binding::source_field(b.proof,dimension,inner,0x70)&&dimension==1&&
        binding::source_field(b.proof,structure,inner,0xd0)&&binding::certified(b.proof,structure,matrix)&&
        binding::named(matrix,L"Matrix")&&binding::source_field(b.proof,extent,structure,0x88)&&extent==64;
}
bool snapshot_unlocked(Bound const& b,api::Source& out,std::uintptr_t* selected=nullptr,bool restoring=false,
    unsigned* failure=nullptr) {
    if(failure)*failure=0;
    // Same existing reads/predicates/short-circuit order. Only the caller's
    // existing bind receipt receives the FIRST failed condition; no sampler.
    const auto bad=[&](unsigned gate,bool failed) noexcept {
        if(failed&&failure&&!*failure)*failure=gate;return failed;
    };
    out={};if(bad(1,GetCurrentThreadId()!=b.thread)||bad(2,!binding::metadata_current())||bad(3,!binding::current(b.proof)))return false;
    unsigned ordinal{};
    for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(bad(100+ordinal++,!existing::object(id)))return false;
    std::uintptr_t linked_mesh{},asset{},node_mesh{},sequence{},table{},method{},pawn{},held{};
    api::Array reference{},desired{},r2{},l2{},c2{},s2{};
    if((!restoring&&(bad(10,!read(b.weapon.address+b.mesh.offset,linked_mesh)||linked_mesh!=b.component.address)||
       bad(11,!read(b.weapon.address+b.instigator.offset,pawn)||pawn!=b.pawn.address)||
       bad(12,!read(b.pawn.address+b.pawn_weapon.offset,held)||held!=b.weapon.address)))||
       bad(20,!read(b.node.address+0x60,node_mesh)||node_mesh!=b.component.address)||
       bad(21,!read(b.node.address+0xfc,sequence))||
       bad(22,!read(b.node.address,table)||!table||!read(table+0x270,method)||method!=base+0xf8af4)||
       bad(23,!read(b.component.address+0x218,asset)||asset!=b.asset.address||!read(asset+0xa4,reference))||
       bad(24,reference.count!=9||reference.capacity<9||reference.capacity>512||!reference.data||
       reference.data>UINTPTR_MAX-std::size_t(reference.capacity)*68))return false;
    const wchar_t* labels[]={L"b_AR_Root",L"b_AR_Chainsaw",L"b_AR_ChainsawEnd",L"b_AR_Chain",L"b_AR_Chainsaw_Handle",L"b_AR_Receiver",L"b_AR_Receiver_Spin",L"b_AR_Latch",L"b_AR_Magazine"};
    constexpr int parents[]={0,0,1,1,0,0,5,0,0};
    for(unsigned i=0;i<9;++i){int parent{};
        if(bad(200+i,!existing::named_bone(reference.data+i*68,labels[i])||!read(reference.data+i*68+0x38,parent)||parent!=parents[i]))return false;}
    if(bad(30,!read(b.component.address+0x27c,out.local))||bad(31,!read(b.node.address+0x80,out.cache))||
       bad(32,!read(b.component.address+0x26c,out.space))||bad(33,!read(b.component.address+0x28c,desired))||
       bad(34,!read(b.component.address+0x338,out.epoch)||!out.epoch)||
       bad(35,desired.count<1||desired.count>9||desired.capacity<desired.count||desired.capacity>512||
       !desired.data||desired.data>UINTPTR_MAX-std::size_t(desired.capacity)))return false;
    bool seen[9]{};for(int i=0;i<desired.count;++i){unsigned char bone{};
        if(bad(40,!read(desired.data+std::size_t(i),bone)||bone>=9||seen[bone]))return false;seen[bone]=true;}
    if(bad(44,!seen[4]))return false;
    out.component=b.component.address;out.asset=asset;out.node=b.node.address;out.identity_generation=b.handle;out.verified=true;
    if(bad(50,!api::valid(out))||bad(51,!read(asset+0xa4,r2))||bad(52,!hand::identity_valid(b.asset))||
       bad(53,!gears_weapon_part_visibility::same(reference,r2))||bad(54,!read(b.component.address+0x27c,l2))||
       bad(55,!read(b.node.address+0x80,c2))||bad(56,!read(b.component.address+0x26c,s2))||
       bad(57,!gears_weapon_part_visibility::same(out.local,l2))||bad(58,!gears_weapon_part_visibility::same(out.cache,c2))||
       bad(59,!gears_weapon_part_visibility::same(out.space,s2)))return false;
    if(selected)*selected=sequence;
    return !bad(60,!binding::current(b.proof))&&!bad(61,!binding::metadata_current());
}
bool snapshot(api::Source& out,std::uintptr_t* seq=nullptr) noexcept {
    try {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        return lock.owns_lock()&&snapshot_unlocked(bound,out,seq,bound.pose.restoring());}catch(...){return false;}
}
bool held_current() noexcept {std::uintptr_t held{};
    return recording.load()&&bound.key.world_generation==committed_world_generation.load()&&
        read(bound.pawn.address+bound.pawn_weapon.offset,held)&&held==bound.weapon.address;
}
bool inactive() noexcept {
    const auto world=committed_world_generation.load();if(world&&world!=bound.key.world_generation)return true;
    std::uintptr_t a{},b{};return read(bound.pawn.address+bound.pawn_weapon.offset,a)&&
        read(bound.pawn.address+bound.pawn_weapon.offset,b)&&a==b&&a!=bound.weapon.address;
}
bool destroyed() noexcept {
    try {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current())return false;
        gears_anchor::Registry objects{},again{};std::uintptr_t a{},b{};
        if(!read(binding::metadata.objects_header,objects)||!binding::cohort(objects,objects)||
           bound.component.registry_index>=unsigned(objects.count))return false;
        const auto at=objects.data+std::size_t(bound.component.registry_index)*8;
        return read(at,a)&&read(at,b)&&a==b&&a!=bound.component.address&&
            read(binding::metadata.objects_header,again)&&binding::cohort(objects,again)&&objects.count==again.count;
    }catch(...){return false;}
}
void retire(bool is_inactive,bool is_destroyed=false) noexcept {
    retired={};retired.handle=bound.handle;retired.writes=bound.pose.writes();retired.angle=bound.pose.wanted();retired.pull_raw=bound.pose.wanted_pull();
    retired.result=is_inactive?api::Result::inactive_restored:api::Result::restored;
    retired.gate=bound.restore_gate;retired.cpu_restored=!is_destroyed;retired.original_inactive=is_inactive;retired.original_destroyed=is_destroyed;
    retired_cookie=bound.key.registration_cookie;bound={};retained.store(0,std::memory_order_release);
}
void drain_on_gt() noexcept {
    if(!retained.load(std::memory_order_acquire)||GetCurrentThreadId()!=owner_thread.load())return;
    if(drain_requested.load()||!held_current())request_restore(drain_requested.load()?801u:802u);
    api::Source source{};if(!snapshot(source)){if(destroyed())retire(true,true);return;}
    bound.pose.observe_cache(source,[](auto at,auto& value){return read(at,value);});
    const bool off=inactive();
    if(bound.pose.restoring()&&bound.pose.restore(source,[](auto at,auto& value){return read(at,value);},write,off))retire(off);
}
// C is exclusively composed here; the old decoder callback no longer writes it.
struct DecoderTicket {};
void decoder_begin(atom::Arguments const&,DecoderTicket&) noexcept {}
void decoder_return(DecoderTicket const&,bool) noexcept {}
void compose_begin(std::uintptr_t component,std::uintptr_t local,std::uintptr_t required,
    unsigned kind,std::uintptr_t caller,ComposeTicket& ticket) noexcept {
    ticket={};if(!retained.load()||GetCurrentThreadId()!=owner_thread.load()||component!=bound.component.address)return;
    if(kind)return;
    if(local!=component+0x27c||required!=component+0x28c){last_gate=701;return;}
    if(caller!=base+gears_cover_pose_phase::compose_caller){
        last_gate=caller>base&&caller-base<UINT32_MAX?unsigned(caller-base):702;return;
    }
    try {
        if(!cover_pose_phase_live::certified||!cover_pose_phase_live::own_entries()){last_gate=703;return;}
        if(drain_requested.load()||!held_current())request_restore(drain_requested.load()?801u:802u);
        api::Source s{};if(!snapshot(s)){last_gate=704;return;}
        // A nested call cannot borrow a pending journal or create another writer.
        if(bound.pose.active()){request_restore(803);return;}
        if(bound.pose.restoring()){ticket.handle=bound.handle;ticket.admitted=true;return;}
        // As with the prior decoder owner, this is the retained mechanical
        // target authorized by begin/update, not a new interaction or world
        // hand pose. Original current held/world/object/topology/header checks
        // above and the exact certified Compose boundary still apply. A
        // different publication phase cannot borrow the old request Frame or
        // manufacture a current one. Every NEW target still passes the exact
        // current-frame render/socket/part admission in update().
        ticket.handle=bound.handle;ticket.admitted=true;
        if(!bound.pose.compose_begin(s,[](auto at,auto& v){return read(at,v);},write))
            request_restore(804);
        last_gate=706;
    }catch(...){request_restore(805);}
}
void compose_end(ComposeTicket const& ticket,bool normal) noexcept {
    if(!ticket.admitted||!retained.load()||ticket.handle!=bound.handle||GetCurrentThreadId()!=owner_thread.load())return;
    try {
        api::Source s{};if(!snapshot(s)){request_restore(806);return;}
        if(!bound.pose.compose_end(s,normal,[](auto at,auto& v){return read(at,v);},write))
            request_restore(807);
        if(bound.pose.restoring())drain_on_gt();
        else last_gate=0;
    }catch(...){request_restore(805);}
}
api::Receipt receipt() noexcept {
    if(!retained.load())return retired;
    api::Receipt r{};r.handle=bound.handle;r.writes=bound.pose.writes();r.angle=bound.pose.wanted();r.pull_raw=bound.pose.wanted_pull();r.gate=bound.restore_gate?bound.restore_gate:last_gate;
    r.cpu_restored=bound.pose.cpu_restored();api::Source s{};
    const bool ready=snapshot(s)&&bound.pose.ready(s,[](auto at,auto& value){return read(at,value);});
    r.epoch=s.epoch;r.result=bound.pose.restoring()?api::Result::restoring:(ready?api::Result::ready:api::Result::pending);return r;
}
api::Result begin(api::Request const* input,api::Receipt* output) noexcept {
    Call call;api::Request q{};api::Receipt r{};last_gate=1;
    const auto reject=[&](){r.gate=last_gate;if(output)safe_copy(output,&r,sizeof r);return api::Result::rejected;};
    try {auto* broker=weapon_hand_runtime::current();
        if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||
           !broker||drain_requested.load()||retained.load()||existing::pending()||!next_handle||next_handle==UINT64_MAX||
           !broker->admit_weapon_render(q.source)||!existing::code_current()||
           !cover_pose_phase_live::certified||!cover_pose_phase_live::own_entries())return reject();
        Bound b{};b.key=q.source.key;b.world=q.source.world;b.pawn=q.source.pawn;b.weapon=q.source.weapon;
        b.component=q.component;b.asset=q.asset;b.node=q.node;b.thread=GetCurrentThreadId();b.handle=next_handle++;
        api::Source s{};last_gate=2;
        {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
            if(!lock.owns_lock()||!binding::metadata_current())return reject();
            for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(!existing::object(id))return reject();
            gears_anchor::Identity asset{};last_gate=3;
            if(!existing::derives(b.weapon,L"Weap_AssaultRifle",L"WarfareGame")||
               !existing::derives(b.component,L"SkeletalMeshComponent",L"Engine")||
               !existing::derives(b.asset,L"SkeletalMesh",L"Engine")||!existing::derives(b.node,L"AnimNodeSequence",L"Engine")||
               !binding::identity(b.asset.address,asset,true)||!binding::named(asset,L"COGAssaultRifle"))return reject();
            last_gate=4;if(!properties(b))return reject();
            last_gate=5;unsigned detail{};if(!snapshot_unlocked(b,s,nullptr,false,&detail)){last_gate=500+detail;return reject();}
            last_gate=6;if(!b.pose.begin(s,[](auto at,auto& value){return read(at,value);}))return reject();
        }
        last_gate=7;if(!broker->admit_weapon_render(q.source))return reject();
        b.initial=s;bound=b;owner_thread.store(b.thread);retained.store(1,std::memory_order_release);last_gate=700;
        r=receipt();if(!safe_copy(output,&r,sizeof r)){request_restore(808);return api::Result::restoring;}return r.result;
    }catch(...){
        // Once adopted, return the retained identity even on failure. A zero
        // handle must never make the consumer forget Native's restore debt.
        if(retained.load()){
            request_restore(809);r={};r.handle=bound.handle;r.result=api::Result::restoring;r.gate=last_gate;
            if(output)safe_copy(output,&r,sizeof r);return api::Result::restoring;
        }
        return reject();
    }
}
api::Result update(api::Control const* input,api::Receipt* output,bool restore) noexcept {
    Call call;api::Control q{};
    if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||!q.handle||!q.cookie||
       GetCurrentThreadId()!=owner_thread.load())return api::Result::rejected;
    if(!retained.load()){if(q.handle!=retired.handle||q.cookie!=retired_cookie)return api::Result::rejected;
        return safe_copy(output,&retired,sizeof retired)?retired.result:api::Result::rejected;}
    if(q.handle!=bound.handle||q.cookie!=bound.key.registration_cookie)return api::Result::rejected;
    if(!restore&&(q.source.key.registration_cookie!=bound.key.registration_cookie||
       q.source.key.lifecycle!=bound.key.lifecycle||q.source.key.game_start!=bound.key.game_start||
       q.source.key.native_started_qpc!=bound.key.native_started_qpc||q.source.key.world_generation!=bound.key.world_generation)){
        request_restore(810);drain_on_gt();const auto r=receipt();
        return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
    }
    if(restore)request_restore(811);
    else {
        auto* b=weapon_hand_runtime::current();
        if(!b||!hand::same(q.source.weapon,bound.weapon)||!hand::same(q.source.world,bound.world)||
           !hand::same(q.source.pawn,bound.pawn)||
           !api::update_admitted_lazy(q.angle,q.pull_raw,bound.pose.wanted(),bound.pose.wanted_pull(),
               b->admit_weapon_render(q.source),[&]() noexcept {return b->admit_part_visibility(q.source);},
               [&]() noexcept {return b->admit_socket_visibility(q.source);},
               q.source.left_tracked&&q.source.left_grip_known)||
           !bound.pose.wanted(q.angle,q.pull_raw))return api::Result::rejected;
    }
    drain_on_gt();const auto r=receipt();return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
}
#else
void compose_begin(std::uintptr_t,std::uintptr_t,std::uintptr_t,unsigned,std::uintptr_t,ComposeTicket&) noexcept {}
void compose_end(ComposeTicket const&,bool) noexcept {}
struct DecoderTicket {};
void decoder_begin(gears_atom_overwrite::Arguments const&,DecoderTicket&) noexcept {}
void decoder_return(DecoderTicket const&,bool) noexcept {}
api::Result begin(api::Request const*,api::Receipt*) noexcept{return api::Result::unavailable;}
api::Result update(api::Control const*,api::Receipt*,bool) noexcept{return api::Result::unavailable;}
#endif
}
