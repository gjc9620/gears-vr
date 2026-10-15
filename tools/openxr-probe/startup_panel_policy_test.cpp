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
}
