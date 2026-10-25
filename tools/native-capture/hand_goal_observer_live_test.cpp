// Real adapter/profile/solver/Core and pinned RO3 inputs. Host Core, Phase,
// CCTRL, native owner, identity and memory endpoints are explicitly stubs.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <mutex>
#include "hand_core_target_adapter.h"
#include "cover_pose_phase.h"
#ifndef GEARS_HAND_GOAL_OBSERVER
#define GEARS_HAND_GOAL_OBSERVER 1
#endif
#ifndef GEARS_COVER_NATIVE_RIG_OBSERVER
#define GEARS_COVER_NATIVE_RIG_OBSERVER 1
#endif
#ifndef GEARS_COVER_BODY_PROTOTYPE
#define GEARS_COVER_BODY_PROTOTYPE 1
#endif
#ifndef GEARS_COVER_CONTROL_OBSERVER
#define GEARS_COVER_CONTROL_OBSERVER 1
#endif
unsigned checks{},failures{};
void check(bool okay,char const* expr){++checks;if(!okay){++failures;fprintf(stderr,"FAIL %s\n",expr);}}
#define C(x) check(bool(x),#x)
namespace a=gears_hand_core_target;namespace co=gears_cover_control_output_pose;namespace m=gears_hand_authored_mount;
namespace v=gears_vr;namespace r=gears_rig_tracking;namespace core=gears_native_rig_tracking;namespace p=gears_cover_pose_phase;
namespace gears_anchor {struct Identity {uintptr_t address{},cls{},outer{};unsigned index{},name_index{},number{};};}
namespace weapon_hand_scene_binding {
std::mutex mutex;bool metadata_ok=true,identity_ok=true,change_asset{};unsigned identity_reads{};gears_anchor::Identity asset{};
bool metadata_current(){return metadata_ok;}
bool same(gears_anchor::Identity const& x,gears_anchor::Identity const& y){return x.address==y.address&&x.cls==y.cls&&x.outer==y.outer&&x.index==y.index&&x.name_index==y.name_index&&x.number==y.number;}
bool identity(uintptr_t at,gears_anchor::Identity& out,bool){++identity_reads;if(change_asset&&identity_reads==3)asset.number++;if(!identity_ok||at!=asset.address)return false;out=asset;return true;}
}
namespace cover_pose_phase_live {
bool enabled=true;struct Owner {p::OwnerKey key{};unsigned thread{};};
struct Scope {Owner owner{};uint64_t invocation{};unsigned kind{},episode{};bool arguments{};};
struct Row {bool snapshot_valid{};p::Snapshot snapshot{};};
struct Episode {p::OwnerKey owner{};uint64_t invocation{};unsigned thread{};bool closed{},exception{},nested{},concurrent{};std::array<Row,1> rows{};};
std::mutex mutex;std::array<Episode,1> episodes{};unsigned episode_count=1;
bool same(p::Identity const& x,p::Identity const& y){return x.address==y.address&&x.cls==y.cls&&x.outer==y.outer&&x.internal_index==y.internal_index&&x.name_index==y.name_index&&x.number==y.number;}
bool same(p::OwnerKey const& x,p::OwnerKey const& y){return same(x.world,y.world)&&same(x.pawn,y.pawn)&&same(x.mesh,y.mesh)&&x.world_generation==y.world_generation&&x.session==y.session&&x.scene==y.scene&&x.scene_qpc==y.scene_qpc;}
}
namespace cover_control_observer_live {
bool enabled=true,current_ok=true;enum class Failure:unsigned {pending,okay};
struct Node {p::Identity node{};p::Metadata metadata{};};
struct Row {p::OwnerKey owner{};uint64_t compose{},outer_tick{},ordinal{},compute_ordinal{};int64_t before_qpc{},after_qpc{};
    uintptr_t node_argument{},mesh_argument{},raw_rdx{};unsigned episode{},kind{},thread{};Failure failure{Failure::pending};Node before{},after{};p::ArrayHeader affected_before{};
    std::array<int32_t,3> affected{};std::array<p::Matrix,3> matrices{};bool before_valid{},after_valid{},returned{},exception{},nested{},concurrent{},compose_exception{};};
struct Compose {cover_pose_phase_live::Scope scope{};uint64_t outer_tick{};bool admitted{},nested{};};
struct Thread {std::array<Compose,1> frames{};unsigned depth=1,overflow{};};inline thread_local Thread thread{};
bool current(Compose const&){return current_ok;}
bool same_metadata(p::Metadata const& x,p::Metadata const& y){return x.asset==y.asset&&x.master==y.master&&x.physics_asset==y.physics_asset&&x.physics_instance==y.physics_instance&&
    p::header_equal(x.reference,y.reference)&&p::header_equal(x.local,y.local)&&p::header_equal(x.space,y.space)&&p::header_equal(x.desired,y.desired)&&!memcmp(&x.component,&y.component,sizeof(p::Matrix));}
}
namespace cover_body_live {bool enabled{};}
namespace cover_input_live {bool enabled{};}
namespace weapon_hand_runtime {bool enabled=true;namespace detail {bool engine_restore_enabled{};}}
namespace rig_tracking_live {
bool enabled=true;int64_t clock_value=2000000000;
int64_t clock_ns(){return clock_value;}
gears_weapon_hand::ObjectIdentity object(gears_anchor::Identity const& x){return {x.address,x.outer,x.cls,x.index,x.name_index,x.number,1};}
struct Request {core::NativeSessionKey native{};gears_weapon_hand::ObjectIdentity world{},pawn{},mesh{};uint64_t world_generation{},scene_invocation{};int64_t scene_qpc{};};
bool owner_matches(Request const& q,core::CoreSnapshot const& s){return core::same(q.native,s.native)&&gears_weapon_hand::same(q.world,s.world)&&gears_weapon_hand::same(q.pawn,s.pawn)&&gears_weapon_hand::same(q.mesh,s.mesh)&&q.world_generation==s.world_generation&&q.scene_invocation==s.scene_invocation&&q.scene_qpc==s.scene_qpc;}
}
namespace cover_native_rig_observer_live {
bool enabled=true;std::atomic<bool> stopping{};bool owner_ok=true,closing_owner_change{};unsigned owner_checks{};
struct Lease {bool world()const{return true;}} lease;
struct Frame {bool active=true,sticky{},attempted=true;unsigned episode=1;uint64_t invocation=800;cover_pose_phase_live::Owner owner{};
    struct Key {rig_tracking_live::Request request{};} key;Lease* lease=&cover_native_rig_observer_live::lease;};inline thread_local Frame frame{};
bool current_owner(cover_pose_phase_live::Owner const&){++owner_checks;return owner_ok&&!(closing_owner_change&&owner_checks==2);}
}
bool continuous_session=true,stereo_xr=true,anchor_observer_enabled=true,simulator_operator{};wchar_t directory[MAX_PATH]{};
constexpr uintptr_t fixture_mesh=0x200000,asset_address=0x210000,reference_address=0x220000;
std::array<unsigned char,0x400> mesh_memory{};std::array<unsigned char,0x200> asset_memory{};
std::array<m::ReferenceRecord68,115> reference{};bool memory_ok=true,closing_metadata_change{},closing_reference_change{},closing_age_change{};unsigned metadata_round{},memory_reads{};
bool within(uintptr_t at,size_t n,uintptr_t first,size_t count){return at>=first&&at-first<=count&&n<=count-(at-first);}
bool safe_copy(void* out,void const* address,size_t n){++memory_reads;if(!memory_ok)return false;const auto at=reinterpret_cast<uintptr_t>(address);
    if(at==fixture_mesh+0x218){++metadata_round;if(metadata_round==2&&closing_metadata_change){const uintptr_t master=9;memcpy(mesh_memory.data()+0x29c,&master,8);}
        if(metadata_round==2&&closing_age_change)rig_tracking_live::clock_value+=50000001;}
    if(within(at,n,fixture_mesh,mesh_memory.size())){memcpy(out,mesh_memory.data()+at-fixture_mesh,n);return true;}
    if(within(at,n,asset_address,asset_memory.size())){memcpy(out,asset_memory.data()+at-asset_address,n);return true;}
    if(within(at,n,reference_address,sizeof reference)){memcpy(out,reinterpret_cast<unsigned char const*>(reference.data())+at-reference_address,n);if(closing_reference_change&&metadata_round==2&&at==reference_address)static_cast<unsigned char*>(out)[0]^=1;return true;}return false;}
