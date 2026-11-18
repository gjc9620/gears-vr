#pragma once
#include "weapon_lancer_original_handle.h"

// Original Boltok component atoms only. Object03's two children follow via the
// ORIGINAL composer. Object01 remains a root child and is never overwritten.
namespace gears_boltok_original {
namespace math=gears_lancer_original_handle;namespace part=gears_weapon_part_visibility;
using Atom=part::Atom;using Source=part::Source;using Matrix=math::Matrix;
inline constexpr double maximum_angle=1.3962634015954636; // Authored VR opening, 80 degrees.
inline constexpr std::array<int,12> parents{{0,0,1,1,0,0,0,0,0,8,9,0}};
inline constexpr std::array<const wchar_t*,12> labels{{L"alow_poly",L"Object03",L"Object08",L"Object02",
    L"Object07",L"Object09",L"Object10",L"Object11",L"Object04",L"Object05",L"Object06",L"Object01"}};
inline bool array(part::Array a,unsigned width) noexcept {
    return a.count==12&&a.capacity>=12&&a.capacity<=512&&a.data&&!(a.data&3)&&
        a.data<=UINTPTR_MAX-std::size_t(a.capacity)*width;
}
inline bool valid(Source const& s) noexcept {
    if(!s.verified||!s.identity_generation||!s.component||!s.asset||!s.node||!s.epoch||
       !array(s.local,32)||!array(s.space,64))return false;
    return s.local.data>=s.space.data+std::size_t(s.space.capacity)*64||
        s.space.data>=s.local.data+std::size_t(s.local.capacity)*32;
}
struct Plan {
    Atom support{},cylinder{};Matrix root{},support_space{};bool conjugated{},known{};
    bool bind(std::array<Atom,12> const& local,std::array<Matrix,12> const& space) noexcept {
        bool conventions[2]{true,true};
        for(unsigned i=0;i<12;++i){
            if(!math::finite(local[i])||!math::proper(space[i]))return false;
            for(unsigned c=0;c<2;++c){
                const auto composed=i?math::multiply(math::matrix(local[i],c!=0),space[unsigned(parents[i])]):
                    math::matrix(local[i],c!=0);
                if(!math::close(composed,space[i]))conventions[c]=false;
            }
        }
        // Identity-quaternion support alone cannot certify conjugation. Use
        // the complete original parent tree, including the nontrivial hammer.
        if(conventions[0]==conventions[1])return false;
        constexpr double p[]={2.089574098587036,12.955615043640137,5.3229660987854};
        for(unsigned i=0;i<3;++i)if(std::abs(local[1].position[i]-p[i])>.002)return false;
        if(std::abs(local[1].rotation[0])+std::abs(local[1].rotation[1])+std::abs(local[1].rotation[2])>.001)return false;
        support=local[1];cylinder=local[3];root=space[0];support_space=space[1];
        conjugated=conventions[1];known=true;return true;
    }
    bool target(double angle,Atom& out) const noexcept {
        if(!known||!std::isfinite(angle)||angle<0||angle>maximum_angle)return false;
        if(angle==0){out=support;return true;}
        const auto local=math::matrix(support,conjugated);
        const auto c=std::cos(angle),s=std::sin(angle),x=local[12],z=local[14];
        // Rotate around ROOT-local Y and the original support pivot. A rotated
        // root must not silently turn this into a component/world Y hinge.
        const Matrix delta{{c,0,-s,0,0,1,0,0,s,0,c,0,x*(1-c)-z*s,0,z*(1-c)+x*s,1}};
        return math::atom(math::multiply(local,delta),conjugated,support,out);
    }
};
// One temporary Compose call. Journal before dispatch, including failed/partial
// writes; restore exact original bytes in finally. No CachedBoneAtoms writes,
// no shared mesh writes, no engine calls and no independent hook installation.
class ComposeLease {
    Source source_{};Plan plan_{};std::array<Atom,2> before_{},wanted_{};
    std::array<bool,2> owned_{};bool bound_{},active_{};
    static std::uintptr_t address(Source const& s,unsigned i) noexcept {return s.local.data+(i?3u:1u)*32;}
    bool matches(Source const& s) const noexcept {return bound_&&valid(s)&&part::identity(source_,s)&&
        part::same(source_.local,s.local)&&part::same(source_.space,s.space);}
public:
    bool active() const noexcept {return active_;}
    Plan const& plan() const noexcept {return plan_;}
    template<class Read>bool bind(Source const& s,Read read) noexcept {
        if(bound_||!valid(s))return false;
        std::array<Atom,12> local{};std::array<Matrix,12> space{};
        for(unsigned i=0;i<12;++i){std::array<float,16> raw{};
            if(!read(s.local.data+i*32,local[i])||!read(s.space.data+i*64,raw))return false;
            std::copy(raw.begin(),raw.end(),space[i].begin());}
        Plan p{};if(!p.bind(local,space))return false;source_=s;plan_=p;bound_=true;return true;
    }
    template<class Read,class Write>bool restore(Source const& s,Read read,Write write) noexcept {
        if(!matches(s))return false;
        bool okay=true;
        for(unsigned i=0;i<2;++i)if(owned_[i]){
            Atom current{};if(!read(address(s,i),current)){okay=false;continue;}
            const auto* p=reinterpret_cast<unsigned char const*>(&current);
            const auto* b=reinterpret_cast<unsigned char const*>(&before_[i]);
            const auto* w=reinterpret_cast<unsigned char const*>(&wanted_[i]);bool ours=true;
            for(unsigned j=0;j<sizeof(Atom);++j)if(p[j]!=b[j]&&p[j]!=w[j])ours=false;
            if(!ours){okay=false;continue;} // Foreign changes remain a retained debt.
            if(!math::same(current,before_[i]))(void)write(address(s,i),before_[i]);
            if(!read(address(s,i),current)||!math::same(current,before_[i])){okay=false;continue;}
            owned_[i]=false;
        }
        active_=owned_[0]||owned_[1];return okay&&!active_;
    }
    template<class Read,class Write>bool apply(Source const& s,double angle,bool hide_cylinder,Read read,Write write) noexcept {
        if(!matches(s)||active_||!plan_.target(angle,wanted_[0]))return false;
        if(!read(address(s,0),before_[0])||!read(address(s,1),before_[1])||
           !math::same(before_[0],plan_.support)||!math::same(before_[1],plan_.cylinder))return false;
        wanted_[1]=before_[1];if(hide_cylinder)wanted_[1].scale=0;
        for(unsigned i=0;i<2;++i)if(!math::same(before_[i],wanted_[i])){
            owned_[i]=true;active_=true;(void)write(address(s,i),wanted_[i]);
            Atom actual{};if(!read(address(s,i),actual)||!math::same(actual,wanted_[i])){
                restore(s,read,write);return false;
            }
        }return true;
    }
};
enum class Result:std::uint32_t {rejected,pending,ready,restoring,restored,inactive_restored};
struct Request {std::uint32_t size{sizeof(Request)},version{1};gears_weapon_hand::Frame source{};
    gears_weapon_hand::ObjectIdentity component{},asset{},node{};};
struct Control {std::uint32_t size{sizeof(Control)},version{1};std::uint64_t handle{},cookie{};
    gears_weapon_hand::Frame source{};double angle{};bool hide_cylinder{};};
struct Receipt {std::uint32_t size{sizeof(Receipt)},version{1};Result result{Result::rejected};
    std::uint64_t handle{},writes{};unsigned epoch{};double angle{};bool hidden{},cpu_restored{};};
// Read-only actual Compose observation. This is data, not a lease or grant.
struct ObservationRequest {
    std::uint32_t size{sizeof(ObservationRequest)},version{1};
    gears_weapon_hand::Frame source{};
    std::uint64_t handle{},cookie{};
    gears_weapon_hand::ObjectIdentity component{},asset{};
};
struct Observation {
    std::uint32_t size{sizeof(Observation)},version{1};
    std::uint64_t handle{},publish_serial{};
    unsigned epoch{},source_epoch{};
    std::int64_t observed_ns{};
    gears_weapon_hand::Frame source{};
    gears_weapon_hand::ObjectIdentity component{},asset{},node{};
    Matrix root{},support{},cylinder{};
    bool hidden{},restoring{},published{},verified{};
};
inline constexpr char bind_export[]="GearsVrBindOriginalBoltokV1",update_export[]="GearsVrPoseOriginalBoltokV1",
    restore_export[]="GearsVrRestoreOriginalBoltokV1",park_export[]="GearsVrParkOriginalBoltokV1",
    observation_export[]="GearsVrReadOriginalBoltokObservationV1";
#if defined(_WIN32)
using Begin=Result(__cdecl*)(Request const*,Receipt*) noexcept;
using Update=Result(__cdecl*)(Control const*,Receipt*) noexcept;
using ObservationRead=bool(__cdecl*)(ObservationRequest const*,Observation*) noexcept;
#endif
}
