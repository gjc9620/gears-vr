#include "transition_card_layout.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main(){
    using namespace transition_card_layout;
    for(auto selected:{DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM}){
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=513;desc.Height=3;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=selected;
        assert(valid_image(desc,selected,513,3));
        desc.Format=selected==DXGI_FORMAT_R8G8B8A8_UNORM?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_TYPELESS;
        assert(valid_image(desc,selected,513,3));
        desc.Format=selected==DXGI_FORMAT_R8G8B8A8_UNORM?DXGI_FORMAT_B8G8R8A8_TYPELESS:DXGI_FORMAT_R8G8B8A8_TYPELESS;
        assert(!valid_image(desc,selected,513,3));
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;assert(!valid_image(desc,selected,513,3));
        desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;assert(!valid_image(desc,selected,513,3));
        desc.Format=selected;desc.Width=512;assert(!valid_image(desc,selected,513,3));desc.Width=513;
        desc.DepthOrArraySize=2;assert(!valid_image(desc,selected,513,3));desc.DepthOrArraySize=1;
        desc.MipLevels=2;assert(!valid_image(desc,selected,513,3));desc.MipLevels=1;
        desc.SampleDesc.Count=2;assert(!valid_image(desc,selected,513,3));desc.SampleDesc.Count=1;
        desc.SampleDesc.Quality=1;assert(!valid_image(desc,selected,513,3));
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT f{};f.Footprint={selected,513,3,1,2304};
        const std::uint64_t bytes=2304*2+513*4;
        assert(valid_footprint(f,bytes,selected,513,3,3,2052));
        assert(!valid_footprint(f,bytes-1,selected,513,3,3,2052));
        assert(!valid_footprint(f,0,selected,513,3,3,2052));
        assert(!valid_footprint(f,bytes,selected,513,3,2,2052));
        assert(!valid_footprint(f,bytes,selected,513,3,3,2051));
        f.Offset=bytes+1;assert(!valid_footprint(f,bytes,selected,513,3,3,2052));
        f.Offset=1;assert(!valid_footprint(f,bytes+512,selected,513,3,3,2052));
        f.Offset=512;assert(valid_footprint(f,bytes+512,selected,513,3,3,2052));
        f.Footprint.RowPitch=2048;assert(!valid_footprint(f,bytes+512,selected,513,3,3,2052));
        f.Footprint.RowPitch=2305;assert(!valid_footprint(f,bytes+512,selected,513,3,3,2052));
        f.Footprint.RowPitch=2304;f.Footprint.Format=desc.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;
        assert(!valid_footprint(f,bytes+512,selected,513,3,3,2052));
        f.Footprint.Format=selected;f.Footprint.Depth=2;assert(!valid_footprint(f,bytes+512,selected,513,3,3,2052));
    }
    assert(!matching_format(DXGI_FORMAT_R8G8B8A8_TYPELESS,DXGI_FORMAT_R8G8B8A8_TYPELESS));
    assert(!matching_format(DXGI_FORMAT_B8G8R8X8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM));
    std::puts("transition card layout tests passed");
}
