#include "death_hdr_match.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace gears_death_hdr_match;
    const Snapshot target{magic,version,packet_bytes,10,2,201,901};
    assert(stable_valid(target,target,10)&&!stable_valid(target,target,11));
    auto unstable=target;++unstable.copy_seq;assert(!stable_valid(target,unstable,10));
    for(unsigned field=0;field<7;++field){auto bad=target;
        switch(field){case 0:++bad.magic;break;case 1:++bad.version;break;case 2:++bad.size;break;case 3:bad.game_pid=0;break;
        case 4:bad.world=0;break;case 5:bad.copy_seq=0;break;case 6:bad.ticket=0;break;}
        assert(!stable_valid(bad,bad,10));}
    Selector selector;assert(selector.decide(901,2,true,true)==Decision::waiting);
    assert(selector.select(target,target,10));assert(!selector.select(unstable,unstable,10)&&selector.selected==target);
    assert(selector.decide(900,2,true,true)==Decision::before&&!selector.finished);
    assert(selector.decide(901,2,true,true)==Decision::matched&&selector.finished);
    assert(selector.decide(901,2,true,true)==Decision::inactive);
    for(unsigned mode=0;mode<3;++mode){Selector s;assert(s.select(target,target,10));
        const auto d=s.decide(mode==0?902:901,mode==1?3:2,true,mode!=2);
        assert(d==(mode==0?Decision::missed_after:mode==1?Decision::missed_world:Decision::missed_capacity));
        assert(s.decide(901,2,true,true)==Decision::inactive);}
    Selector no_world;assert(no_world.select(target,target,10));assert(no_world.decide(901,0,false,true)==Decision::matched);
    Selector required_world;assert(required_world.select(target,target,10));assert(required_world.decide(901,0,true,true)==Decision::missed_world);
#ifdef _WIN32
    Publisher publisher;const auto pid=GetCurrentProcessId();assert(publisher.create(pid));
    Publisher duplicate;assert(!duplicate.create(pid));Reader reader;Snapshot a{},b{};
    assert(reader.read(pid,a,b)&&!stable_valid(a,b,pid));
    assert(publisher.publish(901,2,201)&&!publisher.publish(902,2,202));
    assert(reader.read(pid,a,b)&&stable_valid(a,b,pid)&&a.ticket==901&&a.copy_seq==201);
    reader.close();publisher.close(); // Mapping handles have no GPU lifetime dependency.
#endif
    std::puts("DeathHdrMatchTest passed");
}
