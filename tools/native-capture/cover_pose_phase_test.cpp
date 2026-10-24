#include "cover_pose_phase.h"
#include <algorithm>
#include <cstdio>
#include <vector>
namespace p=gears_cover_pose_phase;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
constexpr std::uintptr_t base=0x100000,mesh=base+0x1000,asset=base+0x2000,refs=base+0x3000,atoms=base+0x10000,spaces=base+0x20000,desired=base+0x30000;
static p::Snapshot scratch{},output{},previous{};
enum class Mutation {none,epoch,header,payload,component,fail};
struct Fixture {
    std::vector<unsigned char> memory=std::vector<unsigned char>(0x40000);
    Mutation mutation{};unsigned reads{},atom_reads{};bool changed{};
    template<class T> void put(std::uintptr_t at,T const& value){std::memcpy(memory.data()+at-base,&value,sizeof value);}
    template<class T> T get(std::uintptr_t at){T value{};std::memcpy(&value,memory.data()+at-base,sizeof value);return value;}
    bool operator()(void* out,void const* ptr,std::size_t count){
        const auto at=reinterpret_cast<std::uintptr_t>(ptr);++reads;
        if(mutation==Mutation::fail&&reads==17)return false;
        if(at<base||at-base>memory.size()||count>memory.size()-(at-base))return false;
        if(at==atoms&&++atom_reads==2&&mutation==Mutation::payload){float value=19;put(atoms+16,value);changed=true;}
        std::memcpy(out,memory.data()+at-base,count);
        if(at==spaces&&!changed){
            if(mutation==Mutation::epoch){put(mesh+0x338,std::uint32_t(100));changed=true;}
            if(mutation==Mutation::header){auto h=get<p::ArrayHeader>(mesh+0x27c);++h.capacity;put(mesh+0x27c,h);changed=true;}
            if(mutation==Mutation::component){put(mesh+0xa0+48,2.f);changed=true;}
        }return true;
    }
    void init(unsigned n=4){
        std::fill(memory.begin(),memory.end(),static_cast<unsigned char>(0));mutation=Mutation::none;reads=atom_reads=0;changed=false;
        put(mesh+0x218,asset);put(mesh+0x248,base+0x500);put(mesh+0x250,base+0x600);
        put(asset+0xa4,p::ArrayHeader{refs,int(n),int(n)});
        put(mesh+0x27c,p::ArrayHeader{atoms,int(n),int(n)});
        put(mesh+0x26c,p::ArrayHeader{spaces,int(n),int(n)});
        put(mesh+0x28c,p::ArrayHeader{desired,int((std::min)(n,256u)),int((std::min)(n,256u))});
        put(mesh+0x334,std::uint32_t(23));put(mesh+0x338,std::uint32_t(24));
        p::Matrix matrix{};matrix.values[0]=matrix.values[5]=matrix.values[10]=matrix.values[15]=1;put(mesh+0xa0,matrix);
        for(unsigned i=0;i<n;++i){p::Atom atom{};atom.q[3]=atom.scale=1;atom.translation[0]=float(i);put(atoms+i*32,atom);put(spaces+i*64,matrix);
            put(refs+i*68+0x38,i?std::int32_t(i-1):std::int32_t(-1));
            if(i<256)put(desired+i,static_cast<unsigned char>(i));}
    }
};
p::Request request(){p::Request r{};r.owner.world.address=base+0x700;r.owner.pawn.address=base+0x800;r.owner.mesh.address=mesh;
    r.owner.world_generation=3;r.owner.session=4;r.owner.scene=5;r.owner.scene_qpc=100;r.invocation=6;r.qpc=101;r.thread=7;
    r.caller_rva=p::compose_caller;r.kind=p::Kind::compose;r.phase=p::Phase::C0;return r;}
