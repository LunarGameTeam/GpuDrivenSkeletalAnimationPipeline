#pragma once
#include"application_demo.h"
#include "../nlohmann_json/json.hpp"
#include <limits.h>
using json = nlohmann::json;

DirectX::XMFLOAT4X4 MatrixCompose(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 rotation, DirectX::XMFLOAT4 scale)
{
    DirectX::XMFLOAT4X4 composeMatrix;
    DirectX::XMVECTOR quatIdentity = DirectX::XMQuaternionIdentity();
    DirectX::XMVECTOR positionVec, scaleVec;
    positionVec = DirectX::XMLoadFloat4(&position);
    scaleVec = DirectX::XMLoadFloat4(&scale);
    DirectX::XMMATRIX matOut;
    matOut = DirectX::XMMatrixAffineTransformation(scaleVec, quatIdentity, quatIdentity, positionVec);
    DirectX::XMStoreFloat4x4(&composeMatrix, DirectX::XMMatrixTranspose(matOut));
    return composeMatrix;
}

void SkeletalMeshRenderBatch::AddInstance(
    DirectX::XMFLOAT4X4 transformMatrix,
    int32_t animationindex,
    float stTime
)
{
    MeshRenderParameter newParam;
    newParam.curPlayTime = stTime;
    newParam.mAnimationindex = animationindex;
    newParam.mTransformMatrix = transformMatrix;
    mAllInstance.push_back(newParam);
}

void SkeletalMeshRenderBatch::LocalPoseToModelPose(const std::vector<SimpleSkeletonJoint>& localPose, std::vector<DirectX::XMMATRIX>& modelPose)
{
    for (int32_t jointIndex = 0; jointIndex < localPose.size(); ++jointIndex)
    {
        const SimpleSkeletonJoint& jointData = localPose[jointIndex];
        DirectX::XMVECTOR rotationQ = DirectX::XMLoadFloat4(&jointData.mBaseRotation);
        DirectX::XMMATRIX localMatrix =
            DirectX::XMMatrixScaling(jointData.mBaseScal.x, jointData.mBaseScal.y, jointData.mBaseScal.z) *
            DirectX::XMMatrixRotationQuaternion(rotationQ) *
            DirectX::XMMatrixTranslation(jointData.mBaseTranslation.x, jointData.mBaseTranslation.y, jointData.mBaseTranslation.z);
        int32_t parentId = jointData.mParentIndex;
        DirectX::XMMATRIX parentMatrix = DirectX::XMMatrixIdentity();
        if (parentId != -1)
        {
            parentMatrix = modelPose[parentId];
        }
        modelPose[jointIndex] = localMatrix * parentMatrix;
    }
}

