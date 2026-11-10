#include "cover_physics_c0_live.inl"
#include "cover_physx_capsule_path_observation.h"
#include <cstdio>
#include <memory>
namespace c0=cover_physics_c0_live;namespace q=gears_cover_physx_query;
static unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
int main(){
    gears_continuous_hand_entry::Entry entry{};gears_native_rig_tracking::CoreSnapshot core{};
    q::Box box{};q::Capsule capsule{};q::Vector delta{};q::Overlap overlap{true};q::Sweep sweep{};sweep.raw_count=99;
    auto path_output=std::make_unique<gears_cover_capsule_path::RawPath>();path_output->collected.raw.raw_count=77;
    C(c0::sweep_capsule_path(entry,core,capsule,delta,gears_cover_capsule_path::Policy{},q::StreamLimits{},*path_output)==q::Code::unavailable&&path_output->collected.raw.raw_count==77);
    C(c0::overlap(entry,core,q::Sphere{{1,2,3},.25f},overlap)==q::Code::unavailable&&overlap.raw_hit);
    C(c0::overlap(entry,core,box,overlap)==q::Code::unavailable&&overlap.raw_hit);
    C(c0::overlap(entry,core,capsule,overlap)==q::Code::unavailable&&overlap.raw_hit);
    C(c0::sweep(entry,core,box,delta,sweep)==q::Code::unavailable&&sweep.raw_count==99);
    C(c0::sweep(entry,core,capsule,delta,sweep)==q::Code::unavailable&&sweep.raw_count==99);
    q::AllSweep all{};all.raw_count=99;
    q::AllOverlap overlaps{};overlaps.raw_count=99;
    C(c0::overlap_all(entry,core,capsule,q::StreamLimits{},overlaps)==q::Code::unavailable&&overlaps.raw_count==99);
    auto overlap_owners=std::make_unique<q::AllOverlapOwners>();overlap_owners->raw.raw_count=99;overlap_owners->owners[0].actor=0xdead;
    C(c0::overlap_all(entry,core,capsule,q::StreamLimits{},*overlap_owners)==q::Code::unavailable&&overlap_owners->raw.raw_count==99&&overlap_owners->owners[0].actor==0xdead);
    C(c0::sweep_all(entry,core,box,delta,q::StreamLimits{},all)==q::Code::unavailable&&all.raw_count==99);
    C(c0::sweep_all(entry,core,capsule,delta,q::StreamLimits{},all)==q::Code::unavailable&&all.raw_count==99);
    auto owners=std::make_unique<q::AllSweepOwners>();owners->raw.raw_count=99;owners->owners[0].actor=0xdead;
    C(c0::sweep_all(entry,core,box,delta,q::StreamLimits{},*owners)==q::Code::unavailable&&owners->raw.raw_count==99&&owners->owners[0].actor==0xdead);
    C(c0::sweep_all(entry,core,capsule,delta,q::StreamLimits{},*owners)==q::Code::unavailable&&owners->raw.raw_count==99&&owners->owners[0].actor==0xdead);
    wchar_t folder[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,folder)>0);C(GetTempFileNameW(folder,L"cvq",0,path)>0);
    C(c0::configure(path,nullptr));C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"1",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"invalid",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",path)!=0);C(c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"1",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"invalid",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_sphere_probe",L"0",path)!=0);C(c0::configure(path,nullptr));
    c0::observe_sphere_once(entry,core,{1,2,3});C(c0::pending_count()==0);
    C(SetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",L"1")!=0);C(!c0::configure(path,nullptr));
    C(SetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",L"invalid")!=0);C(!c0::configure(path,nullptr));
    C(SetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",L"0")!=0);C(c0::configure(path,nullptr));
    C(SetEnvironmentVariableW(L"GEARSVR_COVER_SPHERE_PROBE",nullptr)!=0);
    c0::request_drain();C(c0::pending_count()==0&&c0::poll_stop());C(!GetModuleHandleW(L"PhysXCore.dll"));C(DeleteFileW(path)!=0);
    printf("cover_physics_c0 OFF: %u checks, %u failures; no game/module/query\n",checks,failures);return failures?1:0;
}
