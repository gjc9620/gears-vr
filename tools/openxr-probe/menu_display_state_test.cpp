#include "../vr-common/menu_display_state.h"
#include <cassert>
#include <cstddef>
int main(){
    using namespace gears_menu;
    static_assert(offsetof(Packet,game_pid)==16&&offsetof(Packet,observed_ns)==56);
    Packet p;p.game_pid=11;p.game_creation=22;p.owner_pid=33;p.owner_creation=44;p.sequence=1;p.observed_ns=100;p.mode=2;
    Validator v(11,22,33,44);
    assert(v.accept(p,100)==Mode::death_menu&&v.accept(p,100)==Mode::death_menu);
    assert(v.accept(p,1500000100)==Mode::death_menu&&v.accept(p,1500000101)==Mode::unknown);
    assert(v.accept(p,99)==Mode::unknown);
    auto bad=p;bad.mode=1;assert(v.accept(bad,100)==Mode::unknown); // Same sequence cannot change semantics.
    ++p.sequence;p.mode=1;assert(v.accept(p,101)==Mode::gameplay);
    bad=p;--bad.sequence;assert(v.accept(bad,101)==Mode::unknown);
    for(unsigned field=0;field<8;++field){bad=p;++bad.sequence;
        switch(field){case 0:++bad.magic;break;case 1:++bad.version;break;case 2:--bad.size;break;case 3:bad.mode=3;break;
            case 4:++bad.game_pid;break;case 5:++bad.game_creation;break;case 6:++bad.owner_pid;break;case 7:++bad.owner_creation;break;}
        assert(v.accept(bad,101)==Mode::unknown);
    }
    ++p.sequence;p.mode=0;assert(v.accept(p,101)==Mode::unknown);
}
