#pragma once
#include"sky_box.h"
void SimpleSkyBox::CreateOnCmdListOpen()
{
    skyMesh.Create("demo_asset_data/skybox/skyball.lmesh");
    //D3D12_RESOURCE_BARRIER textureBarrier[3];
    //textureBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(skySpecular.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //textureBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(skyDiffuse.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //textureBarrier[2] = CD3DX12_RESOURCE_BARRIER::Transition(brdf.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //g_pd3dCommandList->ResourceBarrier(3, textureBarrier);
}

void SimpleSkyBox::Create()
{
	GpuResourceUtil::GenerateGraphicPipelineByShader(GpuResourceUtil::globelDrawInputRootParam.Get(), L"demo_asset_data/shader/draw/sky_draw.hlsl", L"demo_asset_data/shader/draw/sky_draw.hlsl", true,false, skyPipeline);
    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/Cubemap.dds",
        skySpecular,
        skySpecularDescriptorOffset
    );

    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/IrradianceMap.dds",
        skyDiffuse,
        skyDiffuseDescriptorOffset
    );

    GpuResourceUtil::GenerateTexture2DSRV(
        L"demo_asset_data/skybox/brdf.dds",
        brdf,
        brdfDescriptorOffset
    );
}

void SimpleSkyBox::Draw(const std::unordered_map<size_t, size_t>& viewBindPoint)
{
    std::unordered_map<size_t, size_t> skyBindPoint = viewBindPoint;
    skyBindPoint.insert({ 3,skySpecularDescriptorOffset });
    GpuResourceUtil::DrawMeshData(skyPipeline.Get(), skyBindPoint, &skyMesh);
}