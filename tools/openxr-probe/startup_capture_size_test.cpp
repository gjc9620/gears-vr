#include "startup_capture_size.h"
#include <cassert>
#include <limits>
int main(){
    unsigned width{},height{};
    // Loading window, fullscreen, then restored window remain bounded while
    // preserving their aspect ratio; transient empty/minimized frames defer.
    assert(startup_capture_size::fit(640,360,width,height)&&width==640&&height==360);
    assert(startup_capture_size::fit(3840,2160,width,height)&&width==1920&&height==1080);
    assert(startup_capture_size::fit(1280,720,width,height)&&width==1280&&height==720);
    assert(startup_capture_size::fit(1080,1920,width,height)&&height==1920);
    assert(!startup_capture_size::fit(0,720,width,height));
    assert(!startup_capture_size::fit(1280,0,width,height));
    assert(!startup_capture_size::fit(1,2161,width,height));
    auto maximum=(std::numeric_limits<unsigned>::max)();
    assert(startup_capture_size::fit(maximum,maximum,width,height)&&width==1920&&height==1920);
}
