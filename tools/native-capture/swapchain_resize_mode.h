#pragma once

// Runtime-size candidates share only the DXGI call wrappers. They must not
// acquire the game-resolution transport's resize/drain/inventory lifecycle.
#if defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE && \
    !(defined(GEARS_GAME_RESOLUTION) && GEARS_GAME_RESOLUTION) && \
    !(defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE)
#error Swapchain startup trace requires game resolution or runtime render size
#endif
#if defined(GEARS_GAME_RESOLUTION) && GEARS_GAME_RESOLUTION
#define GEARS_DXGI_RESIZE_HOOKS 1
#define GEARS_DXGI_RESIZE_TRANSPORT 1
#elif defined(GEARS_RUNTIME_RENDER_SIZE) && GEARS_RUNTIME_RENDER_SIZE && \
    defined(GEARS_SWAPCHAIN_STARTUP_TRACE) && GEARS_SWAPCHAIN_STARTUP_TRACE
#define GEARS_DXGI_RESIZE_HOOKS 1
#define GEARS_DXGI_RESIZE_TRANSPORT 0
#else
#define GEARS_DXGI_RESIZE_HOOKS 0
#define GEARS_DXGI_RESIZE_TRANSPORT 0
#endif
