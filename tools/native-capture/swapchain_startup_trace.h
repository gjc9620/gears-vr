#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <windows.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include "background_zero_extent_policy.h"
#if defined(GEARS_BACKGROUND_ZERO_EXTENT_CONTROL) && GEARS_BACKGROUND_ZERO_EXTENT_CONTROL && (!defined(GEARS_SWAPCHAIN_STARTUP_TRACE) || !GEARS_SWAPCHAIN_STARTUP_TRACE)
#error Background zero extent control requires startup trace
#endif

// Independent CPU evidence. No transport begin/finish, retained resources,
// or window mutations. The separately gated private control returns effective
// dimensions; the caller still owns exactly one original resize invocation.
namespace gears_swapchain_startup {
constexpr unsigned capacity=64, invalid=capacity;
enum Method:unsigned {create_legacy,create_hwnd,create_composition,resize,resize1};
enum Reason:unsigned {not_queried,api_failed,no_chain,desc_failed,buffer_failed,buffer_not_d3d12,complete};
struct PreResize {
 HRESULT desc_result{E_PENDING},buffer_result{E_PENDING},resource_result{E_PENDING};
 DXGI_SWAP_CHAIN_DESC desc{};D3D12_RESOURCE_DESC buffer{};
 gears_background_zero_extent::Evidence evidence{};
 unsigned effective_width{},effective_height{},substituted{};
};
struct Row {
 unsigned method{},prime{},running{},request_bytes{},request_readable{},full_bytes{},full_readable{};
 unsigned width{},height{},count{},format{},flags{},stack_count{},reason{};
 DWORD pid{},thread{};uint64_t creation{};uintptr_t chain{},requested_hwnd{},caller{},module{},caller_rva{};
 unsigned caller_in_module{};long long begin{},end{},frequency{};
 HRESULT result{E_PENDING},desc_result{E_PENDING},buffer_result{E_PENDING},resource_result{E_PENDING};
 std::array<unsigned char,128> request{};std::array<unsigned char,64> fullscreen{};
 std::array<uintptr_t,7> arguments{};
 std::array<void*,8> stack{};DXGI_SWAP_CHAIN_DESC desc{};D3D12_RESOURCE_DESC buffer{};
 PreResize pre{};
};
struct Export {std::array<bool,capacity> emitted{};bool summary{};};
template<unsigned N> struct Store {
 std::array<Row,N> rows{};std::array<std::atomic<bool>,N> ready{};std::atomic<unsigned> attempts{};
 unsigned reserve(){auto i=attempts.fetch_add(1,std::memory_order_relaxed);return i<N?i:invalid;}
 void publish(unsigned i){if(i<N)ready[i].store(true,std::memory_order_release);}
};
inline bool succeeded(HRESULT result){return result>=0;}
inline bool tiny(unsigned width,unsigned height){return width&&height&&width<=8&&height<=8;}
// The failed/absent boundary is shared with synthetic tests; queries are never
// authorized by a failed HRESULT, even when a caller supplied a non-null value.
template<class Query> inline void observe_result(Row& row,HRESULT result,bool chain_present,Query query){
 row.result=result;row.reason=api_failed;
 if(succeeded(result)){row.reason=no_chain;if(chain_present)query(row);}
}
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
inline Store<capacity> records;
inline void window_evidence(HWND hwnd,gears_background_zero_extent::Evidence& e){
 DWORD owner{};e.owned_hwnd=hwnd&&GetWindowThreadProcessId(hwnd,&owner)&&owner==GetCurrentProcessId();
 if(!e.owned_hwnd)return;
 const HWND foreground=GetForegroundWindow();DWORD foreground_pid{};
 e.background_verified=foreground&&GetWindowThreadProcessId(foreground,&foreground_pid)&&foreground_pid&&foreground_pid!=owner;
 e.iconic=IsIconic(hwnd)!=FALSE;RECT client{};
 e.client_valid=GetClientRect(hwnd,&client)&&client.right>=client.left&&client.bottom>=client.top;
 if(e.client_valid){e.client_width=static_cast<unsigned>(client.right-client.left);e.client_height=static_cast<unsigned>(client.bottom-client.top);}
 // Fail closed if foreground changed while sampling.
 e.background_verified=e.background_verified&&foreground==GetForegroundWindow();
}
template<class WindowQuery> inline gears_background_zero_extent::Decision prepare_resize_with(
 unsigned index,IDXGISwapChain* chain,UINT width,UINT height,bool outermost,WindowQuery query){
 const DWORD saved=GetLastError();gears_background_zero_extent::Decision decision{width,height,false};
 // A full trace cannot safely authorize an unrecorded control operation.
 if(index==invalid||!chain)return decision;
 auto& p=records.rows[index].pre;p=PreResize{};auto& e=p.evidence;e.outermost=outermost;
#if defined(GEARS_BACKGROUND_ZERO_EXTENT_CONTROL) && GEARS_BACKGROUND_ZERO_EXTENT_CONTROL
 e.trial_enabled=true;
#endif
 p.desc_result=chain->GetDesc(&p.desc);
 if(succeeded(p.desc_result)){
  e.windowed=p.desc.Windowed!=FALSE;query(p.desc.OutputWindow,e);
  IUnknown* object{};p.buffer_result=chain->GetBuffer(0,__uuidof(IUnknown),reinterpret_cast<void**>(&object));
  if(succeeded(p.buffer_result)&&object){
   ID3D12Resource* resource{};p.resource_result=object->QueryInterface(__uuidof(ID3D12Resource),reinterpret_cast<void**>(&resource));
   if(succeeded(p.resource_result)&&resource){p.buffer=resource->GetDesc();resource->Release();}
   object->Release();
  }
  e.physical_verified=succeeded(p.buffer_result)&&succeeded(p.resource_result)&&
   p.buffer.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&p.desc.BufferCount&&
   p.buffer.Width==p.desc.BufferDesc.Width&&p.buffer.Height==p.desc.BufferDesc.Height&&p.buffer.Format==p.desc.BufferDesc.Format;
  if(e.physical_verified){e.physical_width=p.desc.BufferDesc.Width;e.physical_height=p.desc.BufferDesc.Height;}
 }
 decision=gears_background_zero_extent::resolve(width,height,e);
 p.effective_width=decision.width;p.effective_height=decision.height;p.substituted=decision.substituted;
 SetLastError(saved);return decision;
}
inline gears_background_zero_extent::Decision prepare_resize(unsigned index,IDXGISwapChain* chain,UINT width,UINT height,bool outermost){
 return prepare_resize_with(index,chain,width,height,outermost,window_evidence);
}
inline bool copy_bytes(void* out,const void* source,size_t size){
 if(!source)return false;
 __try {std::memcpy(out,source,size);return true;} __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline unsigned begin(Method method,const void* request,unsigned bytes,HWND hwnd,void* caller,bool prime,bool running,
                      const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen=nullptr){
 const DWORD last_error=GetLastError();const auto index=records.reserve();if(index==invalid)return index;
 auto& row=records.rows[index];row.method=method;row.prime=prime;row.running=running;
 row.pid=GetCurrentProcessId();row.thread=GetCurrentThreadId();row.requested_hwnd=reinterpret_cast<uintptr_t>(hwnd);
 row.request_bytes=bytes;row.request_readable=bytes<=row.request.size()&&copy_bytes(row.request.data(),request,bytes);
 if(fullscreen){row.full_bytes=sizeof(*fullscreen);row.full_readable=copy_bytes(row.fullscreen.data(),fullscreen,sizeof(*fullscreen));}
 FILETIME birth{},exit{},kernel{},user{};
 if(GetProcessTimes(GetCurrentProcess(),&birth,&exit,&kernel,&user))row.creation=(uint64_t(birth.dwHighDateTime)<<32)|birth.dwLowDateTime;
 LARGE_INTEGER qpc{},frequency{};if(QueryPerformanceCounter(&qpc))row.begin=qpc.QuadPart;
 if(QueryPerformanceFrequency(&frequency))row.frequency=frequency.QuadPart;
 row.caller=reinterpret_cast<uintptr_t>(caller);row.module=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
 if(row.module&&copy_bytes(&dos,reinterpret_cast<void*>(row.module),sizeof(dos))&&dos.e_magic==IMAGE_DOS_SIGNATURE&&
    dos.e_lfanew>0&&dos.e_lfanew<0x100000&&copy_bytes(&nt,reinterpret_cast<void*>(row.module+dos.e_lfanew),sizeof(nt))&&
    nt.Signature==IMAGE_NT_SIGNATURE&&row.caller>=row.module&&row.caller-row.module<nt.OptionalHeader.SizeOfImage){
  row.caller_in_module=1;row.caller_rva=row.caller-row.module;
 }
 row.stack_count=CaptureStackBackTrace(1,static_cast<DWORD>(row.stack.size()),row.stack.data(),nullptr);
 SetLastError(last_error);
 return index;
}
inline unsigned begin_factory(Method method,void* factory,IUnknown* input,HWND hwnd,const void* desc,unsigned bytes,
 const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* full,IDXGIOutput* output,void* out,void* caller,bool prime,bool running){
 auto index=begin(method,desc,bytes,hwnd,caller,prime,running,full);
 if(index!=invalid){auto& row=records.rows[index];row.arguments={reinterpret_cast<uintptr_t>(factory),reinterpret_cast<uintptr_t>(input),
  reinterpret_cast<uintptr_t>(hwnd),reinterpret_cast<uintptr_t>(desc),reinterpret_cast<uintptr_t>(full),
  reinterpret_cast<uintptr_t>(output),reinterpret_cast<uintptr_t>(out)};
  if(row.request_readable&&method==create_legacy&&bytes==sizeof(DXGI_SWAP_CHAIN_DESC)){
   DXGI_SWAP_CHAIN_DESC value{};std::memcpy(&value,row.request.data(),sizeof(value));
   row.width=value.BufferDesc.Width;row.height=value.BufferDesc.Height;row.count=value.BufferCount;
   row.format=unsigned(value.BufferDesc.Format);row.flags=value.Flags;row.requested_hwnd=reinterpret_cast<uintptr_t>(value.OutputWindow);
  }else if(row.request_readable&&method!=create_legacy&&bytes==sizeof(DXGI_SWAP_CHAIN_DESC1)){
   DXGI_SWAP_CHAIN_DESC1 value{};std::memcpy(&value,row.request.data(),sizeof(value));
   row.width=value.Width;row.height=value.Height;row.count=value.BufferCount;row.format=unsigned(value.Format);row.flags=value.Flags;
  }
 }
 return index;
}
inline unsigned begin_resize(Method method,IDXGISwapChain* chain,UINT count,UINT width,UINT height,DXGI_FORMAT format,UINT flags,
                             void* caller,bool prime,bool running,const UINT* nodes=nullptr,IUnknown* const* queues=nullptr){
 struct Raw {UINT count,width,height,format,flags,padding;uintptr_t nodes,queues;};
 const Raw raw{count,width,height,UINT(format),flags,0,reinterpret_cast<uintptr_t>(nodes),reinterpret_cast<uintptr_t>(queues)};
 const auto index=begin(method,&raw,sizeof(raw),nullptr,caller,prime,running);
 if(index!=invalid){auto& r=records.rows[index];r.chain=reinterpret_cast<uintptr_t>(chain);
  r.count=count;r.width=width;r.height=height;r.format=UINT(format);r.flags=flags;}
 return index;
}
inline void finish(unsigned index,HRESULT result,IDXGISwapChain* chain){
 if(index==invalid)return;const DWORD last_error=GetLastError();auto& row=records.rows[index];
 row.chain=reinterpret_cast<uintptr_t>(chain);
 // Failed APIs do not authorize interpreting an output pointer or querying it.
 observe_result(row,result,chain!=nullptr,[&](Row& row){
   row.desc_result=chain->GetDesc(&row.desc);row.reason=desc_failed;
   if(succeeded(row.desc_result)){
    IUnknown* object{};row.buffer_result=chain->GetBuffer(0,__uuidof(IUnknown),reinterpret_cast<void**>(&object));row.reason=buffer_failed;
    if(succeeded(row.buffer_result)&&object){ID3D12Resource* resource{};
     row.resource_result=object->QueryInterface(__uuidof(ID3D12Resource),reinterpret_cast<void**>(&resource));row.reason=buffer_not_d3d12;
     if(succeeded(row.resource_result)&&resource){row.buffer=resource->GetDesc();row.reason=complete;resource->Release();}
     object->Release();
    }
   }
 });
 LARGE_INTEGER qpc{};if(QueryPerformanceCounter(&qpc))row.end=qpc.QuadPart;records.publish(index);SetLastError(last_error);
}
inline bool print(FILE* file,Export* cursor=nullptr){
 if(!file)return false;bool wrote=false;
 if(!cursor||!cursor->summary){
  fprintf(file,"startup_swapchain_summary enabled=1 attempts_at_export=%u capacity=64 reason_codes=0:not_queried,1:api_failed,2:no_chain,3:desc_failed,4:buffer_failed,5:buffer_not_d3d12,6:complete\n",records.attempts.load());
  if(cursor)cursor->summary=true;wrote=true;
 }
 for(unsigned i=0;i<capacity;++i)if((!cursor||!cursor->emitted[i])&&records.ready[i].load(std::memory_order_acquire)){
  const auto& r=records.rows[i];
  fprintf(file,"startup_swapchain row=%u method=%u prime=%u running=%u pid=%lu creation=%llu thread=%lu qpc=%lld,%lld frequency=%lld chain=%llx requested_hwnd=%llx caller=%llx module=%llx caller_in_module=%u caller_rva=%llx result=%08lx reason=%u request_bytes=%u request_readable=%u raw=",
   i,r.method,r.prime,r.running,r.pid,r.creation,r.thread,r.begin,r.end,r.frequency,(unsigned long long)r.chain,(unsigned long long)r.requested_hwnd,(unsigned long long)r.caller,(unsigned long long)r.module,r.caller_in_module,(unsigned long long)r.caller_rva,(unsigned long)r.result,r.reason,r.request_bytes,r.request_readable);
  if(r.request_readable)for(unsigned j=0;j<r.request_bytes;++j)fprintf(file,"%02x",r.request[j]);
  fprintf(file," arguments=");for(unsigned j=0;j<r.arguments.size();++j)fprintf(file,"%s%llx",j?",":"",(unsigned long long)r.arguments[j]);
  fprintf(file," fullscreen_bytes=%u fullscreen_readable=%u fullscreen_raw=",r.full_bytes,r.full_readable);
  if(r.full_readable)for(unsigned j=0;j<r.full_bytes;++j)fprintf(file,"%02x",r.fullscreen[j]);
  fprintf(file," request_count_width_height_format_flags=%u,%u,%u,%u,%u desc_hr=%08lx desc_size=%u,%u desc_hwnd=%llx buffer_hr=%08lx resource_hr=%08lx buffer_dimension=%u buffer_size=%llu,%u buffer_format=%u stack=",
   r.count,r.width,r.height,r.format,r.flags,(unsigned long)r.desc_result,r.desc.BufferDesc.Width,r.desc.BufferDesc.Height,(unsigned long long)reinterpret_cast<uintptr_t>(r.desc.OutputWindow),(unsigned long)r.buffer_result,(unsigned long)r.resource_result,unsigned(r.buffer.Dimension),r.buffer.Width,r.buffer.Height,unsigned(r.buffer.Format));
  for(unsigned j=0;j<r.stack_count;++j)fprintf(file,"%s%llx",j?",":"",(unsigned long long)reinterpret_cast<uintptr_t>(r.stack[j]));
  fprintf(file," desc_raw=");if(succeeded(r.desc_result)){
   const auto* raw=reinterpret_cast<const unsigned char*>(&r.desc);for(unsigned j=0;j<sizeof(r.desc);++j)fprintf(file,"%02x",raw[j]);}
  fprintf(file," buffer_raw=");if(r.reason==complete){
   const auto* raw=reinterpret_cast<const unsigned char*>(&r.buffer);for(unsigned j=0;j<sizeof(r.buffer);++j)fprintf(file,"%02x",raw[j]);}
  const auto& p=r.pre;const auto& e=p.evidence;
  fprintf(file," pre_desc_hr=%08lx pre_desc_size=%u,%u pre_hwnd=%llx pre_windowed=%u pre_buffer_hr=%08lx pre_resource_hr=%08lx pre_buffer_dimension=%u pre_buffer_size=%llu,%u pre_buffer_format=%u pre_physical_verified=%u pre_owned=%u pre_background=%u pre_iconic=%u pre_client_valid=%u pre_client_size=%u,%u control_enabled=%u control_outermost=%u effective_size=%u,%u substituted=%u",
   (unsigned long)p.desc_result,p.desc.BufferDesc.Width,p.desc.BufferDesc.Height,(unsigned long long)reinterpret_cast<uintptr_t>(p.desc.OutputWindow),unsigned(e.windowed),
   (unsigned long)p.buffer_result,(unsigned long)p.resource_result,unsigned(p.buffer.Dimension),p.buffer.Width,p.buffer.Height,unsigned(p.buffer.Format),unsigned(e.physical_verified),
   unsigned(e.owned_hwnd),unsigned(e.background_verified),unsigned(e.iconic),unsigned(e.client_valid),e.client_width,e.client_height,unsigned(e.trial_enabled),unsigned(e.outermost),p.effective_width,p.effective_height,p.substituted);
  fputc('\n',file);
  if(cursor)cursor->emitted[i]=true;wrote=true;
 }
 return wrote;
}
#else
inline gears_background_zero_extent::Decision prepare_resize(unsigned,IDXGISwapChain*,UINT width,UINT height,bool){return {width,height,false};}
inline unsigned begin(Method,const void*,unsigned,HWND,void*,bool,bool,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* =nullptr){return invalid;}
inline unsigned begin_factory(Method,void*,IUnknown*,HWND,const void*,unsigned,const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,void*,void*,bool,bool){return invalid;}
inline unsigned begin_resize(Method,IDXGISwapChain*,UINT,UINT,UINT,DXGI_FORMAT,UINT,void*,bool,bool,const UINT* =nullptr,IUnknown* const* =nullptr){return invalid;}
inline void finish(unsigned,HRESULT,IDXGISwapChain*){}
inline bool print(FILE*,Export* =nullptr){return false;}
#endif
}
