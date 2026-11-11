#include "body_source_age_observation.h"
#include <cstdio>
#include <thread>
namespace age=gears_body_source_age_observation;
unsigned checks{},failures{};
#define C(v) do{++checks;if(!(v)){++failures;std::printf("FAIL %d %s\n",__LINE__,#v);}}while(false)
int main(){
    const age::Key key{11,22,33,44,55};age::Report report{};
    age::Row row{};row.world=11;row.pawn=22;row.scene=33;row.thread=55;row.tick=99;row.now=1000;
    row.metadata_begin=100;row.metadata_end=110;row.tracking_begin=120;row.tracking_end=130;
    row.stereo_received=80;row.controller_received=90;row.sync=70;row.capture_begin=140;
    C(!age::armed());age::note(0,row);C(!report.rows[0].observed);
    {age::Scope disabled(false,key,report);C(!age::armed());}
    for(unsigned field=0;field<5;++field){auto invalid=key;
        if(field==0)invalid.world=0;if(field==1)invalid.pawn=0;if(field==2)invalid.scene=0;if(field==3)invalid.compose=0;if(field==4)invalid.thread=0;
        age::Scope refused(true,invalid,report);C(!age::armed());}
    {age::Scope scope(true,key,report);C(age::armed());
        for(unsigned field=0;field<4;++field){auto other=row;
            if(field==0)other.world++;if(field==1)other.pawn++;if(field==2)other.scene++;if(field==3)other.thread++;
            age::note(0,other);C(!report.calls[0]);}
        bool other_armed=true;std::thread worker([&](){other_armed=age::armed();age::note(0,row);});worker.join();C(!other_armed&&!report.calls[0]);
        age::note(2,row);C(!report.calls[0]&&!report.calls[1]);
        age::note(0,row);C(report.calls[0]==1&&report.rows[0].observed&&report.rows[0].now==1000&&!report.rows[2].observed);
        age::note(1,row);C(report.calls[1]==1&&report.rows[1].capture_begin==140);
        row.now=2000;row.verdict=1;age::note(0,row);age::note(1,row);
        C(report.rows[0].now==1000&&report.rows[1].now==1000&&report.rows[2].now==2000&&report.rows[3].now==2000);
        C(report.rows[2].metadata_begin==100&&report.rows[3].sync==70&&report.rows[3].capture_begin==140);
        row.now=3000;row.latched=true;age::note(0,row);C(report.rows[2].now==2000&&!report.rows[2].latched);
        age::Report nested{};{age::Scope inner_scope(true,key,nested);age::note(0,row);C(nested.rows[0].now==3000&&report.rows[0].now==1000);}
        age::note(1,row);C(report.calls[1]==3);
        report.calls[0]=std::numeric_limits<unsigned>::max();age::note(0,row);C(report.overflow&&report.calls[0]==std::numeric_limits<unsigned>::max());
    }
    C(!age::armed());
    {age::Scope fresh(true,key,report);C(!report.rows[0].observed&&!report.calls[0]&&!report.overflow);}
    C(!age::armed());
    std::printf("BodySourceAgeObservation checks=%u failures=%u copied values only; no clock/game/input\n",checks,failures);
    return failures?1:0;
}
