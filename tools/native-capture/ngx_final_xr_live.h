#pragma once
namespace ngx_final_xr {
#if defined(GEARS_NGX_FINAL_XR_TRIAL) && GEARS_NGX_FINAL_XR_TRIAL
bool enabled();bool enable(bool,FILE*);
bool ensure(ID3D12Resource*,gears_ngx_stereo::Identity,gears_ngx_stereo::Extent);
bool rebind(gears_ngx_stereo::Identity,gears_ngx_stereo::Identity,gears_ngx_stereo::Extent);
void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned);
void executed(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*);
void reset(uintptr_t);void print(FILE*);
#else
inline bool enabled(){return false;}
inline bool enable(bool,FILE*){return false;}
inline bool ensure(ID3D12Resource*,gears_ngx_stereo::Identity,gears_ngx_stereo::Extent){return true;}
inline bool rebind(gears_ngx_stereo::Identity,gears_ngx_stereo::Identity,gears_ngx_stereo::Extent){return true;}
inline void draw(ID3D12GraphicsCommandList*,uintptr_t,unsigned,unsigned){}
inline void executed(ID3D12CommandQueue*,UINT,ID3D12CommandList* const*){}
inline void reset(uintptr_t){}inline void print(FILE*){}
#endif
}
