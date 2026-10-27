#define CINTERFACE
#include <windows.h>
#include <d3d12.h>
#include <cstddef>
#include <cstdio>
// Compile the Windows SDK's C vtables, including IUnknown and inherited slots.
// Hand counting previously confused OMSetStencilRef (24) with ResourceBarrier.
#define SLOT(type,member,index) static_assert(offsetof(type,member)==(index)*sizeof(void*),#type "::" #member " slot mismatch")
SLOT(ID3D12GraphicsCommandListVtbl,Reset,10);
SLOT(ID3D12GraphicsCommandListVtbl,ClearState,11);
SLOT(ID3D12GraphicsCommandListVtbl,DrawInstanced,12);
SLOT(ID3D12GraphicsCommandListVtbl,DrawIndexedInstanced,13);
SLOT(ID3D12GraphicsCommandListVtbl,Dispatch,14);
SLOT(ID3D12GraphicsCommandListVtbl,CopyTextureRegion,16);
SLOT(ID3D12GraphicsCommandListVtbl,CopyResource,17);
SLOT(ID3D12GraphicsCommandListVtbl,ResolveSubresource,19);
SLOT(ID3D12GraphicsCommandListVtbl,RSSetViewports,21);
SLOT(ID3D12GraphicsCommandListVtbl,RSSetScissorRects,22);
SLOT(ID3D12GraphicsCommandListVtbl,OMSetStencilRef,24);
SLOT(ID3D12GraphicsCommandListVtbl,SetPipelineState,25);
SLOT(ID3D12GraphicsCommandListVtbl,ResourceBarrier,26);
SLOT(ID3D12GraphicsCommandListVtbl,ExecuteBundle,27);
SLOT(ID3D12GraphicsCommandListVtbl,SetDescriptorHeaps,28);
SLOT(ID3D12GraphicsCommandListVtbl,SetComputeRootSignature,29);
SLOT(ID3D12GraphicsCommandListVtbl,SetComputeRootDescriptorTable,31);
SLOT(ID3D12GraphicsCommandListVtbl,OMSetRenderTargets,46);
SLOT(ID3D12GraphicsCommandListVtbl,ClearRenderTargetView,48);
SLOT(ID3D12GraphicsCommandListVtbl,DiscardResource,51);
SLOT(ID3D12GraphicsCommandListVtbl,ExecuteIndirect,59);
SLOT(ID3D12GraphicsCommandList1Vtbl,ResolveSubresourceRegion,64);
SLOT(ID3D12GraphicsCommandList4Vtbl,BeginRenderPass,68);
SLOT(ID3D12GraphicsCommandList7Vtbl,Barrier,80);
SLOT(ID3D12CommandQueueVtbl,ExecuteCommandLists,10);
SLOT(ID3D12PipelineStateVtbl,GetCachedBlob,8);
SLOT(ID3D12DeviceVtbl,CreateCommandList,12);
SLOT(ID3D12DeviceVtbl,CreateRootSignature,16);
SLOT(ID3D12Device4Vtbl,CreateCommandList1,51);
SLOT(ID3D12DeviceVtbl,CreateRenderTargetView,20);
SLOT(ID3D12DeviceVtbl,CopyDescriptors,23);
SLOT(ID3D12DeviceVtbl,CopyDescriptorsSimple,24);
SLOT(ID3D12DeviceVtbl,CreateCommittedResource,27);
SLOT(ID3D12DeviceVtbl,CreatePlacedResource,29);
SLOT(ID3D12Device4Vtbl,CreateCommittedResource1,53);
SLOT(ID3D12Device8Vtbl,CreateCommittedResource2,69);
SLOT(ID3D12Device8Vtbl,CreatePlacedResource1,70);
SLOT(ID3D12Device10Vtbl,CreateCommittedResource3,76);
SLOT(ID3D12Device10Vtbl,CreatePlacedResource2,77);
int main(){std::puts("D3D12 ABI: 39 SDK vtable offsets verified, CreateRootSignature=16, Dispatch=14, SetDescriptorHeaps=28, SetComputeRootSignature=29, SetComputeRootDescriptorTable=31");}
