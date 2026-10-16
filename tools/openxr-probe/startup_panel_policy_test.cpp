#include "startup_panel_policy.h"
#include <cassert>
#include <initializer_list>
int main(){
    using Wait=StartupPanelPolicy::StartupWaitContext;
    const Wait ready{0,true,true,true,false,false,false,false,true};
    StartupPanelPolicy waiting(true,true);
    assert(waiting.show_startup_wait(ready)); // UNKNOWN permits only this generic card.
    for(unsigned fault=0;fault<9;++fault){auto bad=ready;
        switch(fault){case 0:bad.layer_count=1;break;case 1:bad.should_render=false;break;
        case 2:bad.session_visible=false;break;case 3:bad.running=false;break;
        case 4:bad.stop_requested=true;break;case 5:bad.producer_stopped=true;break;
        case 6:bad.loss=true;break;case 7:bad.structural_error=true;break;
        case 8:bad.ordinary_wait=false;break;}
        assert(!waiting.show_startup_wait(bad));
    }
    assert(!waiting.projection_rendered(false)&&waiting.show_startup_wait(ready));
    assert(waiting.minimum_sample_ns()==0);
    assert(waiting.projection_rendered(true)&&!waiting.show_startup_wait(ready));
    // Resize/world waits cannot undo handed_off_; nor can later authorized menus.
    for(auto mode:{gears_menu::Mode::main_menu,gears_menu::Mode::death_menu,
        gears_menu::Mode::pause_menu,gears_menu::Mode::unknown,gears_menu::Mode::gameplay}){
        waiting.observe(mode,99);assert(!waiting.show_startup_wait(ready));
    }
    StartupPanelPolicy before_menu(true,true);
    before_menu.observe(gears_menu::Mode::main_menu,100);
    assert(before_menu.show_startup_wait(ready)&&before_menu.minimum_sample_ns()==100);
    before_menu.observe(gears_menu::Mode::unknown,101);
    assert(!before_menu.show_startup_wait(ready)); // Still obeys show_panel().
    before_menu.observe(gears_menu::Mode::pause_menu,102);
    assert(!before_menu.show_startup_wait(ready));
    StartupPanelPolicy disabled_wait(false,true),cancelled_wait(true,true);
    cancelled_wait.cancel();
    assert(!disabled_wait.show_startup_wait(ready)&&!cancelled_wait.show_startup_wait(ready));
    StartupPanelPolicy panel(true);assert(panel.show_panel());
    assert(!panel.projection_rendered(false)&&panel.show_panel());
    assert(panel.projection_rendered(true)&&!panel.show_panel());
    assert(!panel.projection_rendered(false)&&!panel.show_panel());
    assert(!panel.projection_rendered(true)&&!panel.show_panel());
    StartupPanelPolicy cancelled(true);cancelled.cancel();
    assert(!cancelled.show_panel()&&!cancelled.projection_rendered(true));
    StartupPanelPolicy absent(false);assert(!absent.show_panel()&&!absent.projection_rendered(true));
    using gears_menu::Mode;
    StartupPanelPolicy reversible(true,true);
    assert(reversible.projection_rendered(true)&&!reversible.show_panel());
    assert(reversible.observe(Mode::death_menu,100));
    assert(reversible.show_panel()&&!reversible.allow_projection()&&!reversible.projection_rendered(true));
    assert(!reversible.observe(Mode::death_menu,101)); // Heartbeat does not reset capture epoch.
    assert(!reversible.observe(Mode::unknown,200));
    assert(!reversible.show_panel()&&!reversible.allow_projection()); // Expired state cannot display old menu.
    assert(!reversible.observe(Mode::gameplay,300));
    assert(reversible.allow_projection()&&!reversible.show_panel()&&reversible.minimum_sample_ns()==300);
    assert(!reversible.projection_rendered(false)&&reversible.minimum_sample_ns()==300);
    assert(reversible.projection_rendered(true)&&reversible.minimum_sample_ns()==0);
    assert(reversible.observe(Mode::death_menu,400));
    assert(reversible.show_panel()&&!reversible.allow_projection());
    reversible.cancel();assert(!reversible.show_panel()&&!reversible.allow_projection());
    StartupPanelPolicy legacy(true);
    assert(!legacy.observe(Mode::death_menu,100)&&legacy.allow_projection());
    StartupPanelPolicy pause(true,true);
    assert(pause.observe(Mode::pause_menu,500));
    assert(pause.show_eye_menu()&&!pause.show_panel()&&!pause.allow_projection());
    assert(!pause.projection_rendered(true)); // A menu is never a stereo handoff.
    assert(!pause.observe(Mode::pause_menu,600)&&pause.minimum_sample_ns()==500);
    assert(!pause.observe(Mode::unknown,700));
    assert(!pause.show_eye_menu()&&!pause.show_panel()&&!pause.allow_projection());
    assert(!pause.observe(Mode::gameplay,800)&&pause.minimum_sample_ns()==800);
    assert(!pause.projection_rendered(false)&&pause.minimum_sample_ns()==800);
    assert(pause.projection_rendered(true)&&pause.minimum_sample_ns()==0&&pause.allow_projection());
    assert(pause.observe(Mode::pause_menu,900));
    assert(pause.observe(Mode::death_menu,1000)&&pause.show_panel()&&!pause.show_eye_menu());
    assert(pause.observe(Mode::pause_menu,1100)&&!pause.show_panel()&&pause.show_eye_menu());
    pause.cancel();assert(!pause.show_eye_menu()&&!pause.allow_projection());
    StartupPanelPolicy main(true,true);
    assert(main.projection_rendered(true));
    assert(main.observe(Mode::main_menu,1200));
    assert(main.show_panel()&&!main.show_eye_menu()&&!main.allow_projection());
    assert(!main.projection_rendered(true)&&main.minimum_sample_ns()==1200);
    assert(!main.observe(Mode::main_menu,1300)&&main.minimum_sample_ns()==1200);
    assert(!main.observe(Mode::unknown,1400));
    assert(!main.show_panel()&&!main.show_eye_menu()&&!main.allow_projection());
    assert(!main.projection_rendered(true)&&main.minimum_sample_ns()==1400);
    assert(main.observe(Mode::main_menu,1500)&&main.show_panel()); // New capture epoch.
    assert(!main.observe(Mode::gameplay,1600)&&main.minimum_sample_ns()==1600);
    assert(main.allow_projection()&&!main.show_panel());
    assert(!main.observe(Mode::gameplay,1700)&&main.minimum_sample_ns()==1600);
    assert(!main.projection_rendered(false)&&main.minimum_sample_ns()==1600);
    assert(main.projection_rendered(true)&&main.minimum_sample_ns()==0);
    assert(main.observe(Mode::pause_menu,1800)&&main.show_eye_menu()&&!main.show_panel());
    assert(main.observe(Mode::main_menu,1900)&&main.show_panel()&&!main.show_eye_menu());
    assert(main.observe(Mode::pause_menu,2000)&&main.show_eye_menu()&&!main.show_panel());
    assert(main.observe(Mode::main_menu,2100)&&main.show_panel());
    main.cancel();assert(!main.show_panel()&&!main.allow_projection());
    StartupPanelPolicy main_before_handoff(true,true);
    assert(main_before_handoff.observe(Mode::main_menu,2200));
    assert(!main_before_handoff.observe(Mode::unknown,2300));
    assert(!main_before_handoff.show_panel()&&!main_before_handoff.allow_projection());
    assert(!main_before_handoff.observe(Mode::gameplay,2400));
    assert(main_before_handoff.allow_projection()&&main_before_handoff.minimum_sample_ns()==2400);
    assert(main_before_handoff.projection_rendered(true));
    // A recovery card is display continuity, never evidence of a new scene.
    StartupPanelPolicy bridge(true,true);
    assert(!bridge.show_transition_panel());
    bridge.observe(Mode::pause_menu,3000);
    bridge.observe(Mode::gameplay,3100);
    assert(!bridge.show_transition_panel()); // Observing a menu is not presenting it.
    bridge.observe(Mode::pause_menu,3200);bridge.menu_rendered();
    assert(!bridge.show_transition_panel());
    bridge.observe(Mode::unknown,3300);
    assert(!bridge.show_transition_panel());
    bridge.observe(Mode::gameplay,3400);
    assert(bridge.show_transition_panel()&&bridge.minimum_sample_ns()==3400);
    assert(!bridge.projection_rendered(false)&&bridge.show_transition_panel());
    bridge.observe(Mode::gameplay,3500);
    assert(bridge.minimum_sample_ns()==3400); // Card does not move the new-image boundary.
    assert(bridge.projection_rendered(true)&&!bridge.show_transition_panel());
    bridge.observe(Mode::pause_menu,3600);bridge.observe(Mode::gameplay,3700);
    assert(!bridge.show_transition_panel()); // Prior menu witness retired with projection.
    bridge.observe(Mode::death_menu,3800);bridge.menu_rendered();bridge.observe(Mode::gameplay,3900);
    assert(bridge.show_transition_panel());bridge.cancel();assert(!bridge.show_transition_panel());
    StartupPanelPolicy no_bridge(false,true),legacy_bridge(true);
    for(auto* p:{&no_bridge,&legacy_bridge}){p->observe(Mode::pause_menu,4000);p->menu_rendered();
        p->observe(Mode::gameplay,4100);assert(!p->show_transition_panel());}
}
