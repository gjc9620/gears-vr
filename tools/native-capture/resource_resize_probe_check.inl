#if defined(GEARS_NATIVE_RESIZE_PROBE) && GEARS_NATIVE_RESIZE_PROBE
namespace {
volatile LONG resize_test_calls{};
__declspec(noinline) uintptr_t __fastcall fake_resource_resize(void* object,uint32_t width,uint32_t height){
 InterlockedIncrement(&resize_test_calls);
 auto dims=reinterpret_cast<uint32_t*>(static_cast<char*>(object)+0xf8);dims[0]=width;dims[1]=height;
 gears_resize_probe::created(5,S_OK);
 return 0x97531fed00000000ull^(uint64_t(width)<<32)^height;
}
}
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureResizeProbeCheck(void*){
 using namespace resource_resize_probe;
 if(buffer.attempts.load()||active.load())return 1;
 std::array<unsigned char,0x100> object{};std::array<unsigned char,32> before{},after{};
 void* target=reinterpret_cast<void*>(fake_resource_resize);
 if(!safe_copy(before.data(),target,before.size())||MH_Initialize()!=MH_OK)return 2;
 if(MH_CreateHook(target,reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original))!=MH_OK)return 3;
 if(MH_EnableHook(target)!=MH_OK)return 4;
 recording=true;self_testing=false;base=0;DWORD error{};
 for(uint32_t i=0;i<4098;++i){const uint32_t w=2048+i,h=4096+i;
  auto result=fake_resource_resize(object.data(),w,h);
  if(result!=(0x97531fed00000000ull^(uint64_t(w)<<32)^h)){error=5;break;}
 }
 unsigned expected_calls=4098;
#if defined(GEARS_NATIVE_RESIZE_REUSE) && GEARS_NATIVE_RESIZE_REUSE
 // Exercise the real detour's policy even AFTER observation capacity is full.
 // The fake function returns a value derived from the forwarded dimensions.
 auto dims=reinterpret_cast<uint32_t*>(object.data()+0xf8);dims[0]=3840;dims[1]=2160;
 // The standalone fixture has no game globals. Supply a readable zero mode
 // at the same relative address without pointing at or changing a game object.
 uint32_t fixture_mode{};base=reinterpret_cast<uintptr_t>(&fixture_mode)-globals[0];
 consuming_renderer=object.data();consuming_frame_ticket=1;consuming_owned_scene=true;
 {gears_render_cost::Window no_profile(nullptr); // Profiling exhausted; ownership remains explicit.
  auto result=fake_resource_resize(object.data(),4096,2048);
  if(result!=(0x97531fed00000000ull^(uint64_t(4096)<<32)^2160)||dims[0]!=4096||dims[1]!=2160)error=9;
 }
 consuming_renderer=nullptr;consuming_frame_ticket=0;consuming_owned_scene=false;
 if(fake_resource_resize(object.data(),3840,2160)!=(0x97531fed00000000ull^(uint64_t(4096)<<32)^2160))error=10;
 expected_calls+=2;
#endif
 recording=false;
 if(MH_DisableHook(target)!=MH_OK||!safe_copy(after.data(),target,after.size())||before!=after)error=6;
 if(active.load()||resize_test_calls!=LONG(expected_calls)||buffer.attempts!=expected_calls)error=7;
 for(size_t i=0;i<buffer.rows.size();++i){const auto& row=buffer.rows[i];
  if(!buffer.ready[i].load()||row.object!=reinterpret_cast<uintptr_t>(object.data())||row.width!=2048+i||row.height!=4096+i||
     row.after.width!=row.width||row.after.height!=row.height||row.calls[5]!=1||row.failed[5]||row.end<row.begin)error=8;
 }
 MH_Uninitialize();return error;
}
#else
extern "C" __declspec(dllexport) DWORD WINAPI GearsCaptureResizeProbeCheck(void*){return ERROR_NOT_SUPPORTED;}
#endif
