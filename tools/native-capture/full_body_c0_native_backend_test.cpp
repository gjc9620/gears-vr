#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "full_body_c0_native_backend.h"
#include <bcrypt.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
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
    bool allowed{true};unsigned calls{},commit_calls{};int expire_commit_call{-1},reject_call{-1},stop_call{-1};n::Backend* stop_backend{};
    tx::Verdict last{tx::Verdict::denied};
    bool current(tx::Stage stage)noexcept override{
        ++calls;last=tx::Verdict::denied;
        if(stop_backend&&int(calls)==stop_call)stop_backend->request_stop();
        if(!allowed||int(calls)==reject_call)return false;
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
    // Explicit Host emulation of original Compose output. This is NOT an
    // engine execution or a physics/renderer certificate.
    void original_space(){
        std::array<body::ik::Matrix,body::bones> matrices{};
        for(unsigned i=0;i<body::bones;++i){
            std::int32_t parent{};std::memcpy(&parent,captured.reference[i].bytes+56,4);
            const auto m=body::d::atom_matrix(atoms()[i]);matrices[i]=i?body::d::multiply(m,matrices[unsigned(parent)]):m;
            body::Matrix64 value{};for(unsigned j=0;j<16;++j)value.values[j]=float(matrices[i].m[j]);
            space.put(i*64,value);
        }
    }

};
unsigned changed(body::Frame const& f,body::Output const& output){unsigned result{};for(unsigned i=0;i<230;++i)result+=unsigned(std::memcmp(reinterpret_cast<unsigned char const*>(f.local.data())+i*16,reinterpret_cast<unsigned char const*>(output.local.data())+i*16,16)!=0);return result;}
int main(int argc,char**argv){if(argc!=2&&argc!=3)return 2;std::string directory=argv[1];auto frame=std::make_unique<body::Frame>();auto workspace=std::make_unique<body::Workspace>();body::Model model{};body::Output output{};
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
    // Actual Native reads + CAS/Kernel on owned pages; only original Compose
    // output generation/current-owner admission are explicit Host seams.
    for(unsigned mode=0;mode<24;++mode){
        Fixture f(*frame);tx::Kernel k;tx::Lease l;C(f.capture());auto key=f.backend.stamp();
        auto observed=std::make_unique<n::ConsumedPose>();observed->observed_qpc=123;
        const auto before=*observed;
        C(f.backend.read_consumed_pose(k,key,0,*observed)==n::ConsumedPoseCode::transaction);
        C(!std::memcmp(observed.get(),&before,sizeof before));
        C(k.prepare(f.backend,key,f.captured,output,f.desired_copy,l).code==tx::Code::prepared);
        C(k.commit(f.backend,l,key).code==tx::Code::committed);const auto serial=k.serial();
        C(f.backend.read_consumed_pose(k,key,serial,*observed)==n::ConsumedPoseCode::transaction);
        if(mode==1)C(k.rollback(f.backend,l,key).code==tx::Code::rolled_back);
        C(k.notify_consumer_entry(f.backend,l,key).code==tx::Code::consumer_marked);
        if(mode==18){
            // The original consumer may rotate a nonselected leaf. Retain that
            // actual Local/SpaceBases instead of demanding all plan atoms.
            auto& leaf=f.atoms()[114];body::Quat original{leaf.q[0],leaf.q[1],leaf.q[2],leaf.q[3]};
            C(body::d::set_quat(leaf,body::d::product(original,body::d::yaw(.15))));
        }
        f.original_space();
        C(k.finish(f.backend,l,key,tx::End::original_returned).code==tx::Code::scope_closed);
        C(k.completed_consumer(f.backend,key,serial)==(mode!=1));
        if(mode==2)++key.scope;
        if(mode==3)f.mesh.put(0x26c,p::ArrayHeader{f.space.address(),114,115});
        if(mode==4){const float value=NAN;f.space.put(body::head*64,value);}
        if(mode==5){auto* m=reinterpret_cast<body::Matrix64*>(f.space.data);m[body::head].values[12]+=.1f;}
        if(mode==6){auto* m=reinterpret_cast<body::Matrix64*>(f.space.data);m[114].values[12]+=.1f;}
        if(mode==7)f.atoms()[114].translation[0]+=.1f;
        if(mode==8){f.atoms()[114].translation[0]+=.1f;f.original_space();}
        if(mode==9){f.atoms()[114].scale=2;f.original_space();}
        if(mode==10)f.context.allowed=false;
        DWORD protection{},ignored{};
        if(mode==11)C(VirtualProtect(f.space.data,f.space.size,PAGE_NOACCESS,&protection)!=FALSE);
        if(mode==21)C(VirtualProtect(f.local.data,f.local.size,PAGE_NOACCESS,&protection)!=FALSE);
        if(mode==12){auto m=frame->component_to_world;m.values[12]+=1;f.mesh.put(0xa0,m);}
        if(mode==13)f.reference.data[1]^=1;
        if(mode==14)std::swap(f.desired.data[0],f.desired.data[1]);
        if(mode==15)f.mesh.put(0x338,uint32_t(89));
        if(mode==16)f.backend.invalidate();
        if(mode==19){f.atoms()[0].translation[0]+=.1f;f.original_space();}
        if(mode==20){auto& head=f.atoms()[body::head];body::Quat q{head.q[0],head.q[1],head.q[2],head.q[3]};
            C(body::d::set_quat(head,body::d::product(q,body::d::yaw(.02))));f.original_space();}
        if(mode==22)f.context.reject_call=int(f.context.calls+2); // final owner guard withdraws
        if(mode==23){f.context.stop_backend=&f.backend;f.context.stop_call=int(f.context.calls+2);}

        const auto pending=k.pending();
        const auto result=f.backend.read_consumed_pose(k,key,mode==17?serial+1:serial,*observed);
        if(mode==11)C(VirtualProtect(f.space.data,f.space.size,protection,&ignored)!=FALSE);
        if(mode==21)C(VirtualProtect(f.local.data,f.local.size,protection,&ignored)!=FALSE);
        const bool accepted=mode==0||mode==18;
        if(accepted){
            C(result==n::ConsumedPoseCode::verified);
            C(observed->observed_qpc>0&&observed->consumed.serial==serial&&tx::same(observed->consumed.stamp,key));
            C(observed->max_position_error<=.002&&observed->max_angle_error<=1e-4&&observed->max_length_error<=.002);
            C(!std::memcmp(observed->local.data(),f.local.data,sizeof observed->local)&&
              !std::memcmp(observed->space.data(),f.space.data,sizeof observed->space));
        }else {C(result!=n::ConsumedPoseCode::verified);C(!std::memcmp(observed.get(),&before,sizeof before));}
        C(k.pending()==pending); // Readback cannot retire or erase write debt.
        if(mode==19||mode==20)C(result==n::ConsumedPoseCode::target);
        if(mode==22)C(result==n::ConsumedPoseCode::context);
        if(mode==23)C(result==n::ConsumedPoseCode::unavailable);
        if(mode==0){
            const auto completed=*observed;const auto calls=f.context.calls;
            n::ConsumedPoseCode foreign_thread=n::ConsumedPoseCode::verified;
            std::thread reader([&]{foreign_thread=f.backend.read_consumed_pose(k,key,serial,*observed);});reader.join();
            C(foreign_thread==n::ConsumedPoseCode::unavailable&&f.context.calls==calls&&
              !std::memcmp(observed.get(),&completed,sizeof completed));
            f.backend.request_stop();C(f.backend.read_consumed_pose(k,key,serial,*observed)==n::ConsumedPoseCode::unavailable);
            C(!std::memcmp(observed.get(),&completed,sizeof completed)&&k.pending()==pending);
        }
    }
    if(argc==3){
        namespace names=gears_cover_engine_bone_map;
        std::ifstream stream(argv[2],std::ios::binary);std::vector<unsigned char> blob((std::istreambuf_iterator<char>(stream)),{});
        C(blob.size()==20027727);
        C(gears_cover_bones::digest_matches(blob.data(),unsigned(blob.size()),"911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6"));
        if(failures)return 1;
        Page code(0xa00000),pairs(4096),buckets(4096);
        for(auto const& chunk:gears_cover_bones::chunks)std::memcpy(code.data+chunk.rva,blob.data()+chunk.rva-0x1000,chunk.size);
        auto scratch=std::make_unique<names::Workspace>();
        names::Observation sentinel{};sentinel.asset=123;
        for(unsigned mode=0;mode<13;++mode){
            Fixture f(*frame);if(mode!=1)C(f.capture());
            std::memset(pairs.data,0,pairs.size);std::array<int,256> heads{};heads.fill(-1);
            for(unsigned i=0;i<115;++i){const auto name=names::named::detail::name(frame->reference[i]);const auto bucket=unsigned(name.index)&255;
                pairs.put(i*16,heads[bucket]);pairs.put(i*16+4,name.index);pairs.put(i*16+8,name.number);pairs.put(i*16+12,i);heads[bucket]=int(i);}
            std::memcpy(buckets.data,heads.data(),sizeof heads);
            f.asset.put(0xb8,pairs.address());f.asset.put(0xc0,int(115));f.asset.put(0xc4,0x12345678u);
            f.asset.put(0xc8,buckets.address());f.asset.put(0xd0,int(256));
            if(mode==2)f.context.allowed=false;
            if(mode==3)f.mesh.put(0x218,uintptr_t(123));
            if(mode==4)f.reference.data[8]^=1;
            if(mode==5)pairs.put(0,int(0));
            if(mode==6)code.data[0x1d38d4+150]^=1;
            if(mode==7)f.context.reject_call=int(f.context.calls+3);
            if(mode==8)f.backend.request_stop();
            if(mode==10){f.context.stop_backend=&f.backend;f.context.stop_call=int(f.context.calls+3);}
            const auto calls=f.context.calls;auto observation=sentinel;names::Status result{};
            if(mode==11)result=f.backend.observe_engine_bone_map(code.address(),model,*scratch,*reinterpret_cast<names::Observation*>(f.local.data));
            else if(mode==12)result=f.backend.observe_engine_bone_map(code.address(),model,*reinterpret_cast<names::Workspace*>(&f.backend),observation);
            else if(mode==9){std::thread foreign([&]{result=f.backend.observe_engine_bone_map(code.address(),model,*scratch,observation);});foreign.join();}
            else result=f.backend.observe_engine_bone_map(code.address(),model,*scratch,observation);
            if(mode==6)code.data[0x1d38d4+150]^=1;
            if(mode==0){C(result==names::Status::okay);C(observation.matched_bones==115&&observation.cache_matches_owned_reference);
                C(!observation.getter_called&&!observation.physical_pose_verified&&!observation.world_sync_verified&&!observation.activation_allowed);}
            else {C(result!=names::Status::okay);C(!std::memcmp(&observation,&sentinel,sizeof sentinel));}
            if(mode==9)C(f.context.calls==calls);
            if(mode==11||mode==12)C(result==names::Status::alias&&f.context.calls==calls);
            C(!std::memcmp(f.local.data,frame->local.data(),sizeof frame->local));
        }
    }
    std::printf("NativeBackend: %u checks, %u failures; actual_read=1 actual_CAS128=1 actual_SEH=1 Host_context_only=1 fresh_denied=1 consumed_pose_original_Compose=Host_math live_reader_wired=0\n",checks,failures);return failures?1:0;
}
