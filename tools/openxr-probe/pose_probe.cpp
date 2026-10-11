// D3D12-bound session and pose diagnostics; no game mutation, no image submission.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;
XrInstance instance = XR_NULL_HANDLE;
XrSession session = XR_NULL_HANDLE;
XrSpace local = XR_NULL_HANDLE;
XrSpace view_space = XR_NULL_HANDLE;
void xr_check(XrResult result, const char* action) {
    if (XR_FAILED(result)) {
        char text[XR_MAX_RESULT_STRING_SIZE]{};
        if(instance) xrResultToString(instance,result,text);
        throw std::runtime_error(std::string(action)+": "+text+" ("+std::to_string(result)+")");
    }
}
void dx_check(HRESULT result, const char* action) {
    if (FAILED(result)) throw std::runtime_error(std::string(action)+": "+std::to_string(result));
}
#include "test_pattern.h"
void cleanup() {
    if(view_space) xrDestroySpace(view_space);
    if(local) xrDestroySpace(local);
    if(session) xrDestroySession(session);
    if(instance) xrDestroyInstance(instance);
}
int main(int argc, char** argv) {
    if(argc!=3 && argc!=4) {fprintf(stderr,"Usage: OpenXRPoseProbe SECONDS OUTPUT.csv [--render]\n");return 2;}
    bool test_render=argc==4 && !strcmp(argv[3],"--render");
    if(argc==4 && !test_render)return 2;
    int seconds=atoi(argv[1]);
    if(seconds<3 || seconds>60) return 2;
    FILE* csv{};fopen_s(&csv,argv[2],"w");
    if(!csv) return 2;
    fprintf(csv,"frame,predicted_time,session_state,should_render,view_flags,head_flags,head_x,head_y,head_z,qx,qy,qz,qw,left_x,left_y,left_z,right_x,right_y,right_z,left_angle_left,left_angle_right,left_angle_up,left_angle_down\n");
    bool running=false;
    try {
        const char* extensions[]={XR_KHR_D3D12_ENABLE_EXTENSION_NAME};
        XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
        strcpy_s(create.applicationInfo.applicationName,"GearsVR Pose Probe");
        strcpy_s(create.applicationInfo.engineName,"GearsVR Research");
        create.applicationInfo.apiVersion=XR_API_VERSION_1_0;
        create.enabledExtensionCount=1;create.enabledExtensionNames=extensions;
        xr_check(xrCreateInstance(&create,&instance),"xrCreateInstance");
        XrSystemGetInfo system_info{XR_TYPE_SYSTEM_GET_INFO};system_info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrSystemId system{};xr_check(xrGetSystem(instance,&system_info,&system),"xrGetSystem");
        PFN_xrGetD3D12GraphicsRequirementsKHR requirements_fn{};
        xr_check(xrGetInstanceProcAddr(instance,"xrGetD3D12GraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements_fn)),"requirements function");
        XrGraphicsRequirementsD3D12KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
        xr_check(requirements_fn(instance,system,&requirements),"D3D12 requirements");
        ComPtr<IDXGIFactory4> factory;dx_check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
        ComPtr<IDXGIAdapter1> adapter;dx_check(factory->EnumAdapterByLuid(requirements.adapterLuid,IID_PPV_ARGS(&adapter)),"runtime adapter");
        ComPtr<ID3D12Device> device;dx_check(D3D12CreateDevice(adapter.Get(),requirements.minFeatureLevel,IID_PPV_ARGS(&device)),"D3D12 device");
        D3D12_COMMAND_QUEUE_DESC desc{};ComPtr<ID3D12CommandQueue> queue;
        dx_check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)),"D3D12 queue");
        XrGraphicsBindingD3D12KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};binding.device=device.Get();binding.queue=queue.Get();
        XrSessionCreateInfo session_create{XR_TYPE_SESSION_CREATE_INFO};session_create.next=&binding;session_create.systemId=system;
        xr_check(xrCreateSession(instance,&session_create,&session),"xrCreateSession");
        XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};space.poseInReferenceSpace.orientation.w=1;
        space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;xr_check(xrCreateReferenceSpace(session,&space,&local),"LOCAL space");
        space.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;xr_check(xrCreateReferenceSpace(session,&space,&view_space),"VIEW space");
        std::unique_ptr<TestPattern> pattern;
        if(test_render)pattern=std::make_unique<TestPattern>(device.Get(),queue.Get(),argv[2]);
        unsigned frames=0,valid=0,tracked=0,render=0,focused=0;XrSessionState state=XR_SESSION_STATE_UNKNOWN;
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
        bool exiting=false;
        while(!exiting && std::chrono::steady_clock::now()<deadline) {
            XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
            XrResult poll;
            while((poll=xrPollEvent(instance,&event))==XR_SUCCESS) {
                if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                    auto change=reinterpret_cast<XrEventDataSessionStateChanged*>(&event);state=change->state;
                    fprintf(stderr,"session_state=%d\n",int(state));
                    if(state==XR_SESSION_STATE_READY) {
                        XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                        xr_check(xrBeginSession(session,&begin),"xrBeginSession");running=true;
                    } else if(state==XR_SESSION_STATE_STOPPING) {xr_check(xrEndSession(session),"xrEndSession");running=false;}
                    else if(state==XR_SESSION_STATE_EXITING || state==XR_SESSION_STATE_LOSS_PENDING) exiting=true;
                } else if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) exiting=true;
                event={XR_TYPE_EVENT_DATA_BUFFER};
            }
            if(poll!=XR_EVENT_UNAVAILABLE) xr_check(poll,"xrPollEvent");
            if(!running) {std::this_thread::sleep_for(std::chrono::milliseconds(10));continue;}
            XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};XrFrameState frame{XR_TYPE_FRAME_STATE};
            xr_check(xrWaitFrame(session,&wait,&frame),"xrWaitFrame");
            XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};xr_check(xrBeginFrame(session,&begin),"xrBeginFrame");
            XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};locate.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            locate.displayTime=frame.predictedDisplayTime;locate.space=local;
            XrViewState view_state{XR_TYPE_VIEW_STATE};XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};uint32_t view_count=0;
            xr_check(xrLocateViews(session,&locate,&view_state,2,&view_count,views),"xrLocateViews");
            XrSpaceLocation head{XR_TYPE_SPACE_LOCATION};xr_check(xrLocateSpace(view_space,local,frame.predictedDisplayTime,&head),"xrLocateSpace");
            XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=frame.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            XrCompositionLayerProjectionView projection_views[2];
            XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
            const XrCompositionLayerBaseHeader* layers[]={reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection)};
            if(pattern && frame.shouldRender && view_count==2 && (view_state.viewStateFlags&3)==3) {
                pattern->render(views,projection_views);projection.space=local;projection.viewCount=2;projection.views=projection_views;
                end.layerCount=1;end.layers=layers;
            }
            xr_check(xrEndFrame(session,&end),"xrEndFrame");
            ++frames;if(frame.shouldRender)++render;if(state==XR_SESSION_STATE_FOCUSED)++focused;
            if((head.locationFlags&3)==3)++valid;if((head.locationFlags&12)==12)++tracked;
            const auto& p=head.pose;const auto& left=views[0];const auto& right=views[1];
            fprintf(csv,"%u,%lld,%d,%u,%llu,%llu,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
                frames,frame.predictedDisplayTime,int(state),frame.shouldRender,view_state.viewStateFlags,head.locationFlags,
                p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w,
                left.pose.position.x,left.pose.position.y,left.pose.position.z,right.pose.position.x,right.pose.position.y,right.pose.position.z,
                left.fov.angleLeft,left.fov.angleRight,left.fov.angleUp,left.fov.angleDown);
            if(frames%60==0)fflush(csv);
        }
        if(running) {
            xr_check(xrRequestExitSession(session),"xrRequestExitSession");
            auto stop_deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
            while(running && std::chrono::steady_clock::now()<stop_deadline) {
                XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
                if(xrPollEvent(instance,&event)==XR_SUCCESS && event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                    auto change=reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                    if(change->state==XR_SESSION_STATE_STOPPING) {xr_check(xrEndSession(session),"end stopped session");running=false;}
                } else std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        printf("{\"frames\":%u,\"valid_pose_frames\":%u,\"tracked_pose_frames\":%u,\"should_render_frames\":%u,\"focused_frames\":%u,\"last_session_state\":%d,\"submitted_image_layers\":%u,\"left_checksum\":%llu,\"right_checksum\":%llu}\n",
               frames,valid,tracked,render,focused,int(state),pattern?pattern->rendered:0,
               pattern?pattern->checksums[0]:0,pattern?pattern->checksums[1]:0);
        pattern.reset();
        cleanup();fclose(csv);return frames?0:1;
    }catch(std::exception const& error) {fprintf(stderr,"Pose probe error: %s\n",error.what());cleanup();fclose(csv);return 1;}
}
