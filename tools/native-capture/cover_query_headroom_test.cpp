#include "cover_query_headroom.h"
#include <array>
#include <cstdio>
namespace h=gears_cover_query_headroom;
static unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
int main(){
  constexpr std::int64_t now=1000000000;constexpr std::uint64_t age=40000000,reserve=30000000;
  std::array<h::Window,3> w{{{now-25977200,age},{now-25938500,age},{now-25969800,age}}};
  auto report=h::assess(now,w.data(),w.size(),reserve);C(report.code==h::Code::defer&&report.remaining_ns==14022800&&report.limiting==0);
  C(h::assess(now+15454000,w.data(),w.size(),reserve).code==h::Code::expired);
  const auto saved=w;for(unsigned n=0;n<1000;++n){
    C(h::assess(now+n,w.data(),w.size(),reserve).code!=h::Code::ready);
    C(w[0].born_ns==saved[0].born_ns&&w[1].born_ns==saved[1].born_ns&&w[2].born_ns==saved[2].born_ns);
  }
  for(unsigned limiting=0;limiting<3;++limiting){
    for(unsigned delta=0;delta<4;++delta){
      for(auto& x:w)x={now,age};w[limiting].born_ns=now-10000000-std::int64_t(delta);
      report=h::assess(now,w.data(),w.size(),reserve);C(report.code==(delta?h::Code::defer:h::Code::ready));C(report.limiting==limiting&&report.remaining_ns==reserve-delta);
    }
    for(auto& x:w)x={now,age};w[limiting].born_ns=now-std::int64_t(age);C(h::assess(now,w.data(),w.size(),1).code==h::Code::defer);
    --w[limiting].born_ns;C(h::assess(now,w.data(),w.size(),1).code==h::Code::expired);
    w[limiting].born_ns=now+1;C(h::assess(now,w.data(),w.size(),1).code==h::Code::invalid);
    w[limiting].born_ns=0;C(h::assess(now,w.data(),w.size(),1).code==h::Code::invalid);
  }
  for(auto& x:w)x={now,age};C(h::assess(now,w.data(),w.size(),reserve).code==h::Code::ready);
  C(h::assess(now,nullptr,3,reserve).code==h::Code::invalid);C(h::assess(now,w.data(),17,reserve).code==h::Code::invalid);
  C(h::assess(now,w.data(),0,reserve).code==h::Code::invalid);C(h::assess(now,w.data(),3,0).code==h::Code::invalid);
  C(h::assess(0,w.data(),3,reserve).code==h::Code::invalid);
  h::Window huge{1,std::numeric_limits<std::uint64_t>::max()};
  report=h::assess(std::numeric_limits<std::int64_t>::max(),&huge,1,std::numeric_limits<std::uint64_t>::max());
  C(report.code==h::Code::defer&&report.remaining_ns==9223372036854775809ull);
  huge.born_ns=now;report=h::assess(now,&huge,1,1);
  C(report.code==h::Code::ready&&report.limiting_born_ns==now&&report.remaining_ns==std::numeric_limits<std::uint64_t>::max());
  // A ready admission never overrides a later expiration on the same sample.
  h::Window fresh{now,age};C(h::assess(now,&fresh,1,reserve).code==h::Code::ready);
  C(h::assess(now+age+1,&fresh,1,1).code==h::Code::expired);
  printf("QueryHeadroom checks=%u failures=%u immutable time arithmetic only; no source/query authority\n",checks,failures);return failures?1:0;
}