void SkeletalMeshRenderBatch::CreateOnCmdListOpen(
    const std::string& meshFile,
    const std::string& skeletonFile,
    const std::vector<std::string>& animationFileList,
    const std::string& materialName
)
{
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(0, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.0f);
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(1, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.5f);
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(2, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.5f);
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(3, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.5f);
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(4, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.5f);
    AddInstance(MatrixCompose(DirectX::XMFLOAT4(5, 1, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, 0.5f);
    mRenderer.CreateOnCmdListOpen(meshFile, skeletonFile, animationFileList, materialName);
    mSkeletalMeshPointer = dynamic_cast<SimpleSkeletalMeshData*>(mRenderer.GetCurrentMesh());
}

void SkeletalMeshRenderBatch::UpdateSkinValue(
    size_t& globelSkinMatrixNum,
    size_t& globelIndirectArgNum,
    SimpleBufferStaging& SkeletonAnimationResultCpu,
    SimpleBufferStaging& skinIndirectArgBufferCpu,
    float delta_time
)
{
    std::vector<DirectX::XMFLOAT4X4> skinResultMatrixArray;
    std::vector<DirectX::XMMATRIX> curSkinBindPose;
    const SimpleSubMesh& curSubMesh = mSkeletalMeshPointer->mSubMesh[0];
    curSkinBindPose.resize(curSubMesh.mRefBonePose.size());
    std::unordered_map<int32_t, int32_t> meshIndexToskeleton;
    SimpleSkeletonData* skeletonData = mSkeletalMeshPointer->GetDefaultSkeleton();
    for (int32_t bindBoneIndex = 0; bindBoneIndex < curSubMesh.mRefBoneName.size(); ++bindBoneIndex)
    {
        auto curItor = skeletonData->mSearchIndex.find(curSubMesh.mRefBoneName[bindBoneIndex]);
        assert(curItor != skeletonData->mSearchIndex.end());
        meshIndexToskeleton[bindBoneIndex] = curItor->second;
    }
    for (int32_t bindBoneIndex = 0; bindBoneIndex < curSubMesh.mRefBonePose.size(); ++bindBoneIndex)
    {
        curSkinBindPose[bindBoneIndex] = DirectX::XMLoadFloat4x4(&curSubMesh.mRefBonePose[bindBoneIndex]);
    }
    skinResultMatrixArray.resize(mAllInstance.size() * curSubMesh.mRefBonePose.size());


    std::vector<SimpleSkeletonJoint> mLocalPoseTree = skeletonData->mBoneTree;
    std::vector<DirectX::XMMATRIX> modelPose;
    modelPose.resize(mLocalPoseTree.size());
    for (auto eachInstanceId = 0; eachInstanceId < mAllInstance.size(); ++eachInstanceId)
    {
        MeshRenderParameter& curParam = mAllInstance[eachInstanceId];
        curParam.curPlayTime += delta_time;
        SimpleAnimationData* curAnimationData = mSkeletalMeshPointer->GetAnimationByIndex(curParam.mAnimationindex);
        if (curParam.curPlayTime + 0.001f > curAnimationData->mAnimClipLength)
        {
            curParam.curPlayTime = 0;
        }
        float animDeltaTime = 1.0f / float(curAnimationData->mFramePerSec);
        uint32_t timeIndexPre = curParam.curPlayTime / animDeltaTime;
        uint32_t timeIndexNext = min(timeIndexPre + 1, (uint32_t)curAnimationData->mKeyTimes.size());
        float lerpDelta = (curParam.curPlayTime - curAnimationData->mKeyTimes[timeIndexPre]) / animDeltaTime;
        DirectX::XMVECTOR posLerp, rotLerp, scaleLerp;
        for (uint32_t i = 0; i < skeletonData->mBoneTree.size(); ++i)
        {
            const SimpleSkeletonJoint& boneData = skeletonData->mBoneTree[i];
            auto itor = curAnimationData->mBoneNameIdRef.find(boneData.mBoneName);
            if (itor != curAnimationData->mBoneNameIdRef.end())
            {
                const DirectX::XMVECTOR posValuePre = DirectX::XMLoadFloat3(&curAnimationData->mRawData[itor->second].mPosKeys[timeIndexPre]);
                const DirectX::XMVECTOR& posValueEnd = DirectX::XMLoadFloat3(&curAnimationData->mRawData[itor->second].mPosKeys[timeIndexNext]);
                posLerp = DirectX::XMVectorLerp(posValuePre, posValueEnd, lerpDelta);

                const DirectX::XMVECTOR& rotValuePre = DirectX::XMLoadFloat4(&curAnimationData->mRawData[itor->second].mRotKeys[timeIndexPre]);
                const DirectX::XMVECTOR& rotValueEnd = DirectX::XMLoadFloat4(&curAnimationData->mRawData[itor->second].mRotKeys[timeIndexNext]);
                rotLerp = DirectX::XMQuaternionSlerp(rotValuePre, rotValueEnd, lerpDelta);


                const DirectX::XMVECTOR& scaleValuePre = DirectX::XMLoadFloat3(&curAnimationData->mRawData[itor->second].mScalKeys[timeIndexPre]);
                const DirectX::XMVECTOR& scaleValueEnd = DirectX::XMLoadFloat3(&curAnimationData->mRawData[itor->second].mScalKeys[timeIndexNext]);
                scaleLerp = DirectX::XMVectorLerp(scaleValuePre, scaleValueEnd, lerpDelta);
            }
            else
            {
                posLerp = DirectX::XMLoadFloat3(&boneData.mBaseTranslation);
                rotLerp = DirectX::XMLoadFloat4(&boneData.mBaseRotation);
                scaleLerp = DirectX::XMLoadFloat3(&boneData.mBaseScal);
            }
            DirectX::XMStoreFloat3(&mLocalPoseTree[i].mBaseScal, scaleLerp);
            DirectX::XMStoreFloat4(&mLocalPoseTree[i].mBaseRotation, rotLerp);
            DirectX::XMStoreFloat3(&mLocalPoseTree[i].mBaseTranslation, posLerp);
        }
        LocalPoseToModelPose(mLocalPoseTree, modelPose);
        int32_t finalWriteOffset = eachInstanceId * curSubMesh.mRefBonePose.size();
        for (int32_t bindBoneIndex = 0; bindBoneIndex < curSubMesh.mRefBonePose.size(); ++bindBoneIndex)
        {
            DirectX::XMMATRIX& curBindAnimationPose = modelPose[meshIndexToskeleton[bindBoneIndex]];
            DirectX::XMMATRIX curSkinPose = curSkinBindPose[bindBoneIndex];
            DirectX::XMStoreFloat4x4(&skinResultMatrixArray[finalWriteOffset + bindBoneIndex], DirectX::XMMatrixTranspose(curSkinPose * curBindAnimationPose));
        }
    }
    //skin matrix
    memcpy(SkeletonAnimationResultCpu.mapPointer + globelSkinMatrixNum * sizeof(DirectX::XMFLOAT4X4), skinResultMatrixArray.data(), skinResultMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4));
    globelSkinMatrixNum += skinResultMatrixArray.size();
    //size_t copySize = SizeAligned2Pow(skinResultMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4), 255);
    //g_pd3dCommandList->CopyBufferRegion(SkeletonAnimationResultGpu.mBufferResource.Get(), 0, SkeletonAnimationResultCpu[currentFrameIndex].mBufferResource.Get(), 0, copySize);

    //ceshi arg buffer
    int32_t verDataCount = curSubMesh.mVertexData.size();
    int32_t threadCount = verDataCount / 64;
    if (verDataCount % 64 != 0)
    {
        threadCount += 1;
    }
    GpuResourceUtil::computePassIndirectCommand newCommand;
    newCommand.constInput = DirectX::XMUINT4(verDataCount, curSubMesh.mRefBoneName.size(), (uint32_t)mSkeletalMeshPointer->mVbOffset, (uint32_t)mSkeletalMeshPointer->mSkinNumOffset);
    newCommand.drawArguments.ThreadGroupCountX = threadCount;
    newCommand.drawArguments.ThreadGroupCountY = mAllInstance.size();
    newCommand.drawArguments.ThreadGroupCountZ = 1;
    memcpy(skinIndirectArgBufferCpu.mapPointer + globelIndirectArgNum * sizeof(GpuResourceUtil::computePassIndirectCommand), &newCommand, sizeof(GpuResourceUtil::computePassIndirectCommand));
    globelIndirectArgNum += 1;
    //size_t indirectArgCopySize = SizeAligned2Pow(allDrawCommand.size() * sizeof(GpuResourceUtil::computePassIndirectCommand), 255);
    //g_pd3dCommandList->CopyBufferRegion(skinIndirectArgBufferGpu.mBufferResource.Get(), 0, skinIndirectArgBufferCpu[currentFrameIndex].mBufferResource.Get(), 0, indirectArgCopySize);
}

void SkeletalMeshRenderBatch::Draw(const std::unordered_map<size_t, size_t>& allBindPoint, ID3D12PipelineState* curPipeline)
{
    mRenderer.Draw(allBindPoint, curPipeline, mInstaceDataOffset, mAllInstance.size());
}

size_t SkeletalMeshRenderBatch::ComputeSkinResultBufferCount()
{
    return mAllInstance.size()* mRenderer.GetCurrentMesh()->mSubMesh[0].mVertexData.size() * 3 * sizeof(DirectX::XMFLOAT4);
}

size_t SkeletalMeshRenderBatch::ComputeSkeletalMatrixBufferCount()
{
    return mAllInstance.size()* mSkeletalMeshPointer->GetDefaultSkeleton()->mBoneTree.size() * sizeof(DirectX::XMFLOAT4X4);
}

void SkeletalMeshRenderBatch::UpdateInstanceOffset(std::vector<DirectX::XMFLOAT4X4> &worldMatrixArray)
{
    mInstaceDataOffset = worldMatrixArray.size();
    for (auto eachInstance : mAllInstance)
    {
        worldMatrixArray.push_back(eachInstance.mTransformMatrix);
    }
}

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
    size_t allMeshVertexBufferSize = 0;
    size_t allSkeletalBufferSize = 0;
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
        allMeshVertexBufferSize += meshValueList[meshIndex].ComputeSkinResultBufferCount();
        allSkeletalBufferSize += meshValueList[meshIndex].ComputeSkeletalMatrixBufferCount();
        //meshValueList[i].Create();
    }
    //skeletal Input
    animationClipInput.Create(262144, sizeof(DirectX::XMFLOAT4));
    bindposeMatrixInput.Create(262144, sizeof(DirectX::XMFLOAT4X4));
    skeletonHierarchyInput.Create(262144, sizeof(UINT32));

    SkeletonAnimationResultGpu.Create(allSkeletalBufferSize, sizeof(DirectX::XMFLOAT4X4));
    worldSpaceSkeletonResultMap0.Create(allSkeletalBufferSize,sizeof(DirectX::XMFLOAT4X4));
    worldSpaceSkeletonResultMap1.Create(allSkeletalBufferSize, sizeof(DirectX::XMFLOAT4X4));

    skinVertexResult.Create(allMeshVertexBufferSize, sizeof(DirectX::XMFLOAT4));
    //skin pass arg buffer
    skinIndirectArgBufferGpu.Create(262144, sizeof(DirectX::XMFLOAT4X4));

    //world & view
    viewBufferGpu.Create(65536);
    viewBufferGpu.mBufferResource->SetName(L"ViewGpuBuffer");
    for (int32_t i = 0; i < 3; ++i)
    {
        worldMatrixBufferCpu[i].Create(65536);
        viewBufferCpu[i].Create(65536);
        SkeletonAnimationResultCpu[i].Create(262144);
        skinIndirectArgBufferCpu[i].Create(262144);
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
    GpuResourceUtil::GenerateGpuSkinPipeline(mAllPipelines.GpuSkinDispatchPipeline);
    
}


