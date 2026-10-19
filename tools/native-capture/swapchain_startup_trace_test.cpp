#include "swapchain_startup_trace.h"
#include <cassert>
#include <thread>
#include <vector>
using namespace gears_swapchain_startup;
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
struct Resource final:ID3D12Resource {
 ULONG refs=1;bool supported=true;D3D12_RESOURCE_DESC desc{};
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{*out=nullptr;if(id==__uuidof(IUnknown)||(supported&&id==__uuidof(ID3D12Resource))){*out=this;AddRef();return S_OK;}return E_NOINTERFACE;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{return --refs;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,UINT,const void*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE SetName(LPCWSTR)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID,void**)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE Map(UINT,const D3D12_RANGE*,void**)override{return E_NOTIMPL;}
 void STDMETHODCALLTYPE Unmap(UINT,const D3D12_RANGE*)override{}
 D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc()override{return desc;}
 D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE GetGPUVirtualAddress()override{return 0;}
 HRESULT STDMETHODCALLTYPE WriteToSubresource(UINT,const D3D12_BOX*,const void*,UINT,UINT)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE ReadFromSubresource(void*,UINT,UINT,UINT,const D3D12_BOX*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetHeapProperties(D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS*)override{return E_NOTIMPL;}
};
struct Chain final:IDXGISwapChain {
 Resource resource;HRESULT desc_hr=S_OK,buffer_hr=S_OK;unsigned desc_calls{},buffer_calls{};DXGI_SWAP_CHAIN_DESC desc{};
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override{return E_NOTIMPL;}
 ULONG STDMETHODCALLTYPE AddRef()override{return 1;}ULONG STDMETHODCALLTYPE Release()override{return 1;}
 HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,UINT,const void*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetParent(REFIID,void**)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetDevice(REFIID,void**)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE Present(UINT,UINT)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetBuffer(UINT index,REFIID id,void** out)override{++buffer_calls;assert(index==0);*out=nullptr;return FAILED(buffer_hr)?buffer_hr:resource.QueryInterface(id,out);}
 HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL,IDXGIOutput*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL*,IDXGIOutput**)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* out)override{++desc_calls;if(SUCCEEDED(desc_hr))*out=desc;return desc_hr;}
 HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT,UINT,UINT,DXGI_FORMAT,UINT)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput**)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS*)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT*)override{return E_NOTIMPL;}
};
#endif
int main(){
 // This boundary must never query a chain after API failure, even if non-null.
 Row failed{};unsigned queries{};
 observe_result(failed,E_FAIL,true,[&](Row&){++queries;});
 assert(!queries&&failed.reason==api_failed&&failed.desc_result==E_PENDING&&failed.buffer.Width==0);
 observe_result(failed,S_OK,false,[&](Row&){++queries;});assert(!queries&&failed.reason==no_chain);
 // Descriptor and actual buffer size are independent observations. Zero-size
 // requests do not mean tiny buffers, nor does a GetDesc size prove buffer size.
 Row zero{};zero.width=zero.height=0;
 observe_result(zero,S_OK,true,[&](Row& r){++queries;r.desc.BufferDesc.Width=8;r.desc.BufferDesc.Height=8;
  r.desc_result=S_OK;r.buffer.Width=1920;r.buffer.Height=1080;r.reason=complete;});
 assert(!tiny(zero.width,zero.height)&&tiny(zero.desc.BufferDesc.Width,zero.desc.BufferDesc.Height));
 assert(!tiny(unsigned(zero.buffer.Width),zero.buffer.Height)&&zero.reason==complete);
 Store<2> bounded;auto first=bounded.reserve(),second=bounded.reserve();assert(first==0&&second==1);
 assert(!bounded.ready[0].load());bounded.publish(first);assert(bounded.ready[0].load());
 assert(bounded.reserve()==invalid);assert(tiny(8,8)&&!tiny(0,8)&&!tiny(8,9));
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
 DXGI_SWAP_CHAIN_DESC1 request{};request.Width=8;request.Height=8;request.BufferCount=2;
 SetLastError(12345);
 auto ordinary=begin_factory(create_hwnd,nullptr,nullptr,nullptr,&request,sizeof(request),nullptr,nullptr,nullptr,nullptr,false,true);
 assert(GetLastError()==12345);request.Width=1920;
 DXGI_SWAP_CHAIN_FULLSCREEN_DESC full{};full.Windowed=TRUE;
 auto prime=begin_factory(create_hwnd,nullptr,nullptr,nullptr,&request,sizeof(request),&full,nullptr,nullptr,nullptr,true,false);
 full.Windowed=FALSE;DXGI_SWAP_CHAIN_FULLSCREEN_DESC saved_full{};
 std::memcpy(&saved_full,records.rows[prime].fullscreen.data(),sizeof(saved_full));assert(saved_full.Windowed==TRUE);
 assert(ordinary==0&&prime==1);DXGI_SWAP_CHAIN_DESC1 saved{};
 std::memcpy(&saved,records.rows[ordinary].request.data(),sizeof(saved));assert(saved.Width==8);
 assert(records.rows[ordinary].running&&!records.rows[ordinary].prime);
 assert(records.rows[prime].prime&&!records.rows[prime].running);
 finish(prime,S_OK,nullptr);assert(GetLastError()==12345);
 FILE* incremental{};assert(tmpfile_s(&incremental)==0&&incremental);Export progress;
 assert(print(incremental,&progress)&&progress.emitted[prime]&&!progress.emitted[ordinary]);
 assert(!print(incremental,&progress));SetLastError(12345);
 finish(ordinary,E_FAIL,reinterpret_cast<IDXGISwapChain*>(1)); // must not dereference
 assert(records.rows[ordinary].reason==api_failed&&records.rows[prime].reason==no_chain);
 assert(GetLastError()==12345);
 assert(print(incremental,&progress)&&progress.emitted[ordinary]&&!print(incremental,&progress));assert(fclose(incremental)==0);
 auto invalid_request=begin(create_hwnd,reinterpret_cast<void*>(1),sizeof(request),nullptr,nullptr,true,false);
 assert(!records.rows[invalid_request].request_readable);finish(invalid_request,E_FAIL,nullptr);
 auto resize_zero=begin_resize(resize,nullptr,0,0,0,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);
 assert(records.rows[resize_zero].request_readable&&records.rows[resize_zero].width==0);
 finish(resize_zero,E_FAIL,nullptr);
 Chain chain;chain.desc.BufferDesc.Width=8;chain.desc.BufferDesc.Height=8;
 chain.resource.desc.Width=1920;chain.resource.desc.Height=1080;chain.resource.desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
 auto actual=begin_resize(resize,&chain,0,0,0,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);finish(actual,S_OK,&chain);
 assert(records.rows[actual].desc.BufferDesc.Width==8&&records.rows[actual].buffer.Width==1920&&records.rows[actual].reason==complete);
 assert(chain.resource.refs==1&&chain.desc_calls==1&&chain.buffer_calls==1);
 chain.desc_hr=E_FAIL;auto bad_desc=begin_resize(resize,&chain,0,8,8,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);finish(bad_desc,S_OK,&chain);
 assert(records.rows[bad_desc].reason==desc_failed&&chain.buffer_calls==1);
 chain.desc_hr=S_OK;chain.buffer_hr=E_FAIL;auto bad_buffer=begin_resize(resize,&chain,0,8,8,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);finish(bad_buffer,S_OK,&chain);
 assert(records.rows[bad_buffer].reason==buffer_failed&&records.rows[bad_buffer].buffer.Width==0);
 chain.buffer_hr=S_OK;chain.resource.supported=false;auto wrong_resource=begin_resize(resize,&chain,0,8,8,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);finish(wrong_resource,S_OK,&chain);
 assert(records.rows[wrong_resource].reason==buffer_not_d3d12&&records.rows[wrong_resource].resource_result==E_NOINTERFACE&&chain.resource.refs==1);
 // Pre-call adapter releases every COM reference before the caller forwards
 // ResizeBuffers. Window state is injected; this test never creates a window.
 chain.resource.supported=true;chain.desc.Windowed=TRUE;chain.desc.BufferCount=2;
 chain.desc.BufferDesc.Width=1920;chain.desc.BufferDesc.Height=1080;
 auto pre_index=begin_resize(resize,&chain,2,0,0,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false);
 const auto window=[](HWND,gears_background_zero_extent::Evidence& e){
  e.owned_hwnd=e.background_verified=e.iconic=true;SetLastError(777);
 };
 SetLastError(12345);
 auto dimensions=prepare_resize_with(pre_index,&chain,0,0,true,window);
 assert(GetLastError()==12345&&chain.resource.refs==1&&records.rows[pre_index].pre.evidence.physical_verified);
#if defined(GEARS_BACKGROUND_ZERO_EXTENT_CONTROL) && GEARS_BACKGROUND_ZERO_EXTENT_CONTROL
 assert(dimensions.substituted&&dimensions.width==1920&&dimensions.height==1080);
#else
 assert(!dimensions.substituted&&!dimensions.width&&!dimensions.height);
#endif
 unsigned forwarded{};const auto forward=[&](unsigned w,unsigned h){
  ++forwarded;assert(chain.resource.refs==1);assert(w==dimensions.width&&h==dimensions.height);return E_FAIL;
 };
 assert(forward(dimensions.width,dimensions.height)==E_FAIL&&forwarded==1);
 assert(!prepare_resize_with(pre_index,&chain,0,0,false,window).substituted);
 chain.desc.BufferCount=0;
 assert(!prepare_resize_with(pre_index,&chain,0,0,true,window).substituted);
 assert(!records.rows[pre_index].pre.evidence.physical_verified&&chain.resource.refs==1);
 chain.desc.BufferCount=2;
 chain.resource.desc.Width=8;assert(!prepare_resize_with(pre_index,&chain,0,0,true,window).substituted);
 chain.resource.desc.Width=1920;chain.desc_hr=E_FAIL;
 const auto buffer_calls=chain.buffer_calls;
 assert(!prepare_resize_with(pre_index,&chain,0,0,true,window).substituted&&chain.buffer_calls==buffer_calls);
 chain.desc_hr=S_OK;chain.buffer_hr=E_FAIL;
 assert(!prepare_resize_with(pre_index,&chain,0,0,true,window).substituted&&chain.resource.refs==1);
 chain.buffer_hr=S_OK;
 dimensions=prepare_resize_with(pre_index,&chain,0,900,true,window);
 assert(dimensions.height==900&&chain.resource.refs==1);
 finish(pre_index,E_FAIL,&chain);
 FILE* output{};assert(tmpfile_s(&output)==0&&output);Export cursor;
 assert(print(output,&cursor)&&!print(output,&cursor));rewind(output);
 std::array<char,32768> text{};const auto length=fread(text.data(),1,text.size()-1,output);assert(length&&fclose(output)==0);
 assert(std::strstr(text.data(),"desc_size=8,8")&&std::strstr(text.data(),"buffer_size=1920,1080")&&std::strstr(text.data(),"buffer_raw="));
 // Bounded concurrent reservations publish disjoint immutable records.
 std::vector<std::thread> workers;
 for(unsigned t=0;t<4;++t)workers.emplace_back([]{for(unsigned n=0;n<24;++n){
  auto i=begin_resize(resize1,nullptr,2,8,8,DXGI_FORMAT_R8G8B8A8_UNORM,0,nullptr,true,false);
  finish(i,E_FAIL,nullptr);}});
 for(auto& w:workers)w.join();assert(records.attempts.load()==105);
 for(auto& r:records.ready)assert(r.load());
 SetLastError(4321);assert(begin(create_hwnd,nullptr,0,nullptr,nullptr,false,false)==invalid&&GetLastError()==4321);
 // Saturation disables the private control as well as evidence capture. No
 // current COM/window query or unrecorded dimension substitution is allowed.
 const auto saturated_desc_calls=chain.desc_calls,saturated_buffer_calls=chain.buffer_calls;
 unsigned saturated_window_calls{};
 const auto saturated=prepare_resize_with(invalid,&chain,0,0,true,
  [&](HWND,gears_background_zero_extent::Evidence&){++saturated_window_calls;});
 assert(!saturated.substituted&&!saturated.width&&!saturated.height);
 assert(chain.desc_calls==saturated_desc_calls&&chain.buffer_calls==saturated_buffer_calls&&!saturated_window_calls);
 assert(GetLastError()==4321&&chain.resource.refs==1);
#else
 // Invalid addresses and a bogus FILE are deliberately safe in the OFF build:
 // no request dereference, COM call, stack capture, or output is performed.
 assert(begin(create_hwnd,reinterpret_cast<void*>(1),999,nullptr,nullptr,true,false)==invalid);
 assert(begin_resize(resize,reinterpret_cast<IDXGISwapChain*>(1),2,8,8,DXGI_FORMAT_UNKNOWN,0,nullptr,true,false)==invalid);
 SetLastError(2468);const auto dimensions=prepare_resize(0,reinterpret_cast<IDXGISwapChain*>(1),0,900,true);
 assert(!dimensions.substituted&&dimensions.width==0&&dimensions.height==900&&GetLastError()==2468);
 finish(0,S_OK,reinterpret_cast<IDXGISwapChain*>(1));print(reinterpret_cast<FILE*>(1));
#endif
 puts("swapchain startup trace boundaries/prime/bounded/off PASS");
}
