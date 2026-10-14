#pragma once
#include "gpu_share.h"
#include <d3dcompiler.h>

namespace gears_vr {
// SDR diagnostic conversion. Preserve source encoded RGB values when writing
// an sRGB target by decoding in the shader before the RTV encodes them again.
// This deliberately does not implement HDR transfer functions or tone mapping.
class GpuBlit {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12DescriptorHeap> srv,rtv;
    DXGI_FORMAT target_format;
public:
    GpuBlit(ID3D12Device* d,DXGI_FORMAT format):device(d),target_format(format){
        bool srgb=format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        std::string shader=R"(
Texture2D<float4> image:register(t0);
cbuffer Crop:register(b0){uint2 source_offset;};
float4 vs(uint id:SV_VertexID):SV_Position {
    float2 p=float2((id<<1)&2,id&2);return float4(p.x*2-1,1-p.y*2,0,1);
}
float4 ps(float4 position:SV_Position):SV_Target {
    float4 color=image.Load(int3(int2(position.xy)+int2(source_offset),0));
)";
        if(srgb)shader+=R"(
    color.rgb=float3(color.r<=0.04045?color.r/12.92:pow((color.r+0.055)/1.055,2.4),
                     color.g<=0.04045?color.g/12.92:pow((color.g+0.055)/1.055,2.4),
                     color.b<=0.04045?color.b/12.92:pow((color.b+0.055)/1.055,2.4));
)";
        shader+="return float4(color.rgb,1);}";
        ComPtr<ID3DBlob> vs,ps,errors;
        gpu_check(D3DCompile(shader.data(),shader.size(),nullptr,nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&errors),"compile blit vertex");
        errors.Reset();gpu_check(D3DCompile(shader.data(),shader.size(),nullptr,nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&errors),"compile blit pixel");
        D3D12_DESCRIPTOR_RANGE range{};range.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;range.NumDescriptors=1;
        D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable.NumDescriptorRanges=1;parameter.DescriptorTable.pDescriptorRanges=&range;parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_PARAMETER parameters[2]={parameter,{}};
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[1].Constants.Num32BitValues=2;parameters[1].Constants.ShaderRegister=0;
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC signature{};signature.NumParameters=2;signature.pParameters=parameters;signature.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ComPtr<ID3DBlob> serialized;errors.Reset();gpu_check(D3D12SerializeRootSignature(&signature,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors),"serialize blit root");
        gpu_check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)),"blit root");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};pso.pRootSignature=root.Get();pso.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pso.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
        auto& blend=pso.BlendState.RenderTarget[0];blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_ZERO;blend.BlendOp=D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        pso.SampleMask=~0u;pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=TRUE;
        pso.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
        pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pso.NumRenderTargets=1;pso.RTVFormats[0]=format;pso.SampleDesc.Count=1;
        gpu_check(device->CreateGraphicsPipelineState(&pso,IID_PPV_ARGS(&pipeline)),"blit pipeline");
        D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=1;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        gpu_check(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv)),"blit SRV heap");heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        gpu_check(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)),"blit RTV heap");
    }
    void draw(ID3D12GraphicsCommandList* commands,ID3D12Resource* source,ID3D12Resource* destination){
        auto from=source->GetDesc(),to=destination->GetDesc();
        if(from.Width!=to.Width||from.Height!=to.Height)throw std::runtime_error("Blit dimensions differ");
        draw_crop(commands,source,destination,0,0);
    }
    void draw_crop(ID3D12GraphicsCommandList* commands,ID3D12Resource* source,ID3D12Resource* destination,unsigned source_x,unsigned source_y){
        auto from=source->GetDesc(),to=destination->GetDesc();
        if(source_x>from.Width||source_y>from.Height||to.Width>from.Width-source_x||to.Height>from.Height-source_y)
            throw std::runtime_error("Blit crop outside source");
        D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=from.Format;view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(source,&view,srv->GetCPUDescriptorHandleForHeapStart());
        D3D12_RENDER_TARGET_VIEW_DESC target{};target.Format=target_format;target.ViewDimension=D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(destination,&target,rtv->GetCPUDescriptorHandleForHeapStart());
        commands->SetPipelineState(pipeline.Get());commands->SetGraphicsRootSignature(root.Get());ID3D12DescriptorHeap* heaps[]={srv.Get()};commands->SetDescriptorHeaps(1,heaps);
        commands->SetGraphicsRootDescriptorTable(0,srv->GetGPUDescriptorHandleForHeapStart());
        const unsigned offset[]={source_x,source_y};commands->SetGraphicsRoot32BitConstants(1,2,offset,0);
        D3D12_VIEWPORT viewport{0,0,float(to.Width),float(to.Height),0,1};D3D12_RECT scissor{0,0,LONG(to.Width),LONG(to.Height)};
        commands->RSSetViewports(1,&viewport);commands->RSSetScissorRects(1,&scissor);auto handle=rtv->GetCPUDescriptorHandleForHeapStart();commands->OMSetRenderTargets(1,&handle,FALSE,nullptr);
        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);commands->DrawInstanced(3,1,0,0);
    }
};
}
