#pragma once
#include "ngx_output_pixels_live.h"
namespace owned_source_pixels {
using CopyFn=ngx_output_pixels::CopyFn;using ExecuteFn=ngx_output_pixels::ExecuteFn;using ResetFn=ngx_output_pixels::ResetFn;
#if defined(GEARS_OWNED_SOURCE_PIXELS) && GEARS_OWNED_SOURCE_PIXELS
bool enable(FILE*);void poll();void stop();void print(FILE*);
void copy_passthrough(ID3D12GraphicsCommandList*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*,uintptr_t,CopyFn);
void execute_passthrough(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*,ExecuteFn);
HRESULT reset_passthrough(ID3D12GraphicsCommandList*,ID3D12CommandAllocator*,ID3D12PipelineState*,ResetFn);
#else
inline bool enable(FILE*){return true;}inline void poll(){}inline void stop(){}inline void print(FILE*){}
inline void copy_passthrough(ID3D12GraphicsCommandList* l,const D3D12_TEXTURE_COPY_LOCATION* d,UINT x,UINT y,UINT z,const D3D12_TEXTURE_COPY_LOCATION* s,const D3D12_BOX* b,uintptr_t c,CopyFn f){ngx_output_pixels::copy_passthrough(l,d,x,y,z,s,b,c,f);}
inline void execute_passthrough(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* l,ExecuteFn f){ngx_output_pixels::execute_passthrough(q,n,l,f);}
inline HRESULT reset_passthrough(ID3D12GraphicsCommandList* l,ID3D12CommandAllocator* a,ID3D12PipelineState* p,ResetFn f){return ngx_output_pixels::reset_passthrough(l,a,p,f);}
#endif
}
