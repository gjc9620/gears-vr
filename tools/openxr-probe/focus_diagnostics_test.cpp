#include "focus_diagnostics.h"
#include <cassert>
#include <string>
int main(){
    FILE* out{};assert(tmpfile_s(&out)==0&&out);gears_focus::Diagnostics d(out);
    d.capabilities(XR_SUCCESS,false,false,false,"extension_enumeration");
    d.event(XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED,true,XR_SESSION_STATE_VISIBLE,123);
    d.event(XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT,true,-1,0,1);
    assert(d.presence==-1&&d.state==XR_SESSION_STATE_VISIBLE);
    d.capabilities(XR_SUCCESS,true,true,true,"system_properties");
    d.event(XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT,false,-1,0,1);
    assert(d.presence==-1);
    d.event(XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT,true,-1,0,1);
    assert(d.presence==1&&d.state==XR_SESSION_STATE_VISIBLE); // Present != focused.
    d.snapshot(1000,true);d.snapshot(2000,true);
    assert(d.frames==2&&d.focused_frames==0&&d.state_frames[4]==2);
    d.event(XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED,false,XR_SESSION_STATE_FOCUSED);
    assert(d.state==XR_SESSION_STATE_VISIBLE);
    d.event(XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED,true,XR_SESSION_STATE_FOCUSED);
    d.snapshot(1000001000,true);assert(d.focused_frames==1);
    d.event(XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT,true,-1,0,0);
    assert(d.presence==0&&d.state==XR_SESSION_STATE_FOCUSED); // Focus != present.
    for(unsigned i=0;i<300;++i)d.event(XR_TYPE_EVENT_DATA_EVENTS_LOST,true,-1,0,-1,2);
    assert(d.omitted>0);d.snapshot(121000001000LL,true);d.snapshot(122000001000LL,true,true);
    rewind(out);std::string text;char buffer[2048];while(fgets(buffer,sizeof buffer,out))text+=buffer;
    assert(text.find("\"final\":true")!=std::string::npos);
    assert(text.find("\"presence_state\":-1")!=std::string::npos);
    assert(text.find("\"session_matches\":false")!=std::string::npos);
    assert(text.find("\"events_lost\":2")!=std::string::npos);
    assert(text.size()<150000);fclose(out);
    puts("Focus diagnostics: unsupported/missing/mismatched presence, independent focus, budgets, final receipt passed");
}
