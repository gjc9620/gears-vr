// Reuse the existing synthetic current-registry fixture, not its game/runtime.
// Real binding/property/source/identity/closing helpers compose with this module.
#include "run_camera_stabilizer.h"
#define main RunCameraBindingFixtureMain
#include "weapon_hand_scene_binding_test.cpp"
#undef main
bool stereo_xr=true;double stereo_units_per_metre=100;
#include "run_camera_live.inl"
namespace run=run_camera_live;
#if GEARS_COVER_BODY_PROTOTYPE
uint64_t fake_ns=1000000000;
uint64_t clock_ns(){return fake_ns;}
uint64_t scene_clock(int64_t value){return value>0?uint64_t(value):0;}
uintptr_t new_properties[8]{},float_class{},int_class{},live_bool_class{},struct_class{},script_struct{},vector_type{},rotator_type{};
gears_vr::StereoResult stereo;
void pfield(uintptr_t address,unsigned index,uintptr_t owner,const wchar_t* name,uintptr_t kind,unsigned width,unsigned offset,uintptr_t extra=0){
 property(address,index,owner,name,offset,extra);put(address+80,kind);put(address+0x74,width);
}
void tick(unsigned sm=0){
 ++scene.invocation;scene.scene_qpc=int64_t(fake_ns);scene.family_qpc=scene.scene_qpc+1;scene.renderer_qpc=scene.scene_qpc+2;
 scene.anchor.location=get<std::array<float,3>>(fixture_pawn+0x280);scene.anchor.eye_height=get<float>(fixture_pawn+0x290);
 put(fixture_pawn+0x191,static_cast<unsigned char>(sm));stereo.status=gears_vr::StereoStatus::accepted;
 stereo.sample.session=1;stereo.sample.space_epoch=1;stereo.sample.sequence=scene.invocation;stereo.sample.received_ns=int64_t(fake_ns);stereo.reference_generation=1;
}
void live_setup(){
 setup();fake_ns=1000000000;float_class=arena+0x6a000;int_class=arena+0x6a400;live_bool_class=arena+0x6a800;struct_class=arena+0x6ac00;script_struct=arena+0x6b000;
 klass(float_class,50,L"FloatProperty",0);klass(int_class,51,L"IntProperty",0);klass(live_bool_class,52,L"BoolProperty",0);klass(struct_class,53,L"StructProperty",0);klass(script_struct,54,L"ScriptStruct",0);
 vector_type=arena+0x6b400;rotator_type=arena+0x6b800;object(vector_type,55,script_struct,0,L"Vector");object(rotator_type,56,script_struct,0,L"Rotator");
 put(vector_type+0x88,12u);put(rotator_type+0x88,12u);
 const wchar_t* names[]={L"X",L"Y",L"Z",L"Pitch",L"Yaw",L"Roll"};
 for(unsigned i=0;i<6;++i){const auto addr=arena+0x6c000+uintptr_t(i)*0x400;const auto owner=i<3?vector_type:rotator_type;
  pfield(addr,57+i,owner,names[i],i<3?float_class:int_class,4,(i%3)*4);put(addr+0x68,i%3<2?addr+0x400:uintptr_t(0));
  if(i%3==0)put(owner+0x80,addr);}
 const wchar_t* props[]={L"Location",L"EyeHeight",L"Rotation",L"bIsCrouched",L"bWantsToCrouch",L"CoverType",L"bDoingCoverActionAnimTransition",L"bDoingMirrorTransition"};
 const unsigned offsets[]={0x280,0x290,0x2a0,0x294,0x294,0x298,0x294,0x294};
 for(unsigned i=0;i<8;++i){auto addr=arena+0x6e000+uintptr_t(i)*0x400;new_properties[i]=addr;
  const auto owner=i==0||i==2?fixture_actor:(i==1||i==3||i==4?fixture_pawn_base:fixture_pawn_class);
  const auto kind=i==0||i==2?struct_class:(i==1?float_class:(i==5?fixture_byte_property:live_bool_class));
  const unsigned width=i==0||i==2?12u:(i==5?1u:4u);const uintptr_t extra=i==0?vector_type:(i==2?rotator_type:(i==3?1:(i==4?2:(i==6?4:(i==7?8:0)))));
  pfield(addr,63+i,owner,props[i],kind,width,offsets[i],extra);
 }
 put(instigator_property+0x68,new_properties[0]);put(new_properties[0]+0x68,new_properties[2]);
 put(mesh_property+0x68,new_properties[1]);put(new_properties[1]+0x68,new_properties[3]);put(new_properties[3]+0x68,new_properties[4]);
 put(special_property+0x68,new_properties[5]);put(new_properties[5]+0x68,new_properties[6]);put(new_properties[6]+0x68,new_properties[7]);
 anchor_config.pawn_location=0x280;anchor_config.eye_height=0x290;anchor_config.controller_rotation=0x2a0;
 put(fixture_pawn+0x280,std::array<float,3>{100,200,300});put(fixture_pawn+0x290,76.f);put(fixture_pc+0x2a0,std::array<int32_t,3>{0,0,0});
 put(fixture_pawn+0x294,0x80u);put(fixture_pawn+0x298,static_cast<unsigned char>(0));
 run::kernel.reset();run::proof={};run::metadata_generation=0;run::metadata_bytes={};run::have_last=false;run::diagnostics={};run::enabled=true;
 run::clock_ns=clock_ns;run::scene_ns=scene_clock;stereo={};continuous_session=stereo_xr=anchor_observer_enabled=true;stereo_units_per_metre=100;tick();reads=0;
}
gears_vr::StereoMatrix original(){return {1,0,0,0,0,1,0,0,0,0,1,0,-7,-8,-9,1};}
bool prepare(gears_vr::StereoMatrix& view,bool fp=true){return run::prepare(&scene,stereo,view,fp);}
void warm(gears_vr::StereoMatrix& view){for(unsigned i=0;i<3;++i){C(!prepare(view,false));fake_ns+=100000000;tick();}C(run::kernel.calibrated());}
int main(){
 arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
 live_setup();auto view=original();auto before=view;run::enabled=false;C(!prepare(view)&&view==before&&!reads);run::enabled=true;
 memcpy(saved_arena.data(),reinterpret_cast<void const*>(arena),saved_arena.size());warm(view);C(view==before);tick(4);C(!prepare(view,false)&&view==before);C(prepare(view));
 auto position=gears_vr::stereo_camera_position(view);C(std::abs(position.x-135)<.001&&std::abs(position.z-386)<.001);
 C(native_calls==0);const auto applied=view;C(prepare(view)&&view==applied&&run::diagnostics.retries==2);
 // Existing stereo math alone composes the raw HMD/eye translations once.
 const gears_vr::StereoMatrix projection{1,0,0,0,0,1,0,0,0,0,0,1,0,0,10,0};
 gears_vr::StereoSample head{};head.source=gears_vr::StereoSource::MetaOperatorSimulated;head.head.position={.12,.23,-.04};
 head.head.orientation={0,0,0,1};for(unsigned i=0;i<2;++i){head.eyes[i].pose={{.12+(i?.032:-.032),.23,-.04},{0,0,0,1}};head.eyes[i].fov={-.8,.8,.8,-.8};}
 const auto raw_head=head;gears_vr::StereoMatrices matrices{};C(gears_vr::stereo_matrices(view,projection,{{0,0,0},{0,0,0,1}},head,100,matrices,true));
 const auto l=gears_vr::stereo_camera_position(matrices.view[0]),r=gears_vr::stereo_camera_position(matrices.view[1]);
 C(std::abs((l.x+r.x)/2-(position.x+4))<.001&&std::abs((l.y+r.y)/2-(position.y+12))<.001&&std::abs((l.z+r.z)/2-(position.z+23))<.001);
 C(gears_vr::stereo_same(raw_head,head)&&view==applied);
 // Terrain/root survives; animated eye height does not; PC intent still turns.
 fake_ns+=100000000;put(fixture_pawn+0x280,std::array<float,3>{100,200,343});put(fixture_pawn+0x290,44.f);put(fixture_pc+0x2a0,std::array<int32_t,3>{16384,16384,9000});tick(4);C(prepare(view));
 position=gears_vr::stereo_camera_position(view);C(std::abs(position.z-429)<.001&&std::abs(position.y-235)<.001);C(std::abs(view[10]-1)<.0001);
 // Same-frame mutation/recenter/age never reuses the cached candidate.
 before=view;put(fixture_pawn+0x290,45.f);scene.anchor.eye_height=45;C(!prepare(view)&&view==before&&!run::kernel.calibrated());
 for(unsigned kind=0;kind<3;++kind){live_setup();view=original();warm(view);tick(4);C(prepare(view));before=view;
  if(kind==0)put(fixture_pawn+0x1d0,fixture_weapon2);if(kind==1)stereo.sample.head.position.x=.1;if(kind==2)stereo.recentered=true;
  C(!prepare(view)&&view==before&&!run::have_last);}
 for(unsigned kind=0;kind<10;++kind){live_setup();view=original();warm(view);tick(4);before=view;
  if(kind==0)recording=false;if(kind==1)scene.thread++;if(kind==2)fake_ns+=50000001;if(kind==3)++stereo.reference_generation;
  if(kind==4)++stereo.sample.space_epoch;if(kind==5)put(new_properties[0]+0xd0,rotator_type);
  if(kind==6)put(new_properties[3]+0xd0,3u);if(kind==7)put(vector_type+0x88,16u);
  if(kind==8)put(new_properties[2]+0x74,4u);if(kind==9)scene.anchor.location[2]++;
  C(!prepare(view)&&view==before);}
 // SM0 alone cannot certify standing: four typed packed bits + CoverType.
 for(unsigned kind=0;kind<5;++kind){live_setup();view=original();if(kind<4)put(fixture_pawn+0x294,0x80u|(1u<<kind));else put(fixture_pawn+0x298,static_cast<unsigned char>(2));
  for(unsigned i=0;i<4;++i){C(!prepare(view));fake_ns+=100000000;tick();}C(!run::kernel.calibrated());tick(4);C(!prepare(view));}
 // An actual closing value/source/world/Scene change must leave output intact.
 for(unsigned kind=0;kind<5;++kind){live_setup();view=original();warm(view);tick(4);before=view;
  if(kind==0)plan(fixture_pawn+0x290,2,fixture_pawn+0x290,0x42800000u,4);
  if(kind==1)plan(fixture_pawn+0x290,2,new_properties[6]+0xd0,16u,4);
  if(kind==2){plan(fixture_pawn+0x290,2,0,0,0);change_generation=true;}
  if(kind==3){plan(fixture_pawn+0x290,2,0,0,0);change_scene=true;}
  if(kind==4){plan(fixture_pawn+0x290,2,0,0,0);throw_read=true;}
  C(!prepare(view)&&view==before);read_hook=nullptr;C(h::mutex.try_lock());h::mutex.unlock();}
 live_setup();view=original();warm(view);tick(4);denied=fixture_pc+0x2a0;before=view;C(!prepare(view)&&view==before);denied=0;
 // Successful preparation itself writes neither the arena nor body/owner state.
 live_setup();view=original();warm(view);tick(4);memcpy(saved_arena.data(),reinterpret_cast<void const*>(arena),saved_arena.size());C(prepare(view));C(!memcmp(saved_arena.data(),reinterpret_cast<void const*>(arena),saved_arena.size()));C(!b::enabled&&!b::layout_ready&&native_calls==0);
 FILE* log{};C(tmpfile_s(&log)==0);wchar_t folder[MAX_PATH]{},ini[MAX_PATH]{};C(GetTempPathW(MAX_PATH,folder)>0);C(GetTempFileNameW(folder,L"run",0,ini)>0);
 run::enabled=false;C(run::configure(ini,log)&&!run::enabled);C(WritePrivateProfileStringW(L"RunCameraStabilizer",L"enabled",L"1",ini)!=0);
 continuous_session=false;C(!run::configure(ini,log));continuous_session=true;C(run::configure(ini,log)&&run::enabled);C(!run::configure(ini,log));run::print(log);C(fclose(log)==0);C(DeleteFileW(ini)!=0);
 C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);printf("run_camera_live: %u checks, %u failures; CPU composition only\n",checks,failures);return failures?1:0;
}
#else
int main(){auto view=gears_vr::StereoMatrix{};const auto before=view;C(!run::prepare(nullptr,{},view,true)&&view==before);C(!run::enabled);printf("run_camera_live OFF: %u checks, %u failures\n",checks,failures);return failures?1:0;}
#endif
