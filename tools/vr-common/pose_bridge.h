#pragma once
#include <windows.h>
#include "pose_gate.h"
#include <cstring>

namespace gears_vr {
// One verified producer per target PID. Both sides use QPC, never XrTime.
struct BridgePacket {
    unsigned magic=0x47565031, version=1, size=sizeof(BridgePacket), target_pid=0;
    long long frequency=0;
    PoseSample sample{};
};
inline long long monotonic_ns() {
    LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);
    return (t.QuadPart/f.QuadPart)*1000000000LL+
        (t.QuadPart%f.QuadPart)*1000000000LL/f.QuadPart;
}
class PoseBridge {
public:
    PoseBridge()=default;
    PoseBridge(PoseBridge const&)=delete;
    ~PoseBridge(){close();}
    bool open(DWORD pid,bool producer) {
        close();wchar_t map_name[100],lock_name[100];
        swprintf_s(map_name,L"Local\\GearsVR.Pose.%lu",pid);
        swprintf_s(lock_name,L"Local\\GearsVR.PoseLock.%lu",pid);
        if(producer) {
            mutex_=CreateMutexW(nullptr,FALSE,lock_name);
            if(!mutex_ || GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            map_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(BridgePacket),map_name);
            if(!map_ || GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        } else {
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,lock_name);
            map_=OpenFileMappingW(FILE_MAP_READ,FALSE,map_name);
        }
        if(!mutex_ || !map_){close();return false;}
        data_=static_cast<BridgePacket*>(MapViewOfFile(map_,producer?FILE_MAP_WRITE:FILE_MAP_READ,0,0,sizeof(BridgePacket)));
        if(!data_){close();return false;}
        pid_=pid;producer_=producer;
        if(producer){BridgePacket initial;initial.target_pid=pid;LARGE_INTEGER f;QueryPerformanceFrequency(&f);initial.frequency=f.QuadPart;publish(initial.sample);}
        return true;
    }
    bool publish(PoseSample sample) {
        if(!producer_ || !data_)return false;
        DWORD wait=WaitForSingleObject(mutex_,0);
        if(wait!=WAIT_OBJECT_0 && wait!=WAIT_ABANDONED)return false;
        BridgePacket value;value.target_pid=pid_;LARGE_INTEGER f;QueryPerformanceFrequency(&f);value.frequency=f.QuadPart;value.sample=sample;
        std::memcpy(data_,&value,sizeof(value));ReleaseMutex(mutex_);return true;
    }
    bool read(PoseSample& sample) {
        if(!data_)return false;
        DWORD wait=WaitForSingleObject(mutex_,0);
        if(wait==WAIT_ABANDONED){ReleaseMutex(mutex_);return false;}
        if(wait!=WAIT_OBJECT_0)return false;
        BridgePacket value;std::memcpy(&value,data_,sizeof(value));ReleaseMutex(mutex_);
        LARGE_INTEGER f;QueryPerformanceFrequency(&f);
        if(value.magic!=0x47565031 || value.version!=1 || value.size!=sizeof(value) ||
           value.target_pid!=pid_ || value.frequency!=f.QuadPart)return false;
        sample=value.sample;return true;
    }
    void close(){if(data_)UnmapViewOfFile(data_);data_=nullptr;if(map_)CloseHandle(map_);map_=nullptr;if(mutex_)CloseHandle(mutex_);mutex_=nullptr;producer_=false;}
private:
    HANDLE map_{},mutex_{};BridgePacket* data_{};DWORD pid_{};bool producer_{};
};
inline Viewpoint bounded_view(Viewpoint clean,Viewpoint result) {
    Vector delta{result.position.x-clean.position.x,result.position.y-clean.position.y,result.position.z-clean.position.z};
    double length=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    if(length>2){delta.x*=2/length;delta.y*=2/length;delta.z*=2/length;}
    auto angle=[](double base,double output){return base+std::clamp(std::remainder(output-base,6.283185307179586),-0.08726646259971647,0.08726646259971647);};
    return {{clean.position.x+delta.x,clean.position.y+delta.y,clean.position.z+delta.z},
        {angle(clean.rotation.pitch,result.rotation.pitch),angle(clean.rotation.yaw,result.rotation.yaw),angle(clean.rotation.roll,result.rotation.roll)}};
}
}
