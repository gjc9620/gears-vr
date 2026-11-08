// Bounded recording of two owned HWNDs. No desktop capture, input, activation,
// window positioning, game hook or gameplay authority.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <wincodec.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <algorithm>
#include <memory>
namespace fs=std::filesystem;
using namespace winrt;
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
long long qpc_ns(){LARGE_INTEGER q{},f{};if(!QueryPerformanceCounter(&q)||!QueryPerformanceFrequency(&f))throw std::runtime_error("QPC");return (long long)((long double)q.QuadPart*1000000000.L/f.QuadPart);}
unsigned long long born(HANDLE h){FILETIME c{},e{},k{},u{};if(!GetProcessTimes(h,&c,&e,&k,&u))throw std::runtime_error("GetProcessTimes");return ((unsigned long long)c.dwHighDateTime<<32)|c.dwLowDateTime;}
unsigned long long wall(){FILETIME t{};GetSystemTimePreciseAsFileTime(&t);return (((unsigned long long)t.dwHighDateTime<<32)|t.dwLowDateTime)-116444736000000000ULL;}
struct Lane {
 const char* name;DWORD pid;unsigned long long creation;HWND hwnd;HANDLE process{};unsigned frames{},max_width{};
 com_ptr<ID3D11Device> device;com_ptr<ID3D11DeviceContext> context;com_ptr<ID3D11Texture2D> staging;
 com_ptr<IWICImagingFactory> wic;IDirect3DDevice capture_device{nullptr};GraphicsCaptureItem item{nullptr};
 Direct3D11CaptureFramePool pool{nullptr};GraphicsCaptureSession capture{nullptr};D3D11_TEXTURE2D_DESC staged{};
 std::ofstream log;long long last_frame{};fs::path output;bool stale_window_recorded{};
 winrt::Windows::Graphics::SizeInt32 pool_size{};unsigned pool_generation{};
 Lane(const char* label,DWORD id,unsigned long long stamp,HWND window,fs::path directory,unsigned limit):name(label),pid(id),creation(stamp),hwnd(window),max_width(limit),output(directory){
  process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid);if(!process||born(process)!=creation)throw std::runtime_error("Capture process identity");
  require();D3D_FEATURE_LEVEL level{};
  check_hresult(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.put(),&level,context.put()));
  com_ptr<IDXGIDevice> dxgi;check_hresult(device->QueryInterface(dxgi.put()));com_ptr<::IInspectable> inspect;
  check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(),inspect.put()));capture_device=inspect.as<IDirect3DDevice>();
  auto interop=get_activation_factory<GraphicsCaptureItem,IGraphicsCaptureItemInterop>();check_hresult(interop->CreateForWindow(hwnd,guid_of<GraphicsCaptureItem>(),put_abi(item)));
  const auto size=item.Size();if(size.Width<=0||size.Height<=0)throw std::runtime_error("Capture item has no size");
  pool_size=size;
  pool=Direct3D11CaptureFramePool::CreateFreeThreaded(capture_device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,pool_size);
  capture=pool.CreateCaptureSession(item);capture.IsCursorCaptureEnabled(false);
  check_hresult(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(wic.put())));
  log.open(output/(std::string(name)+"-frames.jsonl"));if(!log)throw std::runtime_error("Capture manifest unavailable");
  log<<"{\"kind\":\"exact_hwnd_WindowsGraphicsCapture\",\"pid\":"<<pid<<",\"creation_time\":"<<creation<<",\"hwnd\":"<<(uintptr_t)hwnd<<",\"desktop_capture\":false,\"window_mutation\":false}\n";log.flush();
  require();capture.StartCapture();
 }
 void require(){DWORD actual{};GetWindowThreadProcessId(hwnd,&actual);if(!IsWindow(hwnd)||actual!=pid||WaitForSingleObject(process,0)!=WAIT_TIMEOUT||born(process)!=creation)throw std::runtime_error("Owned capture target exited or changed");}
 void observe_first_stale_window(){
  const auto now=qpc_ns();
  if(stale_window_recorded||!frames||now-last_frame<=1000000000LL)return;
  require();DWORD cloaked{};const HRESULT cloak_result=DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
  RECT rect{};const bool have_rect=GetWindowRect(hwnd,&rect)!=FALSE;
  WINDOWPLACEMENT placement{};placement.length=sizeof(placement);const bool have_placement=GetWindowPlacement(hwnd,&placement)!=FALSE;
  require();stale_window_recorded=true;
  // One bounded read-only window snapshot in the existing receipt, not a new
  // capture lane, a recovery/restart or authority to move/show the window.
  log<<"{\"kind\":\"first_stale_window\",\"observed_qpc_ns\":"<<now<<",\"last_capture_qpc_ns\":"<<last_frame
     <<",\"pid\":"<<pid<<",\"creation_time\":"<<creation<<",\"hwnd\":"<<(uintptr_t)hwnd
     <<",\"visible\":"<<(IsWindowVisible(hwnd)?"true":"false")<<",\"iconic\":"<<(IsIconic(hwnd)?"true":"false")
     <<",\"hung\":"<<(IsHungAppWindow(hwnd)?"true":"false")<<",\"window_thread_id\":"<<GetWindowThreadProcessId(hwnd,nullptr)
     <<",\"cloaked_query_succeeded\":"<<(SUCCEEDED(cloak_result)?"true":"false")<<",\"cloaked\":"<<cloaked
     <<",\"placement_query_succeeded\":"<<(have_placement?"true":"false")<<",\"show_command\":"<<placement.showCmd
     <<",\"rect_query_succeeded\":"<<(have_rect?"true":"false")<<",\"rect\":["<<rect.left<<","<<rect.top<<","<<rect.right<<","<<rect.bottom
     <<"],\"window_mutation\":false,\"stale_is_not_render_failure_proof\":true}\n";log.flush();
 }
 void take(){
  require();auto frame=pool.TryGetNextFrame();if(!frame){observe_first_stale_window();return;}
  // WGC queues old frames until they are consumed. Drain bounded pending
  // frames before sampling, so recorder backlog is not mistaken for XR lag.
  for(unsigned n=0;n<4;++n){auto newer=pool.TryGetNextFrame();if(!newer)break;frame.Close();frame=std::move(newer);}
  const auto captured=frame.SystemRelativeTime().count()*100LL;const auto observed=qpc_ns();
  if(captured<=last_frame){frame.Close();return;}last_frame=captured;
  com_ptr<ID3D11Texture2D> source;auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();check_hresult(access->GetInterface(IID_PPV_ARGS(source.put())));
  D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);const auto content=frame.ContentSize();
  if(content.Width<=0||content.Height<=0||content.Width>8192||content.Height>8192||desc.Width>8192||desc.Height>8192)
   throw std::runtime_error("Capture size unsupported");
  // A WGC surface keeps the frame-pool size even when the HWND grows. Never
  // silently save min(surface,content): that presents a top-left crop as the
  // complete application image. Retire this frame before recreating its pool.
  if(content.Width!=pool_size.Width||content.Height!=pool_size.Height||
     desc.Width<unsigned(content.Width)||desc.Height<unsigned(content.Height)){
   log<<"{\"kind\":\"capture_pool_resize_drop\",\"captured_qpc_ns\":"<<captured
      <<",\"old_pool_width\":"<<pool_size.Width<<",\"old_pool_height\":"<<pool_size.Height
      <<",\"content_width\":"<<content.Width<<",\"content_height\":"<<content.Height
      <<",\"surface_width\":"<<desc.Width<<",\"surface_height\":"<<desc.Height
      <<",\"pool_generation\":"<<(pool_generation+1)<<"}\n";log.flush();
   source=nullptr;access=nullptr;frame.Close();frame=nullptr;staging=nullptr;staged={};
   pool_size=content;pool.Recreate(capture_device,DirectXPixelFormat::B8G8R8A8UIntNormalized,2,pool_size);
   ++pool_generation;return;
  }
  const unsigned width=unsigned(content.Width),height=unsigned(content.Height);
  if(!staging||staged.Width!=desc.Width||staged.Height!=desc.Height||staged.Format!=desc.Format){staging=nullptr;staged=desc;auto d=desc;d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d.MiscFlags=0;check_hresult(device->CreateTexture2D(&d,nullptr,staging.put()));}
  context->CopyResource(staging.get(),source.get());D3D11_MAPPED_SUBRESOURCE map{};check_hresult(context->Map(staging.get(),0,D3D11_MAP_READ,0,&map));
  // Bound JPEG/storage work independently of the WGC source dimensions. Read
  // from the mapped original; never resize or move either application's HWND.
  const unsigned saved_width=max_width?(std::min)(width,max_width):width;
  const unsigned saved_height=(std::max)(1u,unsigned((uint64_t(height)*saved_width+width/2)/width));
  std::vector<unsigned char> pixels((size_t)saved_width*saved_height*3);
  for(unsigned y=0;y<saved_height;++y)for(unsigned x=0;x<saved_width;++x){
   const unsigned sy=unsigned(uint64_t(y)*height/saved_height),sx=unsigned(uint64_t(x)*width/saved_width);
   const auto* s=(const unsigned char*)map.pData+(size_t)sy*map.RowPitch+sx*4;
   auto* d=pixels.data()+((size_t)y*saved_width+x)*3;d[0]=s[0];d[1]=s[1];d[2]=s[2];}
  context->Unmap(staging.get(),0);frame.Close();
  char file[80]{};sprintf_s(file,"%s-%05u.jpg",name,frames);const auto path=output/file;
  com_ptr<IWICStream> stream;check_hresult(wic->CreateStream(stream.put()));check_hresult(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE));
  com_ptr<IWICBitmapEncoder> encoder;check_hresult(wic->CreateEncoder(GUID_ContainerFormatJpeg,nullptr,encoder.put()));check_hresult(encoder->Initialize(stream.get(),WICBitmapEncoderNoCache));
  com_ptr<IWICBitmapFrameEncode> encode;check_hresult(encoder->CreateNewFrame(encode.put(),nullptr));check_hresult(encode->Initialize(nullptr));check_hresult(encode->SetSize(saved_width,saved_height));
  WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;check_hresult(encode->SetPixelFormat(&format));if(format!=GUID_WICPixelFormat24bppBGR)throw std::runtime_error("JPEG pixel format");
  check_hresult(encode->WritePixels(saved_height,saved_width*3,(UINT)pixels.size(),pixels.data()));check_hresult(encode->Commit());check_hresult(encoder->Commit());
  log<<"{\"file\":\""<<file<<"\",\"captured_qpc_ns\":"<<captured<<",\"observed_qpc_ns\":"<<observed<<",\"saved_qpc_ns\":"<<qpc_ns()<<",\"saved_unix_100ns\":"<<wall()<<",\"width\":"<<saved_width<<",\"height\":"<<saved_height<<",\"original_width\":"<<width<<",\"original_height\":"<<height
     <<",\"content_width\":"<<content.Width<<",\"content_height\":"<<content.Height
     <<",\"surface_width\":"<<desc.Width<<",\"surface_height\":"<<desc.Height
     <<",\"pool_generation\":"<<pool_generation<<"}\n";log.flush();++frames;
 }
 ~Lane(){try{if(capture)capture.Close();if(pool)pool.Close();}catch(...){}if(process)CloseHandle(process);}
};
int wmain(int argc,wchar_t** argv){
 try {
  if(argc!=9&&argc!=11)throw std::runtime_error("Expected gamePid gameCreation gameHwnd xrPid xrCreation xrHwnd NEWoutput durationSeconds [sampleFps maxSavedWidth]");
  const auto seconds=std::stoul(argv[8]);if(!seconds||seconds>600)throw std::runtime_error("Explicit recording duration must be 1..600 seconds");
  const auto fps=argc==11?std::stoul(argv[9]):8ul;
  const auto max_width=argc==11?std::stoul(argv[10]):0ul;
  if(!fps||fps>60||(max_width&&(max_width<320||max_width>1920)))throw std::runtime_error("Recording bound: fps1..60, saved width320..1920 or0(original)");
  fs::path out=fs::absolute(argv[7]);if(!fs::create_directory(out))throw std::runtime_error("Output must be a new directory");
  init_apartment(apartment_type::multi_threaded);
  Lane game("game",std::stoul(argv[1]),std::stoull(argv[2]),(HWND)(uintptr_t)std::stoull(argv[3]),out,unsigned(max_width));
  const auto xr_pid=std::stoul(argv[4]);const auto xr_creation=std::stoull(argv[5]);const auto xr_hwnd=std::stoull(argv[6]);
  const bool game_only=xr_pid==0&&xr_creation==0&&xr_hwnd==0;
  if(!game_only&&(!xr_pid||!xr_creation||!xr_hwnd))throw std::runtime_error("Optional XR lane must provide all identity fields, or all zero for a physical user's game-window recording");
  std::unique_ptr<Lane> simulator;
  if(!game_only)simulator=std::make_unique<Lane>("simulator",DWORD(xr_pid),xr_creation,(HWND)(uintptr_t)xr_hwnd,out,unsigned(max_width));
  std::ofstream(out/"ready.json")<<"{\"ready\":true,\"desktop_capture\":false,\"window_mutation\":false,\"game_only\":"<<(game_only?"true":"false")<<",\"sample_fps_limit\":"<<fps<<",\"max_saved_width\":"<<max_width<<"}";
  const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
  bool game_exited=false;
  while(std::chrono::steady_clock::now()<end&&!fs::exists(out/"stop")){
   // A human ending their game is normal recording completion. Never send
   // input, stop the game or attach another capture after identity changes.
   if(game_only&&WaitForSingleObject(game.process,0)==WAIT_OBJECT_0){game_exited=true;break;}
   const auto next=std::chrono::steady_clock::now()+std::chrono::nanoseconds(1000000000/fps);
   try{game.take();if(simulator)simulator->take();}
   catch(...){
    // Closing the human's game can race the require/take boundary. Only an
    // actually signalled original process handle establishes normal exit;
    // window replacement, PID reuse and other capture failures still fail.
    if(game_only&&WaitForSingleObject(game.process,0)==WAIT_OBJECT_0){game_exited=true;break;}
    throw;
   }
   std::this_thread::sleep_until(next);
  }
  const bool action_ended=fs::exists(out/"stop")||game_exited;
  std::ofstream(out/"recording-ended.json")<<"{\"ended_by\":\""<<(game_exited?"game_exit":(action_ended?"owned_stop":"duration_limit"))
   <<"\",\"action_recording_complete\":"<<(action_ended?"true":"false")<<",\"duration_limit_seconds\":"<<seconds<<"}";
  printf("{\"game_frames\":%u,\"simulator_frames\":%u,\"game_only\":%s,\"window_mutation\":false,\"action_recording_complete\":%s}\n",game.frames,simulator?simulator->frames:0,game_only?"true":"false",action_ended?"true":"false");
  if(!action_ended)return 3; // A truncated recording cannot certify the full action flow.
  return game.frames>1&&(!simulator||simulator->frames>1)?0:2;
 }catch(hresult_error const& e){fprintf(stderr,"Capture HRESULT %08lx\n",(unsigned long)e.code());return 1;}
 catch(std::exception const& e){fprintf(stderr,"%s\n",e.what());return 1;}
}
