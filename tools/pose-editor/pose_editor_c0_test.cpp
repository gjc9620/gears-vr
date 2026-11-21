#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "../native-capture/body_c0_hands.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace h=gears_body_c0_hands;namespace b=h::body;namespace v=gears_vr;
bool read(std::string const& path,void* out,std::size_t size){
    std::ifstream f(path,std::ios::binary);
    return bool(f.read(static_cast<char*>(out),std::streamsize(size)))&&f.peek()==EOF;
}
bool digest(void const* bytes,std::size_t size,b::rig::Signature& out){
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};
    bool good=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(good)good=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    if(good)good=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(good)good=BCryptFinishHash(hash,out.data(),ULONG(out.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);return good;
}
// Independent local quaternion multiplication oracle.
v::Quaternion oracle_multiply(v::Quaternion a,v::Quaternion c){
    return {a.w*c.x+a.x*c.w+a.y*c.z-a.z*c.y,a.w*c.y-a.x*c.z+a.y*c.w+a.z*c.x,
        a.w*c.z+a.x*c.y-a.y*c.x+a.z*c.w,a.w*c.w-a.x*c.x-a.y*c.y-a.z*c.z};
}
bool same(v::Quaternion a,v::Quaternion c){
    const double sign=a.x*c.x+a.y*c.y+a.z*c.z+a.w*c.w<0?-1.:1.;
    return std::abs(a.x-sign*c.x)+std::abs(a.y-sign*c.y)+std::abs(a.z-sign*c.z)+std::abs(a.w-sign*c.w)<1e-6;
}
int main(int argc,char** argv){
    if(argc!=2)return 2;const std::string path=argv[1];
    auto frame=std::make_unique<b::Frame>();auto model=std::make_unique<b::Model>();
    auto work=std::make_unique<b::Workspace>();
    assert(read(path+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    assert(read(path+"/C0-local.bin",frame->local.data(),sizeof frame->local));
    assert(read(path+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    assert(b::prepare_model(frame->reference,digest,*model));
    for(unsigned i=0;i<16;++i)work->component.m[i]=frame->component_to_world.values[i];
    std::copy(frame->local.begin(),frame->local.end(),work->pose.local);assert(b::d::rebuild(*model,*work));
    std::array<v::Quaternion,15> authored{};
    for(unsigned n=0;n<15;++n){const auto& q=model->reference[36+n].q;authored[n]={q[0],q[1],q[2],q[3]};}
    gears_pose_editor::Config edit{};edit.revision=1;edit.enabled=1;
    edit.fingers[4]={0,0,std::sin(.15),std::cos(.15)};
    const auto check=[&](bool enabled){
        assert(h::detail::fingers(*model,*work,1,1.,{{UINT64_MAX,UINT64_MAX}},
            gears_hand_fingers::Shape::authored,1.,0.,nullptr,authored.data(),&edit));
        for(unsigned i=0;i<b::bones;++i){
            assert(!std::memcmp(work->pose.local[i].translation,frame->local[i].translation,12));
            assert(work->pose.local[i].scale==frame->local[i].scale);
            if(i>=36&&i<=50){const auto& q=work->pose.local[i].q;
                assert(same({q[0],q[1],q[2],q[3]},enabled&&i==40?oracle_multiply(authored[4],edit.fingers[4]):authored[i-36]));
            }else assert(!std::memcmp(&work->pose.local[i],&frame->local[i],sizeof(b::Atom)));
        }
    };
    check(true);edit.enabled=0;check(false);
    std::puts("Original Marcus115 rig: edited local rotation, baseline restore, all115 fixed translations/scales and nonfinger atoms passed; owned CPU buffer only, no game proof");
}
