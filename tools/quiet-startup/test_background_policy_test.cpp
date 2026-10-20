#include "test_background_policy.h"
#ifdef NDEBUG
#error BackgroundPolicyTest requires assertions in every configuration
#endif
#include <cassert>
#include <vector>
int main() {
    using namespace gears_quiet_test;
    for(int command:{SW_SHOWNORMAL,SW_SHOWMAXIMIZED,SW_SHOW,SW_RESTORE,SW_SHOWDEFAULT,SW_SHOWNOACTIVATE}) {
        assert(show_command(command,true)==SW_SHOWNA);
        assert(show_command(command,false)==SW_SHOWNOACTIVATE);
    }
    assert(show_command(SW_HIDE,true)==SW_HIDE);
    assert(show_command(SW_SHOWMINIMIZED,true)==SW_SHOWMINNOACTIVE);
    assert(startup_show(false,false)==SW_SHOWNOACTIVATE);
    assert(startup_show(false,true)==SW_SHOWNA);
    assert(startup_show(true,true)==SW_SHOWMINNOACTIVE);
    const auto previous=reinterpret_cast<HWND>(42);
    assert(insert_after(previous,false)==previous&&insert_after(previous,true)==HWND_BOTTOM);
    const UINT original=SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW;
    const UINT flags=position_flags(original,true);
    assert((flags&SWP_NOZORDER)==0&&(flags&SWP_NOACTIVATE)&&(flags&SWP_NOOWNERZORDER));
    assert((flags&(SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW))==(original&~SWP_NOZORDER));
    assert(position_flags(original,false)==(original|SWP_NOACTIVATE));
    std::vector<int> order;
    assert(first_show(true,[&]{order.push_back(1);return true;},[&](int c){assert(c==SW_SHOWNA);order.push_back(2);}));
    assert((order==std::vector<int>{1}));
    order.clear();assert(!first_show(true,[&]{order.push_back(1);return false;},[&](int){order.push_back(2);}));
    assert((order==std::vector<int>{1}));
    order.clear();assert(first_show(false,[&]{order.push_back(1);return true;},[&](int c){assert(c==SW_SHOWNOACTIVATE);order.push_back(2);}));
    assert((order==std::vector<int>{2}));
    const UINT show_flags=SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW;
    assert(behind_show_flags()==show_flags);
    assert(behind_show_flags(true)==(show_flags|SWP_ASYNCWINDOWPOS));
}
