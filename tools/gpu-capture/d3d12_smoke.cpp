#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <iostream>
using Microsoft::WRL::ComPtr;
int main()
{
  WNDCLASSW wc{}; wc.lpfnWndProc=DefWindowProcW; wc.hInstance=GetModuleHandleW(nullptr);
  wc.lpszClassName=L"GearsVR.RenderDocSmoke"; RegisterClassW(&wc);
  HWND window=CreateWindowW(wc.lpszClassName,L"GearsVR D3D12 capture check",WS_OVERLAPPEDWINDOW,
                           0,0,320,240,nullptr,nullptr,wc.hInstance,nullptr);
  if(!window) return 2;
  ComPtr<IDXGIFactory4> factory; ComPtr<ID3D12Device> device;
  if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) ||
     FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))) return 3;
  D3D12_COMMAND_QUEUE_DESC q{}; ComPtr<ID3D12CommandQueue> queue;
  if(FAILED(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)))) return 4;
  DXGI_SWAP_CHAIN_DESC1 desc{}; desc.Width=320; desc.Height=240; desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count=1; desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount=2; desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
  ComPtr<IDXGISwapChain1> swap1; ComPtr<IDXGISwapChain3> swap;
  if(FAILED(factory->CreateSwapChainForHwnd(queue.Get(),window,&desc,nullptr,nullptr,&swap1))) return 5;
  swap1.As(&swap);
  D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV; hd.NumDescriptors=2;
  ComPtr<ID3D12DescriptorHeap> heap; ComPtr<ID3D12Resource> buffers[2];
  if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)))) return 6;
  const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  auto base=heap->GetCPUDescriptorHandleForHeapStart();
  for(UINT i=0;i<2;++i){if(FAILED(swap->GetBuffer(i,IID_PPV_ARGS(&buffers[i]))))return 7;
    D3D12_CPU_DESCRIPTOR_HANDLE handle{base.ptr+i*step};device->CreateRenderTargetView(buffers[i].Get(),nullptr,handle);}
  ComPtr<ID3D12CommandAllocator> allocator; ComPtr<ID3D12GraphicsCommandList> list; ComPtr<ID3D12Fence> fence;
  if(FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) ||
     FAILED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))) ||
     FAILED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)))) return 8;
  list->Close(); HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr); if(!event)return 9;
  // Visible only in taskbar; no focus stealing or screenshot automation required.
  ShowWindow(window,SW_SHOWMINNOACTIVE);
  std::cout << "D3D12 smoke pid=" << GetCurrentProcessId() << " ready" << std::endl;
  UINT64 value=0; const auto deadline=GetTickCount64()+45000;
  while(GetTickCount64()<deadline && !(GetAsyncKeyState(VK_F8)&0x8000))
  {
    MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    const UINT index=swap->GetCurrentBackBufferIndex();
    if(FAILED(allocator->Reset())||FAILED(list->Reset(allocator.Get(),nullptr)))return 10;
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource=buffers[index].Get();barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;list->ResourceBarrier(1,&barrier);
    D3D12_CPU_DESCRIPTOR_HANDLE handle{base.ptr+index*step};float color[4]={0.08f,0.25f,0.5f,1};
    list->ClearRenderTargetView(handle,color,0,nullptr);
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;list->ResourceBarrier(1,&barrier);
    if(FAILED(list->Close()))return 11;ID3D12CommandList *commands[]={list.Get()};queue->ExecuteCommandLists(1,commands);
    if(FAILED(swap->Present(0,0))||FAILED(queue->Signal(fence.Get(),++value)))return 12;
    if(fence->GetCompletedValue()<value){fence->SetEventOnCompletion(value,event);if(WaitForSingleObject(event,5000)!=WAIT_OBJECT_0)return 13;}
    Sleep(16);
  }
  CloseHandle(event);DestroyWindow(window);return 0;
}
