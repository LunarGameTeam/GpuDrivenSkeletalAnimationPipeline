#pragma once
#include"application_demo.h"
#include "../nlohmann_json/json.hpp"
#include <limits.h>
using json = nlohmann::json;

void AnimationSimulateDemo::CreateOnCmdListOpen(const std::string& configFile)
{
    baseSkyBox.CreateOnCmdListOpen();
    floorMeshRenderer.CreateOnCmdListOpen("demo_asset_data/floor/floorbox.lmesh","demo_asset_data/floor/floor.material");
    //world matrix
    //load model mesh
    json curConfigJson;
    std::ifstream(configFile.c_str()) >> curConfigJson;
    json allMeshDataList = curConfigJson["MeshModelList"];
    int allMeshCount = allMeshDataList.size();
    meshValueList.resize(allMeshCount);
    for (int32_t meshIndex = 0; meshIndex < allMeshCount; ++meshIndex)
    {
        json curMesh = allMeshDataList[meshIndex];
        std::string curMeshName = curMesh["MeshFileName"];
        std::string curSkeletonName = curMesh["SkeletonFileName"];
        json curAnimationList = curMesh["AnimationFileList"];
        std::vector<std::string> animationNameList;
        int allAnimationCount = allMeshDataList.size();
        for (int32_t animIndex = 0; animIndex < allAnimationCount; ++animIndex)
        {
            std::string curAnimationName = curAnimationList[animIndex];
            animationNameList.push_back(curAnimationName);
        }
        meshValueList[meshIndex].CreateOnCmdListOpen(curMeshName, curSkeletonName, animationNameList, "demo_asset_data/skeletal_mesh_demo/lion3/lion3.material");
        //meshValueList[i].Create();
    }
    //skeletal Input
    animationClipInput.Create(262144, sizeof(DirectX::XMFLOAT4));
    bindposeMatrixInput.Create(262144, sizeof(DirectX::XMFLOAT4X4));
    skeletonHierarchyInput.Create(262144, sizeof(UINT32));

    SkeletonAnimationResult.Create(262144, sizeof(DirectX::XMFLOAT4X4));
    worldSpaceSkeletonResultMap0.Create(262144,sizeof(DirectX::XMFLOAT4X4));
    worldSpaceSkeletonResultMap1.Create(262144, sizeof(DirectX::XMFLOAT4X4));
    skinVertexResult.Create(262144, sizeof(DirectX::XMFLOAT4));
    //world & view
    viewBufferGpu.Create(65536);
    viewBufferCpu.Create(65536);
    viewBufferGpu.mBufferResource->SetName(L"ViewGpuBuffer");

    for (int32_t i = 0; i < 3; ++i)
    {
        worldMatrixBufferCpu[i].Create(65536);
    }
    worldMatrixBufferGpu.Create(65536,sizeof(DirectX::XMFLOAT4X4));
    worldMatrixBufferGpu.mBufferResource->SetName(L"WorldGpuBuffer");
    //change mesh resource state to vb and ib
    D3D12_RESOURCE_BARRIER meshBufferBarrier[3];
    meshBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneStaticVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    meshBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneSkinVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    meshBufferBarrier[2] = CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneIndexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    g_pd3dCommandList->ResourceBarrier(3, meshBufferBarrier);
    inited = true;
}
void AnimationSimulateDemo::Create()
{
    D3D12_SAMPLER_DESC curSampler = {};
    curSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    curSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    curSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    curSampler.Filter = D3D12_FILTER::D3D12_FILTER_ANISOTROPIC;
    curSampler.MipLODBias = 0;
    curSampler.MaxAnisotropy = 8;
    curSampler.ComparisonFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_NEVER;
    for (int32_t i = 0; i < 4; ++i)
    {
        curSampler.BorderColor[i] = 0;
    }
    curSampler.MinLOD = 0;
    curSampler.MaxLOD = FLT_MAX;
    samplerDescriptor = CreateSamplerByDesc(curSampler);
    for (int32_t i = 0; i < 3; ++i)
    {
        mDepthStencil[i].Create(g_windowWidth,g_windowHeight);
    }
    
    baseSkyBox.Create();    
    GpuResourceUtil::GenerateGraphicPipelineByShader(GpuResourceUtil::globelDrawInputRootParam.Get(), L"demo_asset_data/shader/draw/floor_vertex_trans.hlsl", L"demo_asset_data/shader/draw/pbr_draw.hlsl", true, false, mAllPipelines.floorDrawPipeline);
    GpuResourceUtil::GenerateGraphicPipelineByShader(GpuResourceUtil::globelDrawInputRootParam.Get(), L"demo_asset_data/shader/draw/static_vertex_trans.hlsl", L"demo_asset_data/shader/draw/pbr_draw.hlsl", true,false, mAllPipelines.meshDrawPipeline);
    GpuResourceUtil::GenerateGraphicPipelineByShader(GpuResourceUtil::globelDrawInputRootParam.Get(), L"demo_asset_data/shader/draw/skin_vertex_trans.hlsl", L"demo_asset_data/shader/draw/pbr_draw.hlsl", true,false, mAllPipelines.skinDrawPipeline);
    
}
struct ViewParam 
{
    DirectX::XMFLOAT4X4 cViewMatrix;
    DirectX::XMFLOAT4X4 cProjectionMatrix;
    DirectX::XMFLOAT4 cCamPos;
};
void AnimationSimulateDemo::DrawDemoData()
{
    floorMeshRenderer.Update(DirectX::XMFLOAT4(0,0,0,1), DirectX::XMFLOAT4(0,0,0,1), DirectX::XMFLOAT4(6000, 1, 6000, 1));
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].Update(DirectX::XMFLOAT4(0, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1));
    }
    //reset barrier
    //D3D12_RESOURCE_BARRIER viewBufferResetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(viewBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    //g_pd3dCommandList->ResourceBarrier(1, &viewBufferResetBarrier);
    //generate view uniform buffer
    ViewParam newParam;
    DirectX::XMStoreFloat4x4(&newParam.cProjectionMatrix, DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, (float)g_windowWidth / (float)g_windowHeight, 0.1f, 1000.0f)));
    baseView.CountViewMatrix(&newParam.cViewMatrix);
    auto matrixView = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&newParam.cViewMatrix));
    DirectX::XMStoreFloat4x4(&newParam.cViewMatrix, matrixView);
    baseView.GetViewPosition(&newParam.cCamPos);
    size_t alignedSize = SizeAligned2Pow(sizeof(newParam), 255);
    size_t offset = g_pSwapChain->GetCurrentBackBufferIndex() * alignedSize;
    memcpy(viewBufferCpu.mapPointer + offset,&newParam,sizeof(newParam));
    g_pd3dCommandList->CopyBufferRegion(viewBufferGpu.mBufferResource.Get(), 0, viewBufferCpu.mBufferResource.Get(), offset, alignedSize);

    //world matrix
    std::vector<DirectX::XMFLOAT4X4> worldMatrixArray;
    worldMatrixArray.push_back(floorMeshRenderer.GetTransForm());
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        worldMatrixArray.push_back(meshValueList[skelMeshIndex].GetTransForm());
    }
    memcpy(worldMatrixBufferCpu[g_pSwapChain->GetCurrentBackBufferIndex()].mapPointer, worldMatrixArray.data(), worldMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4));
    size_t copySize = SizeAligned2Pow(worldMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4), 255);
    g_pd3dCommandList->CopyBufferRegion(worldMatrixBufferGpu.mBufferResource.Get(), 0, worldMatrixBufferCpu[g_pSwapChain->GetCurrentBackBufferIndex()].mBufferResource.Get(), 0, copySize);

    //change view uniform buffer state
    D3D12_RESOURCE_BARRIER worldViewBufferBarrier[] = 
    { 
        CD3DX12_RESOURCE_BARRIER::Transition(worldMatrixBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(viewBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
    };
    g_pd3dCommandList->ResourceBarrier(2, worldViewBufferBarrier);
    //RenderTarget
    UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDsvHandleStart = g_pd3dDsvDescHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle = cpuDsvHandleStart;
    size_t per_descriptor_size = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    dsvCpuHandle.ptr += per_descriptor_size * mDepthStencil[backBufferIdx].mDescriptorOffsetDsv;
    g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, &dsvCpuHandle);
    //viewPort
    D3D12_RECT render_rect;
    render_rect.left = (LONG)0;
    render_rect.top = (LONG)0;
    render_rect.right = (LONG)g_windowWidth;
    render_rect.bottom = (LONG)g_windowHeight;
    D3D12_VIEWPORT curVp;
    curVp.TopLeftX = 0;
    curVp.TopLeftY = 0;
    curVp.Width = g_windowWidth;
    curVp.Height = g_windowHeight;
    curVp.MinDepth = 0;
    curVp.MaxDepth = 1.0f;
    g_pd3dCommandList->RSSetViewports(1, &curVp);
    g_pd3dCommandList->RSSetScissorRects(1, &render_rect);
    //clear target
    float clear_color[] = { 0.45f, 0.55f, 0.60f, 1.00f };
    g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color, 0, nullptr);
    g_pd3dCommandList->ClearDepthStencilView
    (
        dsvCpuHandle,
        D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL,
        1,
        0,
        1,
        &render_rect
    );

    //show all mesh
    std::unordered_map<size_t, size_t> viewBindPoint;
    viewBindPoint.insert({ 5, samplerDescriptor });
    viewBindPoint.insert({ 0, viewBufferGpu.mDescriptorOffsetCBV });
    viewBindPoint.insert({ 1, worldMatrixBufferGpu.mDescriptorOffsetSRV});
    g_pd3dCommandList->SetGraphicsRootSignature(GpuResourceUtil::globelDrawInputRootParam.Get());
    baseSkyBox.Draw(viewBindPoint);
    floorMeshRenderer.Draw(viewBindPoint, mAllPipelines.floorDrawPipeline.Get(),0);
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].Draw(viewBindPoint, mAllPipelines.meshDrawPipeline.Get(), skelMeshIndex + 1);
    }
  
    //std::unordered_map<size_t, size_t> skyBindPoint = viewBindPoint;
    //skyBindPoint.insert({ 3,skySpecularDescriptorOffset });



}