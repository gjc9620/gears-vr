#include "weapon_lancer_refire_loop.h"
#include <cstdio>
#include <vector>
#include <cstdlib>
namespace r=gears_lancer_refire_loop;
unsigned checks{};
void require(bool value){++checks;if(!value){std::fprintf(stderr,"refire loop check%u failed\n",checks);std::exit(1);}}
template<class T>void append(std::vector<unsigned char>& bytes,T const& value){auto p=reinterpret_cast<unsigned char const*>(&value);bytes.insert(bytes.end(),p,p+sizeof value);}
int main(){
    const r::Name interval{101,0},refire{202,0};const std::uint64_t property=0x12345678;
    std::vector<unsigned char> expression{0x61,0x18,0x1b};append(expression,interval);
    expression.push_back(0x00);append(expression,property);expression.insert(expression.end(),{0x16,0x27,0x21});append(expression,refire);
    expression.push_back(0x16);
    require(r::looping_timer(expression.data(),expression.size(),interval,property,refire));
    auto optional=expression;optional.insert(optional.end()-1,0x4a);
    require(r::looping_timer(optional.data(),optional.size(),interval,property,refire));
    auto retail=expression;retail.insert(retail.end()-1,0x50);
    require(r::looping_timer(retail.data(),retail.size(),interval,property,refire));
    // Full immutable copy of the actual original script in session18592.
    const unsigned char observed[]={0x07,0x3a,0x00,0x81,0x1c,0x80,0x3e,0xa3,0x21,0,0,0,0,0x21,0x4a,0x51,0,0,0,0,0,0,0x50,0x16,0x16,
        0x61,0x18,0x1b,0xcd,0x3d,0,0,0,0,0,0,0,0xe0,0x89,0xed,0x35,0,0,0,0,0x16,0x27,0x21,0x4a,0x51,0,0,0,0,0,0,0x50,0x16,0x04,0x0b,0x53};
    require(r::looping_timer(observed,sizeof observed,{0x3dcd,0},0x35ed89e0,{0x514a,0}));
    for(auto tail:{0x17,0x27,0x28,0x49,0x51,0x53}){auto bad=retail;bad[bad.size()-2]=static_cast<unsigned char>(tail);
        require(!r::looping_timer(bad.data(),bad.size(),interval,property,refire));}
    auto framed=expression;framed.insert(framed.begin(),{0x07,0x3a,0x00});framed.insert(framed.end(),{0x04,0x0b,0x53});
    require(r::looping_timer(framed.data(),framed.size(),interval,property,refire));
    for(auto offset:{0u,1u,2u,11u,20u,21u,22u,31u}){auto bad=expression;bad[offset]^=1;
        require(!r::looping_timer(bad.data(),bad.size(),interval,property,refire));}
    for(std::size_t size=0;size<expression.size();++size)
        require(!r::looping_timer(expression.data(),size,interval,property,refire));
    auto duplicate=expression;duplicate.insert(duplicate.end(),expression.begin(),expression.end());
    require(!r::looping_timer(duplicate.data(),duplicate.size(),interval,property,refire));
    require(!r::looping_timer(nullptr,0,interval,property,refire));
    require(!r::looping_timer(expression.data(),257,interval,property,refire));
    require(!r::looping_timer(expression.data(),expression.size(),r::Name{101,1},property,refire));
    require(!r::looping_timer(expression.data(),expression.size(),interval,property+1,refire));
    require(!r::looping_timer(expression.data(),expression.size(),interval,property,r::Name{203,0}));
    std::vector<unsigned char> first{0x1b};append(first,r::Name{303,0});first.push_back(0x16);first.push_back(0x1b);
    append(first,r::Name{404,0});first.push_back(0x01);append(first,property);first.insert(first.end(),{0x16,0x04,0x0b,0x53});
    require(r::first_shot(first.data(),first.size(),{303,0},{404,0},property));
    for(std::size_t i=0;i<first.size();++i){auto bad=first;bad[i]^=1;
        require(!r::first_shot(bad.data(),bad.size(),{303,0},{404,0},property));}
    for(std::size_t n=0;n<first.size();++n)require(!r::first_shot(first.data(),n,{303,0},{404,0},property));
    auto extra=first;extra.push_back(0x53);require(!r::first_shot(extra.data(),extra.size(),{303,0},{404,0},property));
    require(!r::first_shot(nullptr,first.size(),{303,0},{404,0},property));
    require(!r::first_shot(first.data(),first.size(),{303,1},{404,0},property));
    require(!r::first_shot(first.data(),first.size(),{303,0},{404,1},property));
    r::DeferredEvent event;constexpr std::int64_t begin=1000000000;
    require(event.empty()&&!event.claim(begin)&&!event.release_required(begin));
    require(!event.capture(r::EventKind::none,begin)&&!event.capture(r::EventKind::first,0));
    require(event.capture(r::EventKind::first,begin)&&event.first_debt());
    require(event.capture(r::EventKind::first,begin+200000000)); // no renewed deadline
    require(!event.capture(r::EventKind::refire,begin+200000000));
    require(event.dispatchable(begin+250000000));
    require(!event.dispatchable(begin+250000001)&&event.release_required(begin+250000001));
    require(!event.claim(begin+250000001));event.cancel();
    require(event.first_debt()&&!event.claim(begin+1)); // cancellation cannot resurrect old source
    event.reset();require(event.capture(r::EventKind::first,begin));
    require(!event.dispatchable(begin-1)&&event.release_required(begin-1));
    require(event.claim(begin+1)&&event.empty()&&!event.claim(begin+2)); // exact once
    require(event.capture(r::EventKind::refire,begin));event.cancel();require(event.empty()&&!event.first_debt());
    require(event.capture(r::EventKind::refire,begin));
    require(!event.capture(r::EventKind::refire,begin+250000001));
    require(event.release_required(begin+250000001));event.reset();
    require(event.capture(r::EventKind::first,begin));event.cancel();
    require(!event.capture(r::EventKind::first,begin+1)&&event.first_debt());
    std::printf("LancerRefireLoopTest: %u checks, original first-shot body, looping timer and bounded once-only deferral; no Game/VM calls\n",checks);
}
