#pragma once
#include <windows.h>
#include <cstring>
#include "stereo_frame.h"

namespace gears_vr {
struct StereoBridgePacket {
    unsigned magic=0x47565331,version=4,size=sizeof(StereoBridgePacket),target_pid{};
    long long frequency{};
    StereoSample sample{};
};
class StereoBridge {
    HANDLE map_{},mutex_{};StereoBridgePacket* data_{};DWORD pid_{};bool producer_{};
    bool packet_valid(StereoBridgePacket const& p) const {
        LARGE_INTEGER frequency{};
        return QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0&&
            p.magic==0x47565331&&p.version==4&&p.size==sizeof(p)&&p.target_pid==pid_&&p.frequency==frequency.QuadPart&&stereo_source_known(p.sample.source);
    }
public:
    StereoBridge()=default;
    StereoBridge(StereoBridge const&)=delete;
    StereoBridge& operator=(StereoBridge const&)=delete;
    ~StereoBridge(){close();}
    bool open(DWORD pid,bool producer) {
        close();if(!pid)return false;
        wchar_t mapping[100],locking[100];
        swprintf_s(mapping,L"Local\\GearsVR.Stereo.%lu",pid);
        swprintf_s(locking,L"Local\\GearsVR.StereoLock.%lu",pid);
        if(producer){
            mutex_=CreateMutexW(nullptr,FALSE,locking);
            if(!mutex_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
            map_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(StereoBridgePacket),mapping);
            if(!map_||GetLastError()==ERROR_ALREADY_EXISTS){close();return false;}
        } else {
            mutex_=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,locking);
            map_=OpenFileMappingW(FILE_MAP_READ,FALSE,mapping);
        }
        if(!mutex_||!map_){close();return false;}
        data_=static_cast<StereoBridgePacket*>(MapViewOfFile(map_,producer?FILE_MAP_WRITE:FILE_MAP_READ,0,0,sizeof(StereoBridgePacket)));
        if(!data_){close();return false;}
        pid_=pid;producer_=producer;
        if(producer){if(!publish(StereoSample{})){close();return false;}}
        else {StereoSample sample;if(!read(sample)){close();return false;}}
        return true;
    }
    bool publish(StereoSample const& sample) {
        if(!producer_||!data_||!stereo_source_known(sample.source))return false;
        DWORD wait=WaitForSingleObject(mutex_,0);
        if(wait!=WAIT_OBJECT_0&&wait!=WAIT_ABANDONED)return false;
        StereoBridgePacket value;value.target_pid=pid_;LARGE_INTEGER frequency{};
        bool ok=QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0;
        if(ok){value.frequency=frequency.QuadPart;value.sample=sample;std::memcpy(data_,&value,sizeof(value));}
        ReleaseMutex(mutex_);return ok;
    }
    bool read(StereoSample& sample) {
        if(!data_)return false;
        DWORD wait=WaitForSingleObject(mutex_,0);
        if(wait==WAIT_ABANDONED){ReleaseMutex(mutex_);return false;}
        if(wait!=WAIT_OBJECT_0)return false;
        StereoBridgePacket value;std::memcpy(&value,data_,sizeof(value));ReleaseMutex(mutex_);
        if(!packet_valid(value))return false;
        sample=value.sample;return true;
    }
    void close(){if(data_)UnmapViewOfFile(data_);data_=nullptr;if(map_)CloseHandle(map_);map_=nullptr;
        if(mutex_)CloseHandle(mutex_);mutex_=nullptr;pid_=0;producer_=false;}
};
}
