#pragma once
#include "../vr-common/menu_display_state.h"
// Display ownership changes only after a real ticketed projection was submitted.
class StartupPanelPolicy {
    bool enabled_, handed_off_=false, cancelled_=false;
    bool reversible_=false,resume_required_=false,menu_presented_=false;
    gears_menu::Mode mode_{gears_menu::Mode::unknown};
    std::int64_t resume_after_{};
public:
    struct StartupWaitContext {
        unsigned layer_count=0;
        bool should_render=false,session_visible=false,running=false;
        bool stop_requested=false,producer_stopped=false,loss=false,structural_error=false,ordinary_wait=false;
    };
    explicit StartupPanelPolicy(bool enabled,bool reversible=false):enabled_(enabled),reversible_(reversible){}
    bool observe(gears_menu::Mode mode,std::int64_t now){
        if(!reversible_||!enabled_||cancelled_||mode==mode_)return false;
        mode_=mode;
        if(mode==gears_menu::Mode::death_menu||mode==gears_menu::Mode::pause_menu||mode==gears_menu::Mode::main_menu){
            resume_required_=true;resume_after_=now;return true;}
        // Loading is neither a menu witness nor image authority. Require a
        // newer ticket on later gameplay even if no projection has occurred yet.
        if(mode==gears_menu::Mode::loading_display||mode==gears_menu::Mode::movie_display){resume_required_=true;resume_after_=now;return false;}
        // acquisition_wait deliberately follows UNKNOWN here: it cannot create
        // a resume requirement or witness during ordinary ticketed gameplay.
        if(resume_required_)resume_after_=now;
        return false;
    }
    bool show_panel() const {return enabled_&&!cancelled_&&mode_!=gears_menu::Mode::loading_display&&
        (mode_==gears_menu::Mode::death_menu||mode_==gears_menu::Mode::main_menu||(!handed_off_&&!resume_required_));}
    // Pause uses a native stereo projection but retains the menu epoch. It
    // cannot retire the fresh-image boundary required when gameplay resumes.
    bool show_pause_projection()const{return enabled_&&!cancelled_&&mode_==gears_menu::Mode::pause_menu;}
    // Producer readiness alone must never retire the initial fallback capture.
    bool suspend_window_capture()const{return handed_off_&&!show_panel();}
    void menu_rendered(){
        if(enabled_&&!cancelled_&&(mode_==gears_menu::Mode::pause_menu||
           mode_==gears_menu::Mode::main_menu||mode_==gears_menu::Mode::death_menu))menu_presented_=true;
    }
    bool show_transition_panel()const{return enabled_&&reversible_&&!cancelled_&&resume_required_&&
        (mode_==gears_menu::Mode::movie_display||(menu_presented_&&(mode_==gears_menu::Mode::gameplay||mode_==gears_menu::Mode::loading_display||
            mode_==gears_menu::Mode::acquisition_wait||mode_==gears_menu::Mode::main_menu)));}
    bool transition_menu_requested_wait()const{return show_transition_panel()&&
        (mode_==gears_menu::Mode::main_menu||mode_==gears_menu::Mode::acquisition_wait);}
    bool transition_authorized(gears_menu::Mode fresh)const{return fresh==mode_&&show_transition_panel();}
    // A generic session-owned card can fill an empty startup frame. It grants
    // no authority to show a game image, and a real projection retires it forever.
    bool show_startup_wait(StartupWaitContext const& c)const{
        return enabled_&&!cancelled_&&!handed_off_&&(show_panel()||mode_==gears_menu::Mode::loading_display||
            (mode_==gears_menu::Mode::gameplay&&resume_required_))&&
            c.layer_count==0&&c.should_render&&
            c.session_visible&&c.running&&!c.stop_requested&&!c.producer_stopped&&!c.loss&&
            !c.structural_error&&c.ordinary_wait;
    }
    bool allow_projection(bool window_fallback=false)const{return !cancelled_&&mode_!=gears_menu::Mode::death_menu&&mode_!=gears_menu::Mode::pause_menu&&
        mode_!=gears_menu::Mode::main_menu&&(mode_!=gears_menu::Mode::loading_display||window_fallback)&&
        (!resume_required_||mode_==gears_menu::Mode::gameplay||(window_fallback&&
            (mode_==gears_menu::Mode::loading_display||mode_==gears_menu::Mode::movie_display)));}
    std::int64_t minimum_sample_ns()const{return resume_required_?resume_after_:0;}
    bool projection_rendered(bool success,bool window_fallback=false) {
        if(!enabled_||!allow_projection(window_fallback)||!success)return false;
        const bool transition=!handed_off_||resume_required_;
        handed_off_=true;resume_required_=false;menu_presented_=false;return transition;
    }
    void cancel(){cancelled_=true;}
};
