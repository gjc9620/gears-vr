#include "hand_two_bone_pose.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <type_traits>

#ifdef NDEBUG
#error Two-bone CPU assertions must stay active
#endif

namespace {
using namespace gears_hand_two_bone;
constexpr double pi=3.14159265358979323846;
unsigned groups{};
Vector add(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vector sub(Vector a,Vector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vector scale(Vector a,double s){return {a.x*s,a.y*s,a.z*s};}
double size(Vector value){return std::hypot(value.x,value.y,value.z);}
Vector unit(Vector value){const auto n=size(value);assert(n>0);return {value.x/n,value.y/n,value.z/n};}
double dot(Vector a,Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Quaternion axis(Vector direction,double angle){
    direction=unit(direction);const auto s=std::sin(angle/2);
    return {direction.x*s,direction.y*s,direction.z*s,std::cos(angle/2)};
}
Quaternion normal(Quaternion value){
    const double n=std::hypot(std::hypot(value.x,value.y),std::hypot(value.z,value.w));assert(n>0&&std::isfinite(n));
    return {value.x/n,value.y/n,value.z/n,value.w/n};
}
bool close(Vector a,Vector b,double tolerance=2e-11){return size(sub(a,b))<=tolerance;}
bool equivalent(Quaternion a,Quaternion b,double tolerance=2e-11){
    a=normal(a);b=normal(b);
    const double positive=std::hypot(std::hypot(a.x-b.x,a.y-b.y),std::hypot(a.z-b.z,a.w-b.w));
    const double negative=std::hypot(std::hypot(a.x+b.x,a.y+b.y),std::hypot(a.z+b.z,a.w+b.w));
    return (std::min)(positive,negative)<=tolerance;
}
Vector rotate(Quaternion q,Vector v){return gears_vr::transform(gears_vr::rotation_matrix(normal(q)),v);}
Input bent(){
    Input in{};
    in.shoulder={{1,2,3},axis({1,3,2},.7)};
    in.elbow={{1.2,3,3.2},axis({-2,1,4},-.9)};
    in.wrist={{2.2,3.4,3.1},axis({1,1,0},.2)};
    in.target={{1.9,2.5,3.2},gears_vr::multiply(axis({0,1,0},.61),axis({1,0,0},1.13))};
    in.authored={true,{1.3,2.6,4.2}};return in;
}
Input straight(){
    Input in{};in.elbow.position={1,0,0};in.wrist.position={2,0,0};
    in.target.position={1,0,0};return in;
}
void verify(Input const& in,Solution const& out){
    assert(out.shoulder.position.x==in.shoulder.position.x&&out.shoulder.position.y==in.shoulder.position.y&&out.shoulder.position.z==in.shoulder.position.z);
    assert(out.wrist.position.x==in.target.position.x&&out.wrist.position.y==in.target.position.y&&out.wrist.position.z==in.target.position.z);
    assert(equivalent(out.wrist.orientation,in.target.orientation));
    assert(std::isfinite(out.upper_length)&&std::isfinite(out.lower_length)&&out.upper_length>0&&out.lower_length>0);
    const Vector old[]={sub(in.elbow.position,in.shoulder.position),sub(in.wrist.position,in.elbow.position)};
    const Vector next[]={sub(out.elbow.position,out.shoulder.position),sub(out.wrist.position,out.elbow.position)};
    const Quaternion original[]={in.shoulder.orientation,in.elbow.orientation};
    const Quaternion written[]={out.shoulder.orientation,out.elbow.orientation};
    for(unsigned i=0;i<2;++i){
        const auto delta=normal(gears_vr::multiply(normal(written[i]),gears_vr::inverse(normal(original[i]))));
        assert(close(rotate(delta,unit(old[i])),unit(next[i]),5e-11));
        // The direction change's axis is perpendicular to the old bone: the
        // kernel adds swing, never a new twist around an assumed local X axis.
        assert(std::abs(dot({delta.x,delta.y,delta.z},unit(old[i])))<5e-11);
    }
}
Solution solved(Input const& in){Solution out{};assert(solve(in,out)==Status::solved);verify(in,out);return out;}
void failure(Input const& in,Status expected){
    static_assert(std::is_trivially_copyable_v<Solution>);
    Solution out{};out.shoulder.position={71,72,73};out.elbow.position={81,82,83};out.wrist.position={91,92,93};
    out.upper_length=41;out.lower_length=43;out.length_adjusted=true;out.pole=PoleSource::previous;
    std::array<unsigned char,sizeof(Solution)> before{};std::memcpy(before.data(),&out,sizeof(out));
    assert(solve(in,out)==expected);assert(std::memcmp(before.data(),&out,sizeof(out))==0);
}
template<class Test>void run(const char* name,Test test){test();++groups;std::printf("PASS %s\n",name);}
Input transformed(Input in,Quaternion rotation,Vector translation){
    for(auto* pose:{&in.shoulder,&in.elbow,&in.wrist,&in.target}){
        pose->position=add(rotate(rotation,pose->position),translation);
        pose->orientation=gears_vr::multiply(rotation,pose->orientation);
    }
    for(auto* pole:{&in.authored,&in.previous})pole->point=add(rotate(rotation,pole->point),translation);
    return in;
}
void covariance(Input const& base,Quaternion rotation,Vector translation){
    const auto original=solved(base),changed=solved(transformed(base,rotation,translation));
    assert(close(changed.shoulder.position,add(rotate(rotation,original.shoulder.position),translation),1e-9));
    assert(close(changed.elbow.position,add(rotate(rotation,original.elbow.position),translation),1e-9));
    assert(close(changed.wrist.position,add(rotate(rotation,original.wrist.position),translation),1e-9));
    assert(equivalent(changed.shoulder.orientation,gears_vr::multiply(rotation,original.shoulder.orientation),1e-9));
    assert(equivalent(changed.elbow.orientation,gears_vr::multiply(rotation,original.elbow.orientation),1e-9));
    assert(equivalent(changed.wrist.orientation,gears_vr::multiply(rotation,original.wrist.orientation),1e-9));
    assert(changed.pole==original.pole&&changed.lengths==original.lengths);
}
} // namespace

int main(){
    run("arbitrary-bone-axes-swing-and-complete-wrist-roll",[]{
        const auto in=bent();const auto out=solved(in);
        assert(out.pole==PoleSource::authored&&!out.length_adjusted&&out.lengths==LengthResolution::preserved);
        auto changed=in;changed.target.orientation=gears_vr::multiply(in.target.orientation,axis({1,0,0},1.1));
        const auto roll=solved(changed);assert(close(out.elbow.position,roll.elbow.position));
        assert(equivalent(out.shoulder.orientation,roll.shoulder.orientation)&&equivalent(out.elbow.orientation,roll.elbow.orientation));
        assert(!equivalent(out.wrist.orientation,roll.wrist.orientation));
    });
    run("preserve-outside-and-inside-fail-without-output-change",[]{
        auto in=straight();in.target.position={2.01,0,0};failure(in,Status::unreachable);
        in.elbow.position={1.5,0,0};in.wrist.position={2,0,0};in.target.position={.2,0,0};failure(in,Status::unreachable);
        in.target.position={};failure(in,Status::unreachable);
    });
    run("closed-inner-and-outer-reach-boundaries",[]{
        for(double s:{1.,1./1024,1024.}){
            auto in=straight();in.elbow.position={1.5*s,0,0};in.wrist.position={2*s,0,0};
            in.target.position={s,0,0};auto out=solved(in);assert(!out.length_adjusted);
            assert(std::abs(out.upper_length/s-1.5)<1e-12&&std::abs(out.lower_length/s-.5)<1e-12);
            in.target.position={2*s,0,0};out=solved(in);assert(!out.length_adjusted);
            in.target.position={.999*s,0,0};failure(in,Status::unreachable);
        }
    });
    run("adaptive-outer-exact-target-proportional-lengths",[]{
        auto in=straight();in.elbow.position={1.5,0,0};in.lengths.mode=LengthMode::adaptive;
        in.target={{7,-3,2},axis({1,1,1},1.2)};const auto out=solved(in);
        assert(out.length_adjusted&&out.lengths==LengthResolution::extended);
        assert(std::abs(out.upper_length/out.lower_length-3)<1e-11);
    });
    run("adaptive-inner-zero-and-tiny-distance-exact-target",[]{
        auto in=straight();in.elbow.position={0,1.4,0};in.wrist.position={.6,1.4,0};
        in.lengths.mode=LengthMode::adaptive;in.target.position={};const auto folded=solved(in);
        assert(folded.length_adjusted&&folded.lengths==LengthResolution::rebalanced);
        assert(std::abs(folded.upper_length-1)<1e-12&&std::abs(folded.lower_length-1)<1e-12);
        for(double d:{1e-3,1e-12,1e-100}){in.target.position={d,0,0};const auto out=solved(in);
            assert(out.lengths==LengthResolution::rebalanced&&out.wrist.position.x==d);}
        in.target.position={.6,0,0};const auto partial=solved(in);
        assert(std::abs(partial.upper_length+partial.lower_length-2)<1e-12);
        assert(std::abs(partial.upper_length-partial.lower_length)<.6);
    });
    run("preserve-equal-complete-fold-keeps-original-bend",[]{
        auto in=straight();in.elbow.position={0,1,0};in.wrist.position={};in.target.position={};
        const auto out=solved(in);assert(close(out.elbow.position,in.elbow.position)&&out.pole==PoleSource::original_elbow);
    });
    run("explicit-calibrated-length-ratios",[]{
        auto in=straight();in.lengths.upper_scale=1.2;in.lengths.lower_scale=.8;const auto out=solved(in);
        assert(out.length_adjusted&&out.lengths==LengthResolution::calibrated);
        assert(std::abs(out.upper_length-1.2)<1e-12&&std::abs(out.lower_length-.8)<1e-12);
    });
    run("authored-previous-original-frame-pole-order",[]{
        auto in=straight();in.authored={true,{10,0,0}};in.previous={true,{0,0,1}};
        auto out=solved(in);assert(out.pole==PoleSource::previous&&out.elbow.position.z>0);
        in.authored.point={10,1e-16,0};out=solved(in);assert(out.pole==PoleSource::previous);
        in.previous.known=false;out=solved(in);assert(out.pole==PoleSource::shoulder_frame&&out.elbow.position.y>0);
        in.elbow.position={.5,1,0};in.wrist.position={1,0,0};out=solved(in);assert(out.pole==PoleSource::original_elbow&&out.elbow.position.y>0);
        in.authored.point={0,0,-1};out=solved(in);assert(out.pole==PoleSource::authored&&out.elbow.position.z<0);
    });
    run("reverse-and-near-reverse-preserve-original-axis-twist",[]{
        auto in=straight();in.shoulder.orientation=axis({1,0,0},.8);in.elbow.orientation=axis({1,0,0},-.4);
        in.target={{-2,0,0},axis({-2,1,3},1.3)};solved(in);
        in.target.position={-1.8,1e-11,0};in.authored={true,{0,1,0}};solved(in);
    });
    run("straight-arm-full-extension",[]{auto in=straight();in.target.position=in.wrist.position;const auto out=solved(in);assert(close(out.elbow.position,in.elbow.position));});
    run("left-right-mirror-with-mirrored-authored-pole",[]{
        auto left=bent(),right=left;
        const auto mirror=[](Vector v){return Vector{v.x,-v.y,v.z};};
        const auto mirror_q=[](Quaternion q){return Quaternion{-q.x,q.y,-q.z,q.w};};
        for(auto* pose:{&right.shoulder,&right.elbow,&right.wrist,&right.target}){pose->position=mirror(pose->position);pose->orientation=mirror_q(pose->orientation);}
        right.authored.point=mirror(right.authored.point);const auto a=solved(left),b=solved(right);
        assert(close(b.elbow.position,mirror(a.elbow.position))&&equivalent(b.shoulder.orientation,mirror_q(a.shoulder.orientation)));
        assert(equivalent(b.elbow.orientation,mirror_q(a.elbow.orientation))&&equivalent(b.wrist.orientation,mirror_q(a.wrist.orientation)));
    });
    run("unit-scaling-1e-120-through-1e120",[]{
        const auto base=bent();const auto original=solved(base);
        for(double s:{1e-120,1e-9,1e-3,100.,1e9,1e120}){
            auto in=base;for(auto* pose:{&in.shoulder,&in.elbow,&in.wrist,&in.target})pose->position=scale(pose->position,s);
            for(auto* pole:{&in.authored,&in.previous})pole->point=scale(pole->point,s);
            const auto out=solved(in);assert(close(scale(out.elbow.position,1/s),original.elbow.position,1e-10));
            assert(equivalent(out.shoulder.orientation,original.shoulder.orientation,1e-10)&&equivalent(out.elbow.orientation,original.elbow.orientation,1e-10));
        }
    });
    run("rigid-covariance-including-shoulder-basis-tie",[]{
        auto fallback=straight();fallback.shoulder.orientation=axis({1,0,0},pi/4);
        auto folded=straight();folded.elbow.position={0,1.4,0};folded.wrist.position={.6,1.4,0};folded.target.position={};folded.lengths.mode=LengthMode::adaptive;
        std::mt19937 random(0x2b0e);std::uniform_real_distribution<double> value(-1,1);
        for(unsigned i=0;i<128;++i){const auto rotation=axis({value(random),value(random),value(random)},value(random)*pi);
            const Vector shift{value(random)*50,value(random)*50,value(random)*50};
            covariance(bent(),rotation,shift);covariance(fallback,rotation,shift);covariance(folded,rotation,shift);}
    });
    run("four-centimetre-translation-and-return-with-no-radius-clamp",[]{
        // Synthetic coordinates are metres at1 and centimetre units at100;
        // these conversion factors exist only in this CPU test, not the kernel.
        for(double units:{1.,100.}){
            auto in=bent();for(auto* pose:{&in.shoulder,&in.elbow,&in.wrist})pose->position=scale(pose->position,units);
            in.target=in.wrist;in.authored.point=in.elbow.position;const auto original=solved(in);
            in.target.position.x+=.04*units;const auto moved=solved(in);assert(moved.wrist.position.x==in.wrist.position.x+.04*units);
            Input next=in;next.shoulder=moved.shoulder;next.elbow=moved.elbow;next.wrist=moved.wrist;next.target=in.wrist;
            const auto returned=solved(next);assert(close(returned.elbow.position,original.elbow.position,1e-10*units));
            assert(equivalent(returned.shoulder.orientation,original.shoulder.orientation,1e-10)&&equivalent(returned.elbow.orientation,original.elbow.orientation,1e-10));
        }
        auto close_target=straight();close_target.target.position={.001,0,0};const auto close_out=solved(close_target);
        assert(close_out.wrist.position.x==.001);
    });
    run("finite-normalizable-tiny-and-large-quaternions",[]{
        auto in=bent();for(auto* q:{&in.shoulder.orientation,&in.elbow.orientation,&in.wrist.orientation,&in.target.orientation}){
            q->x*=1e-200;q->y*=1e-200;q->z*=1e-200;q->w*=1e-200;}
        solved(in);in=bent();for(auto* q:{&in.shoulder.orientation,&in.elbow.orientation,&in.wrist.orientation,&in.target.orientation}){
            q->x*=1e200;q->y*=1e200;q->z*=1e200;q->w*=1e200;}
        solved(in);
    });
    run("invalid-input-policy-bones-and-range-retain-output",[]{
        const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
        auto in=bent();in.target.position.x=nan;failure(in,Status::invalid_input);
        in=bent();in.authored.point.x=inf;failure(in,Status::invalid_input);
        in=bent();in.target.orientation={0,0,0,0};failure(in,Status::invalid_quaternion);
        in=bent();in.elbow.orientation.x=nan;failure(in,Status::invalid_quaternion);
        in=bent();in.lengths.upper_scale=0;failure(in,Status::invalid_policy);
        in=bent();in.lengths.lower_scale=inf;failure(in,Status::invalid_policy);
        in=bent();in.lengths.mode=static_cast<LengthMode>(99);failure(in,Status::invalid_policy);
        in=bent();in.elbow.position=in.shoulder.position;failure(in,Status::degenerate_bone);
        in=bent();in.wrist.position=in.elbow.position;failure(in,Status::degenerate_bone);
        in=bent();in.shoulder.position.x=-std::numeric_limits<double>::max();in.target.position.x=std::numeric_limits<double>::max();failure(in,Status::numerical_failure);
    });
    std::printf("two-bone actual-header CPU: %u groups PASS; 128 seeded rigid transforms x3 cases; no engine/model/render/Native proof\n",groups);
}
