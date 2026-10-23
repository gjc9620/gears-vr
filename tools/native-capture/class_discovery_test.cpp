#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <stdexcept>
#include <map>
#include <string>
#include "class_discovery_export.inl"
using namespace gears_class_discovery;
using Api=decltype(&GearsCaptureDiscoveryClasses);
struct Memory {
    std::map<Address,unsigned char> bytes;
    std::uint64_t reads{};bool abort{},short_blocks{};
    void put(Address a,void const* data,std::size_t n){auto p=static_cast<unsigned char const*>(data);for(std::size_t i=0;i<n;++i)bytes[a+i]=p[i];}
    template<class T> void value(Address a,T x){put(a,&x,sizeof(x));}
    void object(Address a,Address cls,std::uint32_t name){std::array<unsigned char,0x60> data{};put(a,data.data(),data.size());value(a+80,cls);value(a+72,name);}
    void name(Address a,std::uint32_t index,std::u16string const& text){std::array<unsigned char,148> data{};put(a,data.data(),data.size());value(a,index);put(a+20,text.c_str(),(text.size()+1)*2);}
    static int WINAPI read(void* context,std::uint64_t a,std::uint32_t n,void* destination){
        auto& self=*static_cast<Memory*>(context);++self.reads;if(self.abort)return -1;
        if(self.short_blocks&&n>=4096)return 0;
        auto out=static_cast<unsigned char*>(destination);
        for(std::uint32_t i=0;i<n;++i){auto it=self.bytes.find(a+i);if(it==self.bytes.end())return 0;out[i]=it->second;}
        return 1;
    }
};
static void require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
static void exercise(Api api){
    Memory memory;std::vector<Address> names{0x300000,0x300100,0x300200};
    memory.name(names[0],0,u"Class");memory.name(names[1],1,u"Pawn");memory.name(names[2],2,u"Canvas");
    memory.object(0x50000,0x50000,0);memory.object(0x51000,0x50000,1);memory.object(0x52000,0x50000,2);
    std::vector<Address> pointers{0,0x53000,0x52000,0x50000,0x51000,0x52000};memory.object(0x53000,0x51000,1);
    std::vector<Address> output(pointers.size());std::uint32_t count{};
    auto run=[&](){count=999;return api(1,pointers.data(),static_cast<std::uint32_t>(pointers.size()),names.data(),
        static_cast<std::uint32_t>(names.size()),0x2000,Memory::read,&memory,output.data(),static_cast<std::uint32_t>(output.size()),&count);};
    require(run()==0&&count==4&&output[0]==0x50000&&output[1]==0x51000&&output[2]==0x52000&&output[3]==0x52000,"candidates/order/duplicates");
    memory.short_blocks=true;require(run()==0&&count==4,"unreadable block/page fallback");
    memory.name(names[0],0,u"NotClass");require(run()==0&&count==0,"changed metaclass name");
    memory.name(names[0],0,u"Class");memory.value(names[0],std::uint32_t(9));require(run()==0&&count==0,"name registry index mismatch");
    memory.name(names[0],0,u"Class");memory.abort=true;require(run()==ERROR_CANCELLED&&count==0,"abort propagation");memory.abort=false;
    require(api(2,pointers.data(),static_cast<std::uint32_t>(pointers.size()),names.data(),3,0x2000,Memory::read,&memory,
        output.data(),static_cast<std::uint32_t>(output.size()),&count)==ERROR_INVALID_PARAMETER,"unknown ABI accepted");
    require(api(1,pointers.data(),static_cast<std::uint32_t>(pointers.size()),names.data(),3,0x2000,Memory::read,&memory,
        output.data(),1,&count)==ERROR_INVALID_PARAMETER,"small output accepted");
    puts("Class discovery: sorted duplicates, exact classes, unreadable spans, changed names, bounds and callback abort passed");
}
int wmain(int argc,wchar_t** argv){try{
    HMODULE module{};Api api=&GearsCaptureDiscoveryClasses;
    if(argc==2){module=LoadLibraryW(argv[1]);require(module!=nullptr,"load candidate DLL");api=reinterpret_cast<Api>(GetProcAddress(module,"GearsCaptureDiscoveryClasses"));require(api!=nullptr,"missing export");}
    exercise(api);if(module)FreeLibrary(module);return 0;
}catch(std::exception const& error){fprintf(stderr,"ClassDiscoveryTest failed: %s\n",error.what());return 1;}}