#include "hand_goal_observer_live.inl"
namespace live=hand_goal_observer_live;namespace phase=cover_pose_phase_live;namespace controls=cover_control_observer_live;namespace native=cover_native_rig_observer_live;namespace binding=weapon_hand_scene_binding;
struct Observed {co::Matrix64 limb[3]{},component{},single{};};Observed observed[2]{};core::CoreSnapshot host_source{};
bool exact_file(char const* path,void* out,size_t n){std::ifstream f(path,std::ios::binary);f.read(static_cast<char*>(out),std::streamsize(n));return bool(f)&&f.peek()==std::char_traits<char>::eof();}
bool fixtures(char const* matrices,char const* refs){std::ifstream f(matrices,std::ios::binary);char magic[8]{};uint32_t count{},stride{};
    f.read(magic,8);f.read(reinterpret_cast<char*>(&count),4);f.read(reinterpret_cast<char*>(&stride),4);f.read(reinterpret_cast<char*>(observed),sizeof observed);
    return bool(f)&&!memcmp(magic,"HCTRO3V1",8)&&count==2&&stride==sizeof(Observed)&&f.peek()==std::char_traits<char>::eof()&&exact_file(refs,reference.data(),sizeof reference);}
void write_ini(wchar_t const* path,wchar_t const* key,wchar_t const* value){C(WritePrivateProfileStringW(L"HandGoalObserver",key,value,path)!=0);}
#if GEARS_HAND_GOAL_OBSERVER && GEARS_COVER_NATIVE_RIG_OBSERVER && GEARS_COVER_BODY_PROTOTYPE && GEARS_COVER_CONTROL_OBSERVER
v::Pose world(co::Matrix64 const& matrix,co::Matrix64 const& transform){v::Pose value{};C(co::ik::world_pose(co::ik::detail::multiply(co::detail::unpack(matrix),co::detail::unpack(transform)),value)==co::ik::Status::okay);return value;}
gears_weapon_hand::ObjectIdentity object(uint64_t at){return {at,0x9000,0xa000,1,2,0,1};}
core::CoreSnapshot host_core(){core::CoreSnapshot s{};const auto now=rig_tracking_live::clock_value;s.status=r::Status::ready;s.flags=r::controllers_matched;
    s.native={GetCurrentProcessId(),GetCurrentThreadId(),12,13};s.publication_sequence=17;s.scene_invocation=90;s.world_generation=7;s.binding_generation=20;
    s.reference_generation=21;s.published_ns=now-500000;s.scene_qpc=now-2000000;s.qpc_frequency=1000000000;s.observed={2,3,4,{5,6}};s.native_reference={10,r::reference_stable,0};
    s.controller_publisher_pid=200;s.controller_publisher_start=201;s.view_policy=r::first_person;s.world=object(0x1000);s.controller=object(0x2000);s.pawn=object(0x3000);s.mesh=object(fixture_mesh);
    s.units_per_metre=100;s.source_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};s.base_view=s.source_view;s.stereo.source=v::StereoSource::RuntimeTracked;
    s.stereo.session=22;s.stereo.sequence=23;s.stereo.received_ns=now-1000000;s.stereo.predicted_time=123456;s.stereo.head_flags=15;s.stereo.view_flags=15;s.stereo.focused=1;s.stereo.should_render=1;s.stereo.space_epoch=24;
    s.controllers.source=s.stereo.source;s.controllers.session=s.stereo.session;s.controllers.sequence=s.stereo.sequence;s.controllers.space_epoch=s.stereo.space_epoch;
    s.controllers.profile_generation=25;s.controllers.input_epoch=26;s.controllers.predicted_time=s.stereo.predicted_time;s.controllers.received_ns=now-1100000;s.controllers.sync_ns=now-1200000;s.controllers.focused=1;
    for(auto& hand:s.controllers.hands){hand.grip_active=1;hand.grip_flags=15;}s.world_pose_mask=r::head_pose|r::left_grip_pose|r::right_grip_pose;s.source_ticket=27;s.native_reference_revision=28;return s;}
