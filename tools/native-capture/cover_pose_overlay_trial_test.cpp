#include "cover_pose_overlay_trial.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>

namespace trial=gears_cover_pose_overlay_trial;
namespace pose=gears_cover_pose_phase;
namespace ik=gears_cover_skeleton_ik;
namespace {
unsigned checks{},scenarios{};
void check(bool value,char const* expression,int line){++checks;if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}}
#define CHECK(x) check((x),#x,__LINE__)
constexpr std::uintptr_t base=0x100000,mesh=base+0x1000,asset=base+0x30000,local=base+0x10000,space=base+0x18000,reference=base+0x24000,desired=base+0x38000;
unsigned bones=16;
struct IO {
    std::array<unsigned char,0x40000> memory{};unsigned writes{},fail_on{},stop_on{},deny_writable_at{};
    bool fault{},deny_after_fault{},bad_write{},deny_read{};trial::Trial* owner{};
    unsigned reenter_on{},mutate_desired_on{};trial::Input reenter_input{};trial::Workspace* reenter_work{};trial::Status reenter_status{};
    struct Write {std::uintptr_t address;std::size_t size;};std::array<Write,4096> history{};
    bool bounds(std::uintptr_t a,std::size_t n) const noexcept {return a>=base&&a-base<=memory.size()&&n<=memory.size()-(a-base);}
    bool read(void* out,std::uintptr_t a,std::size_t n) noexcept {if(deny_read||!bounds(a,n))return false;std::memcpy(out,memory.data()+a-base,n);return true;}
    bool writable(std::uintptr_t a,std::size_t n) noexcept {return bounds(a,n)&&a%4==0&&n%4==0&&!(deny_after_fault&&fault)&&a!=deny_writable_at;}
    bool write(std::uintptr_t a,void const* in,std::size_t n) noexcept {
        if(!writable(a,n))return false;CHECK(writes<history.size());history[writes++]={a,n};
        if(writes==fail_on){fault=true;std::memcpy(memory.data()+a-base,in,n/2);return false;}
        std::memcpy(memory.data()+a-base,in,n);if(bad_write){memory[a-base]^=0x80;bad_write=false;}
        if(writes==stop_on&&owner)owner->request_stop();
        if(writes==mutate_desired_on)memory[desired-base]^=0x20;
        if(writes==reenter_on&&owner&&reenter_work)reenter_status=owner->feed(reenter_input,*this,*reenter_work);return true;
    }
    template<class T> void put(std::uintptr_t a,T const& value){CHECK(bounds(a,sizeof value));std::memcpy(memory.data()+a-base,&value,sizeof value);}
    template<class T> T get(std::uintptr_t a){T value{};CHECK(read(&value,a,sizeof value));return value;}
    void clear_faults(){fail_on=stop_on=deny_writable_at=0;fault=deny_after_fault=bad_write=deny_read=false;}
} io;
trial::Trial state;trial::Workspace work;pose::Snapshot scratch{},snapshot{};
ik::Workspace fk_work;std::array<ik::Atom,pose::max_bones> atoms{};std::array<std::int32_t,pose::max_bones> parents{};
std::array<ik::Matrix,pose::max_bones> fk_space{},fk_world{};ik::Matrix component{};
std::array<unsigned char,pose::max_bones*32> original_local{},before_local{};
std::array<unsigned char,pose::max_bones*64> before_space{};
std::array<unsigned char,pose::max_bones> before_desired{};
trial::Params params(){trial::Params p{};p.enabled=true;p.delta={-1,1,0};p.roll=.4;p.ticks=1;return p;}
void native_compose(){
    CHECK(io.read(atoms.data(),local,bones*32));for(unsigned i=0;i<bones;++i)parents[i]=io.get<std::int32_t>(reference+i*68+0x38);
    const auto c=io.get<pose::Matrix>(mesh+0xa0);for(unsigned j=0;j<16;++j)component.m[j]=c.values[j];
    CHECK(ik::forward(atoms.data(),parents.data(),bones,component,fk_work,fk_space.data(),fk_world.data())==ik::Status::okay);
    for(unsigned i=0;i<bones;++i){pose::Matrix packed{};for(unsigned j=0;j<16;++j)packed.values[j]=static_cast<float>(fk_space[i].m[j]);
        // Native non-subtree rounding must remain untouched by the overlay.
        if(i==0||i==5)packed.values[12]+=1e-5f;io.put(space+i*64,packed);}
}
void reset(){
    ++scenarios;bones=16;state.~Trial();new(&state) trial::Trial();io={};io.owner=&state;
    const auto n=static_cast<std::int32_t>(bones);
    pose::Metadata m{};m.asset=asset;m.reference={reference,n,n};m.local={local,n,n};m.space={space,n,n};m.desired={desired,n,n};
    m.component.values[0]=m.component.values[5]=m.component.values[10]=m.component.values[15]=1;
    m.component.values[12]=10;m.component.values[13]=20;m.component.values[14]=30;
    io.put(mesh+0x218,m.asset);io.put(mesh+0x29c,m.master);io.put(mesh+0x248,m.physics_asset);io.put(mesh+0x250,m.physics_instance);
    io.put(asset+0xa4,m.reference);io.put(mesh+0x27c,m.local);io.put(mesh+0x28c,m.desired);io.put(mesh+0x26c,m.space);io.put(mesh+0xa0,m.component);
    for(unsigned i=0;i<bones;++i){pose::Atom a{};a.q[3]=1;a.scale=1;
        if(i==0){a.translation[0]=2;a.translation[1]=3;}
        if(i==10)a.translation[0]=3;if(i==11)a.translation[0]=2;
        if(i==12)a.translation[1]=1;
        const std::int32_t parent=i==8?6:i==10?8:i==11?10:i==12?11:0;
        io.put(local+i*32,a);io.put(reference+i*68+0x38,parent);io.put(desired+i,static_cast<unsigned char>(i));}
    native_compose();CHECK(io.read(original_local.data(),local,original_local.size()));io.writes=0;
}
trial::Input take(pose::Phase phase,std::uint64_t tick){
    pose::Request request{};request.owner.world={0x900000,1,2,1,3,0};request.owner.pawn={0x910000,4,2,5,6,0};request.owner.mesh={mesh,7,0x910000,8,9,0};
    request.owner.world_generation=1;request.owner.session=77;request.owner.scene=100+tick;request.owner.scene_qpc=1000+static_cast<std::int64_t>(tick);
    request.thread=4;request.invocation=phase==pose::Phase::L1?tick:tick*10+1;request.qpc=10000+static_cast<std::int64_t>(tick)*10+static_cast<int>(phase);
    request.phase=phase;request.kind=phase==pose::Phase::L1?pose::Kind::tick:pose::Kind::compose;request.caller_rva=phase==pose::Phase::L1?0x12e537:pose::compose_caller;
    trial::detail::Reader<IO> read{io};CHECK(pose::capture(read,request,scratch,snapshot)==pose::Reject::none);
    trial::Input in{};in.snapshot=&snapshot;in.scope.owner=request.owner;in.scope.nonce=1;in.scope.tick=tick;in.scope.call=request.invocation;
    in.scope.thread=request.thread;in.scope.phase=phase;in.scope.proofs=trial::required_proofs;in.scope.reference_sha256[0]=0xab;return in;
}
trial::Status feed(pose::Phase phase,std::uint64_t tick){return state.feed(take(phase,tick),io,work);}
void save_arrays(){CHECK(io.read(before_local.data(),local,bones*32));CHECK(io.read(before_space.data(),space,bones*64));CHECK(io.read(before_desired.data(),desired,bones));}
void exact_arrays(){CHECK(!std::memcmp(io.memory.data()+local-base,before_local.data(),bones*32));
    CHECK(!std::memcmp(io.memory.data()+space-base,before_space.data(),bones*64));CHECK(!std::memcmp(io.memory.data()+desired-base,before_desired.data(),bones));}