void rejected(Fixture& f,p::Request const& r,p::Reject reason){previous=output;C(p::capture(f,r,scratch,output)==reason);C(!std::memcmp(&previous,&output,sizeof output));}
int main(){Fixture f;f.init();auto r=request();const auto original=f.memory;
    C(p::capture(f,r,scratch,output)==p::Reject::none);C(original==f.memory);C(output.metadata.pose_epoch==24);C(output.local[3].translation[0]==3);
    C(output.request.invocation==6&&output.request.phase==p::Phase::C0);C(p::parent(output.reference[0])==-1);
    r.phase=p::Phase::C1;C(p::capture(f,r,scratch,output)==p::Reject::none);
    for(auto phase:{p::Phase::P0,p::Phase::P1})for(auto caller:{p::physics_main_caller,p::physics_late_caller}){
        r=request();r.kind=p::Kind::physics;r.phase=phase;r.caller_rva=caller;r.physics_matrix=mesh+0xa0;
        C(p::capture(f,r,scratch,output)==p::Reject::none);C(output.metadata.physics.values[0]==1);}
    r=request();r.kind=p::Kind::tick;r.phase=p::Phase::L1;r.caller_rva=0x123456;C(p::capture(f,r,scratch,output)==p::Reject::none);
    r=request();r.caller_rva++;rejected(f,r,p::Reject::request);r=request();r.owner.scene_qpc=0;rejected(f,r,p::Reject::request);
    r=request();r.kind=p::Kind::physics;r.phase=p::Phase::P0;r.caller_rva=p::physics_main_caller;rejected(f,r,p::Reject::request);
    r=request();previous=output;C(p::capture(f,r,output,output)==p::Reject::alias);C(!std::memcmp(&previous,&output,sizeof output));
    for(auto mutation:{Mutation::epoch,Mutation::header,Mutation::component}){f.init();f.mutation=mutation;rejected(f,r,p::Reject::metadata_changed);C(f.changed);}
    f.init();f.mutation=Mutation::payload;rejected(f,r,p::Reject::payload_changed);C(f.changed);
    f.init();f.mutation=Mutation::fail;rejected(f,r,p::Reject::read);
    f.init();f.put(asset+0xa4,p::ArrayHeader{refs,513,513});rejected(f,r,p::Reject::bounds);
    f.init();f.put(mesh+0x27c,p::ArrayHeader{atoms,4,3});rejected(f,r,p::Reject::bounds);
    f.init();f.put(mesh+0x27c,p::ArrayHeader{atoms,4,-1});rejected(f,r,p::Reject::bounds);
    f.init();f.put(mesh+0x26c,p::ArrayHeader{spaces,3,3});rejected(f,r,p::Reject::shape);
    f.init();f.put(mesh+0x27c,p::ArrayHeader{UINTPTR_MAX-15,4,4});rejected(f,r,p::Reject::bounds);
    f.init();f.put(mesh+0x28c,p::ArrayHeader{});C(p::capture(f,r,scratch,output)==p::Reject::none);
    f.init();f.put(refs+68+0x38,std::int32_t(2));f.put(refs+136+0x38,std::int32_t(1));rejected(f,r,p::Reject::parent);
    f.init();f.put(refs+68+0x38,std::int32_t(8));rejected(f,r,p::Reject::parent);
    f.init();f.put(refs+68+0x38,std::int32_t(-1));rejected(f,r,p::Reject::parent);
    f.init();f.put(refs+0x38,std::int32_t(0));C(p::capture(f,r,scratch,output)==p::Reject::none);
    f.init();f.put(desired+1,static_cast<unsigned char>(0));rejected(f,r,p::Reject::desired);
    f.init();f.put(desired+1,static_cast<unsigned char>(4));rejected(f,r,p::Reject::desired);
    for(unsigned n=0;n<8;++n){f.init();f.put(atoms+n*4,(std::numeric_limits<float>::infinity)());rejected(f,r,p::Reject::nonfinite);}
    for(unsigned n=0;n<16;++n){f.init();f.put(spaces+n*4,(std::numeric_limits<float>::quiet_NaN)());rejected(f,r,p::Reject::nonfinite);}
    f.init();f.put(mesh+0xa0,(std::numeric_limits<float>::infinity)());rejected(f,r,p::Reject::nonfinite);
    f.init(512);C(p::capture(f,r,scratch,output)==p::Reject::none);C(output.metadata.reference.count==512);
    f.init();r.owner.mesh.address=UINTPTR_MAX-0x100;rejected(f,r,p::Reject::read);
    C(!p::span(UINTPTR_MAX,2));C(!p::span(0,4));
    std::printf("cover_pose_phase: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
