#pragma once
#include <d3d12.h>
#include <cstdint>
#include <limits>
#include "menu_swapchain_format.h"

namespace transition_card_layout {
inline bool matching_format(DXGI_FORMAT resource,DXGI_FORMAT selected) noexcept {
    return (menu_swapchain_format::rgba(selected)&&
            (resource==selected||resource==DXGI_FORMAT_R8G8B8A8_TYPELESS))||
           (menu_swapchain_format::bgra(selected)&&
            (resource==selected||resource==DXGI_FORMAT_B8G8R8A8_TYPELESS));
}
inline bool valid_image(D3D12_RESOURCE_DESC const& desc,DXGI_FORMAT selected,
                        unsigned width,unsigned height) noexcept {
    return width&&height&&desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        desc.Width==width&&desc.Height==height&&matching_format(desc.Format,selected)&&
        desc.DepthOrArraySize==1&&desc.MipLevels==1&&desc.SampleDesc.Count==1&&desc.SampleDesc.Quality==0;
}
inline bool valid_footprint(D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& layout,
                            std::uint64_t bytes,DXGI_FORMAT selected,unsigned width,unsigned height,
                            unsigned rows,std::uint64_t row_size) noexcept {
    if(!width||!height||!matching_format(selected,selected)||!bytes||
       bytes>(std::numeric_limits<std::size_t>::max)()||layout.Offset>bytes||
       layout.Offset%D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT||
       layout.Footprint.RowPitch%D3D12_TEXTURE_DATA_PITCH_ALIGNMENT||
       layout.Footprint.Format!=selected||layout.Footprint.Width!=width||
       layout.Footprint.Height!=height||layout.Footprint.Depth!=1||rows!=height)return false;
    const auto row_bytes=std::uint64_t(width)*4;
    if(row_size!=row_bytes||layout.Footprint.RowPitch<row_bytes)return false;
    // UINT32 pitch and height ensure this product and addition fit uint64_t.
    const auto copied=std::uint64_t(height-1)*layout.Footprint.RowPitch+row_bytes;
    return copied<=bytes-layout.Offset;
}
}
