#define GEARS_FRESH_POSE_PRODUCER_OBSERVER 0
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdio>
#include <cstdint>
#include "fresh_pose_producer.h"
#include "fresh_pose_producer_live.inl"
int main(){namespace f=fresh_pose_producer_live;wchar_t path[MAX_PATH]{},temporary[MAX_PATH]{};
    if(!GetTempPathW(MAX_PATH,temporary)||swprintf_s(path,L"%sGearsFreshOff-%lu.ini",temporary,GetCurrentProcessId())<0)return 1;
    bool ok=WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",path)!=0&&f::configure(path,nullptr)&&f::reserve()&&!f::pending_count();
    ok=ok&&WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"1",path)!=0&&!f::configure(path,nullptr);
    ok=ok&&WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"enabled",L"0",path)!=0&&WritePrivateProfileStringW(L"FreshPoseProducerObserver",L"record_call_tree",L"1",path)!=0&&!f::configure(path,nullptr);
    f::request_drain();ok=ok&&!f::pending_count()&&!f::reserved_target(nullptr);DeleteFileW(path);
    printf("FreshPose cap OFF: configure0, reject enabled1/tree1, no hooks, zero debt: %s\n",ok?"pass":"FAIL");return ok?0:1;
}