void grip_for(unsigned side,v::Pose const& wrist){m::Mount mount{};C(m::marcus115(m::marcus115_signature,static_cast<m::Side>(side),1,host_source.units_per_metre,mount));
    const auto q=v::normalized(v::multiply(wrist.orientation,v::inverse(mount.calibration.grip_to_wrist_rotation)));const auto metres=mount.calibration.grip_to_wrist_metres;
    const auto delta=v::transform(v::rotation_matrix(q),{metres.x*host_source.units_per_metre,metres.y*host_source.units_per_metre,metres.z*host_source.units_per_metre});host_source.world_grip[side]={{wrist.position.x-delta.x,wrist.position.y-delta.y,wrist.position.z-delta.z},q};}
void memory(p::Metadata const& metadata){auto put=[](auto& buffer,size_t at,auto const& value){memcpy(buffer.data()+at,&value,sizeof(value));};
    put(mesh_memory,0x218,metadata.asset);put(mesh_memory,0x29c,metadata.master);put(mesh_memory,0x248,metadata.physics_asset);put(mesh_memory,0x250,metadata.physics_instance);
    put(mesh_memory,0x27c,metadata.local);put(mesh_memory,0x26c,metadata.space);put(mesh_memory,0x28c,metadata.desired);put(mesh_memory,0xa0,metadata.component);put(asset_memory,0xa4,metadata.reference);}
