// Resource/handle integration only: never BeginSession, EndFrame or create a window.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static void xr(XrResult result,const char* message){if(XR_FAILED(result))throw std::runtime_error(std::string(message)+": "+std::to_string(result));}
static void dx(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(std::string(message)+": "+std::to_string(result));}
struct Handles {
    XrInstance instance{};XrSession session{};
    std::map<XrSwapchain,std::vector<ID3D12Resource*>> live;
    std::set<XrSwapchain> issued;
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;
    ~Handles(){for(auto const& entry:live)xrDestroySwapchain(entry.first);
        if(session)xrDestroySession(session);if(instance)xrDestroyInstance(instance);}
    XrSwapchain create(const char* name,unsigned width,unsigned height,DXGI_FORMAT format){
        XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        info.format=format;info.width=width;info.height=height;info.sampleCount=info.faceCount=info.arraySize=info.mipCount=1;
        XrSwapchain chain{};xr(xrCreateSwapchain(session,&info,&chain),"create swapchain");
        const bool unique=issued.insert(chain).second;
        // Track each distinct live handle exactly once even when the old runtime
        // aliases a create to an existing chain. Cleanup cannot double-destroy it.
        if(live.find(chain)==live.end())live.emplace(chain,std::vector<ID3D12Resource*>{});
        uint32_t count{};xr(xrEnumerateSwapchainImages(chain,0,&count,nullptr),"enumerate count");
        require(count>0&&count<=16,"Unexpected swapchain image count");
        std::vector<XrSwapchainImageD3D12KHR> images(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
        const auto capacity=count;
        xr(xrEnumerateSwapchainImages(chain,capacity,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),"enumerate images");
        require(count==capacity,"Swapchain image count changed");
        bool exact=true,distinct=true;std::set<ID3D12Resource*> resources;
        std::printf("{\"stage\":\"%s\",\"handle\":%llu,\"unique_handle\":%s,\"requested\":[%u,%u,%u],\"images\":[",
                    name,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(chain)),unique?"true":"false",width,height,unsigned(format));
        for(unsigned i=0;i<count;++i){
            auto* image=images[i].texture;require(image!=nullptr,"Null texture");const auto desc=image->GetDesc();
            bool alias=!resources.insert(image).second;
            for(auto const& entry:live)for(auto* prior:entry.second)if(prior==image)alias=true;
            distinct=distinct&&!alias;
            exact=exact&&desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&desc.Width==width&&desc.Height==height&&
                desc.Format==format&&desc.DepthOrArraySize==1&&desc.MipLevels==1&&desc.SampleDesc.Count==1;
            if(i)std::putchar(',');
            std::printf("{\"texture\":%llu,\"width\":%llu,\"height\":%u,\"format\":%u,\"aliases_live_texture\":%s}",
                        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(image)),
                        static_cast<unsigned long long>(desc.Width),desc.Height,unsigned(desc.Format),alias?"true":"false");
        }
        std::puts("]}");std::fflush(stdout);
        require(unique,"Runtime reused an issued swapchain handle");
        require(exact,"Runtime returned a texture with incorrect dimensions/format");
        require(distinct,"Runtime aliased textures across live swapchains/images");
        for(auto const& image:images)live.at(chain).push_back(image.texture);
        return chain;
    }
    void destroy(XrSwapchain chain){
        xr(xrDestroySwapchain(chain),"destroy swapchain");live.erase(chain);
        std::printf("{\"destroyed\":%llu}\n",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(chain)));std::fflush(stdout);
    }
};

