#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "cover_engine_bone_map.h"
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
namespace api=gears_cover_engine_bone_map;namespace body=api::body;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t n,body::rig::Signature& out){
    BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};bool okay=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(n),0)>=0;
    if(okay)okay=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return okay;
}
bool file(std::string const& p,void* to,std::size_t size){std::ifstream f(p,std::ios::binary);return bool(f.read(static_cast<char*>(to),std::streamsize(size)))&&f.peek()==EOF;}
struct Fixture {
    static constexpr uintptr_t base=0x140000000,asset=0x20000000,reference=0x21000000,pairs=0x22000000,buckets=0x23000000;
    body::Frame frame{};body::Model model{};api::Workspace work{};api::Observation output{};
    std::map<uintptr_t,std::vector<unsigned char>> memory;
    unsigned reads{},guards{};std::size_t bytes{};int fail_read{-1},fail_guard{-1};std::function<void(unsigned)> at_guard;
    template<class T>void put(uintptr_t at,T const& value){auto found=memory.upper_bound(at);if(found==memory.begin())std::terminate();--found;
        if(at-found->first+sizeof value>found->second.size())std::terminate();std::memcpy(found->second.data()+at-found->first,&value,sizeof value);}
    bool read(void* to,void const* ptr,std::size_t size){++reads;bytes+=size;if(int(reads)==fail_read)return false;const auto at=reinterpret_cast<uintptr_t>(ptr);
        auto found=memory.upper_bound(at);if(found==memory.begin())return false;--found;
        if(at<found->first||at-found->first>found->second.size()||size>found->second.size()-(at-found->first))return false;
        std::memcpy(to,found->second.data()+at-found->first,size);return true;}
    bool guard(){++guards;if(at_guard)at_guard(guards);return int(guards)!=fail_guard;}
    explicit Fixture(body::Frame const& f,std::vector<unsigned char> const& blob):frame(f){
        if(!body::prepare_model(frame.reference,hash,model))std::terminate();
        memory[asset+0xa4]=std::vector<unsigned char>(16);memory[asset+0xb8]=std::vector<unsigned char>(28);
        put(asset+0xa4,reference);put(asset+0xac,int(115));put(asset+0xb0,int(115));
        put(asset+0xb8,pairs);put(asset+0xc0,int(115));put(asset+0xc4,0x76543210u); // Opaque, never a capacity.
        put(asset+0xc8,buckets);put(asset+0xd0,int(256));
        memory[reference]=std::vector<unsigned char>(sizeof frame.reference);std::memcpy(memory[reference].data(),frame.reference.data(),sizeof frame.reference);
        memory[pairs]=std::vector<unsigned char>(115*16);memory[buckets]=std::vector<unsigned char>(256*4,0xff);
        std::array<int,256> heads{};heads.fill(-1);
        for(unsigned i=0;i<115;++i){const auto name=api::named::detail::name(frame.reference[i]);const auto bucket=unsigned(name.index)&255;
            put(pairs+i*16,heads[bucket]);put(pairs+i*16+4,name.index);put(pairs+i*16+8,name.number);put(pairs+i*16+12,i);heads[bucket]=int(i);}
        std::memcpy(memory[buckets].data(),heads.data(),sizeof heads);
        for(auto const& chunk:gears_cover_bones::chunks){const auto begin=chunk.rva-0x1000;
            if(begin+chunk.size>blob.size())std::terminate();memory[base+chunk.rva]=std::vector<unsigned char>(blob.begin()+begin,blob.begin()+begin+chunk.size);}
    }
    api::Status run(){auto reader=[&](void* to,void const* at,std::size_t size){return read(to,at,size);};auto current=[&](){return guard();};
        return api::observe(base,asset,model,frame,reader,current,work,output);}
};
int main(int argc,char** argv){
    if(argc!=3)return 2;
    auto frame=std::make_unique<body::Frame>();C(file(std::string(argv[1])+"/reference.bin",frame->reference.data(),sizeof frame->reference));
    std::ifstream stream(argv[2],std::ios::binary);std::vector<unsigned char> blob((std::istreambuf_iterator<char>(stream)),{});
    C(blob.size()==20027727);if(failures)return 1;
    C(gears_cover_bones::digest_matches(blob.data(),unsigned(blob.size()),"911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6"));if(failures)return 1;
    auto f=std::make_unique<Fixture>(*frame,blob);const auto before=f->memory;
    C(f->run()==api::Status::okay);C(f->reads==42&&f->bytes==26992&&f->guards==3);C(f->memory==before);
    C(f->output.matched_bones==115&&f->output.bucket_count==256&&f->output.cache_matches_owned_reference&&f->output.closing_checked);
    C(!f->output.getter_called&&!f->output.physical_pose_verified&&!f->output.world_sync_verified&&!f->output.activation_allowed);
    const auto sentinel=f->output;
    auto rejected=[&](Fixture& value,api::Status expected){value.output=sentinel;C(value.run()==expected);C(!std::memcmp(&value.output,&sentinel,sizeof sentinel));};
    for(unsigned stage=1;stage<=3;++stage){f=std::make_unique<Fixture>(*frame,blob);f->fail_guard=int(stage);rejected(*f,api::Status::context);}
    for(unsigned at=1;at<=42;++at){f=std::make_unique<Fixture>(*frame,blob);f->fail_read=int(at);
        // certify intentionally distinguishes a failed code read from payload.
        f->output=sentinel;C(f->run()!=api::Status::okay);C(!std::memcmp(&f->output,&sentinel,sizeof sentinel));C(f->reads==at);}
    for(unsigned defect=0;defect<8;++defect){f=std::make_unique<Fixture>(*frame,blob);
        if(defect==0)f->put(f->asset+0xac,int(114));else if(defect==1)f->put(f->asset+0xb0,int(513));
        else if(defect==2)f->put(f->asset+0xc0,int(116));else if(defect==3)f->put(f->asset+0xd0,int(0));
        else if(defect==4)f->put(f->asset+0xd0,int(255));else if(defect==5)f->put(f->asset+0xd0,int(2048));
        else if(defect==6)f->put(f->asset+0xc8,uintptr_t(UINTPTR_MAX-8));else f->put(f->asset+0xb8,uintptr_t(1));
        rejected(*f,api::Status::header);}
    for(unsigned defect=0;defect<7;++defect){f=std::make_unique<Fixture>(*frame,blob);
        if(defect==0)f->put(f->pairs,int(0)); // Cycle at entry0.
        else if(defect==1)f->put(f->pairs,int(-2));
        else if(defect==2)f->put(f->pairs,int(115));
        else if(defect==3)f->put(f->pairs+12,unsigned(115));
        else if(defect==4)f->put(f->pairs+8,unsigned(1));
        else if(defect==5)f->put(f->pairs+4,int(-1));
        else {const auto key=api::named::detail::name(frame->reference[0]);f->put(f->buckets+(unsigned(key.index)&255)*4,int(-1));}
        f->output=sentinel;C(f->run()!=api::Status::okay);C(!std::memcmp(&f->output,&sentinel,sizeof sentinel));}
    for(unsigned defect=0;defect<5;++defect){f=std::make_unique<Fixture>(*frame,blob);
        f->at_guard=[&](unsigned stage){if(stage!=2)return;
            if(defect==0)f->put(f->reference+8,unsigned(99));else if(defect==1)f->put(f->pairs+12,unsigned(1));
            else if(defect==2)f->put(f->buckets,int(99));else if(defect==3)f->put(f->asset+0xc4,unsigned(99));
            else f->put(f->asset+0xa4,uintptr_t(f->reference+68));};
        rejected(*f,api::Status::changed);}
    f=std::make_unique<Fixture>(*frame,blob);f->memory[f->base+0x1d38d4].back()^=1;rejected(*f,api::Status::code);
    f=std::make_unique<Fixture>(*frame,blob);f->model.parents[55]=0;rejected(*f,api::Status::model);
    std::printf("checks=%u failures=%u complete_fixture_reads=42 complete_fixture_bytes=26992 original_code_plus_synthetic_cache=true getter_called=false current_game_verified=false\n",checks,failures);
    return failures?1:0;
}
