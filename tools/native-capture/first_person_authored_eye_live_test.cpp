// Production eye reader over retained61492 C0 matrices. Registry/Scene binding
// is an explicit CPU seam, not a live identity or atomic-pose certificate.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef GEARS_COVER_BODY_PROTOTYPE
#undef GEARS_COVER_BODY_PROTOTYPE
#endif
#define GEARS_COVER_BODY_PROTOTYPE 1
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "anchor_observer.h"
#include "cover_pose_phase.h"
#include "../vr-common/native_rig_tracking_frame.h"
namespace raw=gears_cover_pose_phase;
std::map<uintptr_t,std::vector<unsigned char>> memory;
uintptr_t denied{},change_address{};unsigned change_read{},change_hits{},visual_calls{};
bool metadata_ok=true,owner_ok=true,closing_ok=true;double stereo_units_per_metre=100;
bool safe_copy(void* out,void const* from,size_t size){
    const auto at=reinterpret_cast<uintptr_t>(from);
    if(at==denied)return false;
    auto found=memory.upper_bound(at);if(found==memory.begin())return false;--found;
    const auto offset=at-found->first;if(offset>found->second.size()||size>found->second.size()-offset)return false;
    memcpy(out,found->second.data()+offset,size);
    if(at==change_address&&++change_hits==change_read&&size)static_cast<unsigned char*>(out)[0]^=1;
    return true;
}
template<class T>void put(uintptr_t at,T const& value){
    const auto* first=reinterpret_cast<unsigned char const*>(&value);memory[at]={first,first+sizeof value};
}
void put_bytes(uintptr_t at,void const* bytes,size_t size){
    const auto* first=static_cast<unsigned char const*>(bytes);memory[at]={first,first+size};
}
namespace weapon_hand_scene_binding {
struct Binding {gears_anchor::Identity mesh{};uint64_t world_generation{};};
std::mutex mutex;Binding fixture{};gears_anchor::Identity asset{};
bool sample_visual(gears_anchor::Receipt const*,Binding& out){
    ++visual_calls;if(!owner_ok||(!closing_ok&&visual_calls>1))return false;out=fixture;return true;
}
bool metadata_current(){return metadata_ok;}
bool identity(uintptr_t at,gears_anchor::Identity& out){if(at!=asset.address)return false;out=asset;return true;}
bool same(gears_anchor::Identity const& a,gears_anchor::Identity const& b){return gears_anchor::equal_identity(a,b);}
bool same_visual_key(Binding const& a,Binding const& b){return same(a.mesh,b.mesh)&&a.world_generation==b.world_generation;}
}
namespace first_person_live {uint64_t authored_eye_applied{};std::array<double,3> authored_eye_last{};}
#include "first_person_authored_eye.inl"
namespace fp=first_person_live;namespace bind=weapon_hand_scene_binding;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
bool read(std::string const& n,void* out,size_t bytes){std::ifstream f(n,std::ios::binary);return bool(f.read(static_cast<char*>(out),std::streamsize(bytes)))&&f.peek()==EOF;}
int main(int argc,char**argv){
    if(argc!=2)return 2;const std::string folder=argv[1];
    raw::Metadata metadata{};gears_native_rig_tracking::CoreSnapshot source{};
    std::array<raw::Matrix,115> space{};std::array<raw::RefBone,115> reference{};std::array<float,3> pawn{};
    C(read(folder+"/C0-metadata.bin",&metadata,sizeof metadata));
    C(read(folder+"/C0-space.bin",space.data(),sizeof space));
    C(read(folder+"/reference.bin",reference.data(),sizeof reference));
    C(read(folder+"/source-CT0.bin",&source,sizeof source));
    C(read(folder+"/pawn-location.bin",pawn.data(),sizeof pawn));
    if(failures)return 1;
    const auto mesh=uintptr_t(source.mesh.address);
    bind::fixture.mesh.address=mesh;bind::fixture.mesh.index=source.mesh.registry_index;
    bind::fixture.world_generation=source.world_generation;bind::asset.address=metadata.asset;bind::asset.index=123;
    gears_anchor::Receipt scene{};scene.invocation=1;
    for(unsigned i=0;i<3;++i)scene.anchor.location[i]=pawn[i];
    const auto reset=[&](){
        memory.clear();denied=change_address=0;change_read=change_hits=visual_calls=0;
        metadata_ok=owner_ok=closing_ok=true;stereo_units_per_metre=100;
        fp::authored_cache.ready=false;
        put(mesh+0x218,metadata.asset);put(mesh+0x29c,metadata.master);
        put(metadata.asset+0xa4,metadata.reference);put(mesh+0xa0,metadata.component);put(mesh+0x26c,metadata.space);
        put_bytes(metadata.reference.data,reference.data(),sizeof reference);put_bytes(metadata.space.data,space.data(),sizeof space);
    };
    reset();gears_vr::Vector eye{-100,-100,-100};
    const bool accepted=fp::authored_eye(&scene,eye);
    C(accepted);
    printf("retained61492 accepted=%u eye=[%.12g,%.12g,%.12g] pawn_z=%.12g mesh_z=%.12g\n",unsigned(accepted),eye.x,eye.y,eye.z,double(pawn[2]),double(metadata.component.values[14]));
    if(!accepted){printf("first_person_authored_eye_live: %u checks, %u failures\n",checks,failures);return 1;}
    const auto expected=eye;
    C(eye.z<double(pawn[2])&&eye.z>metadata.component.values[14]);
    C(std::abs(eye.z-574.4404)<.02);
    // Previously accepted standing-like positions retain their exact eye;
    // low valid mesh-space heights do not inherit the capsule's centre.
    for(double height:{20.,74.+15.,74.+149.}){
        reset();auto h=space[fp::eye_marker::head_bone];
        h.values[14]+=float(height-(expected.z-metadata.component.values[14]));
        put(metadata.space.data+fp::eye_marker::head_bone*sizeof h,h);
        C(fp::authored_eye(&scene,eye));
        C(std::abs(eye.z-(metadata.component.values[14]+height))<.001);
    }
    for(unsigned bad=0;bad<18;++bad){
        reset();auto s=scene;
        if(bad==0)owner_ok=false;
        if(bad==1)closing_ok=false;
        if(bad==2)metadata_ok=false;
        if(bad==3)denied=mesh+0x218;
        if(bad==4)put(mesh+0x29c,uintptr_t(1));
        if(bad==5){auto ref=metadata.reference;--ref.count;put(metadata.asset+0xa4,ref);}
        if(bad==6){auto h=metadata.space;--h.count;put(mesh+0x26c,h);}
        if(bad==7){auto h=space[fp::eye_marker::head_bone];h.values[15]=0;put(mesh+0x26c,raw::ArrayHeader{0x1000,115,115});put(0x1000+fp::eye_marker::head_bone*sizeof h,h);}
        if(bad==8){change_address=mesh+0x218;change_read=2;}
        if(bad==9){change_address=mesh+0xa0;change_read=2;}
        if(bad==10){change_address=metadata.space.data+fp::eye_marker::head_bone*sizeof(raw::Matrix);change_read=2;}
        if(bad==11){s.anchor.location[0]+=201;}
        if(bad==12){auto h=space[fp::eye_marker::head_bone];h.values[14]=-1000;put(metadata.space.data+fp::eye_marker::head_bone*sizeof h,h);}
        if(bad==13){auto h=space[fp::eye_marker::head_bone];h.values[14]=1000;put(metadata.space.data+fp::eye_marker::head_bone*sizeof h,h);}
        if(bad==14){change_address=mesh+0x26c;change_read=2;}
        if(bad==15){auto m=metadata.component;m.values[15]=0;put(mesh+0xa0,m);}
        if(bad==16){auto h=space[fp::eye_marker::head_bone];h.values[14]=NAN;put(metadata.space.data+fp::eye_marker::head_bone*sizeof h,h);}
        if(bad==17){auto h=space[fp::eye_marker::head_bone];h.values[14]+=float(74.+151.-(expected.z-metadata.component.values[14]));put(metadata.space.data+fp::eye_marker::head_bone*sizeof h,h);}
        eye={-123,-234,-345};const auto before=eye;
        C(!fp::authored_eye(&s,eye));C(!memcmp(&before,&eye,sizeof eye));
    }
    reset();C(fp::authored_eye(&scene,eye));C(!memcmp(&expected,&eye,sizeof eye));
    printf("first_person_authored_eye_live: %u checks, %u failures; retained matrices, CPU binding seam; no camera/write permission\n",checks,failures);
    return failures?1:0;
}
