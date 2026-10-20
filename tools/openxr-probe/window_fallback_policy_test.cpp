#include "window_fallback_policy.h"
#include "menu_capture_epoch.h"
#include <cassert>
#include <initializer_list>
#include "startup_panel_policy.h"
int main(){
    using Mode=gears_menu::Mode;
    WindowFallbackPolicy p;
    WindowFallbackPolicy::Context c{true,true,true,true,true,false,false,Mode::movie_display};
    assert(p.enter(c)&&p.active());assert(!p.enter(c));
    p.projection_completed(false);assert(p.active());
    c.native_layer=true;assert(!WindowFallbackPolicy::eligible(c));assert(!p.enter(c));
    p.projection_completed(true);assert(!p.active());c.native_layer=false;
    assert(p.enter(c));p.cancel();assert(!p.active());
    for(auto mode:{Mode::main_menu,Mode::death_menu,Mode::pause_menu,Mode::gameplay,Mode::loading_display,Mode::unknown,Mode::acquisition_wait}){
        c.mode=Mode::movie_display;assert(p.enter(c));
        c.mode=mode;p.observe(c);assert(!p.active());assert(!WindowFallbackPolicy::eligible(c));}
    c.mode=Mode::movie_display;
    for(unsigned i=0;i<7;++i){auto bad=c;
        switch(i){case 0:bad.enabled=false;break;case 1:bad.live=false;break;case 2:bad.visible=false;break;
        case 3:bad.should_render=false;break;case 4:bad.ordinary_wait=false;break;case 5:bad.native_layer=true;break;case 6:bad.other_layer=true;break;}
        assert(!WindowFallbackPolicy::eligible(bad));}
    MenuCaptureEpoch epoch;epoch.begin(100);
    assert(!epoch.accept(100,101));assert(epoch.accept(101,102));
    assert(epoch.fresh(2500101,WindowFallbackPolicy::maximum_age_100ns));
    assert(!epoch.fresh(2500102,WindowFallbackPolicy::maximum_age_100ns));
    assert(epoch.fresh(2500102)); // Menu freshness remains one second.
    epoch.begin(3000000);assert(!epoch.fresh(3000001));
    StartupPanelPolicy startup(true,true);startup.observe(Mode::movie_display,1);
    assert(!startup.allow_projection());assert(startup.allow_projection(true));
    assert(startup.show_transition_panel()); // Generic card if movie capture is unavailable.
    assert(startup.minimum_sample_ns()==1);assert(startup.projection_rendered(true,true));
    assert(startup.minimum_sample_ns()==0);
    startup.observe(Mode::pause_menu,2);assert(!startup.allow_projection(true));
}