void setup(){
    live::enabled=true;host_source=host_core();binding::asset={asset_address,0x7000,0x8000,2,3,0};
    auto& f=native::frame;auto& k=f.owner.key;k.world={host_source.world.address,host_source.world.klass,host_source.world.outer,1,2,0};k.pawn={host_source.pawn.address,host_source.pawn.klass,host_source.pawn.outer,1,2,0};
    k.mesh={host_source.mesh.address,host_source.mesh.klass,host_source.mesh.outer,1,2,0};k.world_generation=7;k.session=13;k.scene=90;k.scene_qpc=host_source.scene_qpc;f.owner.thread=GetCurrentThreadId();
    f.key.request={host_source.native,host_source.world,host_source.pawn,host_source.mesh,7,90,host_source.scene_qpc};
    auto& c=controls::thread.frames[0];c.admitted=true;c.outer_tick=800;c.scope.owner=f.owner;c.scope.kind=0;c.scope.invocation=801;c.scope.episode=1;c.scope.arguments=true;
    auto& e=phase::episodes[0];e.owner=k;e.invocation=800;e.thread=GetCurrentThreadId();e.rows[0].snapshot_valid=true;auto& snapshot=e.rows[0].snapshot;
    snapshot.request={k,801,host_source.scene_qpc,GetCurrentThreadId(),p::compose_caller,p::Kind::compose,p::Phase::C0,0};auto& md=snapshot.metadata;md.asset=asset_address;
    md.reference={reference_address,115,115};md.local={0x230000,115,115};md.space={0x240000,115,115};md.desired={0x250000,115,115};memcpy(&md.component,&observed[0].component,64);
    memcpy(snapshot.reference.data(),reference.data(),sizeof reference);memory(md);
    for(unsigned side=0;side<2;++side){auto wrist=world(observed[side].limb[2],observed[side].component);wrist.orientation=v::normalized(v::multiply(v::Quaternion{.1,-.2,.3,.9},wrist.orientation));grip_for(side,wrist);}
    C(core::core_snapshot_valid(host_source,rig_tracking_live::clock_value,50000000));
}
controls::Row row(unsigned side,unsigned kind,unsigned ordinal){controls::Row x{};x.owner=native::frame.owner.key;x.compose=801;x.outer_tick=800;x.ordinal=x.compute_ordinal=ordinal;x.episode=1;x.kind=kind;x.thread=GetCurrentThreadId();x.failure=controls::Failure::okay;
    x.node_argument=0x500000+side*0x100+kind*0x10;x.mesh_argument=fixture_mesh;x.raw_rdx=side?35:11;x.before.node={x.node_argument,0x600000,0x610000,int32_t(30+side*2+kind),40,0};x.after.node=x.before.node;
    x.before.metadata=x.after.metadata=phase::episodes[0].rows[0].snapshot.metadata;x.before_valid=x.after_valid=x.returned=true;x.affected_before.count=kind?1:3;
    if(kind){x.affected[0]=int32_t(x.raw_rdx);memcpy(&x.matrices[0],&observed[side].single,64);}else{x.affected=side?std::array<int32_t,3>{{32,34,35}}:std::array<int32_t,3>{{8,10,11}};memcpy(x.matrices.data(),observed[side].limb,192);}return x;}
