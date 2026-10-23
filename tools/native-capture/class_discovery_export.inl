// Callback reads always execute in the external owner, never a game hook.
// No process opening, game writes, injection, window or XR calls occur here.
#include "class_discovery.h"
using DiscoveryRead=int (WINAPI*)(void*,std::uint64_t,std::uint32_t,void*);
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureDiscoveryClasses(
    std::uint32_t version,std::uint64_t const* objects,std::uint32_t object_count,
    std::uint64_t const* names,std::uint32_t name_count,std::uint64_t names_header,
    DiscoveryRead read,void* context,std::uint64_t* output,std::uint32_t capacity,std::uint32_t* count){
    if(version!=gears_class_discovery::abi_version||!objects||!names||!read||!output||!count||
       !object_count||object_count>=gears_class_discovery::limit||!name_count||name_count>=gears_class_discovery::limit||
       capacity<object_count)return ERROR_INVALID_PARAMETER;
    *count=0;
    try{
        std::vector<std::uint64_t> object_copy(objects,objects+object_count),name_copy(names,names+name_count);
        const auto result=gears_class_discovery::discover(std::move(object_copy),name_copy,names_header,
            [&](void* destination,std::uint64_t address,std::size_t size){
                const int status=read(context,address,static_cast<std::uint32_t>(size),destination);
                if(status<0)throw gears_class_discovery::Aborted{};return status==1;
            });
        std::copy(result.begin(),result.end(),output);*count=static_cast<std::uint32_t>(result.size());return ERROR_SUCCESS;
    }catch(gears_class_discovery::RegistryChanged const&){return ERROR_RETRY;}
    catch(gears_class_discovery::Aborted const&){return ERROR_CANCELLED;}
    catch(...){return ERROR_GEN_FAILURE;}
}