struct ViewParam 
{
    DirectX::XMFLOAT4X4 cViewMatrix;
    DirectX::XMFLOAT4X4 cProjectionMatrix;
    DirectX::XMFLOAT4 cCamPos;
};

void AnimationSimulateDemo::UpdateSkeletalMeshBatch(std::vector<DirectX::XMFLOAT4X4> &worldMatrixArray,uint32_t currentFrameIndex,float delta_time)
{
    UpdateResultBufferCpu(currentFrameIndex,delta_time);
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].UpdateInstanceOffset(worldMatrixArray);
    }
}

void AnimationSimulateDemo::UpdateResultBufferCpu(uint32_t currentFrameIndex,float delta_time)
{
    size_t globelSkinMatrixNum = 0;
    size_t globelIndirectArgNum = 0;
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].UpdateSkinValue(globelSkinMatrixNum, globelIndirectArgNum, SkeletonAnimationResultCpu[currentFrameIndex], skinIndirectArgBufferCpu[currentFrameIndex], delta_time);
    }
    //skin matrix
    size_t copySize = SizeAligned2Pow(globelSkinMatrixNum * sizeof(DirectX::XMFLOAT4X4), 255);
    g_pd3dCommandList->CopyBufferRegion(SkeletonAnimationResultGpu.mBufferResource.Get(), 0, SkeletonAnimationResultCpu[currentFrameIndex].mBufferResource.Get(), 0, copySize);

    //ceshi arg buffer
    size_t indirectArgCopySize = SizeAligned2Pow(globelIndirectArgNum * sizeof(GpuResourceUtil::computePassIndirectCommand), 255);
    g_pd3dCommandList->CopyBufferRegion(skinIndirectArgBufferGpu.mBufferResource.Get(), 0, skinIndirectArgBufferCpu[currentFrameIndex].mBufferResource.Get(), 0, indirectArgCopySize);
}

