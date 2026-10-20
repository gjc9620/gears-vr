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
        switch(field){case 0:++bad.magic;break;case 1:++bad.version;break;case 2:--bad.size;break;case 3:bad.mode=7;break;
            case 4:++bad.game_pid;break;case 5:++bad.game_creation;break;case 6:++bad.owner_pid;break;case 7:++bad.owner_creation;break;}
        assert(v.accept(bad,101)==Mode::unknown);
    }
    ++p.sequence;p.mode=0;assert(v.accept(p,101)==Mode::unknown);
    ++p.sequence;p.mode=3;p.observed_ns=200;assert(v.accept(p,200)==Mode::pause_menu);
    bad=p;bad.mode=1;assert(v.accept(bad,201)==Mode::unknown);
    assert(v.accept(p,1500000201)==Mode::unknown);
    ++p.sequence;p.mode=1;p.observed_ns=300;assert(v.accept(p,300)==Mode::gameplay);
    static_assert(static_cast<std::uint32_t>(Mode::main_menu)==4);
    ++p.sequence;p.mode=4;p.observed_ns=400;assert(v.accept(p,400)==Mode::main_menu);
    assert(v.accept(p,500)==Mode::main_menu); // Identical live heartbeat.
    assert(v.accept(p,1500000401)==Mode::unknown); // Expiration revokes display.
    assert(v.accept(p,399)==Mode::unknown);
    bad=p;bad.mode=3;assert(v.accept(bad,500)==Mode::unknown);
    bad=p;++bad.sequence;bad.mode=0xffffffffu;assert(v.accept(bad,500)==Mode::unknown);
    bad=p;++bad.sequence;++bad.game_creation;assert(v.accept(bad,500)==Mode::unknown);
    ++p.sequence;p.mode=0;p.observed_ns=500;assert(v.accept(p,500)==Mode::unknown);
    bad=p;--bad.sequence;bad.mode=4;assert(v.accept(bad,500)==Mode::unknown);
    ++p.sequence;p.mode=1;p.observed_ns=600;assert(v.accept(p,600)==Mode::gameplay);
    static_assert(static_cast<std::uint32_t>(Mode::loading_display)==5&&sizeof(Packet)==64);
    ++p.sequence;p.mode=5;p.observed_ns=700;assert(v.accept(p,700)==Mode::loading_display);
    assert(v.accept(p,1500000700)==Mode::loading_display);
    assert(v.accept(p,1500000701)==Mode::unknown); // Loading authority expires normally.
    bad=p;bad.mode=1;assert(v.accept(bad,701)==Mode::unknown);
    ++p.sequence;p.mode=1;p.observed_ns=800;assert(v.accept(p,800)==Mode::gameplay);
    static_assert(static_cast<std::uint32_t>(Mode::acquisition_wait)==6&&sizeof(Packet)==64);
    ++p.sequence;p.mode=6;p.observed_ns=900;assert(v.accept(p,900)==Mode::acquisition_wait);
    assert(v.accept(p,1500000900)==Mode::acquisition_wait);
    assert(v.accept(p,1500000901)==Mode::unknown);
    bad=p;bad.mode=4;assert(v.accept(bad,901)==Mode::unknown);
    ++p.sequence;p.mode=1;p.observed_ns=1000;assert(v.accept(p,1000)==Mode::gameplay);
    Validator movie(11,22,33,44,true);
    ++p.sequence;p.mode=7;p.observed_ns=2000;
    assert(v.accept(p,2000)==Mode::unknown);
    assert(movie.accept(p,2000)==Mode::movie_display);
    assert(movie.accept(p,250002000)==Mode::movie_display);
    assert(movie.accept(p,250002001)==Mode::unknown);
    ++p.sequence;p.mode=8;assert(movie.accept(p,2000)==Mode::unknown);
}
