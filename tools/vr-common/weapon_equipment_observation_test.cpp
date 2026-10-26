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
    template<class Current>bool operator()(EquipmentIdentity const& expected,Current const& current,EquipmentObservation& out) noexcept {
        ++calls;CHECK_EQUIPMENT(equipment_matches(host.frame,expected));CHECK_EQUIPMENT(current());++guard_checks;
        out.game=host.game;out.hold_generation=9;out.component=Host::object(0x7000);out.asset=Host::object(0x7100);
        out.begin_ns=out.end_ns=-99; // Only actual Client clock observations can supply these.
        if(mode==Mode::seh)RaiseException(equipment_exception,0,0,nullptr);
        if(mode==Mode::revoke)host.invalidate();
        if(mode==Mode::reenter){EquipmentObservation nested{};
            CHECK_EQUIPMENT(!client.observe_equipment_on_gt(expected,*this,nested)&&!nested.game.known);}
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
    std::printf("WeaponEquipmentObservationTest: %u checks passed; actual HandClient Gate/epoch with fixed owned metadata continuation\n",equipment_checks);return 0;
}