void apply(){CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();save_arrays();CHECK(feed(pose::Phase::C1,1)==trial::Status::applied);CHECK(state.pending());}
void restored_tick(std::uint64_t tick){
    CHECK(feed(pose::Phase::C0,tick)==trial::Status::restored);CHECK(state.pending());
    CHECK(!std::memcmp(io.memory.data()+local-base,original_local.data(),original_local.size()));
    native_compose();CHECK(feed(pose::Phase::C1,tick)==trial::Status::okay);CHECK(state.pending());
    CHECK(feed(pose::Phase::L1,tick)==trial::Status::drained);CHECK(!state.pending());
}
void normal_and_limits(){
    reset();CHECK(feed(pose::Phase::C0,1)==trial::Status::disabled);auto p=params();p.enabled=false;CHECK(!state.start(p,1));p=params();p.delta.x=6;CHECK(!state.start(p,1));
    p=params();p.roll=.81;CHECK(!state.start(p,1));p=params();p.ticks=17;CHECK(!state.start(p,1));p=params();p.chain.upper=0;CHECK(!state.start(p,1));
    p=params();p.delta.x=(std::numeric_limits<double>::quiet_NaN)();CHECK(!state.start(p,1));CHECK(io.writes==0);apply();
    CHECK(!state.start(params(),2));CHECK(!trial::Trial::physics_outcome_proven&&!trial::Trial::global_exclusivity_proven);
    for(unsigned i=0;i<bones;++i){const auto a=io.get<pose::Atom>(local+i*32);pose::Atom old{};std::memcpy(&old,original_local.data()+i*32,32);
        CHECK(a.scale==old.scale);CHECK(!std::memcmp(a.translation,old.translation,12));if(i!=8&&i!=10&&i!=11)CHECK(!std::memcmp(&a,&old,32));
        if(i!=8&&i!=10&&i!=11&&i!=12)CHECK(!std::memcmp(io.memory.data()+space-base+i*64,before_space.data()+i*64,64));}
    for(unsigned i=0;i<io.writes;++i){const auto h=io.history[i];CHECK(h.address%4==0&&h.size%4==0);
        if(h.address>=space&&h.address<space+bones*64){const auto bone=(h.address-space)/64;CHECK(bone==8||bone==10||bone==11||bone==12);}
        else CHECK(h.address==local+8*32||h.address==local+10*32||h.address==local+11*32);}
    CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);restored_tick(2);CHECK(state.stats().applied_ticks==1&&state.stats().native_l1==1);
    CHECK(!state.start(params(),1));CHECK(state.start(params(),2));
    reset();p=params();p.ticks=16;CHECK(state.start(p,1));
    for(unsigned tick=1;tick<=16;++tick){CHECK(feed(pose::Phase::C0,tick)==(tick==1?trial::Status::okay:trial::Status::restored));native_compose();
        CHECK(feed(pose::Phase::C1,tick)==trial::Status::applied);CHECK(feed(pose::Phase::L1,tick)==trial::Status::okay);CHECK(state.pending());}
    CHECK(state.stats().applied_ticks==16);restored_tick(17);
    reset();p=params();p.delta={};p.roll=0;p.ticks=2;CHECK(state.start(p,1));
    for(unsigned tick=1;tick<=2;++tick){CHECK(feed(pose::Phase::C0,tick)==trial::Status::okay);native_compose();CHECK(feed(pose::Phase::C1,tick)==trial::Status::okay);
        CHECK(!state.pending()&&io.writes==0);CHECK(feed(pose::Phase::L1,tick)==(tick==2?trial::Status::drained:trial::Status::okay));}
    reset();p=params();p.adaptive=true;p.delta={5,0,0};CHECK(state.start(p,1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();
    CHECK(feed(pose::Phase::C1,1)==trial::Status::applied);CHECK(io.get<pose::Atom>(local+10*32).translation[0]>3);
    CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);restored_tick(2);
}
void rollback_cases(){
    for(unsigned failed:{1u,2u,5u}){reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();save_arrays();
        const auto in=take(pose::Phase::C1,1);io.fail_on=failed;CHECK(state.feed(in,io,work)==trial::Status::write_failed);exact_arrays();CHECK(state.pending());
        CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);CHECK(!state.pending());}
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();save_arrays();auto in=take(pose::Phase::C1,1);
    io.fail_on=2;io.deny_after_fault=true;CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(state.pending());CHECK(!state.start(params(),2));
    io.clear_faults();restored_tick(2);
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();save_arrays();in=take(pose::Phase::C1,1);
    io.stop_on=1;CHECK(state.feed(in,io,work)==trial::Status::write_failed);exact_arrays();CHECK(state.pending());
    CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);CHECK(!state.pending());
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();in=take(pose::Phase::C1,1);
    io.deny_writable_at=local+8*32;CHECK(state.feed(in,io,work)==trial::Status::write_failed);CHECK(io.writes==0&&!state.pending());
    io.clear_faults();CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();save_arrays();in=take(pose::Phase::C1,1);
    io.reenter_on=1;io.reenter_input=in;io.reenter_work=&work;CHECK(state.feed(in,io,work)==trial::Status::write_failed);
    CHECK(io.reenter_status==trial::Status::rollback_failed);exact_arrays();CHECK(state.pending());CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);
    reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);io.writes=0;io.fail_on=1;
    CHECK(feed(pose::Phase::C0,2)==trial::Status::rollback_failed);CHECK(state.pending());const auto writes=io.writes;
    CHECK(feed(pose::Phase::C0,2)==trial::Status::rollback_failed);CHECK(io.writes==writes);io.clear_faults();restored_tick(3);
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();in=take(pose::Phase::C1,1);
    io.bad_write=true;CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(state.pending());
}
void context_and_order(){
    for(unsigned variant=0;variant<10;++variant){reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);
        auto in=take(pose::Phase::C0,2);const auto writes=io.writes;
        if(variant==0)in.scope.nonce=2;
        if(variant==1){in.scope.owner.pawn.address+=4;snapshot.request.owner=in.scope.owner;}
        if(variant==2){in.scope.thread=8;snapshot.request.thread=8;}
        if(variant==3)in.scope.ct=1;
        if(variant==4)in.scope.sm=1;
        if(variant==5)in.scope.proofs^=1;
        if(variant==6)in.scope.reference_sha256[1]=1;
        if(variant==7){snapshot.metadata.asset+=8;io.put(mesh+0x218,snapshot.metadata.asset);}
        if(variant==8){snapshot.metadata.local.data+=4;io.put(mesh+0x27c,snapshot.metadata.local);}
        if(variant==9){snapshot.reference[2].bytes[0]=1;io.put(reference+2*68,snapshot.reference[2]);}
        CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(io.writes==writes&&state.pending());}
    reset();apply();const auto writes=io.writes;CHECK(feed(pose::Phase::C1,1)!=trial::Status::applied);CHECK(io.writes==writes&&state.pending());
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C1,1)!=trial::Status::applied);CHECK(io.writes==0&&!state.pending());
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);CHECK(feed(pose::Phase::C0,1)==trial::Status::sequence);CHECK(io.writes==0);
    reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);CHECK(feed(pose::Phase::C0,2)==trial::Status::restored);
    CHECK(feed(pose::Phase::L1,2)==trial::Status::rollback_failed);CHECK(state.pending());restored_tick(3); // Faulted recovery needs a fresh C0, not old L1 replay.
    reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);auto a=io.get<pose::Atom>(local+8*32);a.q[1]+=.002f;io.put(local+8*32,a);
    CHECK(feed(pose::Phase::C0,2)==trial::Status::restored);const auto after=io.get<pose::Atom>(local+8*32);CHECK(!std::memcmp(&a,&after,32));CHECK(state.pending());
    CHECK(state.stats().native_replaced_locals>0);native_compose();CHECK(feed(pose::Phase::C1,2)==trial::Status::rollback_failed);CHECK(state.pending());
    // Some changed Space words still equal old overlay: without real store
    // coverage that remains unresolved, even if this test recomputed them.
    std::memcpy(io.memory.data()+local-base,original_local.data(),bones*32);restored_tick(3);
    reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);auto in=take(pose::Phase::C0,2);io.deny_read=true;
    const auto count=io.writes;CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(io.writes==count&&state.pending());
    reset();apply();state.request_stop();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);restored_tick(2);
    reset();auto p=params();p.delta={5,0,0};CHECK(state.start(p,1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();
    CHECK(feed(pose::Phase::C1,1)==trial::Status::rejected);CHECK(io.writes==0&&!state.pending());CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);
}
void additional_concerns(){
    // A near-FK stale Space must not discharge the exact old overlay journal.
    reset();auto p=params();p.delta={-1e-5,0,0};p.roll=0;CHECK(state.start(p,1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);
    native_compose();CHECK(feed(pose::Phase::C1,1)==trial::Status::applied);CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);
    CHECK(feed(pose::Phase::C0,2)==trial::Status::restored); // Intentionally omit native Space stores this time.
    CHECK(feed(pose::Phase::C1,2)==trial::Status::rollback_failed);CHECK(state.pending());restored_tick(3);
    reset();p=params();p.delta={-1e-5,1e-5,0};p.roll=1e-5;CHECK(state.start(p,1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);
    native_compose();save_arrays();CHECK(feed(pose::Phase::C1,1)==trial::Status::applied);CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);
    CHECK(feed(pose::Phase::C0,2)==trial::Status::restored);unsigned changed{},replaced{};
    for(unsigned j=0;j<16;++j)if(std::memcmp(io.memory.data()+space-base+8*64+j*4,before_space.data()+8*64+j*4,4)){
        ++changed;if(!replaced){std::memcpy(io.memory.data()+space-base+8*64+j*4,before_space.data()+8*64+j*4,4);++replaced;}}
    CHECK(changed>1&&replaced==1);CHECK(feed(pose::Phase::C1,2)==trial::Status::rollback_failed);CHECK(state.pending());restored_tick(3);
    // An L1 fault has an explicit fresh-C0 recovery path, not old-stage replay.
    reset();apply();auto in=take(pose::Phase::L1,1);in.scope.thread=9;snapshot.request.thread=9;
    CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(state.pending());restored_tick(2);
    // A valid one-byte desired list aliases a zero byte of the selected q.
    reset();CHECK(state.start(params(),1));auto h=io.get<pose::ArrayHeader>(mesh+0x28c);h={local+8*32,1,1};io.put(mesh+0x28c,h);
    CHECK(feed(pose::Phase::C0,1)==trial::Status::context);CHECK(io.writes==0&&!state.pending());
    // Native metadata and the POD control object are protected write targets.
    for(unsigned variant=0;variant<2;++variant){reset();CHECK(state.start(params(),1));in=take(pose::Phase::C0,1);
        snapshot.metadata.local.data=variant?reinterpret_cast<std::uintptr_t>(&in.scope):mesh+0xa0;io.put(mesh+0x27c,snapshot.metadata.local);
        CHECK(state.feed(in,io,work)==trial::Status::context);CHECK(io.writes==0);}
    reset();CHECK(state.start(params(),1));CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);native_compose();in=take(pose::Phase::C1,1);io.mutate_desired_on=1;
    CHECK(state.feed(in,io,work)==trial::Status::rollback_failed);CHECK(state.pending()); // Cannot rollback under changed native controls.
    // L1's native pipeline may change locals/Space; it is not C0 or C1 equality.
    reset();apply();CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);CHECK(feed(pose::Phase::C0,2)==trial::Status::restored);
    native_compose();CHECK(feed(pose::Phase::C1,2)==trial::Status::okay);auto atom=io.get<pose::Atom>(local+5*32);atom.translation[1]=.25f;io.put(local+5*32,atom);native_compose();
    const auto writes=io.writes;CHECK(feed(pose::Phase::L1,2)==trial::Status::drained);CHECK(!state.pending()&&io.writes==writes);
    CHECK(state.stats().c1_l1_changed_atoms>0&&state.stats().c1_l1_changed_space>0);CHECK(io.get<pose::Atom>(local+5*32).translation[1]==.25f);
}
std::array<pose::Snapshot,3> actual_pair{};
template<class T> void read(std::ifstream& f,T* out,std::size_t n){f.read(reinterpret_cast<char*>(out),static_cast<std::streamsize>(n*sizeof(T)));CHECK(static_cast<bool>(f));}
void install_payload(pose::Snapshot const& source){
    bones=static_cast<unsigned>(source.metadata.local.count);auto m=source.metadata;m.asset=asset;
    m.reference.data=reference;m.local.data=local;m.space.data=space;m.desired.data=desired;
    io.put(mesh+0x218,m.asset);io.put(mesh+0x29c,m.master);io.put(mesh+0x248,m.physics_asset);io.put(mesh+0x250,m.physics_instance);
    io.put(asset+0xa4,m.reference);io.put(mesh+0x27c,m.local);io.put(mesh+0x28c,m.desired);io.put(mesh+0x26c,m.space);io.put(mesh+0xa0,m.component);
    io.put(mesh+0x334,m.tick_epoch);io.put(mesh+0x338,m.pose_epoch);io.put(mesh+0x318,m.flag318);io.put(mesh+0x32c,m.flag32c);io.put(mesh+0x350,m.flag350);
    std::memcpy(io.memory.data()+local-base,source.local.data(),bones*32);std::memcpy(io.memory.data()+space-base,source.space.data(),bones*64);
    std::memcpy(io.memory.data()+reference-base,source.reference.data(),bones*68);std::memcpy(io.memory.data()+desired-base,source.desired.data(),static_cast<std::size_t>(m.desired.count));
}
void paired_fixture(char const* path){
    std::ifstream f(path,std::ios::binary);CHECK(static_cast<bool>(f));char magic[8]{};read(f,magic,8);CHECK(!std::memcmp(magic,"COVPAIR1",8));
    std::uint32_t size{},rows{};read(f,&size,1);read(f,&rows,1);CHECK(size==sizeof(pose::Snapshot)&&rows==3);read(f,actual_pair.data(),3);CHECK(f.peek()==std::ifstream::traits_type::eof());
    const auto& c0=actual_pair[0];const auto& c1=actual_pair[1];const auto& l1=actual_pair[2];
    CHECK(c0.request.phase==pose::Phase::C0&&c1.request.phase==pose::Phase::C1&&l1.request.phase==pose::Phase::L1);
    CHECK(c0.metadata.local.count==115&&c1.metadata.local.count==115&&l1.metadata.local.count==115);
    for(unsigned i:{8u,10u,11u,32u,34u,35u}){CHECK(std::memcmp(c0.local[i].q,c1.local[i].q,16)!=0);CHECK(std::memcmp(c0.local[i].translation,c1.local[i].translation,12)!=0);}
    for(unsigned mode=0;mode<4;++mode){reset();install_payload(c0);auto p=params();p.delta={.2,-.1,.05};p.adaptive=true;CHECK(state.start(p,1));
        CHECK(feed(pose::Phase::C0,1)==trial::Status::okay);install_payload(c1);save_arrays();auto in=take(pose::Phase::C1,1);
        if(mode>=2)io.fail_on=mode==2?2:6;
        CHECK(state.feed(in,io,work)==(mode>=2?trial::Status::write_failed:trial::Status::applied));CHECK(state.pending());
        CHECK(state.stats().c0_c1_changed_atoms>=6);
        if(mode>=2){exact_arrays();CHECK(!std::memcmp(io.memory.data()+local-base,c1.local.data(),bones*32));
            CHECK(std::memcmp(io.memory.data()+local-base,c0.local.data(),bones*32)!=0);CHECK(feed(pose::Phase::L1,1)==trial::Status::drained);CHECK(!state.pending());continue;}
        for(unsigned i=0;i<bones;++i){CHECK(io.get<pose::Atom>(local+i*32).scale==c1.local[i].scale);
            if(i!=8&&i!=10&&i!=11)CHECK(!std::memcmp(io.memory.data()+local-base+i*32,&c1.local[i],32));}
        CHECK(feed(pose::Phase::L1,1)==trial::Status::okay);const auto writes=io.writes;
        if(mode==1)install_payload(c0); // Fresh native input replaces q AND T.
        CHECK(feed(pose::Phase::C0,2)==trial::Status::restored);CHECK(state.pending());
        if(mode==0){CHECK(!std::memcmp(io.memory.data()+local-base,c1.local.data(),bones*32));CHECK(io.writes>writes);}
        else {CHECK(!std::memcmp(io.memory.data()+local-base,c0.local.data(),bones*32));CHECK(io.writes==writes);CHECK(state.stats().native_replaced_locals>0);}
        install_payload(c1);CHECK(feed(pose::Phase::C1,2)==trial::Status::okay);CHECK(state.pending());CHECK(state.stats().native_replaced_space>0);
        install_payload(l1);const auto ending_writes=io.writes;CHECK(feed(pose::Phase::L1,2)==trial::Status::drained);CHECK(!state.pending()&&io.writes==ending_writes);
        CHECK(!std::memcmp(io.memory.data()+local-base,l1.local.data(),bones*32));CHECK(!std::memcmp(io.memory.data()+space-base,l1.space.data(),bones*64));
    }
    std::printf("actual paired C0/C1/L1: 115 bones, six q/T changes, C1 rollback/cache-retained/native-fresh/drain passed; no live authority\n");
}
} // namespace
int main(int argc,char** argv){CHECK(argc==1||argc==2);normal_and_limits();rollback_cases();context_and_order();additional_concerns();if(argc==2)paired_fixture(argv[1]);std::printf("CoverPoseOverlayTrialTest: %u checks in %u scenarios passed; physics_outcome_proven=0 global_exclusivity_proven=0\n",checks,scenarios);return 0;}
