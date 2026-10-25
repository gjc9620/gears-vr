#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "body_head_authored_mount.h"
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
int main(int argc,char** argv){
    if(argc!=2)return 2;auto frame=std::make_unique<body::Frame>();body::Model model{};auto work=std::make_unique<mount::Workspace>();mount::Marker marker{};
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(body::prepare_model(frame->reference,hash,model));
    const auto model_before=bytes(model);const auto frame_before=bytes(*frame);
    C(mount::prepare(model,*frame,mount::Basis::designer_authored_ue_actor_axes,*work,marker)==mount::Status::okay);
    C(bytes(model)==model_before&&bytes(*frame)==frame_before);
    const auto fk=oracle(model);const V midpoint{(fk[57].position.x+fk[60].position.x)/2,(fk[57].position.y+fk[60].position.y)/2,(fk[57].position.z+fk[60].position.z)/2};
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
    C(bytes(model)==model_before&&bytes(*frame)==frame_before);
    std::printf("{\"Marker\":%zu,\"Workspace\":%zu,\"EyeHmdWorld\":%zu,\"HeadBoneWorld\":%zu,\"head_to_eye_translation\":[%.17g,%.17g,%.17g],\"head_to_eye_xyzw\":[%.17g,%.17g,%.17g,%.17g],\"reference_eye_model\":[%.17g,%.17g,%.17g],\"max_roundtrip_position\":%.17g,\"max_roundtrip_q_component\":%.17g,\"max_independent_FK_error\":%.17g}\n",sizeof(marker),sizeof(*work),sizeof(eye),sizeof(head),marker_local.position.x,marker_local.position.y,marker_local.position.z,marker_local.orientation.x,marker_local.orientation.y,marker_local.orientation.z,marker_local.orientation.w,reference_eye.position.x,reference_eye.position.y,reference_eye.position.z,max_position,max_quaternion,max_oracle);
    std::printf("body_head_authored_mount: %u checks, %u failures; designer_authored=1 visual_HMD_acceptance=0 Game=0\n",checks,failures);return failures?1:0;
}
