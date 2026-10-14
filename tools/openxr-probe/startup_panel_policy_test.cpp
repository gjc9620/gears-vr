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
}
