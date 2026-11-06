#include "weapon_equipment_names.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace gears_equipment_names;
unsigned checks{};
void need(bool value){++checks;if(!value){std::fprintf(stderr,"failed check %u\n",checks);std::exit(1);}}
int main(){
    { Lookup hb({L"b_HB_Root",L"b_HB_MagLatch",L"b_HB_Mag"});
      Table names{1,1000,1024};std::array<unsigned,3> found{};
      const wchar_t* labels[]={L"b_HB_Root",L"b_HB_MagLatch",L"b_HB_Mag"};
      auto table_read=[&](Table& out){out=names;return true;};
      auto name_read=[&](Table,unsigned index,std::array<wchar_t,32>& text){
          text.fill(0);if(index>=10&&index<=12)std::copy_n(labels[index-10],std::wcslen(labels[index-10])+1,text.begin());return true;};
      need(hb.step(table_read,name_read,[](){return true;},[](){return true;},found)==State::ready);
      need(found==std::array<unsigned,3>{10,11,12});
      hb.reset();++names.data;
      need(hb.step(table_read,name_read,[](){return true;},[](){return true;},found)==State::ready);
      need(found==std::array<unsigned,3>{10,11,12}); }
    Lookup lookup;Table table{1,60000,65536};std::array<unsigned,3> output{};
    std::array<unsigned,3> actual{34001,47002,55003};unsigned reads{},calls{};
    bool fresh=true,mutate=false;unsigned expire_at=~0u;
    auto read_table=[&](Table& out){out=table;if(mutate&&++calls==2)++out.data;return true;};
    auto read_name=[&](Table,unsigned i,std::array<wchar_t,32>& text){
        ++reads;if(reads==expire_at)fresh=false;
        // Real FName allocations need not zero the bytes after their NUL.
        // A valid short name followed by nonzero padding must still match;
        // an unterminated unrelated entry must never reach wcscmp.
        text.fill(L'Z');
        for(unsigned n=0;n<3;++n)if(i==actual[n])std::copy_n(Lookup::wanted[n],std::wcslen(Lookup::wanted[n])+1,text.begin());
        return true;
    };
    auto current=[&](){return fresh;};auto budget=[](){return true;};
    // Identity checks may themselves exceed the scan budget. Start that
    // budget at its first loop check, and retain both outer freshness checks.
    Lookup timed;std::uint64_t simulated_ns{},scan_started{};unsigned identity_checks{};
    auto costly_current=[&](){++identity_checks;simulated_ns+=5000000;return true;};
    auto timed_read=[&](Table t,unsigned i,std::array<wchar_t,32>& text){simulated_ns+=1000;return read_name(t,i,text);};
    auto scan_budget=[&](){if(!scan_started)scan_started=simulated_ns;return simulated_ns-scan_started<3000000;};
    need(timed.step(read_table,timed_read,costly_current,scan_budget,output)==State::pending);
    need(timed.cursor()==512&&identity_checks==2&&simulated_ns>3000000);
    // A cold table far larger than one frame must progress without using an
    // expired frame or dispatching a bone getter while preparation is pending.
    State state=State::pending;unsigned steps{};
    while(state==State::pending){reads=0;state=lookup.step(read_table,read_name,current,budget,output);
        need(reads<=512);++steps;if(state==State::pending)need(output==std::array<unsigned,3>{});}
    need(state==State::ready&&output==actual&&steps>100);
    reads=0;need(lookup.step(read_table,read_name,current,budget,output)==State::ready&&reads==3);
    // Name indices cannot survive table replacement without rediscovery.
    ++table.data;reads=0;need(lookup.step(read_table,read_name,current,budget,output)==State::pending);
    need(lookup.cursor()==512&&output==std::array<unsigned,3>{});
    const auto before=lookup.cursor();reads=0;expire_at=17;
    need(lookup.step(read_table,read_name,current,budget,output)==State::expired);
    need(lookup.cursor()==before&&output==std::array<unsigned,3>{});
    fresh=true;expire_at=~0u;reads=0;calls=0;mutate=true;
    need(lookup.step(read_table,read_name,current,budget,output)==State::pending&&lookup.cursor()==0);
    mutate=false;reads=0;unsigned budget_calls{};
    need(lookup.step(read_table,read_name,current,[&](){return ++budget_calls<2;},output)==State::pending);
    need(reads==16&&lookup.cursor()==16);
    // Cached name content is re-read, not trusted merely because its table is unchanged.
    actual={20,30,40};lookup.reset();reads=0;
    need(lookup.step(read_table,read_name,current,budget,output)==State::ready&&output==actual);
    actual={1200,1300,1400};reads=0;
    need(lookup.step(read_table,read_name,current,budget,output)==State::pending);
    need(output==std::array<unsigned,3>{});
    table.count=1000;lookup.reset();
    need(lookup.step(read_table,read_name,current,budget,output)==State::pending);
    need(lookup.step(read_table,read_name,current,budget,output)==State::missing);
    table.capacity=999;
    need(lookup.step(read_table,read_name,current,budget,output)==State::invalid&&lookup.cursor()==0);
    fresh=false;reads=0;
    need(lookup.step(read_table,read_name,current,budget,output)==State::expired&&reads==0);
    std::printf("bounded global-name lookup: %u checks passed\n",checks);
}
