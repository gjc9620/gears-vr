#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "body_head_authored_mount.h"
#include "first_person_body_visibility.h"
#include "first_person_head_section.h"
#include "lancer_saw_pose_policy.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
namespace mount=gears_body_head_authored;namespace body=mount::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,body::rig::Signature& out){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE h{};DWORD needed{},got{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    bool good=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&needed),sizeof needed,&got,0)>=0;
    std::vector<unsigned char> object(needed);if(good)good=BCryptCreateHash(algorithm,&h,object.data(),needed,nullptr,0,0)>=0;
    if(good)good=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(good)good=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);BCryptCloseAlgorithmProvider(algorithm,0);return good;
}
bool read(std::string const& path,void* data,std::size_t size){std::ifstream f(path,std::ios::binary);return bool(f.read(static_cast<char*>(data),std::streamsize(size)))&&f.peek()==EOF;}
template<class T> auto bytes(T const& x){std::array<unsigned char,sizeof(T)> r{};std::memcpy(r.data(),&x,sizeof x);return r;}
// Independent rigid quaternion FK oracle; production marker uses raw row-matrix
// Ref FK. Ref float normalization explains the small comparison tolerance.
using V=body::Vec;using Q=body::Quat;using P=body::Pose;
V sum(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V neg(V v){return {-v.x,-v.y,-v.z};}
Q mul(Q a,Q b){return {a.w*b.x+b.w*a.x+a.y*b.z-a.z*b.y,a.w*b.y+b.w*a.y+a.z*b.x-a.x*b.z,a.w*b.z+b.w*a.z+a.x*b.y-a.y*b.x,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
Q unit(Q q){double n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return {q.x/n,q.y/n,q.z/n,q.w/n};}
Q inv(Q q){return {-q.x,-q.y,-q.z,q.w};}
V turn(Q q,V p){const auto a=mul(mul(q,{p.x,p.y,p.z,0}),inv(q));return {a.x,a.y,a.z};}
double dist(V a,V b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);}
double qdiff(Q a,Q b){a=unit(a);b=unit(b);const double sign=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w<0?-1.:1.;return std::abs(a.x-sign*b.x)+std::abs(a.y-sign*b.y)+std::abs(a.z-sign*b.z)+std::abs(a.w-sign*b.w);}
Q euler(double yaw,double pitch,double roll){return unit(mul(mul({0,0,std::sin(yaw/2),std::cos(yaw/2)},{0,std::sin(pitch/2),0,std::cos(pitch/2)}),{std::sin(roll/2),0,0,std::cos(roll/2)}));}
std::array<P,115> oracle(body::Model const& model){std::array<P,115> p{};for(unsigned i=0;i<115;++i){const auto& a=model.reference[i];Q q=unit({a.q[0],a.q[1],a.q[2],a.q[3]});V v{a.translation[0],a.translation[1],a.translation[2]};if(i){const auto& parent=p[unsigned(model.parents[i])];p[i]={sum(parent.position,turn(parent.orientation,v)),unit(mul(parent.orientation,q))};}else p[i]={v,q};}return p;}
void head_section_test(){
    namespace h=gears_first_person_head_section;
    h::Palette before{},after{};
    for(auto& m:before)m.values={{0,0,0,7,0,0,0,-3,0,0,0,2}};
    before[h::spine_slot].values={{1,0,0,4,0,1,0,5,0,0,1,6}};
    const auto original=bytes(before);
    C(h::plan(before,after)&&bytes(before)==original);
    for(auto const& m:after)C(m.values==before[h::neck_slot].values);
    // Independent weighted-vertex oracle: ordinary mixed spine/neck skin
    // leaves a surface; the head-only palette must collapse every combination
    // to the same point, preserving the full-body source matrix exactly.
    for(unsigned sample=0;sample<128;++sample){
        const std::array<double,3> point{{double(sample)-30.,sample*.125,-double(sample)*.25}};
        std::array<double,3> actual{};
        const unsigned spine_weight=sample*37%256;
        for(unsigned j=0;j<4;++j){
            const unsigned index=j==0?h::spine_slot:(sample+j*3)%unsigned(after.size());
            const unsigned weight=j==0?spine_weight:(j==1?255-spine_weight:0);
            for(unsigned axis=0;axis<3;++axis){const auto& m=after[index].values;const unsigned row=axis*4;
                actual[axis]+=(m[row]*point[0]+m[row+1]*point[1]+m[row+2]*point[2]+m[row+3])*weight/255.;}
        }
        C(std::abs(actual[0]-7)<1e-12&&std::abs(actual[1]+3)<1e-12&&std::abs(actual[2]-2)<1e-12);
    }
    C(before[h::spine_slot].values[0]==1&&before[h::spine_slot].values[5]==1);
    for(unsigned bone=0;bone<before.size();++bone)for(unsigned k=0;k<12;++k){
        auto bad=before;bad[bone].values[k]=std::numeric_limits<float>::quiet_NaN();
        const auto saved=bytes(after);C(!h::plan(bad,after)&&bytes(after)==saved);
    }
    for(unsigned bone=0;bone<before.size();++bone)if(bone!=h::spine_slot){
        auto bad=before;bad[bone].values[0]=.1f;const auto saved=bytes(after);
        C(!h::plan(bad,after)&&bytes(after)==saved);
    }
    C(!h::plan(before,before)&&bytes(before)==original);
}
int main(int argc,char** argv){
    if(argc!=2)return 2;auto frame=std::make_unique<body::Frame>();body::Model model{};auto work=std::make_unique<mount::Workspace>();mount::Marker marker{};
    head_section_test();
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(body::prepare_model(frame->reference,hash,model));
    const auto model_before=bytes(model);const auto frame_before=bytes(*frame);
    C(mount::prepare(model,*frame,mount::Basis::designer_authored_ue_actor_axes,*work,marker)==mount::Status::okay);
    C(bytes(model)==model_before&&bytes(*frame)==frame_before);
    const auto fk=oracle(model);const V midpoint{(fk[57].position.x+fk[60].position.x)/2,(fk[57].position.y+fk[60].position.y)/2,(fk[57].position.z+fk[60].position.z)/2};
    // Render-only suppression inside the upper-spine parent. Non-face LOD entries
    // may be uninitialized; preserve them byte-for-byte, including both arms.
    {
        namespace visibility=gears_first_person_visibility;
        namespace saw_pose=gears_lancer_saw_pose;
        // A completely authenticated held gun may move in ready33, but never
        // during execution34. Losing the held mount may not become free hands.
        C(saw_pose::prepare_hands(33,true,true));
        C(saw_pose::commit_hands(33,true,true,true));
        C(!saw_pose::commit_hands(33,true,true,false));
        C(!saw_pose::commit_hands(33,false,true,true));
        C(!saw_pose::commit_hands(33,true,false,true));
        C(!saw_pose::prepare_hands(34,true,true));
        C(!saw_pose::commit_hands(34,true,true,true));
        C(!saw_pose::commit_hands(35,true,true,true));
        C(!saw_pose::commit_hands(53,true,true,true));
        C(saw_pose::commit_hands(0,false,true,false));
        // Ordinary stopping is visual continuity, not a held-item grant.
        for(unsigned sm:{14u,15u}){
            C(saw_pose::prepare_hands(sm,false,true));
            C(saw_pose::commit_hands(sm,false,true,false));
        }
        // Hits, vaults, pickups and paired actions never enter this path.
        for(unsigned sm=0;sm<256;++sm){
            const bool ordinary=sm==0||sm==14||sm==15;
            C(saw_pose::ordinary_hands_motion(sm)==ordinary);
            C(saw_pose::prepare_hands(sm,false,true)==ordinary);
        }
        C(visibility::original_pickup(53));
        C(visibility::original_lancer_animation(33,true));
        C(visibility::original_lancer_animation(34,true));
        for(unsigned sm=0;sm<256;++sm){
            C(!visibility::original_lancer_animation(sm,false));
            if(sm!=33&&sm!=34)C(!visibility::original_lancer_animation(sm,true));
        }
        for(unsigned sm:{0u,9u,10u,52u,54u,255u})C(!visibility::original_pickup(sm));
        std::array<body::Matrix64,body::bones> palette{},masked{};
        for(auto& matrix:palette)for(auto& x:matrix.values)x=std::numeric_limits<float>::quiet_NaN();
        auto& neck=palette[visibility::neck_bone].values;std::fill(std::begin(neck),std::end(neck),0.f);
        neck[1]=1;neck[4]=-1;neck[10]=neck[15]=1;neck[12]=10;neck[13]=20;neck[14]=30;
        const auto original=bytes(palette);unsigned hidden{};
        C(visibility::mask(model.parents,fk[54].position,palette,masked,hidden)&&hidden>1);
        for(unsigned b=0;b<body::bones;++b){
            if(visibility::neck_descendant(model.parents,b)){
                const auto& m=masked[b].values;
                C(std::hypot(m[12]-(10-fk[54].position.y),m[13]-(20+fk[54].position.x),
                    m[14]-(30+fk[54].position.z))<1e-4&&m[15]==1);
                for(unsigned at=0;at<12;++at)C(m[at]==0);
            }else C(!std::memcmp(&palette[b],&masked[b],sizeof palette[b]));
        }
        C(bytes(palette)==original&&bytes(model)==model_before&&bytes(*frame)==frame_before);
        auto invalid=model.parents;invalid[55]=5;const auto prior=bytes(masked);
        C(!visibility::mask(invalid,fk[54].position,palette,masked,hidden)&&bytes(masked)==prior);
        C(!visibility::mask(model.parents,fk[54].position,palette,palette,hidden)&&bytes(palette)==original);
        palette[54].values[0]=std::numeric_limits<float>::infinity();
        C(!visibility::mask(model.parents,fk[54].position,palette,masked,hidden)&&bytes(masked)==prior);
        // Explicitly unavailable left grip: only the left render branch is
        // collapsed. Keep staged head mask, right arm, torso and source exact.
        palette[54].values[0]=0;auto& shoulder=palette[8].values;
        for(unsigned at=0;at<16;++at)shoulder[at]=0;
        shoulder[0]=shoulder[5]=shoulder[10]=shoulder[15]=1;shoulder[12]=2;shoulder[13]=3;shoulder[14]=4;
        C(visibility::mask(model.parents,fk[54].position,palette,masked,hidden));
        const auto staged=masked,source_before=palette;unsigned left_count{};
        C(visibility::hide_untracked_left(model.parents,fk[8].position,palette,masked,left_count)&&left_count>2);
        for(unsigned b=0;b<body::bones;++b){
            if(visibility::descendant(model.parents,b,8)){
                auto const& m=masked[b].values;C(std::hypot(m[12]-fk[8].position.x-2,m[13]-fk[8].position.y-3,m[14]-fk[8].position.z-4)<1e-4&&m[15]==1);
                for(unsigned j=0;j<12;++j)C(m[j]==0);
            }else C(!std::memcmp(&masked[b],&staged[b],sizeof masked[b]));
        }
        C(bytes(palette)==bytes(source_before));const auto left_masked=bytes(masked);
        auto wrong=model.parents;wrong[8]=5;
        C(!visibility::hide_untracked_left(wrong,fk[8].position,palette,masked,left_count)&&bytes(masked)==left_masked);
        C(!visibility::hide_untracked_left(model.parents,fk[8].position,palette,palette,left_count)&&bytes(palette)==bytes(source_before));
        // Original shoulder pads are isolated rigid leaf bones. Their GPU-only
        // removal must leave all staged hand/arm/head/torso bytes unchanged.
        for(unsigned b:visibility::shoulder_armor_bones){auto& m=palette[b].values;
            std::fill(std::begin(m),std::end(m),0.f);m[0]=m[5]=m[10]=m[15]=1;m[12]=2;m[13]=3;m[14]=4;}
        const std::array<body::Vec,2> armor_reference{{fk[7].position,fk[31].position}};
        const auto armor_source=bytes(palette);const auto armor_staged=masked;
        C(visibility::hide_shoulder_armor(model.parents,armor_reference,palette,masked));
        for(unsigned b=0;b<body::bones;++b){
            if(b==7||b==31){const auto j=b==7?0:1;const auto& m=masked[b].values;
                for(unsigned k=0;k<12;++k)C(m[k]==0);
                C(std::hypot(m[12]-armor_reference[j].x-2,m[13]-armor_reference[j].y-3,
                    m[14]-armor_reference[j].z-4)<1e-4&&m[15]==1);
            }else C(!std::memcmp(&masked[b],&armor_staged[b],sizeof masked[b]));
        }
        C(bytes(palette)==armor_source&&bytes(model)==model_before&&bytes(*frame)==frame_before);
        const auto armor_done=bytes(masked);auto armor_wrong=model.parents;armor_wrong[31]=5;
        C(!visibility::hide_shoulder_armor(armor_wrong,armor_reference,palette,masked)&&bytes(masked)==armor_done);
        armor_wrong=model.parents;armor_wrong[32]=31;
        C(!visibility::hide_shoulder_armor(armor_wrong,armor_reference,palette,masked)&&bytes(masked)==armor_done);
        palette[31].values[0]=std::numeric_limits<float>::quiet_NaN();
        C(!visibility::hide_shoulder_armor(model.parents,armor_reference,palette,masked)&&bytes(masked)==armor_done);
        palette[31].values[0]=1;
        C(!visibility::hide_shoulder_armor(model.parents,armor_reference,palette,palette)&&bytes(palette)==armor_source);
        body::Matrix64 reference{},skin{},completed{};
        for(unsigned i:{0u,5u,10u,15u})reference.values[i]=1;
        reference.values[12]=2;reference.values[13]=3;reference.values[14]=4;
        skin.values[1]=1;skin.values[4]=-1;skin.values[10]=skin.values[15]=1;
        skin.values[12]=10;skin.values[13]=20;skin.values[14]=30;
        completed=skin;completed.values[12]=7;completed.values[13]=22;completed.values[14]=34;
        const auto completed_before=bytes(completed);
        C(visibility::matches_completed_wrist(reference,skin,completed));
        completed.values[12]+=.0005f;C(visibility::matches_completed_wrist(reference,skin,completed));
        completed.values[12]+=49;C(!visibility::matches_completed_wrist(reference,skin,completed));
        completed.values[12]=7;completed.values[1]=-1;C(!visibility::matches_completed_wrist(reference,skin,completed));
        completed.values[1]=1;completed.values[0]=NAN;C(!visibility::matches_completed_wrist(reference,skin,completed));
        completed.values[0]=0;skin.values[3]=INFINITY;C(!visibility::matches_completed_wrist(reference,skin,completed));
        C(bytes(model)==model_before&&bytes(*frame)==frame_before&&completed_before==bytes(completed));
    }
    const auto marker_local=marker.head_to_eye_local(),reference_head=marker.reference_head_model(),reference_eye=marker.reference_eye_model();
    C(dist(reference_head.position,fk[55].position)<1e-4&&qdiff(reference_head.orientation,fk[55].orientation)<1e-6);
    C(dist(reference_eye.position,midpoint)<1e-4&&qdiff(reference_eye.orientation,{0,0,0,1})<1e-12);
    C(dist(marker_local.position,turn(inv(fk[55].orientation),sum(midpoint,neg(fk[55].position))))<1e-4);
    C(dist(marker_local.position,{0,0,0})>1); // eye != Head bone
    mount::EyeHmdWorld eye{reference_eye};mount::HeadBoneWorld head{};mount::EyeHmdWorld roundtrip{};
    C(mount::head_from_eye(marker,model,*frame,eye,head)==mount::Status::okay);
    C(dist(head.pose.position,reference_head.position)<1e-10&&qdiff(head.pose.orientation,reference_head.orientation)<1e-12);
    C(mount::eye_from_head(marker,model,*frame,head,roundtrip)==mount::Status::okay);
    C(dist(roundtrip.pose.position,eye.pose.position)<1e-10&&qdiff(roundtrip.pose.orientation,eye.pose.orientation)<1e-12);
    // Every fixed-eye rotation must return the SAME eye origin. Head position
    // pivots around that origin; upstream physical/Grip motion is not invented.
    std::uint64_t rng=0x9e3779b97f4a7c15ULL;auto next=[&](){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return double(rng&0xffffff)/double(0xffffff)*2-1;};
    double max_position{},max_quaternion{},max_oracle{};V first_head{};bool head_pivoted=false;
    for(unsigned i=0;i<192;++i){const Q orientation=euler(next()*3.141592653589793,next()*1.5707963267948966,next()*3.141592653589793);
        eye.pose={{-31,14,27},orientation};const auto original_eye=bytes(eye);
        C(mount::head_from_eye(marker,model,*frame,eye,head)==mount::Status::okay);
        C(mount::eye_from_head(marker,model,*frame,head,roundtrip)==mount::Status::okay);
        const double p=dist(roundtrip.pose.position,eye.pose.position),q=qdiff(roundtrip.pose.orientation,eye.pose.orientation);
        max_position=(std::max)(max_position,p);max_quaternion=(std::max)(max_quaternion,q);
        C(p<1e-10&&q<1e-12&&bytes(eye)==original_eye);
        // Independent rigid transform of the authored reference configuration.
        const V shift=sum(eye.pose.position,neg(turn(orientation,midpoint)));
        const V expected_head=sum(shift,turn(orientation,fk[55].position));
        const double error=dist(head.pose.position,expected_head);max_oracle=(std::max)(max_oracle,error);
        C(error<1e-4&&qdiff(head.pose.orientation,mul(orientation,fk[55].orientation))<1e-6);
        if(!i)first_head=head.pose.position;else head_pivoted=head_pivoted||dist(first_head,head.pose.position)>.1;
        const auto offset=turn(head.pose.orientation,marker_local.position);C(dist(sum(head.pose.position,offset),eye.pose.position)<1e-10);
    }
    C(head_pivoted);
    // A previously composed physical+Grip translation passes through ONCE.
    eye.pose={{3,7,11},euler(.6,-.4,.2)};mount::HeadBoneWorld base{},shifted{};
    C(mount::head_from_eye(marker,model,*frame,eye,base)==mount::Status::okay);const V composed{5,-2,3};eye.pose.position=sum(eye.pose.position,composed);
    C(mount::head_from_eye(marker,model,*frame,eye,shifted)==mount::Status::okay);
    C(dist(shifted.pose.position,sum(base.pose.position,composed))<1e-10&&qdiff(base.pose.orientation,shifted.pose.orientation)<1e-12);
    // Animated Local / camera CTW cannot influence the authored marker.
    auto animated=*frame;std::memset(animated.local.data(),0xff,sizeof animated.local);for(float& x:animated.component_to_world.values)x=std::numeric_limits<float>::quiet_NaN();
    mount::Marker static_again{};C(mount::prepare(model,animated,mount::Basis::designer_authored_ue_actor_axes,*work,static_again)==mount::Status::okay);
    C(dist(static_again.head_to_eye_local().position,marker_local.position)==0&&qdiff(static_again.head_to_eye_local().orientation,marker_local.orientation)<1e-12);
    C(mount::head_from_eye(static_again,model,animated,eye,head)==mount::Status::okay&&dist(head.pose.position,shifted.pose.position)<1e-10);
    animated.units_per_metre=80;C(mount::head_from_eye(marker,model,animated,eye,head)==mount::Status::okay&&dist(head.pose.position,shifted.pose.position)<1e-10);
    for(unsigned bad=0;bad<9;++bad){auto m=model;auto f=*frame;auto e=eye;mount::Marker out=marker;mount::HeadBoneWorld target=head;
        const auto old_marker=bytes(out);const auto old_target=bytes(target);mount::Basis basis=mount::Basis::designer_authored_ue_actor_axes;
        if(bad==0)m.signature[0]^=1;if(bad==1)m.canonical[100]^=1;if(bad==2)m.parents[57]=55;if(bad==3)f.reference[114].bytes[28]^=1;
        if(bad==4)f.model_scale=2;if(bad==5)f.count=114;if(bad==6)e.pose.position.x=std::numeric_limits<double>::quiet_NaN();
        if(bad==7)e.pose.orientation={0,0,0,2};if(bad==8)basis=mount::Basis::unknown;
        if(bad<6||bad==8){C(mount::prepare(m,f,basis,*work,out)!=mount::Status::okay);C(bytes(out)==old_marker);}
        if(bad<8){C(mount::head_from_eye(marker,m,f,e,target)!=mount::Status::okay);C(bytes(target)==old_target);}
    }
    mount::Marker absent{};const auto old_head=bytes(head);C(mount::head_from_eye(absent,model,*frame,eye,head)==mount::Status::unprepared&&bytes(head)==old_head);
    head.pose.orientation={0,0,0,0};const auto old_eye=bytes(roundtrip);C(mount::eye_from_head(marker,model,*frame,head,roundtrip)==mount::Status::pose&&bytes(roundtrip)==old_eye);

    mount::Marker face{};C(mount::prepare(model,*frame,mount::Basis::marcus_mesh_face_forward_y,*work,face)==mount::Status::okay);
    const Q face_q=euler(1.57079632679489661923,0,0);const auto ref_face=face.reference_eye_model();
    const auto shifted_midpoint=sum(midpoint,{0,.03*frame->units_per_metre,0});
    C(qdiff(ref_face.orientation,face_q)<1e-12&&dist(ref_face.position,shifted_midpoint)<1e-4);
    C(dist(face.head_to_eye_local().position,sum(marker_local.position,turn(inv(reference_head.orientation),{0,.03*frame->units_per_metre,0})))<1e-4);
    C(dist(turn(ref_face.orientation,{1,0,0}),{0,1,0})<1e-12&&dist(turn(ref_face.orientation,{0,1,0}),{-1,0,0})<1e-12);
    for(unsigned i=0;i<64;++i){const Q orientation=euler(next()*3.141592653589793,next()*1.2,next()*1.2);mount::EyeHmdWorld input{{{-31,14,27},orientation}},again{};mount::HeadBoneWorld result{};
        C(mount::head_from_eye(face,model,*frame,input,result)==mount::Status::okay);C(mount::eye_from_head(face,model,*frame,result,again)==mount::Status::okay);
        C(dist(again.pose.position,input.pose.position)<1e-10&&qdiff(again.pose.orientation,input.pose.orientation)<1e-12);
        const auto rigid=mul(orientation,inv(face_q));const auto expected=sum(input.pose.position,turn(rigid,sum(fk[55].position,neg(shifted_midpoint))));
        C(dist(result.pose.position,expected)<1e-4&&qdiff(result.pose.orientation,mul(rigid,fk[55].orientation))<1e-6);
    }
    C(bytes(model)==model_before&&bytes(*frame)==frame_before);
    std::printf("{\"Marker\":%zu,\"Workspace\":%zu,\"EyeHmdWorld\":%zu,\"HeadBoneWorld\":%zu,\"head_to_eye_translation\":[%.17g,%.17g,%.17g],\"head_to_eye_xyzw\":[%.17g,%.17g,%.17g,%.17g],\"reference_eye_model\":[%.17g,%.17g,%.17g],\"max_roundtrip_position\":%.17g,\"max_roundtrip_q_component\":%.17g,\"max_independent_FK_error\":%.17g}\n",sizeof(marker),sizeof(*work),sizeof(eye),sizeof(head),marker_local.position.x,marker_local.position.y,marker_local.position.z,marker_local.orientation.x,marker_local.orientation.y,marker_local.orientation.z,marker_local.orientation.w,reference_eye.position.x,reference_eye.position.y,reference_eye.position.z,max_position,max_quaternion,max_oracle);
    const auto face_eye=face.reference_eye_model();
    C(dist(face_eye.position,sum(midpoint,{0,.03*frame->units_per_metre,0}))<1e-4);
    mount::EyeHmdWorld forward_eye{face_eye},forward_again{};mount::HeadBoneWorld forward_head{};
    C(mount::head_from_eye(face,model,*frame,forward_eye,forward_head)==mount::Status::okay);
    C(mount::eye_from_head(face,model,*frame,forward_head,forward_again)==mount::Status::okay);
    C(dist(forward_again.pose.position,forward_eye.pose.position)<1e-10);
    C(dist(forward_head.pose.position,reference_head.position)<1e-4);
    std::printf("body_head_authored_mount: %u checks, %u failures; designer_authored=1 visual_HMD_acceptance=0 Game=0\n",checks,failures);return failures?1:0;
}
