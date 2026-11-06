#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4715) // The reused test's renamed main loses C++ main's implicit return.
#define main equipment_observation_inherited_client_fixture
#include "weapon_hand_client_test.cpp"
#undef main
#pragma warning(pop)
namespace {
unsigned equipment_checks{};
#define CHECK_EQUIPMENT(x) do {++equipment_checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}} while(false)
EquipmentIdentity identity(Frame const& f){return {f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,f.reference,f.reference_generation};}
enum class Mode {normal,fail,revoke,reenter,seh};
constexpr DWORD equipment_exception=0xe0424782;
struct Reader {
    Host& host;HandClient& client;Mode mode{Mode::normal};unsigned calls{},guard_checks{};
    std::int64_t elapsed{};
    bool registered{};std::uint64_t generation{};
    template<class Current>bool operator()(EquipmentIdentity const& expected,Current const& current,EquipmentObservation& out) noexcept {
        ++calls;CHECK_EQUIPMENT(equipment_matches(host.frame,expected));CHECK_EQUIPMENT(current());++guard_checks;
        out.game=host.game;out.hold_generation=9;out.component=Host::object(0x7000);out.asset=Host::object(0x7100);
        out.begin_ns=out.end_ns=-99; // Only actual Client clock observations can supply these.
        if(mode==Mode::seh)RaiseException(equipment_exception,0,0,nullptr);
        if(mode==Mode::revoke)host.invalidate();
        if(mode==Mode::reenter){EquipmentObservation nested{};
            const auto accepted=registered?client.observe_registered_equipment_on_gt(expected,generation,*this,nested):
                client.observe_equipment_on_gt(expected,*this,nested);
            CHECK_EQUIPMENT(!accepted&&!nested.game.known);}
        host.now+=elapsed;
        if(mode==Mode::revoke||mode==Mode::reenter){CHECK_EQUIPMENT(!current());++guard_checks;}
        return host.metadata_known&&mode!=Mode::fail;
    }
};
bool empty(EquipmentObservation const& out){return !out.begin_ns&&!out.end_ns&&!out.game.known&&!out.hold_generation&&
    same(out.component,ObjectIdentity{})&&same(out.asset,ObjectIdentity{});}
bool query(Host& h,HandClient& c,Reader& reader,EquipmentIdentity const& expected,EquipmentObservation& out){
    GT gt;const auto old_reads=h.reads;const bool okay=c.observe_equipment_on_gt(expected,reader,out);
    CHECK_EQUIPMENT(h.reads==old_reads); // No legacy fresh-Frame adapter observation.
    return okay;
}
bool caught(HandClient& client,Reader& reader,EquipmentIdentity const& expected,EquipmentObservation& out) noexcept {
    __try{client.observe_equipment_on_gt(expected,reader,out);return false;}
    __except(GetExceptionCode()==equipment_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}
std::uint64_t generation(HandClient& c){GT gt;return c.metadata_generation_on_gt();}
bool query_registered(Host& h,HandClient& c,Reader& reader,EquipmentIdentity const& expected,EquipmentObservation& out,
    std::uint64_t captured_generation=0){
    GT gt;const auto old_reads=h.reads;
    reader.registered=true;reader.generation=captured_generation?captured_generation:c.metadata_generation_on_gt();
    const bool okay=c.observe_registered_equipment_on_gt(expected,reader.generation,reader,out);
    CHECK_EQUIPMENT(h.reads==old_reads);return okay;
}
bool caught_registered(HandClient& client,Reader& reader,EquipmentIdentity const& expected,EquipmentObservation& out) noexcept {
    __try{client.observe_registered_equipment_on_gt(expected,reader.generation,reader,out);return false;}
    __except(GetExceptionCode()==equipment_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}
void expire_publication(Host& h,HandClient& c){
    h.now+=110000000;Frame ignored{};GT gt;
    CHECK_EQUIPMENT(!c.copy_frame_on_gt(ignored)&&same_frame(ignored,Frame{})&&!c.armed());
}
}
int main(){
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};auto expected=identity(h.frame);
        h.now=h.frame.now_ns+150000000;EquipmentObservation out{};
        CHECK_EQUIPMENT(query(h,c,reader,expected,out)&&out.game.known&&out.begin_ns==h.now&&out.end_ns==h.now&&reader.calls==1);
        CHECK_EQUIPMENT(out.hold_generation==9&&out.component.address==0x7000&&c.armed());
        Frame old{};{GT gt;CHECK_EQUIPMENT(!c.copy_frame_on_gt(old)&&same_frame(old,Frame{}));}}
    for(std::int64_t elapsed:{50000000LL,50000001LL,-1LL}){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};
        reader.elapsed=elapsed;EquipmentObservation out{};const auto begin=h.now;const auto okay=query(h,c,reader,identity(h.frame),out);
        CHECK_EQUIPMENT(okay==(elapsed==50000000));
        if(okay)CHECK_EQUIPMENT(out.begin_ns==begin&&out.end_ns==begin+elapsed);else CHECK_EQUIPMENT(empty(out));}
    for(unsigned field=0;field<9;++field){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};auto expected=identity(h.frame);
        switch(field){case 0:++expected.key.frame;break;case 1:++expected.world.name_number;break;
        case 2:++expected.controller.name_number;break;case 3:++expected.pawn.name_number;break;
        case 4:++expected.mesh.name_number;break;case 5:++expected.weapon.name_number;break;
        case 6:++expected.reference_generation;break;case 7:expected.reference.position.x+=.01;break;
        case 8:++expected.key.registration_cookie;break;}
        EquipmentObservation out{};out.game.known=true;CHECK_EQUIPMENT(!query(h,c,reader,expected,out)&&empty(out)&&reader.calls==0&&c.armed());}
    for(auto mode:{Mode::fail,Mode::revoke,Mode::reenter}){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};reader.mode=mode;
        EquipmentObservation out{};CHECK_EQUIPMENT(!query(h,c,reader,identity(h.frame),out)&&empty(out)&&reader.calls==1);
        if(mode!=Mode::fail)CHECK_EQUIPMENT(!c.armed());}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};auto expected=identity(h.frame);h.invalidate();
        EquipmentObservation out{};CHECK_EQUIPMENT(!query(h,c,reader,expected,out)&&empty(out)&&reader.calls==0);
        h.next();h.step();CHECK_EQUIPMENT(!query(h,c,reader,expected,out)&&empty(out)&&reader.calls==0);
        CHECK_EQUIPMENT(query(h,c,reader,identity(h.frame),out));}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};EquipmentObservation out{};
        CHECK_EQUIPMENT(!c.observe_equipment_on_gt(identity(h.frame),reader,out)&&empty(out)&&reader.calls==0);}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};h.now=0;EquipmentObservation out{};
        CHECK_EQUIPMENT(!query(h,c,reader,identity(h.frame),out)&&empty(out)&&reader.calls==0);}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};reader.mode=Mode::seh;EquipmentObservation out{};
        {GT gt;CHECK_EQUIPMENT(caught(c,reader,identity(h.frame),out));}
        CHECK_EQUIPMENT(empty(out)&&c.armed());reader.mode=Mode::normal;
        CHECK_EQUIPMENT(query(h,c,reader,identity(h.frame),out)&&reader.calls==2);}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};h.game.busy=true;EquipmentObservation out{};
        CHECK_EQUIPMENT(query(h,c,reader,identity(h.frame),out)&&out.game.busy); // Observation alone grants no eligibility.
        Frame frame{};{GT gt;CHECK_EQUIPMENT(!c.copy_frame_on_gt(frame));}} // Legacy same_game requirement is unchanged.
    // A typed current-body query owns fresh source evidence independently of
    // an expired legacy Frame. Metadata must not renew that Frame or its clock.
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};const auto expected=identity(h.frame);
        const auto epoch=generation(c);CHECK_EQUIPMENT(epoch!=0);expire_publication(h,c);
        CHECK_EQUIPMENT(generation(c)==epoch);const auto old_frame=h.frame;EquipmentObservation out{};
        CHECK_EQUIPMENT(query_registered(h,c,reader,expected,out,epoch)&&out.game.known&&reader.calls==1);
        CHECK_EQUIPMENT(out.begin_ns==h.now&&out.end_ns==h.now&&!c.armed()&&same_frame(h.frame,old_frame));
        CHECK_EQUIPMENT(!query(h,c,reader,expected,out)&&empty(out)&&reader.calls==1);
        Frame ignored{};{GT gt;CHECK_EQUIPMENT(!c.copy_frame_on_gt(ignored));}}
    for(std::int64_t elapsed:{50000000LL,50000001LL,-1LL}){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};
        expire_publication(h,c);reader.elapsed=elapsed;EquipmentObservation out{};const auto begin=h.now;
        const auto okay=query_registered(h,c,reader,identity(h.frame),out);
        CHECK_EQUIPMENT(okay==(elapsed==50000000));
        if(okay)CHECK_EQUIPMENT(out.begin_ns==begin&&out.end_ns==begin+elapsed);else CHECK_EQUIPMENT(empty(out));}
    for(auto mode:{Mode::fail,Mode::revoke,Mode::reenter}){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};reader.mode=mode;
        expire_publication(h,c);EquipmentObservation out{};
        CHECK_EQUIPMENT(!query_registered(h,c,reader,identity(h.frame),out)&&empty(out)&&reader.calls==1&&!c.armed());}
    for(unsigned field=0;field<6;++field){Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};auto expected=identity(h.frame);
        const auto epoch=generation(c);expire_publication(h,c);
        switch(field){case 0:++expected.key.registration_cookie;break;case 1:++expected.key.native_started_qpc;break;
        case 2:++expected.key.game_pid;break;case 3:++expected.key.game_start;break;
        case 4:++expected.key.thread_id;break;case 5:h.invalidate();break;}
        EquipmentObservation out{};out.game.known=true;
        CHECK_EQUIPMENT(!query_registered(h,c,reader,expected,out,epoch)&&empty(out)&&reader.calls==0&&!c.armed());}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};const auto expected=identity(h.frame);const auto epoch=generation(c);
        expire_publication(h,c);EquipmentObservation out{};
        CHECK_EQUIPMENT(!c.metadata_generation_on_gt());
        CHECK_EQUIPMENT(!c.observe_registered_equipment_on_gt(expected,epoch,reader,out)&&empty(out)&&reader.calls==0);}
    {Host h;HandClient c(h.adapter());h.connect(c);h.ready();Reader reader{h,c};expire_publication(h,c);
        reader.registered=true;reader.generation=generation(c);reader.mode=Mode::seh;EquipmentObservation out{};
        {GT gt;CHECK_EQUIPMENT(caught_registered(c,reader,identity(h.frame),out));}
        CHECK_EQUIPMENT(empty(out)&&generation(c)==reader.generation&&!c.armed());reader.mode=Mode::normal;
        CHECK_EQUIPMENT(query_registered(h,c,reader,identity(h.frame),out)&&reader.calls==2&&!c.armed());}
    std::printf("WeaponEquipmentObservationTest: %u checks passed; actual HandClient Gate/epoch, legacy and current-body metadata paths\n",equipment_checks);return 0;
}
