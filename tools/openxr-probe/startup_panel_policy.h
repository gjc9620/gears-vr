#pragma once
// Display ownership changes only after a real ticketed projection was rendered.
class StartupPanelPolicy {
    bool enabled_, handed_off_=false, cancelled_=false;
public:
    explicit StartupPanelPolicy(bool enabled):enabled_(enabled){}
    bool show_panel() const {return enabled_&&!handed_off_&&!cancelled_;}
    bool projection_rendered(bool success) {
        if(!enabled_||cancelled_||handed_off_||!success)return false;
        handed_off_=true;return true;
    }
    void cancel(){cancelled_=true;}
};
