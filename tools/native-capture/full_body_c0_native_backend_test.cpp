#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "full_body_c0_native_backend.h"
#include <bcrypt.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace n=gears_full_body_c0_native;namespace tx=n::tx;namespace body=n::body;namespace p=n::pose;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
bool read_file(std::string const& name,void* to,size_t size){std::ifstream f(name,std::ios::binary);return bool(f.read(static_cast<char*>(to),std::streamsize(size)))&&f.peek()==EOF;}
bool hash(void const* bytes,size_t size,body::rig::Signature& result){BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};
    bool ok=BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;if(ok)ok=BCryptFinishHash(h,result.data(),ULONG(result.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);return ok;}
struct Page {unsigned char* data;size_t size;
    explicit Page(size_t nbytes):data(static_cast<unsigned char*>(VirtualAlloc(nullptr,nbytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))),size(nbytes){if(!data)std::terminate();}
    ~Page(){VirtualFree(data,0,MEM_RELEASE);}uintptr_t address()const{return reinterpret_cast<uintptr_t>(data);}
    template<class T>void put(size_t offset,T const& v){std::memcpy(data+offset,&v,sizeof v);}
};
struct HostContext final:n::Context {
    bool allowed{true};unsigned calls{},commit_calls{};int expire_commit_call{-1};
    tx::Verdict last{tx::Verdict::denied};
    bool current(tx::Stage stage)noexcept override{
        ++calls;last=tx::Verdict::denied;
        if(!allowed)return false;
        if(stage==tx::Stage::commit&&int(++commit_calls)==expire_commit_call){last=tx::Verdict::source_age_expired;return false;}
        return true;
    }
    tx::Verdict rejection()const noexcept override{return last;}
};
struct Fixture {
    Page mesh{4096},asset{4096},local{4096},reference{8192},desired{4096},space{8192};
    HostContext context{};n::Backend backend{};body::Frame captured{};tx::Desired desired_copy{};tx::Stamp seed{};p::OwnerKey owner{};
    explicit Fixture(body::Frame const& f){
        std::memcpy(local.data,f.local.data(),sizeof f.local);std::memcpy(reference.data,f.reference.data(),sizeof f.reference);
        for(unsigned i=0;i<115;++i)desired.data[i]=static_cast<unsigned char>(i);std::memset(space.data,0x5a,space.size);
        mesh.put(0x218,asset.address());mesh.put(0x27c,p::ArrayHeader{local.address(),115,115});mesh.put(0x28c,p::ArrayHeader{desired.address(),115,115});
        mesh.put(0x26c,p::ArrayHeader{space.address(),115,115});asset.put(0xa4,p::ArrayHeader{reference.address(),115,115});mesh.put(0xa0,f.component_to_world);
        mesh.put(0x334,uint32_t(77));mesh.put(0x338,uint32_t(88));
        owner.world.address=111;owner.pawn.address=222;owner.mesh.address=mesh.address();owner.session=333;owner.world_generation=444;owner.scene=555;owner.scene_qpc=666;
        seed.binding.owner=mesh.address();seed.binding.model=asset.address();seed.binding.metadata_generation=444;seed.binding.session=333;seed.binding.world_generation=444;
        seed.frame=10;seed.producer=11;seed.scope=12;
    }
    bool capture(){return backend.capture(context,owner,seed,captured,desired_copy);}
    body::Atom* atoms(){return reinterpret_cast<body::Atom*>(local.data);}
};
unsigned changed(body::Frame const& f,body::Output const& output){unsigned result{};for(unsigned i=0;i<230;++i)result+=unsigned(std::memcmp(reinterpret_cast<unsigned char const*>(f.local.data())+i*16,reinterpret_cast<unsigned char const*>(output.local.data())+i*16,16)!=0);return result;}
int main(int argc,char**argv){if(argc!=2)return 2;std::string directory=argv[1];auto frame=std::make_unique<body::Frame>();auto workspace=std::make_unique<body::Workspace>();body::Model model{};body::Output output{};
    C(read_file(directory+"/reference.bin",frame->reference.data(),sizeof frame->reference));C(read_file(directory+"/C0-local.bin",frame->local.data(),sizeof frame->local));C(read_file(directory+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
    C(body::prepare_model(frame->reference,hash,model));for(unsigned i=0;i<16;++i)workspace->component.m[i]=frame->component_to_world.values[i];std::copy(frame->local.begin(),frame->local.end(),workspace->pose.local);C(body::d::rebuild(model,*workspace));
    body::Goal goal{};goal.scope=body::Scope::fixed_cover;goal.body_yaw_delta=-.1;goal.physical_world={1,2,-3};goal.committed_grip_world={2,1,-2};
    goal.head_world=body::d::pose(workspace->pose.world[55]);goal.head_world.position=body::d::add(goal.head_world.position,{3,3,-5});for(unsigned i=0;i<2;++i)goal.feet_world[i]=body::d::pose(workspace->pose.world[body::legs[i][2]]);
    C(body::plan(model,*frame,goal,*workspace,output).status==body::Status::solved);const unsigned units=changed(*frame,output);C(units==13);
    {Fixture f(*frame);tx::Kernel k;tx::Lease lease;C(f.capture());const auto key=f.backend.stamp();
        std::array<unsigned char,4096> mesh_before{},asset_before{};std::memcpy(mesh_before.data(),f.mesh.data,4096);std::memcpy(asset_before.data(),f.asset.data,4096);
        C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,lease).code==tx::Code::prepared);C(k.commit(f.backend,lease,key).code==tx::Code::committed);
        C(!std::memcmp(f.local.data,output.local.data(),sizeof output.local));C(k.rollback(f.backend,lease,key).code==tx::Code::rolled_back);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));C(k.finish(f.backend,lease,key,tx::End::before_consumer).pending==0);
        C(!std::memcmp(f.mesh.data,mesh_before.data(),4096)&&!std::memcmp(f.asset.data,asset_before.data(),4096)&&!std::memcmp(f.reference.data,frame->reference.data(),sizeof frame->reference));
        for(size_t i=0;i<f.space.size;++i)if(f.space.data[i]!=0x5a){C(false);break;}C(f.context.calls==units*4+4&&f.context.commit_calls==units*2);}
    // Complete Context validation is called once at the end of validate,
    // followed once at exchange. Expiry at either boundary must never mark
    // the failing unit uncertain or leave any bytes from prior units installed.
    for(unsigned at:{0u,3u})for(bool exchange:{false,true}){
        Fixture f(*frame);tx::Kernel k;tx::Lease lease;C(f.capture());const auto key=f.backend.stamp();
        C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,lease).code==tx::Code::prepared);
        f.context.expire_commit_call=int(at*2+(exchange?2:1));
        const auto r=k.commit(f.backend,lease,key);C(r.code==tx::Code::guard_rejected&&r.unit==at);
        C(f.context.commit_calls==unsigned(f.context.expire_commit_call));
        C(k.failure()==tx::Failure::source_age_only&&k.outstanding_fields()==0&&k.pending()==1&&!k.settled_source_age());
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));
        C(k.finish(f.backend,lease,key,tx::End::before_consumer).code==tx::Code::scope_closed);
        C(k.pending()==0&&k.settled_source_age());
    }
    // A hard non-age readback failure must win without consuming or consulting
    // the Context's stale-source reason. All memory remains unchanged.
    {Fixture f(*frame);C(f.capture());tx::Unit u{};u.bone=0;std::memcpy(&u.before,frame->local.data(),16);std::memcpy(&u.ours,output.local.data(),16);
        tx::View view{f.backend.stamp(),f.captured,output,f.desired_copy,&u,1};
        f.context.expire_commit_call=1;f.reference.data[1]^=1;const auto calls=f.context.calls;
        C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::denied&&f.context.calls==calls);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));}
    // An out-of-range commit unit is a hard permit-shape rejection even if
    // Context would report expiry. It must not consume the Context boundary.
    {Fixture f(*frame);C(f.capture());tx::Unit u{};u.bone=0;std::memcpy(&u.before,frame->local.data(),16);std::memcpy(&u.ours,output.local.data(),16);
        tx::View view{f.backend.stamp(),f.captured,output,f.desired_copy,&u,1};f.context.expire_commit_call=1;const auto calls=f.context.calls;
        C(f.backend.validate(view,tx::Stage::commit,1)==tx::Verdict::denied&&f.context.calls==calls);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));}
    // Only an exact armed permit can prove no-CAS expiry. Wrong address,
    // comparison, replacement, reused permit, and stop remain unknown.
    for(unsigned wrong=0;wrong<5;++wrong){Fixture f(*frame);C(f.capture());tx::Unit u{};u.bone=0;std::memcpy(&u.before,frame->local.data(),16);std::memcpy(&u.ours,output.local.data(),16);
        tx::View view{f.backend.stamp(),f.captured,output,f.desired_copy,&u,1};const auto prior=f.context.calls;
        C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::verified&&f.context.calls==prior+1);
        f.context.expire_commit_call=int(f.context.commit_calls+1);auto expected=u.before,replacement=u.ours;auto address=f.local.address();
        if(wrong==0)address=f.reference.address();if(wrong==1)expected.low^=1;if(wrong==2)replacement.high^=1;
        if(wrong==3)f.backend.invalidate();if(wrong==4)f.backend.request_stop();const auto calls=f.context.calls;
        C(f.backend.exchange(address,expected,replacement)==tx::Cas::fault_unknown&&f.context.calls==calls);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));
        C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::fault_unknown);}
    {Fixture f(*frame);C(f.capture());tx::Unit u{};u.bone=0;std::memcpy(&u.before,frame->local.data(),16);std::memcpy(&u.ours,output.local.data(),16);
        tx::View view{f.backend.stamp(),f.captured,output,f.desired_copy,&u,1};C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::verified);
        f.context.expire_commit_call=int(f.context.commit_calls+1);const auto calls=f.context.calls;
        // If atomic_exchange were reached this inaccessible page would return
        // fault_unknown. source_age_no_write is therefore direct no-CAS proof.
        DWORD protection{},ignored{};C(VirtualProtect(f.local.data,4096,PAGE_NOACCESS,&protection)!=FALSE);
        C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::source_age_no_write&&f.context.calls==calls+1);
        C(VirtualProtect(f.local.data,4096,protection,&ignored)!=FALSE);
        C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));
        C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::fault_unknown);}
    for(unsigned bad=0;bad<6;++bad){Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());const auto key=f.backend.stamp();
        if(bad==0)f.atoms()[114].translation[0]+=.25f;if(bad==1)f.reference.data[1]^=1;if(bad==2)std::swap(f.desired.data[0],f.desired.data[1]);
        if(bad==3)f.mesh.put(0x27c,p::ArrayHeader{f.local.address(),114,115});if(bad==4)f.mesh.put(0xa0,body::Matrix64{});if(bad==5)f.context.allowed=false;
        C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,l).code==tx::Code::guard_rejected);C(!k.pending());}
    {Fixture f(*frame);DWORD prior{},ignored{};C(VirtualProtect(f.local.data,4096,PAGE_NOACCESS,&prior)!=FALSE);C(!f.capture());C(VirtualProtect(f.local.data,4096,prior,&ignored)!=FALSE);}
    {Fixture f(*frame);C(f.capture());tx::Unit u{};u.bone=0;std::memcpy(&u.before,frame->local.data(),16);std::memcpy(&u.ours,output.local.data(),16);
        tx::View view{f.backend.stamp(),f.captured,output,f.desired_copy,&u,1};C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::verified);
        tx::Block foreign=u.before;foreign.low^=0x55;std::memcpy(f.local.data,&foreign,16);C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::mismatch);C(!std::memcmp(f.local.data,&foreign,16));
        std::memcpy(f.local.data,&u.before,16);C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::verified);
        C(f.backend.exchange(f.reference.address(),u.before,u.ours)==tx::Cas::fault_unknown);C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::fault_unknown);C(!std::memcmp(f.local.data,&u.before,16));
        C(f.backend.validate(view,tx::Stage::commit,0)==tx::Verdict::verified);DWORD prior{},ignored{};C(VirtualProtect(f.local.data,4096,PAGE_NOACCESS,&prior)!=FALSE);
        C(f.backend.exchange(f.local.address(),u.before,u.ours)==tx::Cas::fault_unknown);C(VirtualProtect(f.local.data,4096,prior,&ignored)!=FALSE);C(!std::memcmp(f.local.data,&u.before,16));}
    {Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());const auto key=f.backend.stamp();C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,l).code==tx::Code::prepared);
        f.backend.request_stop();C(k.commit(f.backend,l,key).code==tx::Code::guard_rejected);C(k.finish(f.backend,l,key,tx::End::before_consumer).pending==0);C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));}
    {Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());const auto key=f.backend.stamp();C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,l).code==tx::Code::prepared);C(k.commit(f.backend,l,key).code==tx::Code::committed);
        C(k.notify_consumer_entry(f.backend,l,key).code==tx::Code::consumer_marked);f.atoms()[114].translation[0]+=.25f; // legitimate simulated original consumer update
        C(k.finish(f.backend,l,key,tx::End::original_returned).code==tx::Code::scope_closed);C(k.outstanding_fields()==units);C(k.rollback(f.backend,l,key).code==tx::Code::wrong_lease);
        const auto before=k.pending();C(k.retire_fresh(f.backend,{123}).code==tx::Code::bad_receipt);std::memcpy(f.local.data,frame->local.data(),sizeof frame->local);C(k.retire_fresh(f.backend,{124}).code==tx::Code::bad_receipt);C(k.pending()==before);}
    std::printf("NativeBackend: %u checks, %u failures; actual_read=1 actual_CAS128=1 actual_SEH=1 Host_context_only=1 fresh_denied=1\n",checks,failures);return failures?1:0;
}
