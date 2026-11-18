#pragma once
#include "weapon_hand_participant.h"
#include <cmath>

// Cosmetic preference only. The caller must supply freshly certified RAW
// distances and withdraw on input/source/lease failure. No pose, press token,
// lease, action or freshness timestamp is produced or retained here.
namespace gears_boomshot_passive_grasp {
namespace hand=gears_weapon_hand;
enum class Part:unsigned {none,support,opening,closing};
struct Scope {
    hand::FrameKey key{};
    std::array<hand::ObjectIdentity,6> objects{}; // world/controller/pawn/mesh/weapon/asset
    std::array<std::uint64_t,17> boundaries{}; // hold/reference/producer/observed/XR/input epochs
    double units{},root_scale{},hand_scale{};
    unsigned source{},control{};
};
inline bool same(Scope a,Scope b) noexcept {
    a.key.frame=b.key.frame=0;
    if(!hand::same(a.key,b.key)||a.boundaries!=b.boundaries||a.units!=b.units||
       a.root_scale!=b.root_scale||a.hand_scale!=b.hand_scale||a.source!=b.source||a.control!=b.control)return false;
    for(unsigned i=0;i<a.objects.size();++i)if(!hand::same(a.objects[i],b.objects[i]))return false;
    return true;
}
class Selector {
public:
    static constexpr double acquire_metres=.12,retain_metres=.20;
    void lost() noexcept {*this={};}
    Part advance(Scope const& scope,std::int64_t source_ns,Part mechanism,double mechanism_metres,double support_metres) noexcept {
        if(source_ns<=0||!scope.key.frame||(mechanism!=Part::opening&&mechanism!=Part::closing&&mechanism!=Part::none)){
            lost();return Part::none;
        }
        const auto valid=[](double value)noexcept{return std::isfinite(value)&&value>=0;};
        const auto distance=[&](Part part)noexcept{
            if(part==Part::support&&valid(support_metres))return support_metres;
            if(part==mechanism&&mechanism!=Part::none&&valid(mechanism_metres))return mechanism_metres;
            return 1e9;
        };
        if(known_&&same(scope_,scope)){
            // A replay/rewind cannot update the preference or extend retention.
            if(scope.key.frame<scope_.key.frame||source_ns<source_ns_){lost();return Part::none;}
            if(scope.key.frame==scope_.key.frame){
                if(source_ns!=source_ns_||distance(part_)>retain_metres){lost();return Part::none;}
                return part_;
            }
            if(source_ns-source_ns_>100000000)lost();
        }else lost();
        // Retain the current eligible part even when a neighbour is closer.
        // Once it leaves its radius, only a fresh acquire may select another.
        if(!known_||distance(part_)>retain_metres){
            part_=Part::none;
            if(mechanism!=Part::none&&valid(mechanism_metres)&&mechanism_metres<=acquire_metres)part_=mechanism;
            if(valid(support_metres)&&support_metres<=acquire_metres&&
               (part_==Part::none||support_metres<mechanism_metres))part_=Part::support;
        }
        if(part_==Part::none){lost();return part_;}
        scope_=scope;source_ns_=source_ns;known_=true;return part_;
    }
private:
    Scope scope_{};std::int64_t source_ns_{};Part part_{};bool known_{};
};
}
