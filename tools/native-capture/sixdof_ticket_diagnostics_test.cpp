#include "../vr-common/sixdof_ticket_mailbox.h"
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
void require(bool value,const char* label){if(!value){std::cerr<<label<<"\n";std::exit(1);}}
using namespace gears_sixdof;
NativeRow row(std::uint64_t ticket=1){
    NativeRow r;r.key={41,42,43,44,ticket};r.reference_generation=7;r.space_epoch=3;r.sequence=ticket*2;r.units_per_metre=100;
    r.reference.position={.1,1.7,-.2};r.head.position={.4,1.7,-.2};r.source_view[0]=17;
    r.expected[0][12]=11;r.expected[1][12]=12;return r;
}
Stage stage(unsigned number){Stage s;s.qpc=100+number;s.count=2;s.reads=3;
    s.actual[0][12]=float(20+number);s.actual[1][12]=float(30+number);return s;}
void retention(){
    NativeRecorder recorder;auto r=row();
    require(recorder.observe(r,0,stage(0)),"initial observation");
    require(recorder.pin(0,r.key,2,99)==1,"pin exact");
    for(unsigned i=1;i<4;++i)require(recorder.observe(r,i,stage(i)),"later stage");
    auto pinned=recorder.pinned(0);require(pinned&&pinned->mask==15,"pinned row gains later stages");
    require(pinned->source_view[0]==17&&pinned->reference_generation==7&&pinned->head.position.x==.4,"metadata frozen");
    require(pinned->stages[3].actual[0][12]==23&&pinned->stages[3].actual[1][12]==33,"independent actual both eyes preserved");
    require(pinned->stages[3].actual[0][12]!=pinned->expected[0][12],"actual not copied from expected");
    auto bad=r.key;bad.session++;
    require(recorder.pin(0,bad,3,100)==3&&same(recorder.pinned(0)->key,r.key),"unknown cannot overwrite pin");
    require(recorder.pin(1,bad,3,100)==2&&!recorder.pinned(1),"session mismatch refused");
    bad=r.key;bad.world++;require(recorder.pin(2,bad,3,100)==2,"world mismatch refused");
    bad=r.key;bad.start++;require(recorder.pin(3,bad,3,100)==2,"process start mismatch refused");
    bad=r.key;bad.pid++;require(recorder.pin(4,bad,3,100)==2,"pid mismatch refused");
    for(unsigned i=2;i<NativeRecorder::recent_capacity+20;++i)recorder.observe(row(i),0,stage(0));
    require(recorder.pinned(0)->stages[3].actual[1][12]==33,"pin survives rolling eviction");
    require(recorder.pin(5,r.key,4,101)==2,"evicted old ticket explicitly missing");
    auto late=row(NativeRecorder::recent_capacity+19);
    require(recorder.pin(6,late.key,5,102)==1,"late-session exact ticket retained");
    auto changed=stage(0);changed.actual[0][12]=999;recorder.observe(late,0,changed);
    require(recorder.pinned(6)->stages[0].actual[0][12]==20,"duplicate does not overwrite first actual");
    FILE* out{};require(tmpfile_s(&out)==0&&out!=nullptr,"tmpfile");recorder.dump(out);std::rewind(out);
    char text[32768]{};auto size=std::fread(text,1,sizeof(text)-1,out);std::fclose(out);
    require(size>0&&std::strstr(text,"prepare_after")&&std::strstr(text,"\"actual\":[")&&std::strstr(text,"\"stage_mask\":15"),"bounded native output contains observations");
}
void submissions(){
    SubmittedRecorder recorder;SubmittedRow request{};request.key=row().key;request.snapshot_index=6;request.source_serial=9;
    require(recorder.request(request),"XR request");auto actual=request;actual.eyes[0].position.x=.3125;actual.eyes[1].position.x=.375;
    actual.fov[1].right=.8125;actual.space_epoch=5;actual.local=true;actual.ns=123;actual.display_time=456;
    recorder.observe(actual);require(!recorder.row(0)->success,"failed end frame never recorded as submitted");
    actual.success=true;auto wrong=actual;wrong.key.world++;recorder.observe(wrong);
    require(!recorder.row(0)->success,"other world not associated");
    wrong=actual;wrong.source_serial++;recorder.observe(wrong);require(!recorder.row(0)->success,"other source serial not associated");
    recorder.observe(actual);require(recorder.row(0)->success&&recorder.row(0)->snapshot_index==6,"successful submission linked");
    require(recorder.row(0)->eyes[0].position.x==.3125&&recorder.row(0)->fov[1].right==.8125&&recorder.row(0)->space_epoch==5,"actual submitted pose/FOV/epoch frozen");
    actual.eyes[0].position.x=777;recorder.observe(actual);require(recorder.row(0)->eyes[0].position.x==.3125,"later submission cannot erase first success");
    for(unsigned i=1;i<8;++i){request.key.ticket++;require(recorder.request(request),"XR bounded slots");}
    request.key.ticket++;require(!recorder.request(request)&&recorder.overflow==1,"XR capacity refuses overwrite");
}
void mailbox(){
    NativeRecorder recorder;PinMailbox native,xr,collision;
    require(native.create(),"native mapping create");require(!collision.create(),"unknown existing mapping not replaced");
    require(xr.open(GetCurrentProcessId()),"XR opens exact process-start mapping");
    auto r=row();r.key.pid=GetCurrentProcessId();r.key.start=native.start;
    recorder.observe(r,0,stage(0));
    auto invalid=r.key;invalid.start++;require(!xr.request(invalid,0,99),"foreign process start request rejected");
    for(unsigned i=0;i<8;++i)require(xr.request(r.key,i,100+i),"append request");
    require(!xr.request(r.key,9,109),"mailbox limit never overwrites slots");
    native.service(recorder);require(recorder.pinned(0)&&recorder.pinned(7),"committed exact requests serviced");
    require(recorder.pinned(7)->source_serial==107&&recorder.pinned(7)->snapshot_index==7,"request payload preserved");
    native.service(recorder);require(recorder.pinned(7)->source_serial==107,"service is idempotent");
}
}
int main(){retention();submissions();mailbox();std::cout<<"SixDoF diagnostic retention, actual-stage, submit-success and mailbox checks passed\n";}