void four(){for(unsigned side=0;side<2;++side)for(unsigned kind=0;kind<2;++kind){const auto x=row(side,kind,1+side*2+kind);const auto before=x;live::observe(x,host_source);C(!memcmp(&x,&before,sizeof x));}}
void positive_checks(){C(live::record_count==4&&live::planned==4&&live::preparations[0]==1&&live::preparations[1]==1&&!live::failed&&live::pending_count()==0);
    for(unsigned side=0;side<2;++side){const auto& limb=live::records[side*2];const auto& single=live::records[side*2+1];C(limb.failure==live::Failure::none&&single.failure==live::Failure::none);C(!memcmp(&limb.wrist_world,&single.wrist_world,sizeof(v::Pose)));C(!memcmp(&limb.source,&single.source,sizeof host_source));
        const auto solved=world(limb.planned[2],limb.component_to_world);C(std::hypot(std::hypot(solved.position.x-limb.wrist_world.position.x,solved.position.y-limb.wrist_world.position.y),solved.position.z-limb.wrist_world.position.z)<.0002);
        for(unsigned at:{3u,7u,11u,12u,13u,14u,15u})C(!memcmp(single.original[0].values+at,single.planned[0].values+at,4));C(single.flags&4);
    }}
void abi(){printf("{\"Header\":%zu,\"Record\":%zu,\"CoreSnapshot\":%zu,\"Header.native\":%zu,\"Header.profile_signature\":%zu,\"Header.profile_id\":%zu,\"Record.source\":%zu,\"Record.original\":%zu,\"Record.planned\":%zu,\"Record.component_to_world\":%zu,\"Record.wrist_world\":%zu,\"Record.geometry_signature\":%zu,\"Record.asset\":%zu,\"Record.failure\":%zu}\n",sizeof(live::Header),sizeof(live::Record),sizeof(core::CoreSnapshot),offsetof(live::Header,native),offsetof(live::Header,profile_signature),offsetof(live::Header,profile_id),offsetof(live::Record,source),offsetof(live::Record,original),offsetof(live::Record,planned),offsetof(live::Record,component_to_world),offsetof(live::Record,wrist_world),offsetof(live::Record,geometry_signature),offsetof(live::Record,asset),offsetof(live::Record,failure));
    printf("{\"HeaderOffsets\":{");
#define H(n) printf("\"" #n "\":%zu,",offsetof(live::Header,n))
    H(magic);H(version);H(header_size);H(record_size);H(count);H(capacity);H(read_only);H(writer);H(atomic_verified);H(native);H(outer_tick);H(scene);H(world_generation);H(scene_qpc);H(episode);H(mode);H(failed);H(reserved);H(profile_signature);
#undef H
    printf("\"profile_id\":%zu},\"RecordOffsets\":{",offsetof(live::Header,profile_id));
#define R(n) printf("\"" #n "\":%zu,",offsetof(live::Record,n))
    R(outer_tick);R(compose);R(ordinal);R(node);R(observed_ns);R(before_qpc);R(after_qpc);R(episode);R(kind);R(side);R(hand_bone);R(mode);R(original_count);R(flags);R(failure);R(prepare_status);R(math_status);R(asset);R(geometry_signature);R(wrist_world);R(original);R(planned);R(component_to_world);
#undef R
    printf("\"source\":%zu}}\n",offsetof(live::Record,source));
}
int run(char const* mode){
    if(!strcmp(mode,"abi")){abi();return 0;}
    if(!strcmp(mode,"configure")){wchar_t temp[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);C(GetTempFileNameW(temp,L"hgp",0,ini)>0);C(!live::enabled&&live::configure(ini,nullptr));
        write_ini(ini,L"enabled",L"1");write_ini(ini,L"length_mode",L"unknown");C(!live::configure(ini,nullptr));write_ini(ini,L"length_mode",L"adaptive");
        bool* gates[]={&native::enabled,&phase::enabled,&controls::enabled,&rig_tracking_live::enabled,&weapon_hand_runtime::enabled,&continuous_session,&stereo_xr,&anchor_observer_enabled};
        for(auto gate:gates){*gate=false;C(!live::configure(ini,nullptr)&&!live::enabled);*gate=true;}
        for(auto writer:{&cover_body_live::enabled,&cover_input_live::enabled,&weapon_hand_runtime::detail::engine_restore_enabled}){*writer=true;C(!live::configure(ini,nullptr)&&!live::enabled);*writer=false;}
        C(live::configure(ini,nullptr)&&live::length_mode==live::LengthMode::adaptive);C(!live::configure(ini,nullptr));C(DeleteFileW(ini)!=0);return failures?1:0;}
    setup();
    if(!strcmp(mode,"adaptive")){live::length_mode=live::LengthMode::adaptive;for(unsigned s=0;s<2;++s){auto goal=world(observed[s].limb[2],observed[s].component);goal.position.x+=100;grip_for(s,goal);}}
    if(!strcmp(mode,"sim")){simulator_operator=true;host_source.stereo.source=v::StereoSource::MetaOperatorSimulated;host_source.stereo.head_flags=3;host_source.controllers.source=host_source.stereo.source;}
    if(!strcmp(mode,"epsilon"))for(unsigned side=0;side<2;++side)observed[side].single.values[15]=1+(side?2:-2)*std::numeric_limits<float>::epsilon();
    if(!strcmp(mode,"head-only")){host_source.flags=0;host_source.world_pose_mask=r::head_pose;host_source.controller_publisher_pid=0;host_source.controller_publisher_start=0;}
    if(!strcmp(mode,"untracked")){host_source.controllers.hands[0].grip_active=0;host_source.world_pose_mask&=~r::left_grip_pose;}
    if(!strcmp(mode,"signature")){float translation{};memcpy(&translation,reference[10].bytes+28,4);translation+=.25f;memcpy(reference[10].bytes+28,&translation,4);memcpy(phase::episodes[0].rows[0].snapshot.reference.data(),reference.data(),sizeof reference);}
    if(!strcmp(mode,"owner"))native::owner_ok=false;
    if(!strcmp(mode,"closing-owner"))native::closing_owner_change=true;
    if(!strcmp(mode,"closing-asset"))binding::change_asset=true;
    if(!strcmp(mode,"closing-metadata"))closing_metadata_change=true;
    if(!strcmp(mode,"closing-reference"))closing_reference_change=true;
    if(!strcmp(mode,"closing-age"))closing_age_change=true;
    if(!strcmp(mode,"c0-invalid"))phase::episodes[0].rows[0].snapshot_valid=false;
    if(!strcmp(mode,"scale")){auto& md=phase::episodes[0].rows[0].snapshot.metadata;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)md.component.values[i*4+j]*=2;memory(md);}
    if(!strcmp(mode,"unreachable")){auto goal=world(observed[0].limb[2],observed[0].component);goal.position.x+=100;grip_for(0,goal);}
    if(!strcmp(mode,"affine"))observed[0].single.values[3]=.01f;
    if(!strcmp(mode,"tail"))observed[0].single.values[15]=1+3*std::numeric_limits<float>::epsilon();
    if(!strcmp(mode,"stop"))live::request_drain();
    if(!strcmp(mode,"alias")){const auto x=row(0,0,1);live::observe(x,live::workspace.source);C(live::failed&&live::record_count==0&&live::preparations[0]==0);return failures?1:0;}
    if(!strcmp(mode,"source-changed")){live::observe(row(0,0,1),host_source);C(live::planned==1);host_source.stereo.sequence++;live::observe(row(0,1,2),host_source);C(live::records[1].failure==live::Failure::source_changed&&live::preparations[0]==1&&live::planned==1);return failures?1:0;}
    if(!strcmp(mode,"role")){auto bad=row(0,0,1);bad.affected[0]=32;live::observe(bad,host_source);C(live::records[0].failure==live::Failure::role&&live::planned==0);return failures?1:0;}
    four();C(live::record_count==4&&live::pending_count()==0);
    if(!strcmp(mode,"normal")||!strcmp(mode,"sim")||!strcmp(mode,"epsilon")||!strcmp(mode,"adaptive")||!strcmp(mode,"file")||!strcmp(mode,"preserve-file"))positive_checks();
    else if(!strcmp(mode,"untracked")){C(live::planned==2&&live::preparations[0]==1&&live::preparations[1]==1);C(live::records[0].prepare_status==a::Status::hand_untracked&&live::records[1].prepare_status==a::Status::hand_untracked);}
    else if(!strcmp(mode,"head-only")){C(live::planned==0&&live::preparations[0]==1&&live::preparations[1]==1);C(live::records[0].failure==live::Failure::preparation);}
    else {if(!strcmp(mode,"signature"))C(live::records[3].failure==live::Failure::geometry);if(!strcmp(mode,"stop"))C(live::records[3].failure==live::Failure::stopped);C(live::planned==(!strcmp(mode,"affine")||!strcmp(mode,"tail")?1u:0u));C(live::rejected>0);}
    if(!strcmp(mode,"closing-owner")||!strcmp(mode,"closing-asset")||!strcmp(mode,"closing-metadata")||!strcmp(mode,"closing-reference")||!strcmp(mode,"closing-age"))for(auto const& planned:live::records[0].planned){co::Matrix64 zero{};C(!memcmp(&planned,&zero,sizeof zero));}
    if(!strcmp(mode,"file")||!strcmp(mode,"preserve-file")){
        wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);swprintf_s(directory,L"%sHGOAL-%lu-%llu",temp,GetCurrentProcessId(),GetTickCount64());C(CreateDirectoryW(directory,nullptr)!=0);swprintf_s(path,L"%s\\hand-target-plans.bin",directory);
        const bool preserve=!strcmp(mode,"preserve-file");if(preserve){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);DWORD n{};C(WriteFile(f,"KEEP",4,&n,nullptr)&&n==4);C(CloseHandle(f)!=0);}
        live::print(nullptr,true);C(!live::file_attempted);live::request_drain();live::print(nullptr,false);C(!live::file_attempted);live::print(nullptr,true);C(live::file_attempted);
        HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);C(f!=INVALID_HANDLE_VALUE);DWORD n{};
        if(preserve){char text[4]{};C(!live::file_written&&(live::file_error==ERROR_FILE_EXISTS||live::file_error==ERROR_ALREADY_EXISTS));C(ReadFile(f,text,4,&n,nullptr)&&n==4&&!memcmp(text,"KEEP",4));}
        else{live::Header h{};C(live::file_written&&ReadFile(f,&h,sizeof h,&n,nullptr)&&n==sizeof h);C(!memcmp(h.magic,"HGOAL001",8)&&h.read_only==1&&!h.writer&&!h.atomic_verified&&h.count==4&&h.record_size==sizeof(live::Record));C(GetFileSize(f,nullptr)==sizeof h+4*sizeof(live::Record));}
        C(CloseHandle(f)!=0);live::print(nullptr,true);C(DeleteFileW(path)!=0);C(RemoveDirectoryW(directory)!=0);
    }return failures?1:0;
}
#else
int run(char const*){wchar_t temp[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);C(GetTempFileNameW(temp,L"hgp",0,ini)>0);C(live::configure(ini,nullptr)&&!live::enabled);write_ini(ini,L"enabled",L"1");C(!live::configure(ini,nullptr));controls::Row x{};live::observe(x,host_source);live::request_drain();C(!live::pending_count());C(DeleteFileW(ini)!=0);return failures?1:0;}
#endif
int main(int argc,char** argv){if(argc!=4||!fixtures(argv[2],argv[3])){fprintf(stderr,"usage: test MODE RO3-MATRICES.bin NATIVE-REFERENCE-RO3.bin\n");return 2;}
    const auto result=run(argv[1]);printf("hand_goal_observer %s: %u checks, %u failures; actual RO3 matrices/Ref + real Core/profile/adapter/math; Host owner/IO/scope stubs, no permission\n",argv[1],checks,failures);return result;}
