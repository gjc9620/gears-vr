#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "cover_physics_bone_map.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
namespace api=gears_cover_physics_bones;
namespace body=api::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,body::rig::Signature& out){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE h{};
    bool okay=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(algorithm,&h,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(okay)okay=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return okay;
}
bool read(std::string const& path,void* data,std::size_t n){
    std::ifstream file(path,std::ios::binary);
    return bool(file.read(static_cast<char*>(data),std::streamsize(n)))&&file.peek()==EOF;
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    auto frame=std::make_unique<body::Frame>();body::Model model{};
    C(read(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    C(body::prepare_model(frame->reference,hash,model));if(failures)return 1;
    std::array<api::Input,api::max_bodies> input{};
    for(unsigned i=0;i<input.size();++i)input[i]={i,api::detail::name(frame->reference[i])};
    api::Mapping out{};C(api::bind(model,*frame,input.data(),29,1,out)==api::Status::okay);
    C(out.count==29&&out.root_body_index==1&&out.root_bone_index==1);
    for(unsigned i=0;i<29;++i){C(out.rows[i].bone_index==i);C(api::same(out.rows[i].name,input[i].bone));}
    // Shuffled input order must find the actual named bone, never assume the
    // body-array index is a skeleton index or require a mapped root bone.
    const unsigned selected[]={55,11,1,10,8,91,92,93};
    for(unsigned i=0;i<8;++i)input[i]={i,api::detail::name(frame->reference[selected[i]])};
    C(api::bind(model,*frame,input.data(),8,2,out)==api::Status::okay);
    C(out.root_bone_index==1);
    for(unsigned i=0;i<8;++i)C(out.rows[i].bone_index==selected[i]);
    C(out.rows[1].nearest_named_body_ancestor==3); // Hand11 -> Forearm10.
    C(out.rows[3].nearest_named_body_ancestor==4); // Forearm10 skips unmapped9 -> Arm8.
    C(out.rows[7].nearest_named_body_ancestor==6); // Foot93 -> Calf92.
    C(out.rows[2].nearest_named_body_ancestor==-1); // Pelvis1's root is unmapped.
    // Nonzero FName numbers must be preserved and compared in full.
    auto numbered=*frame;const std::uint32_t number=7;
    std::memcpy(numbered.reference[55].bytes+4,&number,4);input[0].bone.number=number;
    C(api::bind(model,numbered,input.data(),8,2,out)==api::Status::okay);
    C(out.rows[0].name.number==number);
    auto sentinel=out;
    auto reject=[&](body::Model const& m,body::Frame const& f,api::Input const* in,unsigned n,unsigned root,api::Status expected){
        out=sentinel;C(api::bind(m,f,in,n,root,out)==expected);C(!std::memcmp(&out,&sentinel,sizeof out));
    };
    reject(model,*frame,input.data(),8,2,api::Status::missing_bone);
    input[0].bone.number=0;
    auto bad_input=input;bad_input[0].bone.index=-1;
    reject(model,*frame,bad_input.data(),8,2,api::Status::name);
    bad_input=input;bad_input[1].body_index=0;
    reject(model,*frame,bad_input.data(),8,2,api::Status::body_index);
    bad_input=input;bad_input[1].bone=bad_input[0].bone;
    reject(model,*frame,bad_input.data(),8,2,api::Status::duplicate_body_bone);
    bad_input=input;bad_input[0].bone.index=INT32_MAX;
    reject(model,*frame,bad_input.data(),8,2,api::Status::missing_bone);
    auto bad_frame=*frame;std::memcpy(bad_frame.reference[114].bytes,bad_frame.reference[113].bytes,8);
    reject(model,bad_frame,input.data(),8,2,api::Status::duplicate_reference);
    bad_frame=*frame;const std::int32_t invalid=-1;std::memcpy(bad_frame.reference[114].bytes,&invalid,4);
    reject(model,bad_frame,input.data(),8,2,api::Status::name);
    bad_frame=*frame;bad_frame.reference[114].bytes[12]^=1;
    reject(model,bad_frame,input.data(),8,2,api::Status::model);
    bad_frame=*frame;bad_frame.count=114;reject(model,bad_frame,input.data(),8,2,api::Status::model);
    auto bad_model=model;bad_model.signature[0]^=1;
    reject(bad_model,*frame,input.data(),8,2,api::Status::model);
    bad_model=model;bad_model.parents[55]=0;
    reject(bad_model,*frame,input.data(),8,2,api::Status::model);
    bad_model=model;bad_model.reference[55].scale=2;
    reject(bad_model,*frame,input.data(),8,2,api::Status::model);
    reject(model,*frame,nullptr,8,2,api::Status::request);
    reject(model,*frame,input.data(),0,0,api::Status::request);
    reject(model,*frame,input.data(),65,0,api::Status::request);
    reject(model,*frame,input.data(),8,8,api::Status::request);
    // Aliased caller-owned buffers reject before either input or output writes.
    out=sentinel;
    C(api::bind(model,*frame,reinterpret_cast<api::Input const*>(&out),8,2,out)==api::Status::alias);
    C(!std::memcmp(&out,&sentinel,sizeof out));
    for(unsigned i=0;i<input.size();++i)input[i]={i,api::detail::name(frame->reference[114-i])};
    C(api::bind(model,*frame,input.data(),64,63,out)==api::Status::okay);
    C(out.count==64&&out.root_bone_index==51);
    for(unsigned i=0;i<64;++i)C(out.rows[i].bone_index==114-i);
    std::printf("checks=%u failures=%u retained_reference_plus_synthetic_body_names=true current_mapping_verified=false physics_verified=false\n",checks,failures);
    return failures?1:0;
}
