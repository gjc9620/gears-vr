// Included inside ready_wrist. Observer-owned POD only; never a source/permit.
namespace outer_detail {
namespace p=gears_cover_pose_phase;
namespace wh=gears_weapon_hand;
struct Ticket {
    wire::Record* row{};uint64_t sequence{},tick{};
    p::OwnerKey owner{};p::Metadata metadata{};
    gears_anchor::Identity avatar{};
    main_mount::Response mount{};
    std::array<body::rig::ReferenceRecord68,body::bones> reference{};
};
Ticket ticket{};
struct Layout {
    binding::Proof proof{};
    uintptr_t pawn_class{},weapon_class{},avatar_class{},gun_class{};
    binding::Property pawn_mesh{},pawn_weapon{},instigator{},weapon_mesh{},asset{},attachments{},matrix{};
    binding::ClassPath component_path{};
    uint64_t generation{};unsigned step{},last_step{};bool ready{},attempted{};
} layout;
bool same_identity(wh::ObjectIdentity const& expected) {
    gears_anchor::Identity value{};
    return wh::identity_valid(expected)&&binding::identity(expected.address,value,true)&&
        wh::same(expected,rig_tracking_live::object(value));
}
bool identities(Ticket const& t) {
    auto const& m=t.mount;gears_anchor::Identity avatar{};
    return binding::metadata_current()&&same_identity(m.world)&&same_identity(m.controller)&&
        same_identity(m.pawn)&&same_identity(m.mesh)&&same_identity(m.weapon)&&
        same_identity(m.component)&&same_identity(m.asset)&&
        binding::identity(t.avatar.address,avatar,true)&&binding::same(avatar,t.avatar);
}
bool scope(Ticket const& t,Entry const& e) {
    auto const& f=hand::slot;auto* row=t.row;
    if(!row||row->sequence!=t.sequence||row->key.tick!=t.tick||!t.tick||
       row->key.pid!=GetCurrentProcessId()||row->key.native_started!=uint64_t(session_started_qpc)||
       row->key.thread!=GetCurrentThreadId()||row->key.thread!=owner_thread.load()||
       !row->key.cookie||t.mount.key.registration_cookie!=row->key.cookie||
       t.mount.key.game_pid!=row->key.pid||t.mount.key.game_start!=row->key.game_start||
       t.mount.key.native_started_qpc!=row->key.native_started||t.mount.key.thread_id!=row->key.thread||
       !enabled||stopping.load()||hand::stopping.load()||hand::failed.load()||!f.active||!f.lease||
       f.tick!=t.tick||f.tick_depth!=1||f.hard_control_fault||
       t.owner.world_generation!=committed_world_generation.load()||
       t.owner.session!=uint64_t(session_started_qpc)||
       t.owner.mesh.address!=row->key.mesh||t.mount.mesh.address!=row->key.mesh||
       t.owner.mesh.address!=f.data.owner.key.mesh.address||t.owner.pawn.address!=f.data.owner.key.pawn.address||
       t.owner.world.address!=f.data.owner.key.world.address)return false;
    if(e.kind==0)return current(e)==row&&state.captured&&!state.context.expired;
    return e.kind==2&&e.selected&&e.tick_depth&&!e.compose_depth&&!f.compose_depth&&!f.control_depth&&
        !f.compose_address&&!f.control_address&&f.tick_address==&e&&e.invocation==t.tick&&e.thread==row->key.thread;
}
bool struct_type(binding::Proof& proof,binding::Property const& field,const wchar_t* name,unsigned width) {
    uintptr_t address{};gears_anchor::Identity type{};unsigned extent{};
    return binding::source_field(proof,address,field.id.address,0xd0)&&binding::certified(proof,address,type)&&
        binding::kind(proof,type,L"ScriptStruct")&&binding::named(type,name)&&
        binding::source_field(proof,extent,address,0x88)&&extent==width;
}
// A complete bounded chain proves unique field selection in this ScriptStruct.
bool member(binding::Proof& proof,gears_anchor::Identity const& type,const wchar_t* name,
            const wchar_t* kind,unsigned offset,unsigned width,const wchar_t* nested=nullptr) {
    uintptr_t child{};if(!binding::source_field(proof,child,type.address,0x80))return false;
    std::array<uintptr_t,128> seen{};unsigned count{},matches{};
    while(child&&count<seen.size()) {
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        gears_anchor::Identity id{};if(!binding::identity(child,id,false,true)||id.outer!=type.address)return false;
        if(!wcscmp(id.name.data(),name)) {
            binding::Property field{};field.id=id;
            if(id.number||++matches!=1||!binding::remember(proof,id)||!binding::kind(proof,id,kind)||
               !binding::source_field(proof,field.dimension,child,0x70)||field.dimension!=1||
               !binding::source_field(proof,field.width,child,0x74)||field.width!=width||
               !binding::source_field(proof,field.offset,child,0x94)||field.offset!=offset||
               (nested&&!struct_type(proof,field,nested,width)))return false;
        }
        if(!binding::source_field(proof,child,child,0x68))return false;
    }
    return !child&&matches==1;
}
unsigned declared_mesh_failure(gears_anchor::Identity const& type) {
    if(binding::named(type,L"MeshComponent"))return 10570;
    if(binding::named(type,L"PrimitiveComponent"))return 10571;
    if(binding::named(type,L"ActorComponent"))return 10572;
    if(binding::named(type,L"Object"))return 10573;
    return 10579;
}
// Private diagnostic copy, strict result/read order identical to existing
// binding::property. Only original Weapon.Mesh reference rejection is noted.
using binding::Proof;using binding::Property;
bool weapon_mesh_property(Proof& p,uintptr_t start,const wchar_t* name,const wchar_t* property_kind,unsigned width,const wchar_t* reference_name,Property& out){
    using namespace binding;
    rejected_field=name;std::array<uintptr_t,32> owners{};unsigned owner_count{};
    for(auto owner=start;owner&&owner_count<owners.size();){
        for(unsigned i=0;i<owner_count;++i)if(owners[i]==owner)return false;owners[owner_count++]=owner;
        gears_anchor::Identity owner_id{};unsigned extent{};uintptr_t child{};
        if(!certified(p,owner,owner_id)||!kind(p,owner_id,L"Class")||!source_field(p,extent,owner,0x88)||!extent||extent>0x10000||!source_field(p,child,owner,0x80))return false;
        std::array<uintptr_t,4096> visited{};unsigned count{},matches{};Property selected{};
        while(child&&count<visited.size()){
            for(unsigned i=0;i<count;++i)if(visited[i]==child)return false;visited[count++]=child;
            gears_anchor::Identity id{};if(!identity(child,id,false,true)||id.outer!=owner)return false;
            if(!wcscmp(id.name.data(),name)){
                uintptr_t reference{};
                const bool pointer_kind=!wcscmp(property_kind,L"MeshPointer")?
                    (kind(p,id,L"ComponentProperty")||kind(p,id,L"ObjectProperty")):kind(p,id,property_kind);
                if(id.number||++matches!=1||!certified(p,child,selected.id)||!pointer_kind||
                   !source_field(p,selected.dimension,child,0x70)||!source_field(p,selected.width,child,0x74)||!source_field(p,selected.offset,child,0x94)||
                   selected.dimension!=1||selected.width!=width||extent<width||selected.offset>extent-width)return false;
                if(reference_name){
                    if(!source_field(p,reference,child,0xd0)){layout.step=layout.last_step=10560;return false;}
                    if(!certified(p,reference,selected.reference)){layout.step=layout.last_step=10561;return false;}
                    if(!kind(p,selected.reference,L"Class")){layout.step=layout.last_step=10562;return false;}
                    if(!named(selected.reference,reference_name)){
                        layout.step=layout.last_step=declared_mesh_failure(selected.reference);return false;}
                }
                selected.owner=owner_id;
            }
            if(matches){if(!field(child,child,0x68))return false;}
            else if(!source_field(p,child,child,0x68))return false;
        }
        if(child)return false;if(matches){out=selected;return true;}
        if(!source_field(p,owner,owner,metadata.field_super))return false;
    }return false;
}
bool prepare_layout(Ticket const& t) {
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()){layout.last_step=90;return false;}
    auto const& m=t.mount;
    const bool same=layout.pawn_class==m.pawn.klass&&layout.weapon_class==m.weapon.klass&&
        layout.avatar_class==m.mesh.klass&&layout.gun_class==m.component.klass&&
        layout.generation==committed_world_generation.load();
    if(same&&layout.attempted) {
        layout.last_step=layout.step;
        if(!layout.ready)return false;
        if(!binding::current(layout.proof)){layout.last_step=91;return false;}
        layout.last_step=3;return true;
    }
    layout={};auto& q=layout.proof;
    const auto at=[&](unsigned step){layout.step=layout.last_step=step;};
    layout.pawn_class=m.pawn.klass;layout.weapon_class=m.weapon.klass;layout.avatar_class=m.mesh.klass;
    layout.gun_class=m.component.klass;layout.generation=committed_world_generation.load();layout.attempted=true;
    at(101);if(!identities(t))return false;
    at(102);if(!binding::property(q,m.pawn.klass,L"Mesh",L"MeshPointer",8,L"SkeletalMeshComponent",layout.pawn_mesh))return false;
    at(103);if(!binding::property(q,m.pawn.klass,L"Weapon",L"ObjectProperty",8,L"Weapon",layout.pawn_weapon))return false;
    at(104);if(!binding::property(q,m.weapon.klass,L"Instigator",L"ObjectProperty",8,L"Pawn",layout.instigator))return false;
    at(105);if(!weapon_mesh_property(q,m.weapon.klass,L"Mesh",L"MeshPointer",8,L"MeshComponent",layout.weapon_mesh))return false;
    at(10580);if(!binding::derives(m.component.klass,L"SkeletalMeshComponent",layout.weapon_mesh.reference.address,layout.component_path))return false;
    at(106);if(!binding::property(q,m.component.klass,L"SkeletalMesh",L"ObjectProperty",8,L"SkeletalMesh",layout.asset))return false;
    at(107);if(!binding::property(q,m.component.klass,L"LocalToWorld",L"StructProperty",64,nullptr,layout.matrix))return false;
    at(108);if(!struct_type(q,layout.matrix,L"Matrix",64))return false;
    at(109);if(layout.matrix.offset!=0xa0)return false;
    at(110);if(!binding::property(q,m.mesh.klass,L"Attachments",L"ArrayProperty",16,nullptr,layout.attachments))return false;
    uintptr_t inner_address{},type_address{};gears_anchor::Identity inner{},type{};unsigned dim{},width{},extent{};
    at(201);if(!binding::source_field(q,inner_address,layout.attachments.id.address,0xd0))return false;
    at(202);if(!binding::certified(q,inner_address,inner))return false;
    at(203);if(inner.outer!=layout.attachments.id.address)return false;
    at(204);if(!binding::kind(q,inner,L"StructProperty"))return false;
    at(205);if(!binding::source_field(q,dim,inner_address,0x70)||dim!=1)return false;
    at(206);if(!binding::source_field(q,width,inner_address,0x74)||width!=52)return false;
    at(207);if(!binding::source_field(q,type_address,inner_address,0xd0))return false;
    at(208);if(!binding::certified(q,type_address,type))return false;
    at(209);if(!binding::kind(q,type,L"ScriptStruct"))return false;
    at(210);if(!binding::source_field(q,extent,type_address,0x88))return false;
    at(211);if(extent!=52)return false;
    at(212);if(!member(q,type,L"Component",L"ComponentProperty",0,8))return false;
    at(213);if(!member(q,type,L"BoneName",L"NameProperty",8,8))return false;
    at(214);if(!member(q,type,L"RelativeLocation",L"StructProperty",16,12,L"Vector"))return false;
    at(215);if(!member(q,type,L"RelativeRotation",L"StructProperty",28,12,L"Rotator"))return false;
    at(216);if(!member(q,type,L"RelativeScale",L"StructProperty",40,12,L"Vector"))return false;
    at(217);if(!binding::current(q))return false;
    at(3);layout.ready=true;return true;
}
bool component_type_current(Ticket const& t) {
    binding::ClassPath current{};
    return binding::derives(t.mount.component.klass,L"SkeletalMeshComponent",layout.weapon_mesh.reference.address,current)&&
        binding::same_path(current,layout.component_path);
}
bool links(Ticket const& t) {
    auto const& m=t.mount;uintptr_t value{};
    return binding::field(value,m.pawn.address,layout.pawn_mesh.offset)&&value==m.mesh.address&&
        binding::field(value,m.pawn.address,layout.pawn_weapon.offset)&&value==m.weapon.address&&
        binding::field(value,m.weapon.address,layout.instigator.offset)&&value==m.pawn.address&&
        binding::field(value,m.weapon.address,layout.weapon_mesh.offset)&&value==m.component.address&&
        binding::field(value,m.component.address,layout.asset.offset)&&value==m.asset.address&&component_type_current(t);
}
wire::OuterStatus gun(Ticket const& t,Entry const& e,wire::ComponentStage& out) {
    using S=wire::OuterStatus;out.begin_qpc=uint64_t(phase::qpc());
    const auto done=[&](S s){out.status=uint64_t(s);out.end_qpc=uint64_t(phase::qpc());return s;};
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return done(S::layout);
    if(!scope(t,e))return done(S::scope);
    if(!layout.ready)return done(S::unprepared);
    if(!identities(t))return done(S::identity);
    if(layout.generation!=committed_world_generation.load()||!binding::current(layout.proof)||!links(t))return done(S::layout);
    p::ArrayHeader header{},again{};std::array<unsigned char,128*52> bytes{},repeat{};
    if(!binding::field(header,t.mount.mesh.address,layout.attachments.offset)||header.count<1||header.count>128||
       header.capacity<header.count||header.capacity>128||!p::span(header.data,size_t(header.count)*52)||
       !native::read_memory(bytes.data(),header.data,size_t(header.count)*52))return done(S::attachment);
    unsigned matches{};
    for(int i=0;i<header.count;++i) {
        auto const* at=bytes.data()+i*52;uintptr_t component{};std::memcpy(&component,at,8);
        if(component==t.mount.component.address) {
            if(++matches!=1||std::memcmp(at+8,&t.reference[52],8))return done(S::attachment);
            std::memcpy(out.attachment,at,52);
        }
    }
    if(matches!=1)return done(S::attachment);
    body::Matrix64 matrix{};uint32_t epoch{},epoch2{};
    if(!native::read_memory(&out.matrix,t.mount.component.address+layout.matrix.offset,64)||
       !binding::field(epoch,t.mount.component.address,0x338)||
       !native::read_memory(&matrix,t.mount.component.address+layout.matrix.offset,64)||std::memcmp(&matrix,&out.matrix,64)||
       !binding::field(epoch2,t.mount.component.address,0x338)||epoch!=epoch2)return done(S::payload);
    for(float v:out.matrix.values)if(!std::isfinite(v))return done(S::payload);
    if(!native::read_memory(repeat.data(),header.data,size_t(header.count)*52)||
       std::memcmp(bytes.data(),repeat.data(),size_t(header.count)*52)||
       !binding::field(again,t.mount.mesh.address,layout.attachments.offset)||!p::header_equal(header,again)||
       !identities(t)||!links(t)||!binding::current(layout.proof)||!scope(t,e))return done(S::changed);
    out.weapon=t.mount.weapon;out.component=t.mount.component;out.asset=t.mount.asset;
    out.attachment_data=header.data;out.attachment_count=header.count;out.attachment_capacity=header.capacity;
    out.gun_pose_epoch=epoch;out.proof_generation=layout.generation;return done(S::okay);
}
wire::OuterStatus body_sample(Ticket const& t,Entry const& e,wire::OuterTick& out) {
    using S=wire::OuterStatus;auto& stage=out.body;stage.begin_qpc=uint64_t(phase::qpc());
    const auto done=[&](S s){stage.status=uint64_t(s);stage.end_qpc=uint64_t(phase::qpc());return s;};
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return done(S::layout);
    if(!scope(t,e)||!identities(t))return done(S::scope);
    p::Request request{};request.owner=t.owner;p::Metadata a{},b{};
    auto read=[](void* to,void const* at,size_t n){return native::read_memory(to,reinterpret_cast<uintptr_t>(at),n);};
    if(!p::metadata(read,request,a)||a.asset!=t.metadata.asset||a.master!=t.metadata.master||
       !p::header_equal(a.reference,t.metadata.reference)||!p::header_equal(a.local,t.metadata.local)||
       !p::header_equal(a.space,t.metadata.space)||!p::header_valid(a.local,32)||!p::header_valid(a.space,64)||
       a.local.count!=115||a.space.count!=115||a.reference.count!=115)return done(S::metadata);
    std::array<body::rig::ReferenceRecord68,body::bones> reference{};
    if(!native::read_memory(reference.data(),a.reference.data,sizeof reference)||
       std::memcmp(reference.data(),t.reference.data(),sizeof reference))return done(S::reference);
    for(unsigned i=0;i<3;++i)if(!native::read_memory(&stage.local[i],a.local.data+wire::bones[i]*32,32))return done(S::payload);
    if(!native::read_memory(&stage.space35,a.space.data+35*64,64)||!native::read_memory(&stage.space52,a.space.data+52*64,64))return done(S::payload);
    body::Atom atom{};body::Matrix64 matrix{};
    for(unsigned i=0;i<3;++i)if(!native::read_memory(&atom,a.local.data+wire::bones[i]*32,32)||std::memcmp(&atom,&stage.local[i],32))return done(S::changed);
    if(!native::read_memory(&matrix,a.space.data+35*64,64)||std::memcmp(&matrix,&stage.space35,64)||
       !native::read_memory(&matrix,a.space.data+52*64,64)||std::memcmp(&matrix,&stage.space52,64)||
       !native::read_memory(reference.data(),a.reference.data,sizeof reference)||std::memcmp(reference.data(),t.reference.data(),sizeof reference)||
       !p::metadata(read,request,b)||!p::metadata_equal(a,b)||!identities(t)||!scope(t,e))return done(S::changed);
    stage.pose_epoch=a.pose_epoch;std::memcpy(&stage.component,&a.component,64);stage.space_valid=1;
    out.tick_epoch=a.tick_epoch;out.flag318=a.flag318;out.flag32c=a.flag32c;out.flag350=a.flag350;return done(S::okay);
}
} // namespace outer_detail
void hold_outer(Entry const& e) noexcept {
    if(!recorder.enabled)return;
    try {
        auto* row=current(e);if(!row||!(row->valid&wire::Valid::normal_scope_close)||
            state.right_mount!=hands::adapter::MountKind::held_lancer)return;
        auto& t=outer_detail::ticket;t={};t.row=row;t.sequence=row->sequence;t.tick=row->key.tick;
        t.owner=state.context.owner;t.metadata=state.backend.metadata();t.avatar=state.context.asset;
        t.mount=state.mount_response;t.reference=state.source.reference;
        outer_detail::gun(t,e,row->compose_gun);
    }catch(...){if(auto* row=current(e))row->compose_gun.status=uint64_t(wire::OuterStatus::exception);outer_detail::ticket={};}
}
void outer_end(Entry const& e,bool returned,bool called) noexcept {
    if(!recorder.enabled||GetCurrentThreadId()!=owner_thread.load())return;
    auto& ticket=outer_detail::ticket;
    if(!ticket.row||ticket.tick!=e.invocation)return;
    // Consume once, before any reads; all retained bytes remain observation-only.
    auto t=ticket;ticket={};auto& out=t.row->outer;out.begin_qpc=uint64_t(phase::qpc());
    out.entry=reinterpret_cast<uintptr_t>(&e);out.tick=e.invocation;out.thread=e.thread;out.mesh=e.rcx;
    out.original_called=called;out.original_returned=returned;
    try {
        if(!called||!returned||e.rcx!=t.row->key.mesh||!outer_detail::scope(t,e))out.status=uint64_t(wire::OuterStatus::scope);
        else if(!outer_detail::prepare_layout(t))out.status=uint64_t(wire::OuterStatus::layout);
        else {
            out.last_compose_matches=hand::slot.data.record.compose==t.row->key.compose?1:0;
            auto body_status=outer_detail::body_sample(t,e,out);
            auto gun_status=outer_detail::gun(t,e,out.gun);
            wire::OuterTick close{};
            auto close_status=outer_detail::body_sample(t,e,close);out.body_close=close.body;
            const bool stable=out.body.pose_epoch==close.body.pose_epoch&&out.tick_epoch==close.tick_epoch&&
                out.flag318==close.flag318&&out.flag32c==close.flag32c&&out.flag350==close.flag350&&
                !std::memcmp(out.body.local,close.body.local,sizeof out.body.local)&&
                !std::memcmp(&out.body.space35,&close.body.space35,64)&&!std::memcmp(&out.body.space52,&close.body.space52,64)&&
                !std::memcmp(&out.body.component,&close.body.component,64);
            // Sequential explicit times, never claim a simultaneous/render fence.
            out.status=uint64_t(body_status!=wire::OuterStatus::okay?body_status:
                gun_status!=wire::OuterStatus::okay?gun_status:
                close_status!=wire::OuterStatus::okay?close_status:stable?wire::OuterStatus::okay:wire::OuterStatus::changed);
        }
    }catch(...){out.status=uint64_t(wire::OuterStatus::exception);}
    out.layout_step=outer_detail::layout.last_step;
    out.end_qpc=uint64_t(phase::qpc());
}
