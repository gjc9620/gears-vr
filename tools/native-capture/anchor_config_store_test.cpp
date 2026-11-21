#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cstdio>
#include <type_traits>
#include "anchor_config_store.h"
unsigned checks;
#define C(x) do{++checks;if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){gears_anchor_native::ConfigStore<3> s;static_assert(std::is_const_v<std::remove_reference_t<decltype(s.view())>>);gears_anchor::Config a{},b{};a.identity_count=5;b=a;b.controller_pawn=77;
 C(!s.revision());C(s.publish(a)&&s.revision()==1);C(s.publish(a)&&s.revision()==1);
 C(s.publish(b)&&s.revision()==2&&s.view().controller_pawn==77);
 C(s.publish(a)&&s.revision()==3&&s.view().controller_pawn==0);
 C(s.preflight(a)&&s.publish(a)&&s.revision()==3);
 C(!s.preflight(b)&&s.revision()==3);C(!s.publish(a)&&!s.publish(b)&&s.revision()==3);
 gears_anchor_native::ConfigStore<0> zero;C(!zero.preflight(a)&&!zero.publish(a)&&!zero.revision());
 C(s.view().identity_count==5&&s.view().controller_pawn==0); // Failed preflight did not write config.
 printf("AnchorConfigStore: %u checks 0 failures; bounded-limit actual overflow algorithm\n",checks);return 0;}
