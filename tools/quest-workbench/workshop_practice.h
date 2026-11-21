#pragma once
#include "armory_practice.h"
namespace workbench {
class WorkshopPractice {
    Practice lancer_;ArmoryPractice armory_;bool generic_{};
public:
    bool generic()const{return generic_;}ArmoryPractice const& armory()const{return armory_;}
    void configure(Json const& config){generic_=!config.is_null()&&!config.empty();if(generic_)armory_.configure(config);}
    void reset(P gun,P head,bool empty=false){if(generic_)armory_.reset(gun,head,empty);else lancer_.reset(gun,head,empty);}
    void resume(PracticeSettings const& s,std::uint64_t epoch){if(generic_)armory_.resume(s,epoch);else lancer_.resume(s,epoch);}
    void settings(PracticeSettings const& s){if(generic_)armory_.settings(s);else lancer_.settings(s);}
    void body(P p){if(generic_)armory_.body(p);else lancer_.body(p);}
    void reposition(P p){if(generic_)armory_.reposition(p);else lancer_.reposition(p);}
    void interrupt(){if(generic_)armory_.interrupt();else lancer_.interrupt();}
    void step(PracticeFrame const& f,std::vector<Part> const& parts){if(generic_)armory_.step(f,parts);else lancer_.step(f,parts);}
    void acknowledge_render(std::uint64_t serial){if(generic_)armory_.acknowledge_render(serial);else lancer_.acknowledge_render(serial);}
    P bone_delta(unsigned i)const{return generic_?armory_.bone_delta(i):P{};}
    bool bone_visible(unsigned i)const{return !generic_||armory_.bone_visible(i);}
    P part_delta(std::string const& id)const{return generic_?armory_.delta(id):P{};}
    bool ammo_held()const{return generic_&&armory_.ammo_held();}
    unsigned held_quantity()const{return generic_?armory_.held_quantity():0;}
    bool beam_active()const{return generic_&&armory_.beam_active();}
#define QWB_VALUE(name) auto name()const{return generic_?armory_.name():lancer_.name();}
    QWB_VALUE(gun) QWB_VALUE(frame) QWB_VALUE(magazine) QWB_VALUE(board)
    QWB_VALUE(main_held) QWB_VALUE(off_part) QWB_VALUE(mag_visible) QWB_VALUE(mag_seated)
    QWB_VALUE(motor) QWB_VALUE(chambered) QWB_VALUE(charge_progress) QWB_VALUE(starter_progress)
    QWB_VALUE(loaded) QWB_VALUE(reserve) QWB_VALUE(shots) QWB_VALUE(hits) QWB_VALUE(cycles)
    QWB_VALUE(visual_gun) QWB_VALUE(shot_ray) QWB_VALUE(shoulder)
#undef QWB_VALUE
    bool shoulder_contains(V p)const{return generic_?armory_.shoulder_contains(p):lancer_.shoulder_contains(p);}
    V shoulder_adjust_for(V p)const{return generic_?armory_.shoulder_adjust_for(p):lancer_.shoulder_adjust_for(p);}
    std::string const& status()const{return generic_?armory_.status():lancer_.status();}
    auto const& hand_wrists()const{return generic_?armory_.hand_wrists():lancer_.hand_wrists();}
    auto const& hand_parts()const{return generic_?armory_.hand_parts():lancer_.hand_parts();}
    auto const& feedback()const{return generic_?armory_.feedback():lancer_.feedback();}
    auto const& impacts()const{return generic_?armory_.impacts():lancer_.impacts();}
};

// Distinct, editable starting templates. No raw tracking flags or captured timestamps.
inline void seed_workshop_poses(std::vector<Part>& parts,Json const& info){
    auto main=from_raw_wrist({{},matrix_quaternion(v::ue_matrix({0,1.5707963267948966,0}))}).orientation;
    auto support=from_raw_wrist(gears_lancer_owner_grabposes::support_wrist_in_gun).orientation;
    auto pinch=from_raw_wrist(gears_lancer_owner_grabposes::loading_wrist_in_gun).orientation;
    auto magazine=from_raw_wrist(gears_lancer_owner_grabposes::magazine_wrist_in_item).orientation;
    for(unsigned i=0;i<parts.size();++i){auto& part=parts[i];std::string role=info.at("parts")[i].value("role",std::string{});unsigned preferred=info.at("parts")[i].value("side",0u);
        for(unsigned side=0;side<2;++side){bool cylinder=role=="ammo"&&info.at("practice").at("kind")=="revolver";if(part.known[side]&&!(cylinder&&part.poses[side].authored_default))continue;if(role=="main"&&side!=1)continue;if(role=="support"&&side!=0)continue;if(role=="mechanism"&&side!=preferred)continue;if(role=="ammo"&&side!=0)continue;
            auto& p=part.poses[side];p={};p.side=side;p.mapped=p.authored_default=p.part_delta_known=true;Q q=role=="main"?(side==1?main:support):preferred==1?main:role=="support"?support:role=="ammo"?(cylinder?magazine:pinch):magazine;
            auto mount=controller_wrist_mount(side,false);auto offset=rotate(v::inverse(mount.orientation),mount.position);auto point=part.volume.local.position;if(role=="ammo"&&info.at("practice").at("kind")!="magazine"&&info.at("practice").at("kind")!="pistol")point=add(vector(info.at("practice").at("feed")),{0,0,cylinder?-.035:.065});p.wrist_in_object={add(point,rotate(q,offset)),q};
            for(unsigned j=0;j<15;++j)p.local[j]=side==1?from_raw_joint(gears_lancer_main_hand::rest[j]):role=="support"||role=="main"?from_raw_joint(gears_lancer_natural_hand::support_closed[j]):role=="ammo"&&!cylinder?from_raw_joint(gears_lancer_part_hand::loading[j]):from_raw_joint(gears_lancer_part_hand::magazine[j]);part.known[side]=true;
        }
    }
}
}
