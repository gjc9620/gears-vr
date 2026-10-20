#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc==3&&!wcscmp(argv[1],L"--prime-owner-child")){
        HANDLE event=OpenEventW(SYNCHRONIZE,FALSE,argv[2]);if(!event)return 240;
        DWORD result=WaitForSingleObject(event,30000);CloseHandle(event);return result==WAIT_OBJECT_0?0:241;
    }
    if(argc!=2 && argc!=3) return 2;
    bool ui_bypass=argc==3&&!wcscmp(argv[2],L"--ui-bypass");
    bool ui_material_bypass=argc==3&&!wcscmp(argv[2],L"--ui-material-bypass");
    bool ui_root_bypass=argc==3&&!wcscmp(argv[2],L"--ui-root-bypass");
    bool viewport=argc==3 && !wcscmp(argv[2],L"--viewport");
    bool prime=argc==3 && !wcscmp(argv[2],L"--prime");
    bool ngx_mv_descriptors=argc==3&&!wcscmp(argv[2],L"--ngx-mv-descriptors");
    bool handoff=argc==3 && !wcscmp(argv[2],L"--prime-handoff");
    bool owned_cancel=argc==3&&!wcscmp(argv[2],L"--prime-owned-cancel");
    bool owned_exit=argc==3&&!wcscmp(argv[2],L"--prime-owned-exit");
    bool owned_handoff=argc==3&&!wcscmp(argv[2],L"--prime-owned-handoff");
    bool quad=argc==3 && !wcscmp(argv[2],L"--quad");
    bool geometry=argc==3 && !wcscmp(argv[2],L"--geometry");
    bool identity=argc==3 && !wcscmp(argv[2],L"--input-resource");
    bool color_copy=argc==3 && !wcscmp(argv[2],L"--color-copy");
    bool color_copy_system=argc==3 && !wcscmp(argv[2],L"--color-copy-system");
    bool first_view_mask=argc==3 && !wcscmp(argv[2],L"--first-view-mask");
    bool view_union_mask=argc==3 && !wcscmp(argv[2],L"--view-union-mask");
    bool frame_link=argc==3 && !wcscmp(argv[2],L"--frame-link");
    bool continuous=argc==3 && !wcscmp(argv[2],L"--continuous");
    if(ui_bypass||ui_material_bypass||ui_root_bypass||quad||geometry||identity||color_copy||color_copy_system||first_view_mask||view_union_mask||frame_link||continuous||owned_cancel||owned_exit||owned_handoff||ngx_mv_descriptors)argc=2;
    bool gpu=argc==3 && !wcscmp(argv[2],L"--gpu"),family=argc==3 && !wcscmp(argv[2],L"--family"),clone=argc==3 && !wcscmp(argv[2],L"--clone"),renderer=argc==3 && !wcscmp(argv[2],L"--renderer"),lifetime=argc==3 && !wcscmp(argv[2],L"--lifetime"),prepare=argc==3 && !wcscmp(argv[2],L"--prepare"),state=argc==3 && !wcscmp(argv[2],L"--state");if(argc==3 && !gpu && !family && !clone && !renderer && !lifetime && !prepare && !state && !viewport && !prime && !handoff)return 2;
    HMODULE module=LoadLibraryW(argv[1]);
    if(!module) {printf("Load failed %lu\n",GetLastError());return 3;}
    if(ngx_mv_descriptors){
        auto descriptor_test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(module,"GearsCaptureNgxMvDescriptorsCheck"));
        if(!descriptor_test)return 4;
        DWORD result=descriptor_test(nullptr);
        if(result==ERROR_NOT_SUPPORTED)printf("NGX MV descriptor self-test unsupported: observer flag OFF\n");
        else printf("NGX MV descriptor self-test result=%lu (0=pass)\n",result);
        return static_cast<int>(result);
    }
    if(owned_cancel||owned_exit||owned_handoff){
        auto owned_test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(module,"GearsCapturePrimeOwnedCheck"));
        if(!owned_test)return 4;
        DWORD mode=owned_exit?1:owned_handoff?2:0;DWORD result=owned_test(&mode);
        printf("Owned prime self-test mode=%lu result=%lu (0=pass)\n",mode,result);return static_cast<int>(result);
    }
    auto test=reinterpret_cast<DWORD(WINAPI*)(void*)>(GetProcAddress(module,ui_root_bypass?"GearsCaptureUiRootBypassCheck":ui_material_bypass?"GearsCaptureUiMaterialBypassCheck":ui_bypass?"GearsCaptureUiBypassCheck":continuous?"GearsCaptureContinuousCheck":frame_link?"GearsCaptureFrameLinkCheck":view_union_mask?"GearsCaptureViewUnionMaskCheck":first_view_mask?"GearsCaptureFirstViewMaskCheck":color_copy_system?"GearsCaptureColorCopySystemCheck":color_copy?"GearsCaptureColorCopyCheck":identity?"GearsCaptureResourceIdentityCheck":geometry?"GearsCaptureGeometryCheck":quad?"GearsCaptureQuadCheck":handoff?"GearsCapturePrimeHandoffCheck":prime?"GearsCapturePrimeCheck":viewport?"GearsCaptureViewportCheck":gpu?"GearsCaptureGPUCheck":family?"GearsCaptureFamilyCheck":clone?"GearsCaptureCloneCheck":renderer?"GearsCaptureRendererCheck":lifetime?"GearsCaptureLifetimeCheck":prepare?"GearsCapturePrepareCheck":state?"GearsCaptureStateCheck":"GearsCaptureSelfTest"));
    if(!test) return 4;
    DWORD result=test(nullptr);
    printf("Capture ABI/self-test result=%lu (0=pass)\n",result);
    // The instrumentation DLL deliberately remains resident until process exit.
    return static_cast<int>(result);
}
