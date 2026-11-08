#pragma once
#include "pose_math.h"
#include <algorithm>
#include <cstdint>

// Cosmetic authoring for the authenticated Marcus115 rig. The live caller
// owns its LOD bones and supplies the current squeeze; this grants no grip,
// item lease, tracking, wrist target or skeleton-write permission.
namespace gears_hand_fingers {
enum class Shape:unsigned {free,magazine,loading,starter,support,weapon,grenade};
inline gears_vr::Quaternion product(gears_vr::Quaternion a,gears_vr::Quaternion b) noexcept {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline gears_vr::Quaternion turn(unsigned axis,double radians) noexcept {
    gears_vr::Quaternion q{};q.w=std::cos(radians*.5);
    const double s=std::sin(radians*.5);if(axis==1)q.y=s;else q.z=s;return q;
}
inline bool rotation(unsigned side,unsigned finger_joint,double amount,
    gears_vr::Quaternion reference,gears_vr::Quaternion& out,Shape shape=Shape::free,double shape_weight=1.,double trigger=0.) noexcept {
    if(side>1||finger_joint>=15||unsigned(shape)>unsigned(Shape::grenade)||!std::isfinite(amount)||amount<0||amount>1||
       !std::isfinite(trigger)||trigger<0||trigger>1||
       !std::isfinite(shape_weight)||shape_weight<0||shape_weight>1||
       !std::isfinite(reference.x)||!std::isfinite(reference.y)||!std::isfinite(reference.z)||!std::isfinite(reference.w))return false;
    const double norm=reference.x*reference.x+reference.y*reference.y+reference.z*reference.z+reference.w*reference.w;
    if(norm<.98||norm>1.02)return false;
    if(amount==0){out=reference;return true;}
    constexpr double radians=.017453292519943295;
    // In this rig both hands flex about local -Z. Mirroring changes the
    // thumb's opposition axis, not finger flexion. Include MCP flexion: the
    // original left support pose bent only distal joints, leaving the palm
    // open under the held magazine. These are designer-authored joint angles,
    // not a measurement of the user's fingers or a retargeted stock frame.
    constexpr double flex[4][3]={{30,65,35},{35,70,35},{35,70,35},{40,65,30}};
    // Distinct authored contacts: a broad magazine, a two-finger mechanism
    // grip, and a small cylindrical starter. These remain visual profiles;
    // only the current object lease selects them, never raw squeeze alone.
    // Either button can initiate the magazine's same four-finger box wrap.
    // The original static magazine in its measured wrist mount extends
    // X7.62..19.41, Y-7.61..-3.13. The former relaxed middle/ring/pinky went
    // through that volume; curl around its near end rather than through it.
    // E remains a distinct index/thumb tab pinch; C is cylindrical.
    constexpr double magazine[4][3]={{82,74,20},{86,72,22},{88,70,24},{90,68,26}};
    // Keep the measured index/thumb pinch on E. The other three fingers
    // curl into the palm instead of hanging across the receiver/rail; they
    // neither establish contact nor contribute mechanical charging travel.
    constexpr double loading[4][3]={{60,62,24},{60,65,25},{65,65,27},{70,65,28}};
    // Wrap the original narrow C barrel at the PIP/DIP joints as well as
    // the knuckles. The former shallow distal bend left the fingertip skin
    // beyond the barrel and against the palm. Ease MCP flexion slightly;
    // preserve the authored thumb, every bone translation and all other parts.
    constexpr double starter[4][3]={{82,100,65},{85,105,65},{85,105,65},{85,100,60}};
    // Original LOD0 skin against the fixed underside palm contact: the former
    // MCP curl buried the distal pads in the receiver. Extend the knuckles,
    // then wrap the longer fingers around the far surface. The index follows
    // the shallower front contour; bone lengths and the wrist stay unchanged.
    constexpr double support[4][3]={{15,10,15},{5,70,25},{0,90,25},{5,90,25}};
    // The actual held-rifle context owns the wrap even after the acquisition
    // Grip is released. Only its index follows the real firing trigger. Do
    // not inherit a reload/saw animation's open or twisted stock fingers.
    const double weapon[4][3]={{25+20*trigger,35+20*trigger,15+7*trigger},{65,70,30},{70,72,32},{75,70,30}};
    // Bolo handle wrap, including the index: trigger release throws but must
    // not masquerade as opening the grasp or become a firearm trigger pose.
    // This is authored skin fitting, not physical hand tracking or permission.
    constexpr double grenade[4][3]={{72,68,30},{76,72,32},{80,72,32},{84,70,30}};
    const auto& bends=shape==Shape::grenade?grenade:(shape==Shape::weapon?weapon:(shape==Shape::support?support:(shape==Shape::magazine?magazine:(shape==Shape::loading?loading:(shape==Shape::starter?starter:flex)))));
    auto closed=reference;
    if(finger_joint<12)closed=product(reference,turn(2,-radians*bends[finger_joint/3][finger_joint%3]));
    else if(finger_joint==12){
        // The former deeply flexed thumb went under the curled fingers.
        // Reference-joint authoring keeps its base open and opposes the pad
        // across the object, while the two distal joints supply the wrap.
        // E's thumb pad must oppose the index at the small tab. The former
        // extended base left the thumb beside the index and the old palm
        // contact put the tab behind both pads. This is skin authoring only.
        const double flexion=shape==Shape::support?15:(shape==Shape::grenade?4:(shape==Shape::weapon?5:(shape==Shape::magazine?10:(shape==Shape::loading?6:(shape==Shape::starter?-5:20)))));
        // Support thumb lies forward along the near side. Opposition35 drove
        // its original skin through the gun despite a correct palm-up wrist.
        const double opposition=shape==Shape::support?0:(shape==Shape::grenade?30:(shape==Shape::weapon?28:(shape==Shape::magazine?38:(shape==Shape::loading?26:(shape==Shape::starter?30:35)))));
        closed=product(turn(2,-flexion*radians),product(turn(1,(side?opposition:-opposition)*radians),reference));
    }else{
        const double flexion=shape==Shape::support?(finger_joint==13?15:10):(shape==Shape::magazine?(finger_joint==13?35:20):
            (shape==Shape::loading?(finger_joint==13?45:30):(shape==Shape::starter?(finger_joint==13?45:30):(finger_joint==13?25:20))));
        closed=product(reference,turn(2,-flexion*radians));
    }
    // The sole C0 wrist attachment supplies this same120ms blend weight.
    // A newly admitted object must not instantly swap the finger silhouette
    // while the palm is still turning toward it. Raw squeeze still owns
    // opening/closure; a cosmetic shape blend grants no grip or part motion.
    if(shape!=Shape::free&&shape_weight<1.){
        gears_vr::Quaternion ordinary{};
        if(!rotation(side,finger_joint,1.,reference,ordinary,Shape::free))return false;
        const double sign=ordinary.x*closed.x+ordinary.y*closed.y+ordinary.z*closed.z+ordinary.w*closed.w<0?-1.:1.;
        const double w=shape_weight;
        closed={ordinary.x*(1-w)+closed.x*w*sign,ordinary.y*(1-w)+closed.y*w*sign,
            ordinary.z*(1-w)+closed.z*w*sign,ordinary.w*(1-w)+closed.w*w*sign};
        const double n=std::sqrt(closed.x*closed.x+closed.y*closed.y+closed.z*closed.z+closed.w*closed.w);
        if(!std::isfinite(n)||n<1e-8)return false;
        closed={closed.x/n,closed.y/n,closed.z/n,closed.w/n};
    }
    const double sign=reference.x*closed.x+reference.y*closed.y+reference.z*closed.z+reference.w*closed.w<0?-1.:1.;
    const gears_vr::Quaternion blended{reference.x*(1-amount)+closed.x*amount*sign,
        reference.y*(1-amount)+closed.y*amount*sign,reference.z*(1-amount)+closed.z*amount*sign,
        reference.w*(1-amount)+closed.w*amount*sign};
    const double length=std::sqrt(blended.x*blended.x+blended.y*blended.y+blended.z*blended.z+blended.w*blended.w);
    if(!std::isfinite(length)||length<1e-8)return false;
    out={blended.x/length,blended.y/length,blended.z/length,blended.w/length};return true;
}
// Bounded skin-only rotation on each fresh C0 solve. It covers release as
// well as attachment and changes between contacts; a raw trigger release
// must not instantly replace the fitted fingers while the palm blends out.
// No old world pose or input/lease is retained by this cosmetic state.
struct PoseBlend {
    gears_vr::Quaternion joints[15]{};bool known[15]{};
    std::int64_t last_ns{};double step{};
    bool begin(std::int64_t now) noexcept {
        if(now<=0||now<last_ns)return false;
        step=last_ns?std::clamp(double(now-last_ns)/1e9,0.,.05)*20.:0.;
        last_ns=now;return true; // At most about115deg over100ms.
    }
    bool joint(unsigned n,gears_vr::Quaternion reference,gears_vr::Quaternion desired,gears_vr::Quaternion& out) noexcept {
        if(n>=15)return false;
        if(!known[n]){joints[n]=reference;known[n]=true;}
        auto a=joints[n];
        double dot=a.x*desired.x+a.y*desired.y+a.z*desired.z+a.w*desired.w;
        if(dot<0){desired={-desired.x,-desired.y,-desired.z,-desired.w};dot=-dot;}
        const double angle=2*std::acos(std::clamp(dot,0.,1.));
        const double t=angle>1e-8?std::min(1.,step/angle):1.;
        out=t>=1.?desired:gears_vr::normalized({a.x+(desired.x-a.x)*t,a.y+(desired.y-a.y)*t,
            a.z+(desired.z-a.z)*t,a.w+(desired.w-a.w)*t});
        joints[n]=out;return true;
    }
};
}
