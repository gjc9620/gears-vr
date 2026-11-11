#ifndef GEARS_COVER_COMPONENT_ROLES_LIVE_INCLUDED
#define GEARS_COVER_COMPONENT_ROLES_LIVE_INCLUDED
#include "cover_collider_roles_live.inl"
#include "cover_physx_components.h"
#include <algorithm>

// Current Pawn/known Weapon and recursively declared Actor.Attached component
// arrays, the current Pawn Mesh and recursive skeletal Attachment components.
// The C0 caller supplies its actual owner guard. Neither membership nor Owner
// alone declares a component self or all active physics covered. Other native
// sources, lifetime and world synchronization remain separate
// obligations. Optional skeletal PAA rows join the same-Scene qualification in
// cover_physx_components. No engine method is called or body written here.
namespace cover_component_roles_live {
namespace binding=weapon_hand_scene_binding;
namespace roles=cover_physics_roles_live;
namespace collider=cover_collider_roles_live;
namespace source=gears_cover_physx_query_source;
using Code=roles::Code;
constexpr unsigned root_capacity=16,list_capacity=64,component_capacity=128,asset_body_capacity=256;
constexpr unsigned attachment_capacity=256,attachment_stride_capacity=128;
enum class BodyState:unsigned {not_sampled,nonprimitive,body_instance_field_null,body_data_field_null,body_data_nonnull};
enum class AssetState:unsigned {not_sampled,nonskeletal,instance_field_null,instance_present};
struct AssetScope {
    AssetState state{};gears_anchor::Identity asset{},instance{},owner{};
    std::array<unsigned,3> instance_offsets{};
    source::pose::ArrayHeader array{};std::int32_t root{};unsigned first_body{};
};
struct AssetBody {
    roles::Body body{};gears_anchor::Identity owner{},owner_actor{};unsigned owner_roots{};
};
struct AttachmentScope {
    bool sampled{};gears_anchor::Identity structure{};unsigned offset{},stride{},component_offset{},first_row{};source::pose::ArrayHeader array{};
};
using AttachmentRow=gears_cover_physx_components::AttachmentRow;
struct Root {
    gears_anchor::Identity actor{},base{};
    std::array<unsigned,2> offsets{};
    std::array<source::pose::ArrayHeader,2> lists{};
    std::array<std::array<std::uintptr_t,list_capacity>,2> members{};
    unsigned base_offset{},attached_offset{};source::pose::ArrayHeader attached{};
    std::array<std::uintptr_t,list_capacity> attached_members{};
};
struct Component {
    gears_anchor::Identity identity{},owner{};
    gears_anchor::Identity body_owner{},body_owner_actor{};
    // One bit per observed actor node, starting with Pawn/known Weapon seeds.
    // A bit records address equality, not permission to classify an actor self.
    unsigned owner_roots{},body_owner_roots{};bool primitive{},skeletal{};
    BodyState body_state{};unsigned body_instance_offset{};roles::Body body{};AssetScope physics{};AttachmentScope attachments{};
};
struct Snapshot {
    unsigned seed_count{},root_count{},component_count{},asset_body_count{},attachment_count{};bool pawn_mesh_listed{};gears_anchor::Identity mesh_seed{};
    std::array<Root,root_capacity> roots{};
    // First occurrence order; original array rows (nulls/duplicates included)
    // remain in Root::members instead of being silently dropped.
    std::array<Component,component_capacity> components{};
    // One row per original array slot, including nulls and shared bodies.
    // Pool exhaustion rejects the whole observation instead of truncating it.
    std::array<AssetBody,asset_body_capacity> asset_bodies{};
    std::array<AttachmentRow,attachment_capacity> attachment_rows{};
};
inline bool same_body(roles::Body const& a,roles::Body const& b){return gears_anchor::equal_identity(a.identity,b.identity)&&
    a.owner==b.owner&&a.main==b.main&&a.auxiliary==b.auxiliary&&a.index==b.index;}
inline bool same(AssetScope const& a,AssetScope const& b){return a.state==b.state&&
    gears_anchor::equal_identity(a.asset,b.asset)&&gears_anchor::equal_identity(a.instance,b.instance)&&
    gears_anchor::equal_identity(a.owner,b.owner)&&a.instance_offsets==b.instance_offsets&&
    source::pose::header_equal(a.array,b.array)&&a.root==b.root&&a.first_body==b.first_body;}
inline bool same(AssetBody const& a,AssetBody const& b){return same_body(a.body,b.body)&&
    gears_anchor::equal_identity(a.owner,b.owner)&&gears_anchor::equal_identity(a.owner_actor,b.owner_actor)&&a.owner_roots==b.owner_roots;}
inline bool same(AttachmentScope const& a,AttachmentScope const& b){return a.sampled==b.sampled&&gears_anchor::equal_identity(a.structure,b.structure)&&
    a.offset==b.offset&&a.stride==b.stride&&a.component_offset==b.component_offset&&a.first_row==b.first_row&&source::pose::header_equal(a.array,b.array);}
inline bool same(Snapshot const& a,Snapshot const& b){
    if(a.seed_count!=b.seed_count||a.root_count!=b.root_count||a.component_count!=b.component_count||a.asset_body_count!=b.asset_body_count||
       a.attachment_count!=b.attachment_count||a.pawn_mesh_listed!=b.pawn_mesh_listed||!gears_anchor::equal_identity(a.mesh_seed,b.mesh_seed))return false;
    for(unsigned i=0;i<root_capacity;++i){auto const& x=a.roots[i];auto const& y=b.roots[i];
        if(!gears_anchor::equal_identity(x.actor,y.actor)||!gears_anchor::equal_identity(x.base,y.base)||x.offsets!=y.offsets||x.members!=y.members||
           x.base_offset!=y.base_offset||x.attached_offset!=y.attached_offset||x.attached_members!=y.attached_members||
           !source::pose::header_equal(x.attached,y.attached))return false;
        for(unsigned n=0;n<2;++n)if(!source::pose::header_equal(x.lists[n],y.lists[n]))return false;}
    for(unsigned i=0;i<component_capacity;++i){auto const& x=a.components[i];auto const& y=b.components[i];
        if(!gears_anchor::equal_identity(x.identity,y.identity)||!gears_anchor::equal_identity(x.owner,y.owner)||
           !gears_anchor::equal_identity(x.body_owner,y.body_owner)||!gears_anchor::equal_identity(x.body_owner_actor,y.body_owner_actor)||
           x.owner_roots!=y.owner_roots||x.body_owner_roots!=y.body_owner_roots||x.primitive!=y.primitive||x.skeletal!=y.skeletal||x.body_state!=y.body_state||
           x.body_instance_offset!=y.body_instance_offset||!same_body(x.body,y.body)||!same(x.physics,y.physics)||!same(x.attachments,y.attachments))return false;}
    for(unsigned i=0;i<asset_body_capacity;++i)if(!same(a.asset_bodies[i],b.asset_bodies[i]))return false;
    for(unsigned i=0;i<attachment_capacity;++i)if(!gears_cover_physx_components::same(a.attachment_rows[i],b.attachment_rows[i]))return false;
    return true;
}
inline gears_cover_physx_components::Component fields(Component const& value){
    namespace p=gears_cover_physx_components;
    p::Component row{};row.component=value.identity.address;row.owner=value.owner.address;row.body=value.body.identity.address;
    row.body_owner=value.body_owner.address;row.body_owner_actor=value.body_owner_actor.address;row.owner_roots=value.owner_roots;
    row.body_owner_roots=value.body_owner_roots;row.body_instance_offset=value.body_instance_offset;row.primitive=value.primitive;
    row.skeletal=value.skeletal;row.body_index=value.body.index;row.body_data=value.body.main;row.bone_spring=value.body.auxiliary;
    switch(value.body_state){
    case BodyState::not_sampled:row.state=p::State::not_sampled;break;
    case BodyState::nonprimitive:row.state=p::State::nonprimitive;break;
    case BodyState::body_instance_field_null:row.state=p::State::body_instance_field_null;break;
    case BodyState::body_data_field_null:row.state=p::State::body_data_field_null;break;
    case BodyState::body_data_nonnull:row.state=p::State::body_data_nonnull;break;
    }return row;
}
inline gears_cover_physx_components::AssetScope fields(AssetScope const& value){
    gears_cover_physx_components::AssetScope row{};
    row.sampled=value.state==AssetState::instance_field_null||value.state==AssetState::instance_present;
    row.asset=value.asset.address;row.instance=value.instance.address;row.owner=value.owner.address;
    row.offsets=value.instance_offsets;row.array=value.array;row.root=value.root;row.first_body=value.first_body;return row;
}
inline gears_cover_physx_components::AssetBody fields(AssetBody const& value){
    return {value.body.identity.address,value.owner.address,value.owner_actor.address,value.owner_roots,value.body.index,value.body.main,value.body.auxiliary};
}
inline gears_cover_physx_components::AttachmentScope fields(AttachmentScope const& value){
    return {value.sampled,value.structure.address,value.offset,value.stride,value.component_offset,value.first_row,value.array};
}
inline bool input(Snapshot const& typed,gears_cover_physx_components::Source& output){
    if(source::aliases(&typed,sizeof typed,&output,sizeof output))return false;
    output.seed_count=typed.seed_count;output.root_count=typed.root_count;output.component_count=typed.component_count;output.asset_body_count=typed.asset_body_count;output.pawn_mesh_listed=typed.pawn_mesh_listed;
    output.mesh_seed=typed.mesh_seed.address;output.attachment_count=typed.attachment_count;output.attachment_rows=typed.attachment_rows;
    for(unsigned n=0;n<root_capacity;++n){output.roots[n].actor=typed.roots[n].actor.address;output.roots[n].offsets=typed.roots[n].offsets;
        output.roots[n].lists=typed.roots[n].lists;output.roots[n].members=typed.roots[n].members;
        output.roots[n].base=typed.roots[n].base.address;output.roots[n].base_offset=typed.roots[n].base_offset;
        output.roots[n].attached_offset=typed.roots[n].attached_offset;output.roots[n].attached=typed.roots[n].attached;
        output.roots[n].attached_members=typed.roots[n].attached_members;}
    for(unsigned n=0;n<component_capacity;++n){output.components[n]=fields(typed.components[n]);output.assets[n]=fields(typed.components[n].physics);output.attachments[n]=fields(typed.components[n].attachments);}
    for(unsigned n=0;n<asset_body_capacity;++n)output.asset_bodies[n]=fields(typed.asset_bodies[n]);
    return gears_cover_physx_components::valid(output);
}
inline bool matches(Snapshot const& typed,gears_cover_physx_components::Snapshot const& value){
    namespace p=gears_cover_physx_components;auto const& raw=value.fields;
    if(typed.seed_count!=raw.seed_count||typed.root_count!=raw.root_count||typed.component_count!=raw.component_count||typed.asset_body_count!=raw.asset_body_count||typed.pawn_mesh_listed!=raw.pawn_mesh_listed)return false;
    if(typed.mesh_seed.address!=raw.mesh_seed||typed.attachment_count!=raw.attachment_count)return false;
    for(unsigned n=0;n<root_capacity;++n){auto const& a=typed.roots[n];auto const& b=raw.roots[n];
        if(a.actor.address!=b.actor||a.base.address!=b.base||a.offsets!=b.offsets||a.members!=b.members||a.base_offset!=b.base_offset||
           a.attached_offset!=b.attached_offset||a.attached_members!=b.attached_members||!source::pose::header_equal(a.attached,b.attached))return false;
        for(unsigned l=0;l<2;++l)if(!source::pose::header_equal(a.lists[l],b.lists[l]))return false;}
    for(unsigned n=0;n<component_capacity;++n)if(!p::same(fields(typed.components[n]),raw.components[n])||
        !p::same(fields(typed.components[n].physics),raw.assets[n])||!p::same(fields(typed.components[n].attachments),raw.attachments[n]))return false;
    for(unsigned n=0;n<asset_body_capacity;++n)if(!p::same(fields(typed.asset_bodies[n]),raw.asset_bodies[n]))return false;
    for(unsigned n=0;n<attachment_capacity;++n)if(!p::same(typed.attachment_rows[n],raw.attachment_rows[n]))return false;
    return p::consistent(value);
}
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
struct RootLayout {binding::Proof proof{};std::array<binding::Property,5> fields{};};
struct ComponentLayout {
    binding::Proof proof{};gears_anchor::Identity cls{},attachment_struct{};std::array<binding::Property,5> fields{},attachment_fields{};
    unsigned attachment_stride{};bool primitive{},skeletal{};
};
struct AssetLayout {binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,3> fields{};};
struct BodyLayouts {gears_anchor::Identity body_class{};std::array<roles::BodyLayout,8> layouts{};unsigned layout_count{};};
struct Workspace {
    roles::MemberStore members{};
    std::array<RootLayout,root_capacity> roots{};
    std::array<ComponentLayout,8> layouts{};unsigned layout_count{};
    std::array<AssetLayout,8> assets{};unsigned asset_layout_count{};
    gears_anchor::Identity component_base{};BodyLayouts bodies{};
    Snapshot first{},last{};binding::Binding sample_owner{};bool recheck_ready{};
    std::array<unsigned char,sizeof(gears_anchor::Config)> metadata{};std::uint64_t generation{};
};
inline bool separate(unsigned a,unsigned aw,unsigned b,unsigned bw){return a<=b?b-a>=aw:a-b>=bw;}
inline bool array_schema(binding::Proof& p,binding::Property const& field,gears_anchor::Identity& component_base){
    std::uintptr_t inner_descriptor{},target{};gears_anchor::Identity id{},type{};unsigned dim{},width{},offset{};
    if(!binding::source_field(p,inner_descriptor,field.id.address,0xd0)||!binding::certified(p,inner_descriptor,id)||id.number||id.outer!=field.id.address||
       !binding::kind(p,id,L"ComponentProperty")||!binding::source_field(p,dim,inner_descriptor,0x70)||dim!=1||
       !binding::source_field(p,width,inner_descriptor,0x74)||width!=8||!binding::source_field(p,offset,inner_descriptor,0x94)||offset||
       !binding::source_field(p,target,inner_descriptor,0xd0)||!binding::certified(p,target,type)||!binding::kind(p,type,L"Class")||
       !binding::named(type,L"ActorComponent"))return false;
    if(component_base.address&&!binding::same(component_base,type))return false;component_base=type;return true;
}
inline bool actor_array_schema(binding::Proof& p,binding::Property const& field){
    std::uintptr_t inner_descriptor{},target{};gears_anchor::Identity id{},type{};unsigned dim{},width{},offset{};
    return binding::source_field(p,inner_descriptor,field.id.address,0xd0)&&binding::certified(p,inner_descriptor,id)&&!id.number&&id.outer==field.id.address&&
        binding::kind(p,id,L"ObjectProperty")&&binding::source_field(p,dim,inner_descriptor,0x70)&&dim==1&&
        binding::source_field(p,width,inner_descriptor,0x74)&&width==8&&binding::source_field(p,offset,inner_descriptor,0x94)&&!offset&&
        binding::source_field(p,target,inner_descriptor,0xd0)&&binding::certified(p,target,type)&&binding::kind(p,type,L"Class")&&binding::named(type,L"Actor");
}
// Select the five root fields together. All visited member identities and
// links remain in this sample's full closing witness, including later siblings.
// A sibling root may reuse only the same owner/head; selected fields are fresh.
inline bool root_schema(Workspace& w,RootLayout& layout,std::uintptr_t start){
    constexpr std::array<wchar_t const*,5> names{L"WorldInfo",L"Components",L"AllComponents",L"Base",L"Attached"};
    constexpr std::array<unsigned,5> widths{8,16,16,8,16};
    constexpr std::array<wchar_t const*,5> kinds{L"ObjectProperty",L"ArrayProperty",L"ArrayProperty",L"ObjectProperty",L"ArrayProperty"};
    constexpr std::array<wchar_t const*,5> refs{L"WorldInfo",nullptr,nullptr,L"Actor",nullptr};
    auto& p=layout.proof;std::array<std::uintptr_t,32> owners{};unsigned owner_count{},found{};
    for(auto owner=start;owner&&owner_count<owners.size();){
        for(unsigned n=0;n<owner_count;++n)if(owners[n]==owner)return false;
        owners[owner_count++]=owner;gears_anchor::Identity owner_id{};unsigned extent{};std::uintptr_t head{};
        if(!binding::certified(p,owner,owner_id)||!binding::kind(p,owner_id,L"Class")||
           !binding::source_field(p,extent,owner,0x88)||!extent||extent>0x10000||
           !binding::source_field(p,head,owner,0x80))return false;
        unsigned first=w.members.count,count{};bool cached{};
        for(unsigned n=0;n<w.members.chain_count;++n){auto const& chain=w.members.chains[n];
            if(chain.owner!=owner)continue;
            if(chain.head!=head||chain.first>w.members.count||chain.count>w.members.count-chain.first)return false;
            first=chain.first;count=chain.count;cached=true;break;
        }
        if(!cached&&head){
            std::array<std::uintptr_t,4096> visited{};auto child=head;
            while(child&&count<visited.size()){
                for(unsigned n=0;n<count;++n)if(visited[n]==child)return false;
                visited[count++]=child;gears_anchor::Identity id{};
                if(!binding::identity(child,id)||id.outer!=owner||!w.members.link(id,child))return false;
            }
            if(child||!w.members.finish_chain(owner,head,first,count))return false;
        }
        const auto already=found;std::array<unsigned,5> matches{};
        for(unsigned n=0;n<count;++n){auto const& retained=w.members.members[first+n].identity;
            for(unsigned k=0;k<names.size();++k){
                if((already&(1u<<k))||std::wcscmp(retained.name.data(),names[k]))continue;
                auto& selected=layout.fields[k];std::uintptr_t reference{};
                if(++matches[k]!=1||retained.number||!binding::certified(p,retained.address,selected.id)||
                   !binding::same(selected.id,retained)||!binding::kind(p,selected.id,kinds[k])||
                   !binding::source_field(p,selected.dimension,retained.address,0x70)||selected.dimension!=1||
                   !binding::source_field(p,selected.width,retained.address,0x74)||selected.width!=widths[k]||
                   !binding::source_field(p,selected.offset,retained.address,0x94)||extent<widths[k]||selected.offset>extent-widths[k])return false;
                if(refs[k]&&(!binding::source_field(p,reference,retained.address,0xd0)||
                   !binding::certified(p,reference,selected.reference)||!binding::kind(p,selected.reference,L"Class")||
                   !binding::named(selected.reference,refs[k])))return false;
                selected.owner=owner_id;found|=1u<<k;
            }
        }
        if(found==31)return actor_array_schema(p,layout.fields[4]);
        if(!binding::source_field(p,owner,owner,binding::metadata.field_super))return false;
    }
    return false;
}
inline Code root(Workspace& w,unsigned index,std::uintptr_t at,binding::Binding const& context,Root& output,bool repeat=false){
    Root next{};auto& l=w.roots[index];auto& p=l.proof;const auto seeds=context.weapon.address?2u:1u;
    if(!collider::instance(p,at,next.actor))return Code::read;
    if(index<seeds&&!binding::same(next.actor,index?context.weapon:context.pawn))return Code::changed;
    // Only the second values pass may reuse this root's selected schema.
    // Re-read its complete first-pass proof before consuming any offset.
    // A new/reordered actor node cannot consume another root's cached fields.
    if(repeat){
        if(index>=w.first.root_count||!binding::same(next.actor,w.first.roots[index].actor)||
           !binding::metadata_current()||w.metadata!=binding::metadata_bytes||w.generation!=binding::metadata_generation||
           !binding::current(p))return Code::changed;
    }else if(!roles::class_role(p,next.actor.cls,index<seeds?(index?L"Weapon":L"Pawn"):L"Actor")||
       !root_schema(w,l,next.actor.cls))return Code::layout;
    for(unsigned a=0;a<5;++a)for(unsigned b=a+1;b<5;++b)
        if(!separate(l.fields[a].offset,a==0||a==3?8u:16u,l.fields[b].offset,b==0||b==3?8u:16u))return Code::layout;
    std::uintptr_t world{},base_actor{};
    if(!binding::source_field(p,world,next.actor.address,l.fields[0].offset)||world!=context.world.address)return Code::changed;
    next.base_offset=l.fields[3].offset;next.attached_offset=l.fields[4].offset;
    if(!binding::source_field(p,base_actor,next.actor.address,next.base_offset))return Code::read;
    if(base_actor&&(!collider::instance(p,base_actor,next.base)||!roles::class_role(p,next.base.cls,L"Actor",l.fields[3].reference.address)))return Code::layout;
    for(unsigned n=0;n<2;++n){auto const& field=l.fields[n+1];auto& list=next.lists[n];
        next.offsets[n]=field.offset;
        if(!repeat&&!array_schema(p,field,w.component_base))return Code::layout;
        if(!binding::source_field(p,list.data,next.actor.address,field.offset)||
           !binding::source_field(p,list.count,next.actor.address,field.offset+8)||
           !binding::source_field(p,list.capacity,next.actor.address,field.offset+12))return Code::read;
        if(list.count<0||list.count>int(list_capacity)||list.capacity<list.count||list.capacity>256||
           (!list.data?(list.count||list.capacity):!source::pointer(list.data,std::size_t((std::max)(1,list.capacity))*8)))return Code::capacity;
        if(list.count&&!safe_copy(next.members[n].data(),reinterpret_cast<void const*>(list.data),std::size_t(list.count)*8))return Code::read;
    }
    auto& list=next.attached;
    if(!binding::source_field(p,list.data,next.actor.address,next.attached_offset)||
       !binding::source_field(p,list.count,next.actor.address,next.attached_offset+8)||
       !binding::source_field(p,list.capacity,next.actor.address,next.attached_offset+12))return Code::read;
    if(list.count<0||list.count>int(list_capacity)||list.capacity<list.count||list.capacity>256||
       (!list.data?(list.count||list.capacity):!source::pointer(list.data,std::size_t((std::max)(1,list.capacity))*8)))return Code::capacity;
    if(list.count&&!safe_copy(next.attached_members.data(),reinterpret_cast<void const*>(list.data),std::size_t(list.count)*8))return Code::read;
    output=next;return Code::current;
}
inline bool attachment_schema(Workspace& w,ComponentLayout& layout){
    auto& p=layout.proof;auto const& field=layout.fields[4];std::uintptr_t inner_descriptor{},structure{},parent{},child{};
    gears_anchor::Identity inner_id{};unsigned dim{},inner_offset{},extent{};
    if(!binding::source_field(p,inner_descriptor,field.id.address,0xd0)||!binding::certified(p,inner_descriptor,inner_id)||
       inner_id.number||inner_id.outer!=field.id.address||!binding::kind(p,inner_id,L"StructProperty")||
       !binding::source_field(p,dim,inner_descriptor,0x70)||dim!=1||!binding::source_field(p,inner_offset,inner_descriptor,0x94)||inner_offset||
       !binding::source_field(p,layout.attachment_stride,inner_descriptor,0x74)||layout.attachment_stride<8||layout.attachment_stride>attachment_stride_capacity||
       !binding::source_field(p,structure,inner_descriptor,0xd0)||!binding::certified(p,structure,layout.attachment_struct)||
       !binding::kind(p,layout.attachment_struct,L"ScriptStruct")||!binding::named(layout.attachment_struct,L"Attachment")||
       layout.attachment_struct.outer!=field.owner.address||!binding::source_field(p,extent,structure,0x88)||extent!=layout.attachment_stride||
       !binding::source_field(p,parent,structure,binding::metadata.field_super)||parent||!binding::source_field(p,child,structure,0x80))return false;
    // binding::property is deliberately class-only. Close this ScriptStruct's
    // actual members here rather than pretending it is a UClass.
    constexpr std::array<wchar_t const*,5> names{L"Component",L"BoneName",L"RelativeLocation",L"RelativeRotation",L"RelativeScale"};
    std::array<std::uintptr_t,128> visited{};unsigned count{},found{};
    while(child&&count<visited.size()){
        for(unsigned n=0;n<count;++n)if(visited[n]==child)return false;visited[count++]=child;
        gears_anchor::Identity id{};if(!binding::certified(p,child,id)||id.outer!=structure)return false;
        for(unsigned n=0;n<names.size();++n)if(!std::wcscmp(id.name.data(),names[n])){
            if(id.number||(found&(1u<<n)))return false;auto& selected=layout.attachment_fields[n];selected.id=id;selected.owner=layout.attachment_struct;
            const unsigned width=n<2?8u:12u;
            if(!binding::kind(p,id,n==0?L"ComponentProperty":n==1?L"NameProperty":L"StructProperty")||
               !binding::source_field(p,selected.dimension,child,0x70)||selected.dimension!=1||
               !binding::source_field(p,selected.width,child,0x74)||selected.width!=width||
               !binding::source_field(p,selected.offset,child,0x94)||extent<width||selected.offset>extent-width)return false;
            if(n!=1){std::uintptr_t target{};if(!binding::source_field(p,target,child,0xd0)||!binding::certified(p,target,selected.reference))return false;
                if(!n){if(!binding::kind(p,selected.reference,L"Class")||!binding::same(selected.reference,w.component_base))return false;}
                else{unsigned size{};if(!binding::kind(p,selected.reference,L"ScriptStruct")||
                    !binding::named(selected.reference,n==3?L"Rotator":L"Vector")||!binding::source_field(p,size,target,0x88)||size!=12)return false;}}
            found|=1u<<n;
        }
        if(!binding::source_field(p,child,child,0x68))return false;
    }
    if(child||found!=31)return false;
    for(unsigned a=0;a<5;++a)for(unsigned b=a+1;b<5;++b)
        if(!separate(layout.attachment_fields[a].offset,layout.attachment_fields[a].width,
            layout.attachment_fields[b].offset,layout.attachment_fields[b].width))return false;
    return true;
}
inline ComponentLayout* component_layout(Workspace& w,gears_anchor::Identity const& id){
    for(unsigned n=0;n<w.layout_count;++n)if(w.layouts[n].cls.address==id.cls)return &w.layouts[n];
    if(w.layout_count==w.layouts.size())return nullptr;
    auto& l=w.layouts[w.layout_count];auto& p=l.proof;binding::ClassPath path{};
    if(!binding::certified(p,id.cls,l.cls)||!roles::class_role(p,id.cls,L"ActorComponent",w.component_base.address)||
       !binding::derives(id.cls,L"ActorComponent",w.component_base.address,path)||
       !roles::role_property(p,id.cls,L"Owner",L"ObjectProperty",8,L"Actor",l.fields[0],&w.members))return nullptr;
    for(unsigned i=0;i<path.count;++i){l.primitive=l.primitive||binding::named(path.ids[i],L"PrimitiveComponent");
        l.skeletal=l.skeletal||binding::named(path.ids[i],L"SkeletalMeshComponent");}
    if(l.primitive){
        if(!roles::role_property(p,id.cls,L"BodyInstance",L"ObjectProperty",8,L"RB_BodyInstance",l.fields[1],&w.members))return nullptr;
        if(!separate(l.fields[0].offset,8,l.fields[1].offset,8))return nullptr;
        if(w.bodies.body_class.address&&!binding::same(w.bodies.body_class,l.fields[1].reference))return nullptr;
        w.bodies.body_class=l.fields[1].reference;
    }
    if(l.skeletal){
        if(!l.primitive||!roles::role_property(p,id.cls,L"PhysicsAsset",L"ObjectProperty",8,L"PhysicsAsset",l.fields[2],&w.members)||
           !roles::role_property(p,id.cls,L"PhysicsAssetInstance",L"ObjectProperty",8,L"PhysicsAssetInstance",l.fields[3],&w.members)||
           l.fields[2].offset!=0x248||l.fields[3].offset!=0x250||
           !roles::role_property(p,id.cls,L"Attachments",L"ArrayProperty",16,nullptr,l.fields[4],&w.members)||!attachment_schema(w,l))return nullptr;
        for(unsigned a=0;a<5;++a)for(unsigned b=a+1;b<5;++b)
            if(!separate(l.fields[a].offset,a==4?16u:8u,l.fields[b].offset,b==4?16u:8u))return nullptr;
    }
    ++w.layout_count;return &l;
}
inline AssetLayout* asset_layout(Workspace& w,gears_anchor::Identity const& id,std::uintptr_t reference){
    for(unsigned n=0;n<w.asset_layout_count;++n)if(w.assets[n].cls.address==id.cls)return &w.assets[n];
    if(w.asset_layout_count==w.assets.size())return nullptr;
    auto& l=w.assets[w.asset_layout_count];auto& p=l.proof;
    if(!binding::certified(p,id.cls,l.cls)||!roles::class_role(p,id.cls,L"PhysicsAssetInstance",reference)||
       !roles::role_property(p,id.cls,L"Owner",L"MeshPointer",8,nullptr,l.fields[0],&w.members)||!roles::reference_class(p,l.fields[0])||
       !roles::role_property(p,id.cls,L"Bodies",L"ArrayProperty",16,nullptr,l.fields[1],&w.members)||l.fields[1].offset!=0x6c||
       !roles::role_property(p,id.cls,L"RootBodyIndex",L"IntProperty",4,nullptr,l.fields[2],&w.members))return nullptr;
    for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b)
        if(!separate(l.fields[a].offset,a==1?16u:a==2?4u:8u,l.fields[b].offset,b==1?16u:b==2?4u:8u))return nullptr;
    std::uintptr_t inner_descriptor{},target{};gears_anchor::Identity id_inner{},type{};unsigned dim{},width{},offset{};
    if(!binding::source_field(p,inner_descriptor,l.fields[1].id.address,0xd0)||!binding::certified(p,inner_descriptor,id_inner)||
       id_inner.number||id_inner.outer!=l.fields[1].id.address||!binding::kind(p,id_inner,L"ObjectProperty")||
       !binding::source_field(p,dim,inner_descriptor,0x70)||dim!=1||!binding::source_field(p,width,inner_descriptor,0x74)||width!=8||
       !binding::source_field(p,offset,inner_descriptor,0x94)||offset||!binding::source_field(p,target,inner_descriptor,0xd0)||
       !binding::certified(p,target,type)||!binding::kind(p,type,L"Class")||!binding::named(type,L"RB_BodyInstance")||
       (w.bodies.body_class.address&&!binding::same(w.bodies.body_class,type)))return nullptr;
    w.bodies.body_class=type;++w.asset_layout_count;return &l;
}
inline Code asset_body(Workspace& w,Snapshot const& roots,std::uintptr_t at,AssetBody& output){
    AssetBody next{};if(!at){output=next;return Code::current;}
    if(!binding::identity(at,next.body.identity,true))return Code::read;
    auto* l=roles::body_layout(w.bodies,next.body.identity,nullptr,&w.members);if(!l)return w.bodies.layout_count==w.bodies.layouts.size()?Code::capacity:Code::layout;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)
        if(!separate(l->fields[a].offset,a?8u:4u,l->fields[b].offset,b?8u:4u))return Code::layout;
    if(!binding::field(next.body.owner,at,l->fields[1].offset)||!binding::identity(next.body.owner,next.owner,true)||
       !binding::field(next.body.index,at,l->fields[0].offset)||!binding::field(next.body.main,at,l->fields[2].offset)||
       !binding::field(next.body.auxiliary,at,l->fields[3].offset)||
       (next.body.main&&!source::pointer(next.body.main,8))||(next.body.auxiliary&&!source::pointer(next.body.auxiliary,8)))return Code::changed;
    if(!roles::class_role(l->proof,next.owner.cls,L"PrimitiveComponent",l->fields[1].reference.address))return Code::layout;
    auto* backing=component_layout(w,next.owner);if(!backing)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
    std::uintptr_t owner_actor{};
    if(!binding::field(owner_actor,next.body.owner,backing->fields[0].offset)||!binding::identity(owner_actor,next.owner_actor,true))return Code::read;
    if(!roles::class_role(backing->proof,next.owner_actor.cls,L"Actor",backing->fields[0].reference.address))return Code::layout;
    for(unsigned i=0;i<roots.root_count;++i)if(binding::same(next.owner_actor,roots.roots[i].actor))next.owner_roots|=1u<<i;
    output=next;return Code::current;
}
inline Code component_asset(Workspace& w,Snapshot& snapshot,Component& row){
    auto& next=row.physics;next={};
    if(!row.skeletal){next.state=AssetState::nonskeletal;return Code::current;}
    auto* mesh=component_layout(w,row.identity);if(!mesh)return Code::layout;
    std::uintptr_t asset{},instance{};
    if(!binding::field(asset,row.identity.address,mesh->fields[2].offset)||
       !binding::field(instance,row.identity.address,mesh->fields[3].offset))return Code::read;
    // Asset and instance are independent nullable fields. A shared instance's
    // actual Owner is retained; membership does not make this mesh its Owner.
    if(asset&&(!binding::identity(asset,next.asset,true)||
       !roles::class_role(mesh->proof,next.asset.cls,L"PhysicsAsset",mesh->fields[2].reference.address)))return Code::layout;
    if(!instance){next.state=AssetState::instance_field_null;return Code::current;}
    if(!binding::identity(instance,next.instance,true))return Code::read;
    if(!roles::class_role(mesh->proof,next.instance.cls,L"PhysicsAssetInstance",mesh->fields[3].reference.address))return Code::layout;
    auto* l=asset_layout(w,next.instance,mesh->fields[3].reference.address);
    if(!l)return w.asset_layout_count==w.assets.size()?Code::capacity:Code::layout;
    for(unsigned n=0;n<3;++n)next.instance_offsets[n]=l->fields[n].offset;
    std::uintptr_t owner{};
    if(!binding::field(owner,instance,l->fields[0].offset)||!binding::field(next.array,instance,l->fields[1].offset)||
       !binding::field(next.root,instance,l->fields[2].offset))return Code::read;
    if(owner&&(!binding::identity(owner,next.owner,true)||
       !roles::class_role(l->proof,next.owner.cls,l->fields[0].reference.name.data(),l->fields[0].reference.address)))return Code::layout;
    auto const& h=next.array;
    if(h.count<0||h.count>int(list_capacity)||h.capacity<h.count||h.capacity>256||
       (!h.data?(h.count||h.capacity):!source::pointer(h.data,std::size_t((std::max)(1,h.capacity))*8)))return Code::capacity;
    // Optional/disabled components may have an empty body array or no selected
    // root. The strict primary Pawn reader keeps its stronger root contract.
    if(next.root < -1||(next.root>=0&&next.root>=h.count))return Code::layout;
    if(unsigned(h.count)>asset_body_capacity-snapshot.asset_body_count)return Code::capacity;
    next.first_body=snapshot.asset_body_count;
    std::array<std::uintptr_t,list_capacity> members{};
    if(h.count&&!safe_copy(members.data(),reinterpret_cast<void const*>(h.data),std::size_t(h.count)*8))return Code::read;
    for(int n=0;n<h.count;++n){const auto code=asset_body(w,snapshot,members[unsigned(n)],snapshot.asset_bodies[snapshot.asset_body_count]);
        if(code!=Code::current)return code;++snapshot.asset_body_count;}
    next.state=AssetState::instance_present;return Code::current;
}
inline Code component(Workspace& w,Snapshot const& roots,std::uintptr_t at,Component& output){
    Component next{};
    if(!binding::identity(at,next.identity,true))return Code::read;
    auto* layout=component_layout(w,next.identity);if(!layout)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
    std::uintptr_t owner{},body{};
    if(!binding::field(owner,at,layout->fields[0].offset)||!binding::identity(owner,next.owner,true))return Code::read;
    if(!roles::class_role(layout->proof,next.owner.cls,L"Actor",layout->fields[0].reference.address))return Code::layout;
    for(unsigned i=0;i<roots.root_count;++i)if(binding::same(next.owner,roots.roots[i].actor))next.owner_roots|=1u<<i;
    next.primitive=layout->primitive;next.skeletal=layout->skeletal;
    if(!next.primitive){next.body_state=BodyState::nonprimitive;output=next;return Code::current;}
    next.body_instance_offset=layout->fields[1].offset;
    if(!binding::field(body,at,next.body_instance_offset))return Code::read;
    if(!body){next.body_state=BodyState::body_instance_field_null;output=next;return Code::current;}
    if(!binding::identity(body,next.body.identity,true))return Code::read;
    auto* l=roles::body_layout(w.bodies,next.body.identity,nullptr,&w.members);if(!l)return w.bodies.layout_count==w.bodies.layouts.size()?Code::capacity:Code::layout;
    for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)
        if(!separate(l->fields[a].offset,a?8u:4u,l->fields[b].offset,b?8u:4u))return Code::layout;
    if(!roles::class_role(l->proof,next.identity.cls,L"PrimitiveComponent",l->fields[1].reference.address))return Code::layout;
    if(!binding::field(next.body.owner,body,l->fields[1].offset)||!binding::identity(next.body.owner,next.body_owner,true)||
       !binding::field(next.body.index,body,l->fields[0].offset)||!binding::field(next.body.main,body,l->fields[2].offset)||
       !binding::field(next.body.auxiliary,body,l->fields[3].offset)||
       (next.body.main&&!source::pointer(next.body.main,8))||(next.body.auxiliary&&!source::pointer(next.body.auxiliary,8)))return Code::changed;
    if(!roles::class_role(l->proof,next.body_owner.cls,L"PrimitiveComponent",l->fields[1].reference.address))return Code::layout;
    auto* backing=component_layout(w,next.body_owner);if(!backing)return w.layout_count==w.layouts.size()?Code::capacity:Code::layout;
    std::uintptr_t backing_actor{};
    if(!binding::field(backing_actor,next.body.owner,backing->fields[0].offset)||!binding::identity(backing_actor,next.body_owner_actor,true))return Code::read;
    if(!roles::class_role(backing->proof,next.body_owner_actor.cls,L"Actor",backing->fields[0].reference.address))return Code::layout;
    for(unsigned i=0;i<roots.root_count;++i)if(binding::same(next.body_owner_actor,roots.roots[i].actor))next.body_owner_roots|=1u<<i;
    next.body_state=next.body.main?BodyState::body_data_nonnull:BodyState::body_data_field_null;output=next;return Code::current;
}
inline Code enqueue_component(std::array<std::uintptr_t,component_capacity>& queue,unsigned& count,std::uintptr_t at){
    if(!at)return Code::current;if(!source::pointer(at,88))return Code::layout;
    for(unsigned n=0;n<count;++n)if(queue[n]==at)return Code::current;
    if(count==component_capacity)return Code::capacity;queue[count++]=at;return Code::current;
}
inline Code component_attachments(Workspace& w,Snapshot& snapshot,Component& row,std::array<std::uintptr_t,component_capacity>& queue,unsigned& queued){
    auto& next=row.attachments;next={};if(!row.skeletal)return Code::current;
    auto* layout=component_layout(w,row.identity);if(!layout)return Code::layout;
    next.sampled=true;next.structure=layout->attachment_struct;next.offset=layout->fields[4].offset;
    next.stride=layout->attachment_stride;next.component_offset=layout->attachment_fields[0].offset;next.first_row=snapshot.attachment_count;
    if(!binding::field(next.array,row.identity.address,next.offset))return Code::read;auto const& h=next.array;
    if(h.count<0||h.count>int(list_capacity)||h.capacity<h.count||h.capacity>256||
       (!h.data?(h.count||h.capacity):!source::pointer(h.data,std::size_t((std::max)(1,h.capacity))*next.stride))||
       unsigned(h.count)>attachment_capacity-snapshot.attachment_count)return Code::capacity;
    for(int n=0;n<h.count;++n){auto& value=snapshot.attachment_rows[snapshot.attachment_count];value={};
        if(!safe_copy(value.bytes.data(),reinterpret_cast<void const*>(h.data+std::size_t(n)*next.stride),next.stride))return Code::read;
        std::memcpy(&value.component,value.bytes.data()+next.component_offset,sizeof value.component);
        const auto code=enqueue_component(queue,queued,value.component);if(code!=Code::current)return code;++snapshot.attachment_count;
    }return Code::current;
}
inline Code values(Workspace& w,binding::Binding const& context,Snapshot& output,bool repeat=false){
    // Work candidates are already member-owned; do not put the large catalog
    // on the game stack or publish a partial candidate to the caller.
    auto& next=output;next.seed_count=next.root_count=context.weapon.address?2u:1u;
    next.component_count=next.asset_body_count=next.attachment_count=0;next.pawn_mesh_listed=false;next.mesh_seed=context.mesh;
    for(auto& item:next.roots)item={};for(auto& item:next.components)item={};
    for(auto& item:next.asset_bodies)item={};
    for(auto& item:next.attachment_rows)item={};
    std::array<std::uintptr_t,root_capacity> actors{};actors[0]=context.pawn.address;if(next.seed_count==2)actors[1]=context.weapon.address;
    // Breadth-first first-occurrence order terminates cycles while preserving
    // every original array row. An attached node is observed, not declared self.
    for(unsigned i=0;i<next.root_count;++i){const auto code=root(w,i,actors[i],context,next.roots[i],repeat);if(code!=Code::current)return code;
        for(int k=0;k<next.roots[i].attached.count;++k){const auto at=next.roots[i].attached_members[unsigned(k)];if(!at)continue;
            bool duplicate=false;for(unsigned n=0;n<next.root_count;++n)duplicate=duplicate||actors[n]==at;if(duplicate)continue;
            if(next.root_count==root_capacity)return Code::capacity;actors[next.root_count++]=at;}}
    std::array<std::uintptr_t,component_capacity> queue{};unsigned queued{};
    for(unsigned r=0;r<next.root_count;++r)for(unsigned l=0;l<2;++l){auto const& list=next.roots[r];
        for(int i=0;i<list.lists[l].count;++i){const auto at=list.members[l][unsigned(i)];if(!at)continue;
            if(!r&&at==context.mesh.address)next.pawn_mesh_listed=true;
            const auto code=enqueue_component(queue,queued,at);if(code!=Code::current)return code;
        }}
    // The actual Pawn Mesh remains an explicit source even if neither Actor
    // array lists it. This does not change the separate mesh-listed fact.
    auto code=enqueue_component(queue,queued,context.mesh.address);if(code!=Code::current)return code;
    for(unsigned n=0;n<queued;++n){auto& row=next.components[n];code=component(w,next,queue[n],row);if(code!=Code::current)return code;
        if(row.identity.address==context.mesh.address&&!binding::same(row.identity,context.mesh))return Code::changed;
        code=component_asset(w,next,row);if(code!=Code::current)return code;
        code=component_attachments(w,next,row,queue,queued);if(code!=Code::current)return code;++next.component_count;
    }
    return Code::current;
}
inline bool current(Workspace const& w,unsigned roots){
    if(!binding::metadata_current()||w.metadata!=binding::metadata_bytes||w.generation!=binding::metadata_generation)return false;
    if(!w.members.current())return false;
    for(unsigned i=0;i<roots;++i)if(!binding::current(w.roots[i].proof))return false;
    for(unsigned i=0;i<w.layout_count;++i)if(!binding::current(w.layouts[i].proof))return false;
    for(unsigned i=0;i<w.asset_layout_count;++i)if(!binding::current(w.assets[i].proof))return false;
    for(unsigned i=0;i<w.bodies.layout_count;++i)if(!binding::current(w.bodies.layouts[i].proof))return false;
    return true;
}
inline void clear_proof(binding::Proof& proof){
    // Do not create an aggregate containing all eight large Proof objects on
    // the game stack. Zero active lengths/keys: every selected source and
    // identity is re-read this invocation; inactive scratch is never consumed.
    proof.source_count=proof.identity_count=proof.attempts=0;proof.last_attempt=0;
    proof.attempted=proof.ready=false;proof.fields={};proof.keys={};
}
inline void clear(Workspace& work){
    work.recheck_ready=false;
    work.members.clear();
    for(auto& r:work.roots){clear_proof(r.proof);r.fields={};}
    for(auto& l:work.layouts){clear_proof(l.proof);l.cls={};l.fields={};l.attachment_struct={};l.attachment_fields={};l.attachment_stride=0;l.primitive=l.skeletal=false;}
    for(auto& l:work.assets){clear_proof(l.proof);l.cls={};l.fields={};}work.asset_layout_count=0;
    work.layout_count=0;work.component_base={};work.bodies.body_class={};work.bodies.layout_count=0;
    for(auto& l:work.bodies.layouts){clear_proof(l.proof);l.cls={};l.fields={};}
}
template<class Guard> Code sample(binding::Binding const& context,Guard guard,Workspace& work,Snapshot& output)noexcept{
    if(!context.context_valid||!context.world_generation||!context.weapon_known||!context.pawn.address||!context.world.address||!context.mesh.address||
       (!context.weapon.address&&!gears_anchor::equal_identity(context.weapon,gears_anchor::Identity{}))||
       source::aliases(&work,sizeof work,&output,sizeof output)||source::aliases(&work,sizeof work,&context,sizeof context)||
       source::aliases(&output,sizeof output,&context,sizeof context))return Code::request;
    work.recheck_ready=false;
    const auto owner=context;
    try{
        if(!guard())return Code::guard;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||!binding::metadata_current()||context.world_generation!=binding::metadata_generation)return Code::metadata;
            clear(work);
            work.metadata=binding::metadata_bytes;work.generation=binding::metadata_generation;
            auto code=values(work,owner,work.first);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            code=values(work,owner,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!current(work,work.first.root_count))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(owner,context)||owner.scene_invocation!=context.scene_invocation||owner.special_move!=context.special_move)return Code::changed;
        output=work.first;work.sample_owner=owner;work.recheck_ready=true;return Code::current;
    }catch(...){return Code::exception;}
}

