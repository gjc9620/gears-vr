#include "../vr-common/first_person_display_policy.h"
#include <cstdio>
#include <initializer_list>
int main() {
    using gears_menu::Mode;
    unsigned checks{},failures{};
    auto check=[&](bool ok){++checks;if(!ok)++failures;};
    for(unsigned flags=0;flags<8;++flags)for(unsigned mode=0;mode<8;++mode){
        const auto m=static_cast<Mode>(mode);
        check(gears_first_person_display::waiting_fallback(
            (flags&1)!=0,(flags&2)!=0,(flags&4)!=0,m)==
            (flags==7 && (m==Mode::unknown || m==Mode::gameplay)));
    }
    gears_menu::Validator validator(1,2,3,4,true);
    gears_menu::Packet packet{};
    packet.game_pid=1;packet.game_creation=2;packet.owner_pid=3;
    packet.owner_creation=4;packet.sequence=1;packet.observed_ns=100;
    packet.mode=static_cast<unsigned>(Mode::gameplay);
    check(validator.accept(packet,100)==Mode::gameplay);
    const auto expired=validator.accept(packet,1500000101);
    check(expired==Mode::unknown);
    check(gears_first_person_display::waiting_fallback(true,true,true,expired));
    check(packet.observed_ns==100); // No renewed display or tracking authority.
    for(auto mode:{Mode::main_menu,Mode::pause_menu,Mode::death_menu,
                   Mode::loading_display,Mode::acquisition_wait,Mode::movie_display})
        check(!gears_first_person_display::waiting_fallback(true,true,true,mode));
    std::printf("first_person_display_policy: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
