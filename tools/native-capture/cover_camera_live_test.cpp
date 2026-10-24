// Synthetic end-to-end neutral-provider -> preview -> stereo-ticket tests.
// Only a private arena is readable. There is no game/process/hook operation.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef GEARS_COVER_BODY_PROTOTYPE
#undef GEARS_COVER_BODY_PROTOTYPE
#endif
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <windows.h>
#include <array>
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <functional>
#include <stdexcept>
#include <string>
#include "anchor_observer.h"
#include "../vr-common/pose_math.h"
#include "MinHook.h"
uintptr_t base=0x140000000;
std::atomic<uint64_t> committed_world_generation{1};std::atomic<unsigned> active{};
std::atomic<bool> recording{true};bool continuous_session=true,anchor_observer_enabled=true,stereo_xr=true;
double stereo_units_per_metre=100;gears_anchor::Config anchor_config;std::mutex stereo_mutex;
struct Scope {bool world()const{return true;}};
uintptr_t arena{},denied{};unsigned reads{};
std::function<void(uintptr_t,size_t)> read_hook;
bool safe_copy(void* out,void const* from,size_t size){
    const auto address=reinterpret_cast<uintptr_t>(from);++reads;
    if(!address||address==denied||address<arena||size>1024*1024||address>arena+1024*1024-size)return false;
    memcpy(out,from,size);if(read_hook)read_hook(address,size);return true;
}
long long now(){return static_cast<long long>(GetTickCount64());}
extern "C" MH_STATUS WINAPI MH_CreateHook(void*,void*,void**){return MH_ERROR_UNSUPPORTED_FUNCTION;}
#include "cover_body_live.inl"
bool entries_valid=true;
namespace cover_body_live {bool own_entries(){return entries_valid;}}
extern "C" void GearsCoverBlendTickHook(){}
extern "C" void GearsCoverChildPropagateHook(){}
extern "C" void GearsCoverRawInvoke(cover_body_live::CallPacket*,void*) noexcept(false){}
#include "cover_neutral_eye_live.inl"
#include "cover_camera_live.inl"
namespace b=cover_body_live;namespace n=cover_neutral_eye_live;namespace camera=cover_camera_live;
unsigned checks{},failures{},queries{};uint64_t clock_ms{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
uintptr_t fixture_pawn{},fixture_mesh{},fixture_link{},fixture_world{},pawn_class{},link_class{},fixture_actor_class{},fixture_world_class{},fixture_pc_class{},fixture_world_prop{},props[5]{};
gears_anchor::Receipt fixture_scene;gears_vr::Vector head_offset{2,3,35};
template<class T>void put(uintptr_t address,T const& value){memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
template<class T>T get(uintptr_t address){T value{};memcpy(&value,reinterpret_cast<void const*>(address),sizeof value);return value;}
gears_anchor::Identity object(uintptr_t address,unsigned index,uintptr_t cls,uintptr_t outer,const wchar_t* name){
    gears_anchor::Identity id{};id.address=address;id.index=id.name_index=index;id.cls=cls;id.outer=outer;wcscpy_s(id.name.data(),id.name.size(),name);
    put(address+8,id.index);put(address+72,id.name_index);put(address+76,id.number);put(address+80,id.cls);put(address+64,id.outer);
    auto objects=get<gears_anchor::Registry>(b::metadata.objects_header),names=get<gears_anchor::Registry>(b::metadata.names_header);
    put(objects.data+uintptr_t(index)*8,address);auto entry=arena+0x20000+uintptr_t(index)*0x100;put(names.data+uintptr_t(index)*8,entry);put(entry,index);put(entry+20,id.name);return id;
}
void refresh_scene(){
    gears_anchor::PawnBinding binding{};C(gears_anchor::capture_pawn_binding(b::metadata,safe_copy,binding));fixture_scene={};fixture_scene.invocation=7;fixture_scene.thread=GetCurrentThreadId();fixture_scene.scene_qpc=1;fixture_scene.family_qpc=2;fixture_scene.renderer_qpc=3;
    fixture_scene.anchor.pawn=binding.pawn.address;fixture_scene.anchor.pawn_identity=binding.pawn;fixture_scene.anchor.controller=binding.controller;fixture_scene.anchor.camera=binding.camera;fixture_scene.anchor.level=binding.level;
    fixture_scene.anchor.location=get<std::array<float,3>>(fixture_pawn+b::metadata.pawn_location);fixture_scene.anchor.eye_height=76;
}
void setup(){
    read_hook={};memset(reinterpret_cast<void*>(arena),0,1024*1024);auto& config=b::metadata;config={};config.objects_header=arena+0x100;config.names_header=arena+0x200;
    config.object_internal_index=8;config.object_name=72;config.object_class=80;config.object_outer=64;config.field_super=96;config.name_entry_index=0;config.name_entry_text=20;config.identity_count=5;
    config.role_controller=0;config.role_local=1;config.role_camera=2;config.role_pawn=3;config.role_level=4;config.controller_pawn=config.pawn_controller=0x100;config.player_camera=config.pc_owner=0x108;config.player=0x110;config.camera_view_target=0x100;config.pawn_location=0x1a0;config.pawn_rotation=0x1c0;
    put(config.objects_header,gears_anchor::Registry{arena+0x1000,2001,2100});put(config.names_header,gears_anchor::Registry{arena+0x6000,2001,2100});
    auto meta=arena+0x60000,generic=arena+0x60400;object(meta,10,meta,0,L"Class");object(generic,11,meta,0,L"Object");
    pawn_class=arena+0x60800;object(pawn_class,12,meta,0,L"WarPawn");put(pawn_class+0x88,0x1000u);
    auto mesh_class=arena+0x60c00;object(mesh_class,13,meta,0,L"SkeletalMeshComponent");link_class=arena+0x61000;object(link_class,14,meta,0,L"CoverLink");
    const wchar_t* types[]={L"ObjectProperty",L"IntProperty",L"ByteProperty",L"BoolProperty"};uintptr_t type_classes[4]{};
    for(unsigned i=0;i<4;++i){type_classes[i]=arena+0x61400+uintptr_t(i)*0x400;object(type_classes[i],15+i,meta,0,types[i]);}
    fixture_actor_class=arena+0x62400;fixture_world_class=arena+0x62800;fixture_pc_class=arena+0x62c00;fixture_world_prop=arena+0x63000;
    object(fixture_actor_class,19,meta,0,L"Actor");object(fixture_world_class,20,meta,0,L"WorldInfo");object(fixture_pc_class,21,meta,0,L"WarPC");
    for(auto cls:{pawn_class,link_class,fixture_pc_class,fixture_world_class}){put(cls+0x60,fixture_actor_class);put(cls+0x88,0x1000u);}put(fixture_actor_class+0x60,generic);put(fixture_actor_class+0x88,0x1000u);
    object(fixture_world_prop,22,type_classes[0],fixture_actor_class,L"WorldInfo");put(fixture_actor_class+0x80,fixture_world_prop);put(fixture_world_prop+0x70,1u);put(fixture_world_prop+0x74,8u);put(fixture_world_prop+0x94,0x188u);put(fixture_world_prop+0xd0,fixture_world_class);
    auto level=arena+0x38000;const uintptr_t addresses[]={arena+0x32000,arena+0x34000,arena+0x36000,arena+0x30000,level};
    const wchar_t* labels[]={L"SyntheticPC",L"SyntheticLocal",L"SyntheticCamera",L"SyntheticPawn",L"SyntheticLevel"};
    for(unsigned i=0;i<5;++i)config.identities[i]=object(addresses[i],i,i==3?pawn_class:(i==0?fixture_pc_class:generic),i==3?level:0,labels[i]);
    fixture_pawn=addresses[3];fixture_world=arena+0x3a000;fixture_mesh=arena+0x3c000;fixture_link=arena+0x3e000;
    object(fixture_world,5,fixture_world_class,level,L"SyntheticWorld");object(fixture_mesh,6,mesh_class,fixture_pawn,L"SyntheticMesh");object(fixture_link,7,link_class,level,L"SyntheticLink");
    put(addresses[0]+config.controller_pawn,fixture_pawn);put(addresses[0]+config.player_camera,addresses[2]);put(addresses[0]+config.player,addresses[1]);
    put(fixture_pawn+config.pawn_controller,addresses[0]);put(addresses[2]+config.camera_view_target,fixture_pawn);put(addresses[2]+config.pc_owner,addresses[0]);
    b::layout={};b::layout.mesh=0x180;b::layout.world=0x188;b::layout.cover=0x190;b::layout.special=0x191;
    put(fixture_link+0x188,fixture_world);put(addresses[0]+0x188,fixture_world);
    put(fixture_pawn+0x180,fixture_mesh);put(fixture_pawn+0x188,fixture_world);put(fixture_pawn+0x190,static_cast<unsigned char>(2));put(fixture_pawn+0x191,static_cast<unsigned char>(0));put(fixture_pawn+config.pawn_location,std::array<float,3>{100,200,300});put(fixture_pawn+config.pawn_rotation,std::array<int32_t,3>{17000,-31858,12345});
    for(unsigned i=0;i<n::field_count;++i){props[i]=arena+0x65000+uintptr_t(i)*0x400;auto type=type_classes[i==0?0:(i==1?1:(i==4?3:2))];object(props[i],30+i,type,pawn_class,n::specs[i].name);
        put(props[i]+0x70,1u);put(props[i]+0x74,n::specs[i].width);put(props[i]+0x94,i==0?0x200u:(i==1?0x208u:(i==2?0x20cu:(i==3?0x20du:0x210u))));put(props[i]+0x68,i+1<n::field_count?props[i]+0x400:uintptr_t(0));}
    put(pawn_class+0x80,props[0]);put(props[0]+0xd0,link_class);put(props[4]+0xd0,0x20u);put(fixture_pawn+0x200,fixture_link);put(fixture_pawn+0x208,0);put(fixture_pawn+0x210,0x801u);
    auto names=get<gears_anchor::Registry>(config.names_header);const auto head_entry=arena+0x20000+199*0x100;put(names.data+199*8,head_entry);put(head_entry,199u);std::array<wchar_t,10> head{L'b',L'_',L'M',L'F',L'_',L'H',L'e',L'a',L'd',0};put(head_entry+20,head);
    b::layout_ready=true;b::game_thread=GetCurrentThreadId();b::initial_world=1;committed_world_generation=1;b::failed=false;b::external_change=false;b::stopping=false;b::transactions=0;b::injecting=false;b::step=0;b::journals={};b::enabled=true;entries_valid=true;
    n::proof={};n::calibration={};n::last_reason=n::Reason::body_unavailable;n::context_stage="not_sampled";n::rejected_field=L"none";
    camera::enabled=true;camera::facing={};camera::diagnostics={};anchor_config=config;continuous_session=stereo_xr=true;
    clock_ms=queries=reads=0;denied=0;head_offset={2,3,35};stereo_units_per_metre=100;refresh_scene();reads=0;
}
struct Query {
    template<class Read,class Guard>bool operator()(uintptr_t,uintptr_t,uint64_t name,unsigned,Read,Guard guard,gears_cover_bones::Result& out)const{
        ++queries;C(name==199);if(!guard())return false;auto location=get<std::array<float,3>>(fixture_pawn+b::metadata.pawn_location);out={};out.index=27;out.world.values[0]=out.world.values[5]=out.world.values[10]=out.world.values[15]=1;
        out.world.values[12]=float(location[0]+head_offset.x);out.world.values[13]=float(location[1]+head_offset.y);out.world.values[14]=float(location[2]+head_offset.z);return guard();
    }
};
bool preview(gears_vr::StereoMatrix& view){return camera::prepare_with(&fixture_scene,view,[](gears_anchor::Receipt const* receipt,n::EyeSample& out){return n::sample_with(receipt,out,[]{return clock_ms;},Query{});});}
bool calibrate(gears_vr::StereoMatrix& view){C(!preview(view));clock_ms+=100;return preview(view);}
gears_vr::StereoMatrix input_view(){return {1,0,0,0,0,1,0,0,0,0,1,0,-100,-200,-376,1};}
bool close_enough(double a,double value,double tolerance=2e-4){return std::abs(a-value)<=tolerance;}
bool close_enough(gears_vr::Vector a,gears_vr::Vector value,double tolerance=2e-4){return gears_vr::stereo_distance(a,value)<=tolerance;}
gears_vr::StereoSample head_sample(gears_vr::Pose head){
    gears_vr::StereoSample result{};result.source=gears_vr::StereoSource::MetaOperatorSimulated;result.session=1;result.sequence=1;result.space_epoch=1;
    result.head_flags=3;result.view_flags=15;result.focused=result.visible=result.should_render=1;result.head=head;
    for(unsigned i=0;i<2;++i){const auto delta=gears_vr::transform(gears_vr::rotation_matrix(head.orientation),{i?.032:-.032,0,0});result.eyes[i].pose={{head.position.x+delta.x,head.position.y+delta.y,head.position.z+delta.z},head.orientation};result.eyes[i].fov={-.8,.8,.8,-.8};}return result;
}
gears_vr::Vector midpoint(gears_vr::StereoMatrices const& matrices){const auto left=gears_vr::stereo_camera_position(matrices.view[0]),right=gears_vr::stereo_camera_position(matrices.view[1]);return {(left.x+right.x)/2,(left.y+right.y)/2,(left.z+right.z)/2};}
std::array<unsigned char,1024*1024> arena_baseline{};
int main(){
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,arena_baseline.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    FILE* log{};if(tmpfile_s(&log)||!log)return 98;wchar_t temp[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,temp)>0);C(GetTempFileNameW(temp,L"gcp",0,ini)!=0);
    auto ini_value=[&](wchar_t const* value){C(WritePrivateProfileStringW(L"CoverCameraPreview",L"enabled",value,ini)!=0);};
    // Real INI opt-in requires all three owners; absent/disabled stays inert.
    setup();camera::enabled=false;C(camera::configure(ini,log)&&!camera::enabled);ini_value(L"0");C(camera::configure(ini,log)&&!camera::enabled);
    ini_value(L"2");C(!camera::configure(ini,log)&&!camera::enabled);ini_value(L"1");
    continuous_session=false;C(!camera::configure(ini,log));continuous_session=true;stereo_xr=false;C(!camera::configure(ini,log));stereo_xr=true;b::enabled=false;C(!camera::configure(ini,log));b::enabled=true;
    C(camera::configure(ini,log)&&camera::enabled);C(!camera::configure(ini,log));C(DeleteFileW(ini)!=0);
    setup();camera::enabled=false;auto view=input_view();auto ordinary=view;C(!preview(view)&&view==ordinary&&!reads&&!queries);camera::enabled=true;
    memcpy(arena_baseline.data(),reinterpret_cast<void*>(arena),arena_baseline.size());const auto body_layout=b::layout;
    C(!preview(view)&&view==ordinary&&camera::diagnostics.calibrating==1);clock_ms=99;C(!preview(view)&&view==ordinary);clock_ms=100;C(preview(view));
    C(queries==1&&close_enough(gears_vr::stereo_camera_position(view),{102,203,335}));C(camera::diagnostics.bindings==1&&camera::diagnostics.applied==1);
    const auto fixed_forward=camera::facing.forward;constexpr double tau=6.2831853071795864769;const double expected_yaw=-31858.*tau/65536.+tau/2;
    C(close_enough(fixed_forward.x,std::cos(expected_yaw),1e-12)&&close_enough(fixed_forward.y,std::sin(expected_yaw),1e-12)&&fixed_forward.z==0);
    C(camera::diagnostics.latest_calibration.original_eye_height==76&&camera::diagnostics.latest_calibration.head_index==27);
    C(!memcmp(arena_baseline.data(),reinterpret_cast<void*>(arena),arena_baseline.size()));C(!memcmp(&body_layout,&b::layout,sizeof body_layout));
    // Real ah failure: streamed Link and Persistent WorldInfo can have a
    // different Level Outer while all three Actor.WorldInfo fields agree.
    setup();const auto streamed=arena+0x39000;object(streamed,23,get<uintptr_t>(fixture_scene.anchor.level+80),0,L"StreamedLevel");put(fixture_link+64,streamed);put(fixture_world+64,streamed);view=input_view();
    memcpy(arena_baseline.data(),reinterpret_cast<void*>(arena),arena_baseline.size());const auto streamed_layout=b::layout;
    C(calibrate(view)&&camera::diagnostics.applied==1&&camera::facing.key.link.outer!=fixture_scene.anchor.level);
    C(close_enough(gears_vr::stereo_camera_position(view),{102,203,335})&&n::proof.shared_actor_world);
    C(!memcmp(arena_baseline.data(),reinterpret_cast<void*>(arena),arena_baseline.size())&&!memcmp(&streamed_layout,&b::layout,sizeof streamed_layout));
    for(unsigned role=0;role<3;++role){setup();const auto alien=arena+0x3f000;object(alien,8,fixture_world_class,fixture_scene.anchor.level,L"OtherWorld");put((role==0?fixture_pawn:(role==1?fixture_link:b::metadata.identities[0].address))+0x188,alien);view=input_view();ordinary=view;C(!preview(view)&&view==ordinary&&!queries&&!camera::diagnostics.applied);}
    setup();view=input_view();C(calibrate(view));put(fixture_world_prop+0x94,0x198u);ordinary=input_view();view=ordinary;C(!preview(view)&&view==ordinary&&camera::diagnostics.last_neutral==n::Reason::layout_changed);
    setup();view=input_view();C(calibrate(view));
    // Native LT/pose/camera changes do not requery animated Head or rotate the
    // base. Only legitimate current Pawn root translation is composed.
    put(fixture_pawn+0x1a0,std::array<float,3>{110,220,330});put(fixture_pawn+0x1c0,std::array<int32_t,3>{0,2000,0});put(fixture_pawn+0x20c,static_cast<unsigned char>(7));put(fixture_pawn+0x20d,static_cast<unsigned char>(7));put(fixture_pawn+0x191,static_cast<unsigned char>(12));put(fixture_pawn+0x210,0x821u);b::injecting=true;b::step=3;head_offset.z=180;clock_ms=200;
    C(preview(view)&&queries==1&&close_enough(gears_vr::stereo_camera_position(view),{112,223,365}));C(close_enough(camera::facing.forward,fixed_forward,1e-12)&&camera::diagnostics.bindings==1);
    C(close_enough(camera::diagnostics.latest_base_eye,{112,223,365})&&close_enough(camera::diagnostics.latest_matrix_eye,{112,223,365})&&camera::diagnostics.latest_invocation==fixture_scene.invocation);
    ordinary=input_view();view=ordinary;fixture_scene.thread=GetCurrentThreadId()+1;C(!preview(view)&&view==ordinary&&camera::diagnostics.last_neutral==n::Reason::wrong_thread);fixture_scene.thread=GetCurrentThreadId();C(preview(view)&&queries==1&&close_enough(camera::facing.forward,fixed_forward,1e-12));
    // The stereo reference/head/eye mapping is composed once after the base.
    setup();view=input_view();C(calibrate(view));const auto base_view=view;const auto origin=gears_vr::stereo_camera_position(view);const auto f=camera::facing.forward,r=camera::facing.right;
    const gears_vr::StereoMatrix projection{1,0,0,0,0,1,0,0,0,0,0,1,0,0,10,0};const gears_vr::Pose reference{{.4,1.55,-.2},{0,std::sin(.37),0,std::cos(.37)}};
    for(auto rotation:{gears_vr::Quaternion{},gears_vr::Quaternion{std::sin(.4),0,0,std::cos(.4)},gears_vr::Quaternion{0,std::sin(.8),0,std::cos(.8)}}){gears_vr::Pose head{reference.position,rotation};gears_vr::StereoMatrices matrices{};C(gears_vr::stereo_matrices(view,projection,reference,head_sample(head),100,matrices,true));C(close_enough(midpoint(matrices),origin));C(preview(view)&&view==base_view);}
    const gears_vr::Vector local{.18,.42,-.08};const auto delta=gears_vr::transform(gears_vr::rotation_matrix(reference.orientation),local);gears_vr::Pose moved{{reference.position.x+delta.x,reference.position.y+delta.y,reference.position.z+delta.z},reference.orientation};gears_vr::StereoMatrices matrices{};
    C(gears_vr::stereo_matrices(view,projection,reference,head_sample(moved),100,matrices,true));C(close_enough(midpoint(matrices),{origin.x+100*(r.x*local.x-f.x*local.z),origin.y+100*(r.y*local.x-f.y*local.z),origin.z+100*local.y}));
    // New world/Pawn/Mesh/Link/slot keys wait for a fresh neutral calibration;
    // none can borrow the previously calibrated view or fixed facing.
    for(unsigned which=0;which<5;++which){setup();view=input_view();C(calibrate(view));
        if(which==0){committed_world_generation=2;b::initial_world=2;}
        if(which==1){const auto replacement=arena+0x31000,new_mesh=arena+0x3f000;memcpy(reinterpret_cast<void*>(replacement),reinterpret_cast<void*>(fixture_pawn),0x400);object(replacement,8,pawn_class,fixture_scene.anchor.level,L"SyntheticPawn2");object(new_mesh,9,get<uintptr_t>(fixture_mesh+80),replacement,L"SyntheticMesh2");put(replacement+0x180,new_mesh);put(b::metadata.identities[0].address+b::metadata.controller_pawn,replacement);put(b::metadata.identities[2].address+b::metadata.camera_view_target,replacement);fixture_pawn=replacement;fixture_mesh=new_mesh;refresh_scene();}
        if(which==2){const auto replacement=arena+0x3f000;object(replacement,8,get<uintptr_t>(fixture_mesh+80),fixture_pawn,L"SyntheticMesh2");put(fixture_pawn+0x180,replacement);}
        if(which==3){const auto replacement=arena+0x3f000;object(replacement,8,link_class,fixture_scene.anchor.level,L"SyntheticLink2");put(replacement+0x188,fixture_world);put(fixture_pawn+0x200,replacement);}
        if(which==4)put(fixture_pawn+0x208,1);
        put(fixture_pawn+0x1c0,std::array<int32_t,3>{0,12000,0});clock_ms=1200;view=ordinary;C(!preview(view)&&view==ordinary);clock_ms=1300;C(preview(view)&&queries==2&&camera::diagnostics.bindings==2);C(!close_enough(camera::facing.forward,fixed_forward,.01));}
    setup();C(calibrate(view));committed_world_generation=2;view=ordinary;C(!preview(view)&&view==ordinary&&camera::diagnostics.last_neutral==n::Reason::link);
    setup();C(calibrate(view));put(fixture_mesh+76,1u);clock_ms=1200;view=ordinary;C(!preview(view)&&view==ordinary);clock_ms=1300;C(preview(view)&&camera::facing.key.mesh.number==1&&queries==2);
    // A cached position cannot establish first facing while already peeking.
    setup();n::EyeSample eye{};C(!n::sample_with(&fixture_scene,eye,[]{return clock_ms;},Query{}));clock_ms=100;C(n::sample_with(&fixture_scene,eye,[]{return clock_ms;},Query{}));put(fixture_pawn+0x20c,static_cast<unsigned char>(7));view=ordinary;C(!preview(view)&&view==ordinary&&camera::diagnostics.last==camera::Reason::facing_wait&&!camera::facing.ready);
    // Initial-facing reads are bracketed by the certified current context.
    for(unsigned which=0;which<7;++which){setup();view=ordinary;C(!preview(view));clock_ms=100;bool changed=false;
        read_hook=[&](uintptr_t address,size_t size){if(changed||address!=fixture_pawn+0x1c0||size!=12)return;changed=true;
            if(which==0)put(address,std::array<int32_t,3>{0,1000,0});if(which==1)put(fixture_pawn+0x208,1);if(which==2)committed_world_generation=2;if(which==3)put(fixture_pawn+0x180,uintptr_t(0));if(which==4)anchor_config.pawn_rotation=0x1c4;if(which==5)put(props[4]+0xd0,0x40u);if(which==6)throw std::runtime_error("synthetic own reader failure");};
        C(!preview(view)&&changed&&view==ordinary&&!camera::facing.ready);C(camera::diagnostics.last==(which==6?camera::Reason::exception:camera::Reason::facing_guard));C(b::mutex.try_lock());b::mutex.unlock();read_hook={};}
    setup();anchor_config.pawn_rotation=0;view=ordinary;C(!calibrate(view)&&view==ordinary&&camera::diagnostics.last==camera::Reason::facing_guard);
    setup();anchor_config.pawn_rotation=0x1c4;view=ordinary;C(!calibrate(view)&&view==ordinary);setup();denied=fixture_pawn+0x1c0;view=ordinary;C(!calibrate(view)&&view==ordinary);
    // An invalid prepared base / own exception cannot corrupt the original.
    setup();C(calibrate(view));view=ordinary;C(!camera::prepare_with(&fixture_scene,view,[](gears_anchor::Receipt const*,n::EyeSample& out){out=camera::facing.key;out.neutral_eye_world.x=std::numeric_limits<double>::quiet_NaN();return true;}));C(view==ordinary&&camera::diagnostics.last==camera::Reason::matrix);
    C(!camera::prepare_with(&fixture_scene,view,[](gears_anchor::Receipt const*,n::EyeSample&)->bool{throw std::runtime_error("synthetic provider failure");}));C(view==ordinary&&camera::diagnostics.last==camera::Reason::exception);
    // Diagnostics remain bounded, retain the first neutral receipts and always
    // expose the latest binding in current progress even after retention fills.
    setup();C(calibrate(view));for(int i=1;i<20;++i){put(fixture_pawn+0x208,i);clock_ms+=2000;C(!preview(view));clock_ms+=100;C(preview(view));}
    C(camera::diagnostics.calibration_count==8&&camera::diagnostics.calibration_dropped==12&&camera::diagnostics.latest_calibration.slot==19);
    for(unsigned i=0;i<1000;++i){view=ordinary;fixture_scene.thread=GetCurrentThreadId()+1;C(!preview(view)&&view==ordinary);}fixture_scene.thread=GetCurrentThreadId();C(camera::diagnostics.rejection_count<=camera::diagnostics.rejections.size());
    camera::progress(log);camera::print(log);fflush(log);C(fseek(log,0,SEEK_END)==0);const auto length=ftell(log);C(length>0&&length<32768);C(fseek(log,0,SEEK_SET)==0);std::string text(size_t(length),'\0');C(fread(text.data(),1,text.size(),log)==text.size());C(text.find("mesh_body_facing_verified=0")!=std::string::npos&&text.find("collision_verified=0")!=std::string::npos&&text.find("cover_camera_slot=19")!=std::string::npos&&text.find("head_index=27")!=std::string::npos);
    C(fclose(log)==0);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("cover_camera_live: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