// Reuse only the schema of this Session's initial successful sample. Every
// dynamic observation is fresh; complete proofs precede each offsets pass.
// Failure consumes readiness even if the external owner later recovers.
template<class Guard> Code recheck(binding::Binding const& context,Guard guard,Workspace& work,Snapshot& output)noexcept{
    if(!context.context_valid||!context.world_generation||!context.weapon_known||!context.pawn.address||!context.world.address||!context.mesh.address||
       (!context.weapon.address&&!gears_anchor::equal_identity(context.weapon,gears_anchor::Identity{}))||
       source::aliases(&work,sizeof work,&output,sizeof output)||source::aliases(&work,sizeof work,&context,sizeof context)||
       source::aliases(&output,sizeof output,&context,sizeof context))return Code::request;
    const auto owner=context;const bool prepared=work.recheck_ready;work.recheck_ready=false;
    try{
        if(!guard())return Code::guard;
        if(!prepared||!binding::same_key(owner,work.sample_owner)||owner.scene_invocation!=work.sample_owner.scene_invocation||
           owner.special_move!=work.sample_owner.special_move)return Code::changed;
        {
            std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);if(!lock.owns_lock())return Code::metadata;
            if(!binding::context_ready()||context.world_generation!=binding::metadata_generation||!current(work,work.first.root_count))return Code::metadata;
            auto code=values(work,owner,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!current(work,work.first.root_count))return Code::changed;
            code=values(work,owner,work.last,true);if(code!=Code::current)return work.members.exhausted?Code::capacity:code;
            if(!same(work.first,work.last)||!current(work,work.first.root_count))return Code::changed;
        }
        if(!guard())return Code::guard;
        if(!binding::same_key(owner,context)||owner.scene_invocation!=context.scene_invocation||owner.special_move!=context.special_move)return Code::changed;
        output=work.last;work.recheck_ready=true;return Code::current;
    }catch(...){return Code::exception;}
}

#else
struct Workspace {};
template<class Guard> Code sample(binding::Binding const&,Guard,Workspace&,Snapshot&)noexcept{return Code::unavailable;}
template<class Guard> Code recheck(binding::Binding const&,Guard,Workspace&,Snapshot&)noexcept{return Code::unavailable;}
#endif
}
#endif
