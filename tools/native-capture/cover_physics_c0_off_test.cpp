#include "cover_physics_c0_live.inl"
#include <cstdio>
namespace c0=cover_physics_c0_live;namespace q=gears_cover_physx_query;
static unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
int main(){
    gears_continuous_hand_entry::Entry entry{};gears_native_rig_tracking::CoreSnapshot core{};
    q::Box box{};q::Capsule capsule{};q::Vector delta{};q::Overlap overlap{true};q::Sweep sweep{};sweep.raw_count=99;
    C(c0::overlap(entry,core,box,overlap)==q::Code::unavailable&&overlap.raw_hit);
    C(c0::overlap(entry,core,capsule,overlap)==q::Code::unavailable&&overlap.raw_hit);
    C(c0::sweep(entry,core,box,delta,sweep)==q::Code::unavailable&&sweep.raw_count==99);
    C(c0::sweep(entry,core,capsule,delta,sweep)==q::Code::unavailable&&sweep.raw_count==99);
    wchar_t folder[MAX_PATH]{},path[MAX_PATH]{};C(GetTempPathW(MAX_PATH,folder)>0);C(GetTempFileNameW(folder,L"cvq",0,path)>0);
    C(c0::configure(path,nullptr));C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"1",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"invalid",path)!=0);C(!c0::configure(path,nullptr));
    C(WritePrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",path)!=0);C(c0::configure(path,nullptr));
    c0::request_drain();C(c0::pending_count()==0&&c0::poll_stop());C(!GetModuleHandleW(L"PhysXCore.dll"));C(DeleteFileW(path)!=0);
    printf("cover_physics_c0 OFF: %u checks, %u failures; no game/module/query\n",checks,failures);return failures?1:0;
}
