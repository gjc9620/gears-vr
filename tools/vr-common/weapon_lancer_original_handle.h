#pragma once
#include "weapon_part_visibility.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

// The existing Native decoder owner drives ONLY original Lancer bone4.
// No new mesh/material, MorphTarget, pointer to a shared asset or action grant.
namespace gears_lancer_original_handle {
namespace part=gears_weapon_part_visibility;namespace hand=gears_weapon_hand;
using Atom=part::Atom;using Array=part::Array;using Source=part::Source;
using Matrix=std::array<double,16>;
inline constexpr double maximum_angle=1.5707963267948966;
// Authored VR presentation: grasp unfolds the existing crossbar, then pushing
// toward the muzzle turns the saw on and pulling home turns it off. Neither
// the replica reference nor these distances establish an
// original-game character animation or a measured real mechanism.
inline constexpr double maximum_pull_raw=8.;
inline constexpr std::array<double,3> closed_grasp{{4.9944004760,25.0155129328,10.1799370098}};
inline bool finite(Atom const& a) noexcept {
    for(float v:a.rotation)if(!std::isfinite(v))return false;
    for(float v:a.position)if(!std::isfinite(v))return false;
    double n{};for(float v:a.rotation)n+=double(v)*v;
    return std::abs(n-1)<.001&&a.scale==1.f;
}
inline bool same(Atom const& a,Atom const& b) noexcept {return !std::memcmp(&a,&b,sizeof a);}
inline bool original(Atom const& a) noexcept {
    constexpr double p[]={32.84494400024414,-4.68596076965332,10.670499801635742};
    if(!finite(a)||std::abs(a.rotation[0])>.001||std::abs(a.rotation[1])>.001||
       std::abs(std::abs(a.rotation[2])-std::sqrt(.5))>.001||
       std::abs(std::abs(a.rotation[3])-std::sqrt(.5))>.001)return false;
    for(unsigned i=0;i<3;++i)if(std::abs(a.position[i]-p[i])>.002)return false;
    return true;
}
inline Matrix identity() noexcept {Matrix m{};m[0]=m[5]=m[10]=m[15]=1;return m;}
inline Matrix multiply(Matrix const& a,Matrix const& b) noexcept {
    Matrix c{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)for(unsigned k=0;k<4;++k)c[i*4+j]+=a[i*4+k]*b[k*4+j];return c;
}
inline bool proper(Matrix const& m) noexcept {
    for(double v:m)if(!std::isfinite(v))return false;
    if(std::abs(m[3])+std::abs(m[7])+std::abs(m[11])+std::abs(m[15]-1)>.001)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){double d{};for(unsigned k=0;k<3;++k)d+=m[i*4+k]*m[j*4+k];if(std::abs(d-(i==j?1.:0.))>.002)return false;}
    const double det=m[0]*(m[5]*m[10]-m[6]*m[9])-m[1]*(m[4]*m[10]-m[6]*m[8])+m[2]*(m[4]*m[9]-m[5]*m[8]);
    return std::abs(det-1)<.003;
}
inline Matrix inverse(Matrix const& m) noexcept {
    Matrix n=identity();for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)n[i*4+j]=m[j*4+i];
    for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)n[12+j]-=m[12+k]*n[k*4+j];return n;
}
inline bool close(Matrix const& a,Matrix const& b) noexcept {
    for(unsigned i=0;i<16;++i)if(std::abs(a[i]-b[i])>(i>=12&&i<15?.02:.001))return false;return true;
}
inline Matrix matrix(Atom const& a,bool conjugated) noexcept {
    const double sign=conjugated?-1.:1.,x=a.rotation[0]*sign,y=a.rotation[1]*sign,z=a.rotation[2]*sign,w=a.rotation[3];
    Matrix m{{1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w),0,
      2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w),0,
      2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y),0,
      a.position[0],a.position[1],a.position[2],1}};return m;
}
inline bool atom(Matrix const& m,bool conjugated,Atom const& previous,Atom& out) noexcept {
    if(!proper(m))return false;
    // Standard quaternion of the TRANSPOSE of the row-vector rotation.
    double q[4]{},s{},tr=m[0]+m[5]+m[10];
    if(tr>0){s=std::sqrt(tr+1)*2;q[3]=.25*s;q[0]=(m[6]-m[9])/s;q[1]=(m[8]-m[2])/s;q[2]=(m[1]-m[4])/s;}
    else if(m[0]>m[5]&&m[0]>m[10]){s=std::sqrt(1+m[0]-m[5]-m[10])*2;q[0]=.25*s;q[1]=(m[4]+m[1])/s;q[2]=(m[8]+m[2])/s;q[3]=(m[6]-m[9])/s;}
    else if(m[5]>m[10]){s=std::sqrt(1+m[5]-m[0]-m[10])*2;q[0]=(m[4]+m[1])/s;q[1]=.25*s;q[2]=(m[9]+m[6])/s;q[3]=(m[8]-m[2])/s;}
    else{s=std::sqrt(1+m[10]-m[0]-m[5])*2;q[0]=(m[8]+m[2])/s;q[1]=(m[9]+m[6])/s;q[2]=.25*s;q[3]=(m[1]-m[4])/s;}
    if(conjugated)for(unsigned i=0;i<3;++i)q[i]=-q[i];
    double n{},dot{};for(unsigned i=0;i<4;++i){n+=q[i]*q[i];dot+=q[i]*previous.rotation[i];}
    if(!std::isfinite(n)||n<.99||n>1.01)return false;
    Atom candidate{};for(unsigned i=0;i<4;++i)candidate.rotation[i]=float(q[i]/std::sqrt(n)*(dot<0?-1:1));
    for(unsigned i=0;i<3;++i)candidate.position[i]=float(m[12+i]);candidate.scale=1;
    if(!finite(candidate)||!close(matrix(candidate,conjugated),m))return false;out=candidate;return true;
}
struct Plan {
    Atom closed{};Matrix root{},space{};bool conjugated{},known{};
    bool bind(Atom const& a,Matrix const& r,Matrix const& h) noexcept {
        if(!original(a)||!proper(r)||!proper(h))return false;
        const bool direct=close(multiply(matrix(a,false),r),h),inverse_q=close(multiply(matrix(a,true),r),h);
        if(direct==inverse_q)return false; // No guessing from a bone name.
        closed=a;root=r;space=h;conjugated=inverse_q;known=true;return true;
    }
    bool target(double angle,double pull,Atom& a,Matrix& h) const noexcept {
        if(!known||!std::isfinite(angle)||angle<0||angle>maximum_angle||
           !std::isfinite(pull)||pull<0||pull>maximum_pull_raw)return false;
        if(angle==0&&pull==0){a=closed;h=space;return true;}
        const double c=std::cos(angle),s=std::sin(angle),x=space[12],y=space[13];
        const Matrix delta{{c,s,0,0,-s,c,0,0,0,0,1,0,x*(1-c)+y*s,y*(1-c)-x*s,0,1}};
        h=multiply(space,delta);h[13]+=pull; // +model Y is toward the muzzle.
        return atom(multiply(h,inverse(root)),conjugated,closed,a);
    }
    bool target(double angle,Atom& a,Matrix& h) const noexcept {return target(angle,0,a,h);}
};
inline bool array(Array a,unsigned width,bool empty=false) noexcept {
    return (a.count==9||(empty&&a.count==0))&&a.capacity>=a.count&&a.capacity<=512&&
      (!a.capacity||(a.data&&!(a.data&3)&&a.data<=UINTPTR_MAX-std::size_t(a.capacity)*width));
}
inline bool valid(Source const& s) noexcept {
    if(!s.verified||!s.identity_generation||!s.component||!s.asset||!s.node||!s.epoch||
       !array(s.local,32)||!array(s.cache,32,true)||!array(s.space,64))return false;
    const Array a[]={s.local,s.cache,s.space};const unsigned w[]={32,32,64};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)if(a[i].capacity&&a[j].capacity&&
       a[i].data<a[j].data+std::size_t(a[j].capacity)*w[j]&&a[j].data<a[i].data+std::size_t(a[i].capacity)*w[i])return false;
    return true;
}
// Journals the one original atom and the original evaluator's exact cache
// copy. No local/cache pointer is followed after reallocation or identity loss.
class Lease {
    Source source_{};Array cache_{};Plan plan_{};Atom original_{},written_{},cache_original_{},cache_written_{};
    bool bound_{},restoring_{},local_owned_{},cache_owned_{};double angle_{},pull_{};
    std::uint64_t writes_{};unsigned epoch_{};
    static std::uintptr_t at(Array a) noexcept {return a.data+4*32;}
    bool match(Source const& s) const noexcept {return bound_&&valid(s)&&part::identity(source_,s)&&part::same(source_.local,s.local);}
    bool closed(Atom const& a) const noexcept {
        return original(a)&&close(matrix(a,plan_.conjugated),matrix(plan_.closed,plan_.conjugated));
    }
public:
    bool bound() const noexcept {return bound_;}bool restoring() const noexcept{return restoring_;}
    bool cpu_restored() const noexcept{return restoring_&&!local_owned_&&!cache_owned_;}
    std::uint64_t writes() const noexcept{return writes_;}double wanted() const noexcept{return angle_;}
    double wanted_pull() const noexcept{return pull_;}
    Plan const& plan() const noexcept{return plan_;}
    void request_restore() noexcept{if(bound_)restoring_=true;}
    template<class Read>bool begin(Source const& s,Read read) noexcept {
        if(bound_||!valid(s))return false;Atom a{};std::array<float,16> r{},h{};Matrix root{},space{};
        if(!read(at(s.local),a)||!read(s.space.data,r)||!read(s.space.data+4*64,h))return false;
        std::copy(r.begin(),r.end(),root.begin());std::copy(h.begin(),h.end(),space.begin());
        Plan candidate{};if(!candidate.bind(a,root,space))return false;
        source_=s;plan_=candidate;original_=a;bound_=true;return true;
    }
    bool wanted(double angle,double pull=0) noexcept {
        if(!bound_||restoring_||!std::isfinite(angle)||angle<0||angle>maximum_angle||
           !std::isfinite(pull)||pull<0||pull>maximum_pull_raw)return false;angle_=angle;pull_=pull;return true;
    }
    bool matches(Source const& s,std::uintptr_t out,std::uint64_t track) const noexcept {
        return !restoring_&&match(s)&&out==at(s.local)&&track==4;
    }
    template<class Read>bool observe_cache(Source const& s,Read read) noexcept {
        if(!match(s))return false;
        if(cache_owned_&&!part::same(cache_,s.cache)){
            Atom a{};if(!s.cache.count||(read(at(s.cache),a)&&closed(a))){cache_owned_=false;cache_={};}else return false;
        }
        if(!writes_||!s.cache.count)return true;Atom local{},cached{};
        if(!read(at(s.local),local)||!read(at(s.cache),cached))return false;
        if(same(local,written_)&&same(cached,written_)){
            cache_=s.cache;cache_original_=original_;cache_written_=written_;cache_owned_=true;
        }
        return true;
    }
    template<class Read,class Write>bool decoder_return(Source const& s,std::uintptr_t out,std::uint64_t track,Read read,Write write) noexcept {
        if(!matches(s,out,track)||!observe_cache(s,read))return false;
        Atom before{},canonical{},desired{};Matrix target{};
        if(!read(out,before))return false;
        // Certified E08A0 always negates W for non-root bones after the
        // E0A73 decoder return. Calibrate/journal the FINAL LocalAtom, and
        // encode the inverse of that exact remaining operation here. Treating
        // the raw decoder result as the final atom reverses the rotation.
        canonical=before;canonical.rotation[3]=-canonical.rotation[3];
        if(!original(canonical)||!plan_.target(angle_,pull_,desired,target))return false;
        // Original motion may hand ownership back, but a different atom may
        // never be projected onto our selected axis to manufacture a pass.
        if(!close(matrix(canonical,plan_.conjugated),matrix(plan_.closed,plan_.conjugated)))return false;
        original_=canonical;written_=desired;local_owned_=true;
        Atom raw_desired=desired;raw_desired.rotation[3]=-raw_desired.rotation[3];
        const bool stored=write(out,raw_desired);Atom after{};
        if(!read(out,after))return false;
        if(same(after,raw_desired)){++writes_;epoch_=s.epoch;return true;}
        if(stored&&same(after,before))local_owned_=false;
        return false;
    }
    template<class Read>bool ready(Source const& s,Read read) noexcept {
        // The verified original rest pose needs no synthetic write. Requiring
        // a decoder write here deadlocks contact on an idle cached animation.
        // A moved handle still needs a completed write and evaluated geometry.
        if(!match(s)||restoring_||((angle_!=0||pull_!=0)&&!writes_)||s.epoch<epoch_)return false;
        Atom a{};Matrix expected{},actual{},root{};std::array<float,16> raw{},r{};
        if(!plan_.target(angle_,pull_,a,expected)||!read(s.space.data+4*64,raw)||!read(s.space.data,r))return false;
        std::copy(raw.begin(),raw.end(),actual.begin());std::copy(r.begin(),r.end(),root.begin());
        return close(root,plan_.root)&&close(actual,expected);
    }
    template<class Read,class Write>bool restore(Source const& s,Read read,Write write,bool inactive=false) noexcept {
        if(!restoring_||!match(s)||!observe_cache(s,read))return false;
        const auto restore_one=[&](Array a,bool& owned,Atom const& expected,Atom const& original_value){
            if(!owned)return true;Atom before{};if(!read(at(a),before))return false;
            if(closed(before)){owned=false;return true;}
            if(!same(before,expected))return false;
            (void)write(at(a),original_value);Atom after{};
            if(!read(at(a),after)||!same(after,original_value))return false;owned=false;return true;
        };
        if(cache_owned_&&!part::same(cache_,s.cache))return false;
        if(!restore_one(s.local,local_owned_,written_,original_)||!restore_one(cache_,cache_owned_,cache_written_,cache_original_))return false;
        if(!inactive){std::array<float,16> raw{};Matrix actual{};
            if(s.epoch<epoch_||!read(s.space.data+4*64,raw))return false;
            std::copy(raw.begin(),raw.end(),actual.begin());if(!close(actual,plan_.space))return false;}
        bound_=false;return true;
    }
};
// Display-only bone ownership. Gameplay still requires its separate current
// hand lease and the actual original-handle/weapon observations.
enum class Result:std::uint32_t {rejected,pending,ready,restoring,restored,unavailable,inactive_restored};
struct Request {std::uint32_t size{sizeof(Request)},version{2};hand::Frame source{};hand::ObjectIdentity component{},asset{},node{};};
struct Control {std::uint32_t size{sizeof(Control)},version{2};std::uint64_t handle{},cookie{};hand::Frame source{};double angle{},pull_raw{};};
struct Receipt {std::uint32_t size{sizeof(Receipt)},version{2};Result result{Result::rejected};std::uint64_t handle{},writes{};unsigned epoch{},gate{};double angle{},pull_raw{};bool cpu_restored{},original_inactive{},original_destroyed{};};
inline constexpr char bind_export[]="GearsVrBindOriginalLancerStarterV2",update_export[]="GearsVrPoseOriginalLancerStarterV2",restore_export[]="GearsVrRestoreOriginalLancerStarterV2";
#if defined(_WIN32)
using Begin=Result(__cdecl*)(Request const*,Receipt*) noexcept;using Update=Result(__cdecl*)(Control const*,Receipt*) noexcept;
#endif
}
