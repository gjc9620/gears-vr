#pragma once
#include "../vr-common/menu_display_state.h"
// Display ownership changes only after a real ticketed projection was submitted.
class StartupPanelPolicy {
    bool enabled_, handed_off_=false, cancelled_=false;
    bool reversible_=false,resume_required_=false,menu_presented_=false;
    gears_menu::Mode mode_{gears_menu::Mode::unknown};
    std::int64_t resume_after_{};
public:
    explicit StartupPanelPolicy(bool enabled,bool reversible=false):enabled_(enabled),reversible_(reversible){}
    bool observe(gears_menu::Mode mode,std::int64_t now){
        if(!reversible_||!enabled_||cancelled_||mode==mode_)return false;
        mode_=mode;
        if(mode==gears_menu::Mode::death_menu||mode==gears_menu::Mode::pause_menu||mode==gears_menu::Mode::main_menu){
            resume_required_=true;resume_after_=now;return true;}
        if(resume_required_)resume_after_=now;
        return false;
    }
    bool show_panel() const {return enabled_&&!cancelled_&&
        (mode_==gears_menu::Mode::death_menu||mode_==gears_menu::Mode::main_menu||(!handed_off_&&!resume_required_));}
    bool show_eye_menu()const{return enabled_&&!cancelled_&&mode_==gears_menu::Mode::pause_menu;}
    void menu_rendered(){
        if(enabled_&&!cancelled_&&(mode_==gears_menu::Mode::pause_menu||
           mode_==gears_menu::Mode::main_menu||mode_==gears_menu::Mode::death_menu))menu_presented_=true;
    }
    bool show_transition_panel()const{return enabled_&&reversible_&&!cancelled_&&resume_required_&&
        menu_presented_&&mode_==gears_menu::Mode::gameplay;}
    bool allow_projection()const{return !cancelled_&&mode_!=gears_menu::Mode::death_menu&&mode_!=gears_menu::Mode::pause_menu&&
        mode_!=gears_menu::Mode::main_menu&&
        (!resume_required_||mode_==gears_menu::Mode::gameplay);}
    std::int64_t minimum_sample_ns()const{return resume_required_?resume_after_:0;}
    bool projection_rendered(bool success) {
        if(!enabled_||!allow_projection()||!success)return false;
        const bool transition=!handed_off_||resume_required_;
        handed_off_=true;resume_required_=false;menu_presented_=false;return transition;
    }
    void cancel(){cancelled_=true;}
};
