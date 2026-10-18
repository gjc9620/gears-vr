#pragma once
#include "../vr-common/render_size_bridge.h"
namespace gears_native_render_size {
inline gears_vr::RenderSizePacket packet{};
inline bool loaded{};
inline bool initialize() {
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 gears_vr::RenderSizeBridge reader;loaded=reader.read_current(packet);return loaded;
#else
 loaded=true;return true;
#endif
}
inline unsigned width(){
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 return loaded?packet.plan.source_width:0;
#else
 return 4096;
#endif
}
inline unsigned height(){
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 return loaded?packet.plan.source_height:0;
#else
 return 2048;
#endif
}
inline bool matches_session(std::uint64_t session){
#if defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE
 return loaded&&session&&packet.session==session;
#else
 (void)session;return true;
#endif
}
}
