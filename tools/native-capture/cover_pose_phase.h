#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

// Passive copies only. The caller certifies current code/layouts and brackets
// this operation with its live owner-identity checks. Repeated equal reads do
// not prove an atomic pose, exclusive ownership, or absence of engine readers.
// No getter, animation evaluation, allocator, cache write or root write occurs.
namespace gears_cover_pose_phase {
constexpr unsigned max_bones=512;
constexpr std::uint32_t compose_rva=0x128920,physics_rva=0xf26d4,tick_rva=0xfa730;
constexpr std::uint32_t compose_caller=0xf948c,physics_main_caller=0xf9aeb,physics_late_caller=0xfac69;
enum class Kind:unsigned {compose,physics,tick};
enum class Phase:unsigned {C0,C1,P0,P1,L1};
enum class Reject:unsigned {none,request,read,bounds,shape,nonfinite,parent,desired,metadata_changed,payload_changed,alias};
struct Identity {
    std::uintptr_t address{},cls{},outer{};
    std::int32_t internal_index{},name_index{};
    std::uint32_t number{};
};
struct OwnerKey {
    Identity world{},pawn{},mesh{};
    std::uint64_t world_generation{},session{},scene{};
    std::int64_t scene_qpc{};
};
struct Request {
    OwnerKey owner{};
    std::uint64_t invocation{};
    std::int64_t qpc{};
    std::uint32_t thread{},caller_rva{};
    Kind kind{};Phase phase{};
    // Physics RDX, copied while its native caller is still alive. For main
    // physics this is mesh+A0; late physics may use the caller's stack matrix.
    std::uintptr_t physics_matrix{};
};
struct ArrayHeader {std::uintptr_t data{};std::int32_t count{},capacity{};};
struct RefBone {std::array<unsigned char,68> bytes{};};
struct Atom {float q[4]{},translation[3]{},scale{};};
struct Matrix {float values[16]{};};
struct Metadata {
    std::uintptr_t asset{},master{},physics_asset{},physics_instance{};
    ArrayHeader reference{},local{},desired{},space{};
    std::uint32_t tick_epoch{},pose_epoch{},flag318{},flag32c{},flag350{};
    Matrix component{},physics{};
};
struct Snapshot {
    Request request{};Metadata metadata{};
    std::array<RefBone,max_bones> reference{};
    std::array<Atom,max_bones> local{};
    std::array<unsigned char,max_bones> desired{};
    std::array<Matrix,max_bones> space{};
};
// Byte-identical to the existing GearsCoverRawInvoke packet.
struct alignas(16) CallPacket {
    std::array<std::uint64_t,7> gpr{};
    std::uint64_t flags{};
    std::array<std::array<unsigned char,16>,6> xmm{};
};
static_assert(sizeof(std::uintptr_t)==8&&sizeof(ArrayHeader)==16&&sizeof(Atom)==32&&sizeof(Matrix)==64&&sizeof(RefBone)==68);
static_assert(sizeof(CallPacket)==0xa0&&offsetof(CallPacket,flags)==0x38&&offsetof(CallPacket,xmm)==0x40);
static_assert(std::is_trivially_copyable<Snapshot>::value&&std::is_trivially_destructible<Snapshot>::value);
inline bool span(std::uintptr_t address,std::size_t bytes){return address&&bytes&&bytes-1<=(std::numeric_limits<std::uintptr_t>::max)()-address;}
inline bool add(std::uintptr_t address,std::size_t offset,std::uintptr_t& result){
    if(!address||offset>(std::numeric_limits<std::uintptr_t>::max)()-address)return false;
    result=address+offset;return true;
}
inline bool header_equal(ArrayHeader const& a,ArrayHeader const& b){return a.data==b.data&&a.count==b.count&&a.capacity==b.capacity;}
inline bool metadata_equal(Metadata const& a,Metadata const& b){
    return a.asset==b.asset&&a.master==b.master&&a.physics_asset==b.physics_asset&&a.physics_instance==b.physics_instance&&
        header_equal(a.reference,b.reference)&&header_equal(a.local,b.local)&&header_equal(a.desired,b.desired)&&header_equal(a.space,b.space)&&
        a.tick_epoch==b.tick_epoch&&a.pose_epoch==b.pose_epoch&&a.flag318==b.flag318&&a.flag32c==b.flag32c&&a.flag350==b.flag350&&
        !std::memcmp(&a.component,&b.component,sizeof(Matrix))&&!std::memcmp(&a.physics,&b.physics,sizeof(Matrix));
}
inline bool caller_allowed(Kind kind,std::uint32_t caller){
    return kind==Kind::compose?caller==compose_caller:kind==Kind::physics?
        (caller==physics_main_caller||caller==physics_late_caller):kind==Kind::tick&&caller!=0;
}
inline bool request_valid(Request const& r){
    const auto& o=r.owner;
    return o.world.address&&o.pawn.address&&o.mesh.address&&o.world_generation&&o.session&&o.scene&&o.scene_qpc>0&&
        r.invocation&&r.qpc>0&&r.thread&&caller_allowed(r.kind,r.caller_rva)&&
        ((r.kind==Kind::compose&&(r.phase==Phase::C0||r.phase==Phase::C1)&&!r.physics_matrix)||
         (r.kind==Kind::physics&&(r.phase==Phase::P0||r.phase==Phase::P1)&&span(r.physics_matrix,sizeof(Matrix)))||
         (r.kind==Kind::tick&&r.phase==Phase::L1&&!r.physics_matrix));
}
template<class Read> inline bool bytes(Read& read,void* out,std::uintptr_t address,std::size_t count){
    return span(address,count)&&read(out,reinterpret_cast<void const*>(address),count);
}
template<class Read,class T> inline bool field(Read& read,T& out,std::uintptr_t address,std::size_t offset){
    std::uintptr_t at{};return add(address,offset,at)&&bytes(read,&out,at,sizeof(T));
}
template<class Read> inline bool metadata(Read& read,Request const& request,Metadata& out){
    const auto mesh_address=request.owner.mesh.address;
    return field(read,out.asset,mesh_address,0x218)&&out.asset&&field(read,out.master,mesh_address,0x29c)&&
        field(read,out.physics_asset,mesh_address,0x248)&&field(read,out.physics_instance,mesh_address,0x250)&&
        field(read,out.reference,out.asset,0xa4)&&field(read,out.local,mesh_address,0x27c)&&field(read,out.desired,mesh_address,0x28c)&&field(read,out.space,mesh_address,0x26c)&&
        field(read,out.tick_epoch,mesh_address,0x334)&&field(read,out.pose_epoch,mesh_address,0x338)&&field(read,out.flag318,mesh_address,0x318)&&
        field(read,out.flag32c,mesh_address,0x32c)&&field(read,out.flag350,mesh_address,0x350)&&field(read,out.component,mesh_address,0xa0)&&
        (!request.physics_matrix||bytes(read,&out.physics,request.physics_matrix,sizeof(Matrix)));
}
inline bool header_valid(ArrayHeader const& h,std::size_t stride,bool empty=false){
    if(h.count<0||h.capacity<0||h.count>int(max_bones)||h.capacity>int(max_bones)||h.capacity<h.count||(!empty&&!h.count))return false;
    return h.capacity?span(h.data,std::size_t(h.capacity)*stride):!h.data;
}
inline bool finite(Matrix const& m){for(float v:m.values)if(!std::isfinite(v))return false;return true;}
inline std::int32_t parent(RefBone const& b){std::int32_t value{};std::memcpy(&value,b.bytes.data()+0x38,4);return value;}
inline Reject validate(Snapshot const& s){
    const auto n=unsigned(s.metadata.reference.count);
    if(!finite(s.metadata.component)||(s.request.physics_matrix&&!finite(s.metadata.physics)))return Reject::nonfinite;
    for(unsigned i=0;i<n;++i){
        const auto& a=s.local[i];
        for(float v:a.q)if(!std::isfinite(v))return Reject::nonfinite;
        for(float v:a.translation)if(!std::isfinite(v))return Reject::nonfinite;
        if(!std::isfinite(a.scale)||!finite(s.space[i]))return Reject::nonfinite;
        const auto p=parent(s.reference[i]);
        if(i==0){if(p!=-1&&p!=0)return Reject::parent;}
        else if(p<0||p>=std::int32_t(n)||p==std::int32_t(i))return Reject::parent;
    }
    // Bounded ancestry walk: root 0 may use either -1 or itself as sentinel.
    for(unsigned i=1;i<n;++i){unsigned current=i,steps=0;
        while(current){if(++steps>=n)return Reject::parent;current=unsigned(parent(s.reference[current]));}
    }
    std::array<bool,max_bones> seen{};
    for(int i=0;i<s.metadata.desired.count;++i){const auto bone=s.desired[unsigned(i)];if(bone>=n||seen[bone])return Reject::desired;seen[bone]=true;}
    return Reject::none;
}
template<class Read> inline bool payload(Read& read,Snapshot& s){
    const auto& m=s.metadata;
    return bytes(read,s.reference.data(),m.reference.data,std::size_t(m.reference.count)*sizeof(RefBone))&&
        bytes(read,s.local.data(),m.local.data,std::size_t(m.local.count)*sizeof(Atom))&&
        (!m.desired.count||bytes(read,s.desired.data(),m.desired.data,std::size_t(m.desired.count)))&&
        bytes(read,s.space.data(),m.space.data,std::size_t(m.space.count)*sizeof(Matrix));
}
template<class Read> inline Reject repeat(Read& read,void const* expected,std::uintptr_t address,std::size_t count){
    std::array<unsigned char,1024> block{};auto* source=static_cast<unsigned char const*>(expected);
    for(std::size_t offset=0;offset<count;){const auto take=(count-offset<block.size()?count-offset:block.size());std::uintptr_t at{};
        if(!add(address,offset,at)||!bytes(read,block.data(),at,take))return Reject::read;
        if(std::memcmp(block.data(),source+offset,take))return Reject::payload_changed;offset+=take;
    }return Reject::none;
}
// scratch must be separate caller-owned storage (prefer bounded TLS/heap, not
// a large live stack). output is assigned only on complete success. Callers
// must not publish scratch, or keep a non-POD RAII object across native SEH.
template<class Read> inline Reject capture(Read& read,Request const& request,Snapshot& scratch,Snapshot& output){
    const auto a=reinterpret_cast<std::uintptr_t>(&scratch),b=reinterpret_cast<std::uintptr_t>(&output);
    if(a==b||(a<b?b-a:a-b)<sizeof(Snapshot))return Reject::alias;
    if(!request_valid(request))return Reject::request;
    std::memset(&scratch,0,sizeof scratch);scratch.request=request;
    if(!metadata(read,request,scratch.metadata))return Reject::read;
    const auto& m=scratch.metadata;
    if(!header_valid(m.reference,sizeof(RefBone))||!header_valid(m.local,sizeof(Atom))||!header_valid(m.space,sizeof(Matrix))||!header_valid(m.desired,1,true))return Reject::bounds;
    if(m.reference.count!=m.local.count||m.reference.count!=m.space.count||m.desired.count>m.reference.count)return Reject::shape;
    if(!payload(read,scratch))return Reject::read;
    const auto valid=validate(scratch);if(valid!=Reject::none)return valid;
    Metadata again{};if(!metadata(read,request,again))return Reject::read;
    if(!metadata_equal(m,again))return Reject::metadata_changed;
    Reject result=repeat(read,scratch.reference.data(),m.reference.data,std::size_t(m.reference.count)*sizeof(RefBone));if(result!=Reject::none)return result;
    result=repeat(read,scratch.local.data(),m.local.data,std::size_t(m.local.count)*sizeof(Atom));if(result!=Reject::none)return result;
    result=repeat(read,scratch.desired.data(),m.desired.data,std::size_t(m.desired.count));if(result!=Reject::none)return result;
    result=repeat(read,scratch.space.data(),m.space.data,std::size_t(m.space.count)*sizeof(Matrix));if(result!=Reject::none)return result;
    if(!metadata(read,request,again))return Reject::read;
    if(!metadata_equal(m,again))return Reject::metadata_changed;
    output=scratch;return Reject::none;
}
} // namespace gears_cover_pose_phase
