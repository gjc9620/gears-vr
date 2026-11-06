#include "physical_window_pixels.h"
#include <cassert>
#include <cstdio>
namespace {
struct Api {
    using Context=int;
    static inline int context=1,restored{},origin_calls{};
    static inline bool entry_ok=true,rect_ok=true,origin_ok=true,restore_ok=true;
    static Context enter() noexcept {if(!entry_ok)return 0;const auto old=context;context=2;return old;}
    static bool restore(Context old) noexcept {++restored;if(!restore_ok)return false;context=old;return true;}
    static bool client(HWND,RECT& out) noexcept {assert(context==2);out={0,0,3840,2160};return rect_ok;}
    static bool origin(HWND,POINT& out) noexcept {assert(context==2);++origin_calls;out={-1034,-2160};return origin_ok;}
    static void reset(){context=1;restored=origin_calls=0;entry_ok=rect_ok=origin_ok=restore_ok=true;}
};
}
int main(){
    RECT rect{1,2,3,4};POINT origin{5,6};
    assert(physical_window_pixels::client<Api>(nullptr,rect,&origin));
    assert(rect.right==3840&&rect.bottom==2160&&origin.x==-1034&&origin.y==-2160);
    assert(Api::context==1&&Api::restored==1&&Api::origin_calls==1);
    Api::reset();Api::rect_ok=false;rect={1,2,3,4};
    assert(!physical_window_pixels::client<Api>(nullptr,rect,&origin)&&Api::context==1&&Api::restored==1&&rect.right==3);
    assert(Api::origin_calls==0);
    Api::reset();Api::origin_ok=false;
    assert(!physical_window_pixels::client<Api>(nullptr,rect,&origin)&&Api::context==1&&Api::restored==1);
    Api::reset();Api::entry_ok=false;
    assert(!physical_window_pixels::client<Api>(nullptr,rect)&&Api::restored==0&&Api::context==1);
    Api::reset();Api::restore_ok=false;
    assert(!physical_window_pixels::client<Api>(nullptr,rect)&&Api::restored==1&&rect.right==3);
    Api::reset();assert(physical_window_pixels::client<Api>(nullptr,rect)&&Api::context==1&&Api::origin_calls==0);
    std::puts("Physical client/origin units, negative monitor, context restoration and failed-read rejection passed; no desktop window touched.");
}
