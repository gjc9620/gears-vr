#include "startup_panel_policy.h"
#include <cassert>
int main(){
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
}