static int child_test(){
    wchar_t marker[2]{};require(GetEnvironmentVariableW(L"GEARSVR_SWAPCHAIN_TEST_CHILD",marker,2)==1&&marker[0]==L'1',"Use --runtime MANIFEST parent mode");
    Handles h;
    const char* extension=XR_KHR_D3D12_ENABLE_EXTENSION_NAME;
    XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};create.applicationInfo.apiVersion=XR_CURRENT_API_VERSION;
    std::strcpy(create.applicationInfo.applicationName,"SwapchainHandleRuntimeTest");
    create.enabledExtensionCount=1;create.enabledExtensionNames=&extension;
    xr(xrCreateInstance(&create,&h.instance),"create instance");
    XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};xr(xrGetInstanceProperties(h.instance,&properties),"runtime identity");
    require(!std::strcmp(properties.runtimeName,"OpenXR Simulator Runtime"),"Refusing a non-OpenXR Simulator Runtime");
    std::printf("{\"runtime\":\"OpenXR Simulator Runtime\",\"preview_requested\":false}\n");
    XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};system_info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system{};xr(xrGetSystem(h.instance,&system_info,&system),"get system");
    PFN_xrGetD3D12GraphicsRequirementsKHR requirements_fn{};
    xr(xrGetInstanceProcAddr(h.instance,"xrGetD3D12GraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements_fn)),"get requirements function");
    XrGraphicsRequirementsD3D12KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
    xr(requirements_fn(h.instance,system,&requirements),"graphics requirements");
    ComPtr<IDXGIFactory4> factory;dx(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
    ComPtr<IDXGIAdapter1> adapter;dx(factory->EnumAdapterByLuid(requirements.adapterLuid,IID_PPV_ARGS(&adapter)),"runtime adapter");
    dx(D3D12CreateDevice(adapter.Get(),requirements.minFeatureLevel,IID_PPV_ARGS(&h.device)),"D3D12 device");
    D3D12_COMMAND_QUEUE_DESC queue{};dx(h.device->CreateCommandQueue(&queue,IID_PPV_ARGS(&h.queue)),"D3D12 queue");
    XrGraphicsBindingD3D12KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};binding.device=h.device.Get();binding.queue=h.queue.Get();
    XrSessionCreateInfo session{XR_TYPE_SESSION_CREATE_INFO};session.next=&binding;session.systemId=system;
    xr(xrCreateSession(h.instance,&session,&h.session),"create session");
    uint32_t count{};xr(xrEnumerateSwapchainFormats(h.session,0,&count,nullptr),"format count");
    std::vector<int64_t> formats(count);xr(xrEnumerateSwapchainFormats(h.session,count,&count,formats.data()),"formats");
    for(auto format:{DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_R10G10B10A2_UNORM})
        require(std::find(formats.begin(),formats.end(),int64_t(format))!=formats.end(),"Required reproduction format unavailable");
    const auto menu=h.create("menu",1920,764,DXGI_FORMAT_B8G8R8A8_UNORM);
    const auto replacement=h.create("replacement",1920,1080,DXGI_FORMAT_B8G8R8A8_UNORM);
    h.destroy(menu);
    auto left=h.create("eye_left",960,1080,DXGI_FORMAT_R10G10B10A2_UNORM);
    auto right=h.create("eye_right",960,1080,DXGI_FORMAT_R10G10B10A2_UNORM);
    h.destroy(left);left=h.create("eye_left_recreated",1280,720,DXGI_FORMAT_R10G10B10A2_UNORM);
    h.destroy(right);right=h.create("eye_right_recreated",640,360,DXGI_FORMAT_R10G10B10A2_UNORM);
    h.destroy(left);h.destroy(right);h.destroy(replacement);
    std::puts("{\"passed\":true,\"end_frame_calls\":0,\"preview_requested\":false}");
    return 0;
}

struct CaseLess {bool operator()(std::wstring const& a,std::wstring const& b)const{return _wcsicmp(a.c_str(),b.c_str())<0;}};
static int run_child(std::filesystem::path const& manifest){
    require(std::filesystem::is_regular_file(manifest),"Runtime manifest not found");
    std::map<std::wstring,std::wstring,CaseLess> values;
    auto environment=GetEnvironmentStringsW();require(environment!=nullptr,"Read environment");
    for(auto* p=environment;*p;p+=std::wcslen(p)+1){std::wstring entry=p;const auto split=entry.find(L'=',entry[0]==L'='?1:0);
        if(split!=std::wstring::npos)values[entry.substr(0,split)]=entry.substr(split+1);}
    FreeEnvironmentStringsW(environment);
    values[L"XR_RUNTIME_JSON"]=std::filesystem::canonical(manifest).wstring();
    values[L"XR_SIMULATOR_NO_ACTIVATE"]=L"1";
    values[L"GEARSVR_SWAPCHAIN_TEST_CHILD"]=L"1";
    std::vector<wchar_t> block;
    for(auto const& value:values){const auto entry=value.first+L"="+value.second;
        block.insert(block.end(),entry.begin(),entry.end());block.push_back(0);}block.push_back(0);
    wchar_t exe[32768]{};const DWORD length=GetModuleFileNameW(nullptr,exe,32768);
    require(length>0&&length<32768,"Test executable path");
    std::wstring command=L"\""+std::wstring(exe)+L"\" --child";
    SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};HANDLE read{},write{};
    require(CreatePipe(&read,&write,&security,0)!=FALSE,"Create child output pipe");
    if(!SetHandleInformation(read,HANDLE_FLAG_INHERIT,0)){CloseHandle(read);CloseHandle(write);throw std::runtime_error("Pipe inheritance");}
    STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow=SW_HIDE;startup.hStdOutput=write;startup.hStdError=write;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION child{};
    const BOOL launched=CreateProcessW(exe,command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_UNICODE_ENVIRONMENT,
                                       block.data(),nullptr,&startup,&child);
    CloseHandle(write);
    if(!launched){CloseHandle(read);throw std::runtime_error("Create isolated test child");}
    CloseHandle(child.hThread);char buffer[4096];DWORD bytes{};
    while(ReadFile(read,buffer,sizeof(buffer),&bytes,nullptr)&&bytes)std::fwrite(buffer,1,bytes,stdout);
    CloseHandle(read);WaitForSingleObject(child.hProcess,INFINITE);DWORD code=1;
    GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hProcess);return int(code);
}
int wmain(int argc,wchar_t** argv){
    try{
        if(argc==2&&!std::wcscmp(argv[1],L"--child"))return child_test();
        if(argc==3&&!std::wcscmp(argv[1],L"--runtime"))return run_child(argv[2]);
        std::fputs("Usage: SwapchainRuntimeTest.exe --runtime MANIFEST\n",stderr);return 2;
    }catch(std::exception const& error){std::fprintf(stderr,"Swapchain runtime test failed: %s\n",error.what());return 1;}
}
