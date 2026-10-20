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
    // Capture stays alive for the first startup/CG fallback until a ticketed
    // projection actually succeeds; readiness or failed submission is insufficient.
    StartupPanelPolicy capture(true,true);
    assert(!capture.suspend_window_capture());
    capture.observe(Mode::gameplay,1);
    assert(!capture.projection_rendered(false)&&!capture.suspend_window_capture());
    capture.observe(Mode::loading_display,2);
    assert(!capture.suspend_window_capture());
    capture.observe(Mode::gameplay,3);
    assert(capture.projection_rendered(true)&&capture.suspend_window_capture());
    capture.observe(Mode::main_menu,4);
    assert(capture.show_panel()&&!capture.suspend_window_capture());
    capture.observe(Mode::main_menu,5);
    assert(!capture.suspend_window_capture()); // Heartbeat keeps the binding.
    capture.observe(Mode::pause_menu,6);
    assert(capture.show_pause_projection()&&capture.suspend_window_capture());
    capture.observe(Mode::death_menu,7);
    assert(capture.show_panel()&&!capture.suspend_window_capture());
    capture.observe(Mode::loading_display,8);
    assert(capture.suspend_window_capture());
    capture.observe(Mode::gameplay,9);
    assert(capture.minimum_sample_ns()==9&&capture.suspend_window_capture());
    assert(!capture.projection_rendered(false)&&capture.minimum_sample_ns()==9);
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
    assert(pause.show_pause_projection()&&!pause.show_panel()&&!pause.allow_projection());
    assert(!pause.projection_rendered(true)); // Pause stereo is not a gameplay handoff.
    pause.menu_rendered();
    assert(pause.show_pause_projection()&&pause.minimum_sample_ns()==500);
    assert(!pause.projection_rendered(false)&&pause.minimum_sample_ns()==500);
    assert(!pause.projection_rendered(true)&&pause.minimum_sample_ns()==500);
    assert(!pause.observe(Mode::pause_menu,600)&&pause.minimum_sample_ns()==500);
    assert(!pause.observe(Mode::unknown,700));
    assert(!pause.show_pause_projection()&&!pause.show_panel()&&!pause.allow_projection());
    assert(!pause.observe(Mode::gameplay,800)&&pause.minimum_sample_ns()==800);
    assert(pause.show_transition_panel()&&!pause.show_pause_projection());
    assert(!pause.projection_rendered(false)&&pause.minimum_sample_ns()==800);
    assert(pause.projection_rendered(true)&&pause.minimum_sample_ns()==0&&pause.allow_projection());
    assert(pause.observe(Mode::pause_menu,900));
    assert(pause.observe(Mode::death_menu,1000)&&pause.show_panel()&&!pause.show_pause_projection());
    assert(pause.observe(Mode::pause_menu,1100)&&!pause.show_panel()&&pause.show_pause_projection());
    pause.cancel();assert(!pause.show_pause_projection()&&!pause.allow_projection());
    StartupPanelPolicy main(true,true);
    assert(main.projection_rendered(true));
    assert(main.observe(Mode::main_menu,1200));
    assert(main.show_panel()&&!main.show_pause_projection()&&!main.allow_projection());
    assert(!main.projection_rendered(true)&&main.minimum_sample_ns()==1200);
    assert(!main.observe(Mode::main_menu,1300)&&main.minimum_sample_ns()==1200);
    assert(!main.observe(Mode::unknown,1400));
    assert(!main.show_panel()&&!main.show_pause_projection()&&!main.allow_projection());
    assert(!main.projection_rendered(true)&&main.minimum_sample_ns()==1400);
    assert(main.observe(Mode::main_menu,1500)&&main.show_panel()); // New capture epoch.
    assert(!main.observe(Mode::gameplay,1600)&&main.minimum_sample_ns()==1600);
    assert(main.allow_projection()&&!main.show_panel());
    assert(!main.observe(Mode::gameplay,1700)&&main.minimum_sample_ns()==1600);
    assert(!main.projection_rendered(false)&&main.minimum_sample_ns()==1600);
    assert(main.projection_rendered(true)&&main.minimum_sample_ns()==0);
    assert(main.observe(Mode::pause_menu,1800)&&main.show_pause_projection()&&!main.show_panel());
    assert(main.observe(Mode::main_menu,1900)&&main.show_panel()&&!main.show_pause_projection());
    assert(main.observe(Mode::pause_menu,2000)&&main.show_pause_projection()&&!main.show_panel());
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
    // An old valid ticket never bypasses an explicit loading state, including
    // loading observed before the first projection or before a menu witness.
    StartupPanelPolicy loading_startup(true,true);
    assert(!loading_startup.observe(Mode::loading_display,4200));
    assert(!loading_startup.allow_projection()&&!loading_startup.show_panel()&&!loading_startup.show_pause_projection());
    assert(!loading_startup.projection_rendered(true)&&!loading_startup.show_transition_panel());
    assert(loading_startup.show_startup_wait(ready)&&loading_startup.minimum_sample_ns()==4200);
    loading_startup.menu_rendered();assert(!loading_startup.show_transition_panel()); // No fabricated menu witness.
    loading_startup.observe(Mode::unknown,4300);
    assert(!loading_startup.show_startup_wait(ready)&&!loading_startup.show_transition_panel());
    loading_startup.observe(Mode::gameplay,4400);
    assert(loading_startup.allow_projection()&&loading_startup.minimum_sample_ns()==4400);
    assert(!loading_startup.show_transition_panel()&&loading_startup.show_startup_wait(ready));
    auto projection_wins=ready;projection_wins.layer_count=1;
    assert(!loading_startup.show_startup_wait(projection_wins));
    assert(!loading_startup.projection_rendered(false)&&loading_startup.show_startup_wait(ready));
    assert(loading_startup.projection_rendered(true)&&!loading_startup.show_startup_wait(ready));
    loading_startup.observe(Mode::loading_display,4500);
    assert(!loading_startup.allow_projection()&&!loading_startup.show_panel()&&!loading_startup.show_pause_projection());
    assert(!loading_startup.show_transition_panel()&&!loading_startup.show_startup_wait(ready));
    StartupPanelPolicy loading_bridge(true,true);
    assert(loading_bridge.projection_rendered(true));
    loading_bridge.observe(Mode::main_menu,4600);loading_bridge.menu_rendered();
    loading_bridge.observe(Mode::loading_display,4700);
    assert(loading_bridge.show_transition_panel()&&!loading_bridge.allow_projection());
    assert(!loading_bridge.show_panel()&&!loading_bridge.show_pause_projection()&&!loading_bridge.show_startup_wait(ready));
    assert(!loading_bridge.projection_rendered(true)&&loading_bridge.minimum_sample_ns()==4700);
    loading_bridge.observe(Mode::loading_display,4800);assert(loading_bridge.minimum_sample_ns()==4700);
    // Actual packet lease expiration yields UNKNOWN and revokes the generic card.
    gears_menu::Packet packet;packet.mode=5;packet.game_pid=11;packet.game_creation=22;
    packet.owner_pid=33;packet.owner_creation=44;packet.sequence=1;packet.observed_ns=5000;
    gears_menu::Validator validator(11,22,33,44);
    loading_bridge.observe(validator.accept(packet,5000),5000);assert(loading_bridge.show_transition_panel());
    loading_bridge.observe(validator.accept(packet,1500005001),1500005001);
    assert(!loading_bridge.show_transition_panel()&&!loading_bridge.allow_projection());
    loading_bridge.observe(Mode::gameplay,1500005100);
    assert(loading_bridge.show_transition_panel()&&loading_bridge.minimum_sample_ns()==1500005100);
    assert(loading_bridge.projection_rendered(true)&&!loading_bridge.show_transition_panel());
    loading_bridge.observe(Mode::main_menu,1500005200);loading_bridge.menu_rendered();
    loading_bridge.observe(Mode::loading_display,1500005300);loading_bridge.cancel();
    assert(!loading_bridge.show_transition_panel()&&!loading_bridge.allow_projection()&&!loading_bridge.show_startup_wait(ready));
    // Acquisition wait preserves UNKNOWN's ordinary native projection choice.
    // It cannot fabricate menu recovery, move a new boundary or resurrect a card.
    StartupPanelPolicy initial_acquisition(true,true),initial_unknown(true,true);
    initial_acquisition.observe(Mode::acquisition_wait,5900);
    assert(initial_acquisition.show_panel()==initial_unknown.show_panel());
    assert(initial_acquisition.allow_projection()==initial_unknown.allow_projection());
    assert(initial_acquisition.minimum_sample_ns()==0&&!initial_acquisition.show_transition_panel());
    StartupPanelPolicy acquisition_play(true,true);
    assert(acquisition_play.projection_rendered(true));
    assert(!acquisition_play.observe(Mode::acquisition_wait,6000));
    assert(acquisition_play.allow_projection()&&acquisition_play.minimum_sample_ns()==0);
    assert(!acquisition_play.show_transition_panel()&&!acquisition_play.show_panel()&&!acquisition_play.show_pause_projection());
    acquisition_play.menu_rendered();assert(!acquisition_play.show_transition_panel());
    assert(!acquisition_play.transition_menu_requested_wait()&&!acquisition_play.transition_authorized(Mode::acquisition_wait));
    acquisition_play.observe(Mode::gameplay,6100);
    assert(acquisition_play.allow_projection()&&acquisition_play.minimum_sample_ns()==0);
    assert(!acquisition_play.projection_rendered(true)&&!acquisition_play.show_transition_panel());
    StartupPanelPolicy acquisition_no_witness(true,true);
    acquisition_no_witness.observe(Mode::pause_menu,6200);
    acquisition_no_witness.observe(Mode::acquisition_wait,6300);
    assert(!acquisition_no_witness.allow_projection()&&!acquisition_no_witness.show_transition_panel());
    assert(!acquisition_no_witness.transition_menu_requested_wait());
    StartupPanelPolicy acquisition_bridge(true,true);
    acquisition_bridge.projection_rendered(true);
    acquisition_bridge.observe(Mode::pause_menu,6400);acquisition_bridge.menu_rendered();
    acquisition_bridge.observe(Mode::acquisition_wait,6500);
    assert(acquisition_bridge.show_transition_panel()&&!acquisition_bridge.allow_projection());
    assert(acquisition_bridge.transition_authorized(Mode::acquisition_wait)&&acquisition_bridge.transition_menu_requested_wait());
    assert(!acquisition_bridge.transition_authorized(Mode::unknown));
    acquisition_bridge.observe(Mode::main_menu,6600);
    assert(acquisition_bridge.show_panel()&&acquisition_bridge.show_transition_panel()); // Caller submits WGC first.
    assert(acquisition_bridge.transition_authorized(Mode::main_menu)&&acquisition_bridge.transition_menu_requested_wait());
    acquisition_bridge.observe(Mode::gameplay,6700);
    assert(acquisition_bridge.show_transition_panel()&&!acquisition_bridge.transition_menu_requested_wait());
    acquisition_bridge.observe(Mode::loading_display,6800);
    assert(acquisition_bridge.show_transition_panel()&&!acquisition_bridge.transition_menu_requested_wait());
    packet.mode=6;packet.sequence=2;packet.observed_ns=7000;
    acquisition_bridge.observe(validator.accept(packet,7000),7000);
    assert(acquisition_bridge.show_transition_panel());
    acquisition_bridge.observe(validator.accept(packet,1500007001),1500007001);
    assert(!acquisition_bridge.show_transition_panel()&&!acquisition_bridge.transition_menu_requested_wait());
    assert(!acquisition_bridge.transition_authorized(Mode::unknown));
    acquisition_bridge.observe(Mode::main_menu,1500007100);acquisition_bridge.cancel();
    assert(!acquisition_bridge.show_transition_panel()&&!acquisition_bridge.transition_menu_requested_wait());
}