void AnimationSimulateDemo::DrawDemoData(float delta_time)
{
    uint32_t currentFrameIndex = g_pSwapChain->GetCurrentBackBufferIndex();
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
    size_t offset = currentFrameIndex * alignedSize;
    memcpy(viewBufferCpu[currentFrameIndex].mapPointer + offset, &newParam, sizeof(newParam));
    g_pd3dCommandList->CopyBufferRegion(viewBufferGpu.mBufferResource.Get(), 0, viewBufferCpu[currentFrameIndex].mBufferResource.Get(), offset, alignedSize);

    //world matrix
    std::vector<DirectX::XMFLOAT4X4> worldMatrixArray;
    worldMatrixArray.push_back(MatrixCompose(DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(6000, 1, 6000, 1)));

    UpdateSkeletalMeshBatch(worldMatrixArray, currentFrameIndex, delta_time);

    memcpy(worldMatrixBufferCpu[currentFrameIndex].mapPointer, worldMatrixArray.data(), worldMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4));
    size_t copySize = SizeAligned2Pow(worldMatrixArray.size() * sizeof(DirectX::XMFLOAT4X4), 255);
    g_pd3dCommandList->CopyBufferRegion(worldMatrixBufferGpu.mBufferResource.Get(), 0, worldMatrixBufferCpu[currentFrameIndex].mBufferResource.Get(), 0, copySize);

    //change view uniform buffer state
    D3D12_RESOURCE_BARRIER worldViewBufferBarrier[] = 
    { 
        CD3DX12_RESOURCE_BARRIER::Transition(worldMatrixBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(viewBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(SkeletonAnimationResultGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    g_pd3dCommandList->ResourceBarrier(3, worldViewBufferBarrier);
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
    //gpuskin
    D3D12_RESOURCE_BARRIER skeletonSkinBufferBarrier[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneStaticVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneSkinVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(skinVertexResult.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(skinIndirectArgBufferGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
    };
    g_pd3dCommandList->ResourceBarrier(4, skeletonSkinBufferBarrier);
    g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuSkinInputRootParam.Get());
    GpuResourceUtil::BindDescriptorToPipelineCS(0, SkeletonAnimationResultGpu.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(1, g_gpuSceneStaticVertexBuffer.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(2, g_gpuSceneSkinVertexBuffer.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(3, skinVertexResult.mDescriptorOffsetUAV);
    g_pd3dCommandList->SetPipelineState(mAllPipelines.GpuSkinDispatchPipeline.Get());
    g_pd3dCommandList->ExecuteIndirect(
        GpuResourceUtil::skinPassIndirectSignature.Get(),
        1,
        skinIndirectArgBufferGpu.mBufferResource.Get(),
        0,
        nullptr,
        0
        );
    D3D12_RESOURCE_BARRIER skeletonSkinBufferReturnBarrier[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneStaticVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE , D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(g_gpuSceneSkinVertexBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        CD3DX12_RESOURCE_BARRIER::Transition(skinVertexResult.mBufferResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    g_pd3dCommandList->ResourceBarrier(3, skeletonSkinBufferReturnBarrier);
    //show all mesh
    std::unordered_map<size_t, size_t> viewBindPoint;
    viewBindPoint.insert({ 5, samplerDescriptor });
    viewBindPoint.insert({ 0, viewBufferGpu.mDescriptorOffsetCBV });
    viewBindPoint.insert({ 1, worldMatrixBufferGpu.mDescriptorOffsetSRV});
    viewBindPoint.insert({ 2, skinVertexResult.mDescriptorOffsetSRV });
    g_pd3dCommandList->SetGraphicsRootSignature(GpuResourceUtil::globelDrawInputRootParam.Get());
    baseSkyBox.Draw(viewBindPoint);
    floorMeshRenderer.Draw(viewBindPoint, mAllPipelines.floorDrawPipeline.Get(),0,1);
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].Draw(viewBindPoint, mAllPipelines.skinDrawPipeline.Get());
    }
  
    //std::unordered_map<size_t, size_t> skyBindPoint = viewBindPoint;
    //skyBindPoint.insert({ 3,skySpecularDescriptorOffset });



}