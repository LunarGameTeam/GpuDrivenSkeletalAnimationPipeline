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
    const int32_t meshIndex,
    const std::string& meshFile,
    const std::string& skeletonFile,
    const std::vector<std::string>& animationFileList,
    const std::string& materialName
)
{
    for (int32_t i = 0; i < 100; ++i)
    {
        for (int32_t j = 0; j < 100; ++j)
        {
            float rand_time = 0;
            rand_time /= 1000.0f;
            AddInstance(MatrixCompose(DirectX::XMFLOAT4(i, 1, 2 * j, 1), DirectX::XMFLOAT4(0, 0, 0, 1), DirectX::XMFLOAT4(1, 1, 1, 1)), 0, rand_time);
        }
    }
    mRenderer.CreateOnCmdListOpen(meshFile, skeletonFile, animationFileList, materialName);
    mSkeletalMeshPointer = dynamic_cast<SimpleSkeletalMeshData*>(mRenderer.GetCurrentMesh());
}

void SkeletalMeshRenderBatch::UpdateSkinValue(
    size_t& globelSkinVertNum,
    size_t& globelSkinMatrixNum,
    size_t& globelIndirectArgNum,
    SimpleBufferStaging& SkeletonAnimationResultCpu,
    SimpleBufferStaging& skinIndirectArgBufferCpu,
    float delta_time
)
{
    mVertexDataOffset = globelSkinVertNum;
    size_t curSkinMatrixOffset = globelSkinMatrixNum;
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
    globelSkinVertNum += verDataCount * mAllInstance.size();
    int32_t threadCount = verDataCount / 64;
    if (verDataCount % 64 != 0)
    {
        threadCount += 1;
    }
    GpuResourceUtil::computePassIndirectCommand newCommand;
    newCommand.constInput1 = DirectX::XMUINT4(verDataCount, curSubMesh.mRefBoneName.size(), (uint32_t)mSkeletalMeshPointer->mVbOffset, (uint32_t)mSkeletalMeshPointer->mSkinNumOffset);
    newCommand.constInput2 = DirectX::XMUINT4(mVertexDataOffset, curSkinMatrixOffset, 0, 0);
    
    
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
    
    mRenderer.Draw(allBindPoint, curPipeline, mVertexDataOffset,mInstaceDataOffset, mAllInstance.size());
}

size_t SkeletalMeshRenderBatch::ComputeSkinResultBufferCount()
{
    return mAllInstance.size()* mRenderer.GetCurrentMesh()->mSubMesh[0].mVertexData.size() * 3 * sizeof(DirectX::XMFLOAT4);
}

size_t SkeletalMeshRenderBatch::ComputeSkeletalMatrixBufferCount()
{
    return mAllInstance.size()* mSkeletalMeshPointer->GetDefaultSkeleton()->mBoneTree.size() * sizeof(DirectX::XMFLOAT4X4);
}

int32_t SkeletalMeshRenderBatch::ComputeAssetAlignedSkeletalBlockCount() const
{
    int32_t alignedJointNum = SizeAligned2Pow((int32_t)(mSkeletalMeshPointer->GetDefaultSkeleton()->mBoneTree.size()), JointNumAligned);
    int32_t commandCount = (int32_t)(alignedJointNum / JointNumAligned);
    return commandCount;
}

int32_t SkeletalMeshRenderBatch::ComputeAlignedSkeletalBlockCount() const
{
    int32_t commandCount = (int32_t)mAllInstance.size() * ComputeAssetAlignedSkeletalBlockCount();
    return commandCount;
}

void SkeletalMeshRenderBatch::CollectAnimationData(
    std::vector<DirectX::XMUINT4>& animationMessagePack,
    std::vector<DirectX::XMFLOAT4>& animationDataPack
)
{
    for (int32_t curAnimId = 0; curAnimId < mSkeletalMeshPointer->mAnimationList.size(); ++curAnimId)
    {
        int32_t animationGlobelIdOffset = animationMessagePack.size();
        int32_t animationGlobelDataOffset = animationDataPack.size() / 3;
        SimpleAnimationData& curAnimationValue = mSkeletalMeshPointer->mAnimationList[curAnimId];
        DirectX::XMUINT4 curAnimationMessage;
        curAnimationMessage.x = animationGlobelDataOffset;
        curAnimationMessage.y = curAnimationValue.mKeyTimes.size();
        curAnimationMessage.z = curAnimationValue.mFramePerSec;
        curAnimationMessage.w = (uint32_t)mSkeletalMeshPointer->mSkeleton.mBoneTree.size();
        mAnimationDataGlobelId.push_back(animationGlobelIdOffset);
        mAnimationDataGlobelOffset.push_back(animationGlobelDataOffset);
        animationMessagePack.push_back(curAnimationMessage);
        for (int32_t sampleTimeId = 0; sampleTimeId < curAnimationMessage.y; ++sampleTimeId)
        {
            for (int32_t jointId = 0; jointId < curAnimationValue.mRawData.size(); ++jointId)
            {
                DirectX::XMFLOAT4 positionData, rotationData, scaleData;
                positionData.x = curAnimationValue.mRawData[jointId].mPosKeys[sampleTimeId].x;
                positionData.y = curAnimationValue.mRawData[jointId].mPosKeys[sampleTimeId].y;
                positionData.z = curAnimationValue.mRawData[jointId].mPosKeys[sampleTimeId].z;
                positionData.w = 1;
                rotationData = curAnimationValue.mRawData[jointId].mRotKeys[sampleTimeId];
                scaleData.x = curAnimationValue.mRawData[jointId].mScalKeys[sampleTimeId].x;
                scaleData.y = curAnimationValue.mRawData[jointId].mScalKeys[sampleTimeId].y;
                scaleData.z = curAnimationValue.mRawData[jointId].mScalKeys[sampleTimeId].z;
                scaleData.w = 1;
                animationDataPack.push_back(positionData);
                animationDataPack.push_back(rotationData);
                animationDataPack.push_back(scaleData);
            }
        }
    }
}

void SkeletalMeshRenderBatch::CollectSkeletonParentData(std::vector<int32_t>& skeletonMessagePack)
{
    uint32_t globelOffset = skeletonMessagePack.size();
    auto& curBoneTree = mSkeletalMeshPointer->mSkeleton.mBoneTree;
    uint32_t boneCount = curBoneTree.size();
    uint32_t alignedJointNum = SizeAligned2Pow((uint32_t)(boneCount), JointNumAligned);
    uint32_t commandCount = alignedJointNum / JointNumAligned;
    for (uint32_t commandId = 0; commandId < commandCount; ++commandId)
    {
        for (uint32_t jointId = 0; jointId < JointNumAligned; ++jointId)
        {
            skeletonMessagePack.push_back(-1);
        }
    }
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        if (curBoneTree[curJointId].mParentIndex == int32_t(-1))
        {
            skeletonMessagePack[globelOffset + curJointId] = -1;
        }
        skeletonMessagePack[globelOffset + curJointId] = curBoneTree[curJointId].mParentIndex;
    }
}

void SkeletalMeshRenderBatch::CollectLeafNodeMessage(int32_t& leafLayerCount, SkeletonLeafParentIdLayer& leafParentId)
{
    uint32_t fullCount = 21;
    uint32_t globelOffset = leafParentId.mParentIdData.size();
    auto& curBoneTree = mSkeletalMeshPointer->mSkeleton.mBoneTree;
    uint32_t boneCount = curBoneTree.size();
    uint32_t alignedJointNum = SizeAligned2Pow((uint32_t)(boneCount), JointNumAligned);
    uint32_t commandCount = alignedJointNum / JointNumAligned;
    std::vector<std::vector<uint32_t>> parentList;
    parentList.resize(boneCount);
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        uint32_t curFilterId = curJointId;
        while (true)
        {
            SimpleSkeletonJoint& eachJoint = curBoneTree[curFilterId];
            if (eachJoint.mParentIndex == int32_t(-1))
            {
                break;
            }
            parentList[curJointId].push_back(eachJoint.mParentIndex);
            curFilterId = eachJoint.mParentIndex;
        }
    }
}

void SkeletalMeshRenderBatch::CollectSkeletonHierarchyData2(std::vector<int32_t>& skeletonMessagePack)
{
    uint32_t fullCount = 21;
    uint32_t globelOffset = skeletonMessagePack.size();
    auto& curBoneTree = mSkeletalMeshPointer->mSkeleton.mBoneTree;
    uint32_t boneCount = curBoneTree.size();
    uint32_t alignedJointNum = SizeAligned2Pow((uint32_t)(boneCount), JointNumAligned);
    uint32_t commandCount = alignedJointNum / JointNumAligned;
    for (uint32_t commandId = 0; commandId < commandCount; ++commandId)
    {
        for (uint32_t jointId = 0; jointId < JointNumAligned; ++jointId)
        {
            for (uint32_t parentId = 0; parentId < fullCount; ++parentId)
            {
                skeletonMessagePack.push_back(-1);
            }
        }
    }
    std::vector<std::vector<uint32_t>> parentList;
    parentList.resize(boneCount);
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        uint32_t curFilterId = curJointId;
        while (true)
        {
            SimpleSkeletonJoint& eachJoint = curBoneTree[curFilterId];
            if (eachJoint.mParentIndex == int32_t(-1))
            {
                break;
            }
            parentList[curJointId].push_back(eachJoint.mParentIndex);
            curFilterId = eachJoint.mParentIndex;
        }
    }
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        auto& curParentList = parentList[curJointId];
        int32_t curBeginIndex = globelOffset + curJointId * fullCount;
        for (int32_t pid = 0; pid < 7; ++pid)
        {
            if (curParentList.size() > pid)
            {
                skeletonMessagePack[curBeginIndex + pid] = curParentList[pid];
            }
            else
            {
                break;
            }
        }
        for (int32_t pid = 1; pid < 8; ++pid)
        {
            int32_t curFindParent = pid * 8 - 1;
            if (curParentList.size() > curFindParent)
            {
                skeletonMessagePack[curBeginIndex + 7 + pid - 1] = curParentList[curFindParent];
            }
        }
        for (int32_t pid = 1; pid < 8; ++pid)
        {
            int32_t curFindParent = pid * 64 - 1;
            if (curParentList.size() > curFindParent)
            {
                skeletonMessagePack[curBeginIndex + 14 + pid - 1] = curParentList[curFindParent];
            }
        }

    }
}

void SkeletalMeshRenderBatch::CollectSkeletonHierarchyData(std::vector<SkeletonParentIdLayer>& skeletonMessagePack)
{
    auto& curBoneTree = mSkeletalMeshPointer->mSkeleton.mBoneTree;
    uint32_t boneCount = curBoneTree.size();
    uint32_t alignedJointNum = SizeAligned2Pow((uint32_t)(boneCount), JointNumAligned);
    uint32_t commandCount = alignedJointNum / JointNumAligned;

    mSkeletonParentDataGlobelOffset.push_back(skeletonMessagePack[0].mBlockCount);
    skeletonMessagePack[0].mBlockCount += commandCount;
    
    std::vector<std::vector<uint32_t>> parentList;
    parentList.resize(boneCount);
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        uint32_t curFilterId = curJointId;
        while (true)
        {
            SimpleSkeletonJoint& eachJoint = curBoneTree[curFilterId];
            if (eachJoint.mParentIndex == int32_t(-1))
            {
                break;
            }
            parentList[curJointId].push_back(eachJoint.mParentIndex);
            curFilterId = eachJoint.mParentIndex;
        }
    }
    //prefix multi layer
    uint32_t parentIdOffset0 = mSkeletonParentDataGlobelOffset[0] * JointNumAligned * 6;
    for (uint32_t commandId = 0; commandId < commandCount; ++commandId)
    {
        for (uint32_t jointId = 0; jointId < JointNumAligned; ++jointId)
        {
            for (uint32_t parentId = 0; parentId < 6; ++parentId)
            {
                skeletonMessagePack[0].mParentIdData.push_back(-1);
            }
        }
    }
    for (uint32_t curJointId = 0; curJointId < curBoneTree.size(); ++curJointId)
    {
        uint32_t curJointWriteOffset = parentIdOffset0 + curJointId * 6;
        std::vector<uint32_t>& parentListJoint = parentList[curJointId];
        uint32_t curMinParentId = (curJointId / JointNumAligned) * JointNumAligned;
        for (uint32_t parentMulti = 0; parentMulti < 6; ++parentMulti)
        {
            uint32_t curFind = std::pow(2, parentMulti);
            if (parentListJoint.size() >= curFind && parentListJoint[curFind - 1] >= curMinParentId)
            {
                skeletonMessagePack[0].mParentIdData[curJointWriteOffset + parentMulti] = parentListJoint[curFind - 1];
            }
        }
    }
    mSkeletonParentDataGlobelOffset.push_back(skeletonMessagePack[1].mBlockCount);
    skeletonMessagePack[1].mBlockCount += commandCount;
    for (uint32_t jointId = 0; jointId < JointNumAligned; ++jointId)
    {
        skeletonMessagePack[1].mParentIdData.push_back(-1);
    }
    //Merge layer
    for (int32_t curMergeCommandDstId = 1; curMergeCommandDstId < commandCount; ++curMergeCommandDstId)
    {
        int32_t curMergeCommandSourceId = curMergeCommandDstId - 1;
        for (uint32_t jointId = 0; jointId < JointNumAligned; ++jointId)
        {
            uint32_t curJointId = curMergeCommandDstId * JointNumAligned + jointId;
            skeletonMessagePack[1].mParentIdData.push_back(-1);
            if (curJointId >= boneCount)
            {
                continue;
            }
            std::vector<uint32_t>& parentListJoint = parentList[curJointId];
            uint32_t maxIndex = (curMergeCommandSourceId + 1) * JointNumAligned;
            for (uint32_t parentIndex = 0; parentIndex < parentListJoint.size(); ++parentIndex)
            {
                if (parentListJoint[parentIndex] < maxIndex)
                {
                    skeletonMessagePack[1].mParentIdData.back() = parentListJoint[parentIndex];
                    break;
                }
            }
        }
    }
}

void SkeletalMeshRenderBatch::CollectLocalToWorldUniformMessage(int32_t index, std::vector<DirectX::XMUINT4>& skeletonMessagePack, int32_t &globelBlockOffset)
{
    int32_t curCommandCount = ComputeAssetAlignedSkeletalBlockCount();
    if (mSkeletonParentDataGlobelOffset.size() > index)
    {
        for (auto eachInstanceId = 0; eachInstanceId < mAllInstance.size(); ++eachInstanceId)
        {
            for (int32_t eachBlock = 0; eachBlock < curCommandCount; ++eachBlock)
            {
                DirectX::XMUINT4 commandUniform;
                commandUniform.x = eachBlock;
                commandUniform.y = mSkeletonParentDataGlobelOffset[index];
                commandUniform.z = globelBlockOffset + eachBlock;
                commandUniform.w = 0;
                skeletonMessagePack.push_back(commandUniform);
            }
            globelBlockOffset += curCommandCount;
        }
    }
}

void SkeletalMeshRenderBatch::UpdateSkinValueGpu(
    size_t& globelSkinVertNum,
    size_t& globelSkinMatrixNum,
    size_t& globelIndirectArgNum,
    SimpleBufferStaging& skinIndirectArgBufferCpu
)
{
    mVertexDataOffset = globelSkinVertNum;
    size_t curSkinMatrixOffset = globelSkinMatrixNum;
    const SimpleSubMesh& curSubMesh = mSkeletalMeshPointer->mSubMesh[0];
    int32_t alignedJointNum = SizeAligned2Pow((int32_t)(mSkeletalMeshPointer->GetDefaultSkeleton()->mBoneTree.size()), JointNumAligned);
    globelSkinMatrixNum += mAllInstance.size() * alignedJointNum;
    //ceshi arg buffer
    int32_t verDataCount = curSubMesh.mVertexData.size();
    globelSkinVertNum += verDataCount * mAllInstance.size();
    int32_t threadCount = verDataCount / 64;
    if (verDataCount % 64 != 0)
    {
        threadCount += 1;
    }
    GpuResourceUtil::computePassIndirectCommand newCommand;
    newCommand.constInput1 = DirectX::XMUINT4(verDataCount, alignedJointNum, (uint32_t)mSkeletalMeshPointer->mVbOffset, (uint32_t)mSkeletalMeshPointer->mSkinNumOffset);
    newCommand.constInput2 = DirectX::XMUINT4(mVertexDataOffset, curSkinMatrixOffset, 0, 0);
    newCommand.drawArguments.ThreadGroupCountX = threadCount;
    newCommand.drawArguments.ThreadGroupCountY = mAllInstance.size();
    newCommand.drawArguments.ThreadGroupCountZ = 1;
    memcpy(skinIndirectArgBufferCpu.mapPointer + globelIndirectArgNum * sizeof(GpuResourceUtil::computePassIndirectCommand), &newCommand, sizeof(GpuResourceUtil::computePassIndirectCommand));
    globelIndirectArgNum += 1;
}

void SkeletalMeshRenderBatch::CollectAnimationSimulationData(
    std::vector<DirectX::XMFLOAT4> &animationSimulationUniform,
    int32_t& outputOffset,
    float delta_time
) 
{
    int32_t alignedJointNum = SizeAligned2Pow((int32_t)(mSkeletalMeshPointer->GetDefaultSkeleton()->mBoneTree.size()), JointNumAligned);
    int32_t commandCount = alignedJointNum / JointNumAligned;
    for (int32_t eachInstanceId = 0; eachInstanceId < mAllInstance.size(); ++eachInstanceId)
    {
        MeshRenderParameter& curParam = mAllInstance[eachInstanceId];
        curParam.curPlayTime += delta_time;
        SimpleAnimationData* curAnimationData = mSkeletalMeshPointer->GetAnimationByIndex(curParam.mAnimationindex);
        if (curParam.curPlayTime + 0.001f > curAnimationData->mAnimClipLength)
        {
            curParam.curPlayTime = 0;
        }
        uint32_t globelAnimationId = mAnimationDataGlobelId[curParam.mAnimationindex];
        for (int32_t commandIndex = 0; commandIndex < commandCount; ++commandIndex)
        {
            DirectX::XMFLOAT4 curInstance;
            curInstance.x = curParam.curPlayTime;
            curInstance.y = globelAnimationId;
            curInstance.z = outputOffset;
            curInstance.w = commandIndex * JointNumAligned;
            animationSimulationUniform.push_back(curInstance);
        }
        outputOffset += alignedJointNum;
    }
}

void SkeletalMeshRenderBatch::UpdateInstanceOffset(std::vector<DirectX::XMFLOAT4X4> &worldMatrixArray)
{
    mInstaceDataOffset = worldMatrixArray.size();
    for (auto eachInstance : mAllInstance)
    {
        worldMatrixArray.push_back(eachInstance.mTransformMatrix);
    }
}

void SkeletalMeshRenderBatch::CollectMeshBindMessage(std::vector<DirectX::XMFLOAT4X4>& BindPosePack, std::vector<uint32_t>& bindReflectId)
{
    int32_t curBlockCount = ComputeAssetAlignedSkeletalBlockCount();
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
    for (int32_t bindBoneIndex = 0; bindBoneIndex < JointNumAligned * curBlockCount; ++bindBoneIndex)
    {
        if (bindBoneIndex < curSubMesh.mRefBonePose.size())
        {
            BindPosePack.push_back(curSubMesh.mRefBonePose[bindBoneIndex]);
            bindReflectId.push_back(meshIndexToskeleton[bindBoneIndex]);
        }
        else 
        {
            BindPosePack.push_back(DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
            bindReflectId.push_back(0);
        }
    }
}

void GpuAnimSimulation::CreateOnCmdListOpen(std::vector<SkeletalMeshRenderBatch>& meshValueList)
{
    std::vector<DirectX::XMUINT4> animationMessagePack;
    std::vector<DirectX::XMFLOAT4> animationDataPack;
    for (int32_t meshIndex = 0; meshIndex < meshValueList.size(); ++meshIndex)
    {
        meshValueList[meshIndex].CollectAnimationData(animationMessagePack, animationDataPack);
        allGpuSimulationBlock += meshValueList[meshIndex].ComputeAlignedSkeletalBlockCount();
    }
    int32_t animationGpuSimulationUniformSize = (allGpuSimulationBlock + 100) * sizeof(DirectX::XMFLOAT4);
    int32_t animationGpuSimulationResultSize = (allGpuSimulationBlock + 100) * JointNumAligned * sizeof(DirectX::XMFLOAT4X4);
    animationUniformGpu.Create(animationGpuSimulationUniformSize, sizeof(DirectX::XMFLOAT4));
    animationResultOutput.Create(animationGpuSimulationResultSize, sizeof(DirectX::XMFLOAT4X4));
    for (int32_t i = 0; i < 3; ++i)
    {
        animationUniformCpu[i].Create(animationGpuSimulationUniformSize);
    }
    animationClipMessage.Create((animationMessagePack.size() + 100) * sizeof(DirectX::XMUINT4), sizeof(DirectX::XMUINT4));
    animationClipInput.Create((animationDataPack.size() + 100) * sizeof(DirectX::XMFLOAT4), sizeof(DirectX::XMFLOAT4));
    animationStagingBuffer.Create((animationMessagePack.size() + animationDataPack.size() + 500) * sizeof(DirectX::XMFLOAT4));
    size_t animationMessageBufferSize = animationMessagePack.size() * sizeof(DirectX::XMUINT4);
    size_t animationDataBufferSize = animationDataPack.size() * sizeof(DirectX::XMFLOAT4);
    memcpy(animationStagingBuffer.mapPointer, animationMessagePack.data(), animationMessageBufferSize);
    memcpy(animationStagingBuffer.mapPointer + animationMessageBufferSize, animationDataPack.data(), animationDataBufferSize);
    g_pd3dCommandList->CopyBufferRegion(animationClipMessage.mBufferResource.Get(), 0, animationStagingBuffer.mBufferResource.Get(), 0, animationMessageBufferSize);
    g_pd3dCommandList->CopyBufferRegion(animationClipInput.mBufferResource.Get(), 0, animationStagingBuffer.mBufferResource.Get(), animationMessageBufferSize, animationDataBufferSize);

    D3D12_RESOURCE_BARRIER meshBufferBarrier[3];
    meshBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(animationClipMessage.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(animationClipInput.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[2] = CD3DX12_RESOURCE_BARRIER::Transition(animationResultOutput.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    g_pd3dCommandList->ResourceBarrier(3, meshBufferBarrier);
}

void GpuAnimSimulation::OnUpdate(
    std::vector<SkeletalMeshRenderBatch>& meshValueList,
    uint32_t currentFrameIndex,
    float delta_time
)
{
    std::vector<DirectX::XMFLOAT4> animationSimulationUniform;
    int32_t outputOffset = 0;
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].CollectAnimationSimulationData(animationSimulationUniform, outputOffset, delta_time);

    }
    memcpy(animationUniformCpu[currentFrameIndex].mapPointer, animationSimulationUniform.data(), animationSimulationUniform.size() * sizeof(DirectX::XMFLOAT4));
    size_t copySize = SizeAligned2Pow(animationSimulationUniform.size() * sizeof(DirectX::XMFLOAT4), 255);
    g_pd3dCommandList->CopyBufferRegion(animationUniformGpu.mBufferResource.Get(), 0, animationUniformCpu[currentFrameIndex].mBufferResource.Get(), 0, copySize);
    animationUniformCount = animationSimulationUniform.size();
}

void GpuAnimSimulation::OnDispatch(GpuResourceUtil::GlobelPipelineManager& allPepelines)
{
    //gpuskin
    D3D12_RESOURCE_BARRIER gpuAnimationSimulateBeginBarrier[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(animationResultOutput.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(animationUniformGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    g_pd3dCommandList->ResourceBarrier(2, gpuAnimationSimulateBeginBarrier);
    g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationSumulationInputRootParam.Get());
    GpuResourceUtil::BindDescriptorToPipelineCS(0, animationClipMessage.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(1, animationClipInput.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(2, animationUniformGpu.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(3, animationResultOutput.mDescriptorOffsetUAV);
    g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationSimulationPipeline.Get());
    g_pd3dCommandList->Dispatch(animationUniformCount,1 ,1);
}

void GpuSkeletonTreeLocalToWorld::CreateOnCmdListOpen(std::vector<SkeletalMeshRenderBatch>& meshValueList)
{
    size_t allSkeletalBlockCount = 0;
    skeletonMessagePack.resize(2);
    for (int32_t meshIndex = 0; meshIndex < meshValueList.size(); ++meshIndex)
    {
        meshValueList[meshIndex].CollectSkeletonParentData(skeletonParentPack);
        meshValueList[meshIndex].CollectSkeletonHierarchyData(skeletonMessagePack);
        meshValueList[meshIndex].CollectSkeletonHierarchyData2(skeletonParentPrefixPack2);
        allSkeletalBlockCount += meshValueList[meshIndex].ComputeAlignedSkeletalBlockCount();
    }
    jointPrefixMessage2.Create((skeletonParentPrefixPack2.size() + 1024) * sizeof(int32_t), sizeof(int32_t));
    jointSamuelAlgorithmMessage.Create((skeletonParentPack.size() + 1024) * sizeof(int32_t), sizeof(int32_t));
    jointPrefixMessage.Create((skeletonMessagePack[0].mParentIdData.size() + 1024) * sizeof(int32_t), sizeof(int32_t));
    jointMergeInput.Create((skeletonMessagePack[1].mParentIdData.size() + 1024) * sizeof(int32_t), sizeof(int32_t));
    trieStagingBuffer.Create(
        (
        skeletonMessagePack[0].mParentIdData.size()
        + skeletonMessagePack[1].mParentIdData.size()
        + skeletonParentPack.size()
        + skeletonParentPrefixPack2.size()
        + 1024
        ) * sizeof(int32_t)
    );

    worldSpaceSkeletonResultMap0.Create((allSkeletalBlockCount * 64 + 1024) * sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT4X4));
    worldSpaceSkeletonResultMap1.Create((allSkeletalBlockCount * 64 + 1024) * sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT4X4));
    size_t jointParentBufferSize = skeletonParentPack.size() * sizeof(int32_t);
    size_t jointPrefixBufferSize = skeletonMessagePack[0].mParentIdData.size() * sizeof(int32_t);
    size_t jointMergeBufferSize = skeletonMessagePack[1].mParentIdData.size() * sizeof(int32_t);
    size_t jointPrefix2BufferSize = skeletonParentPrefixPack2.size() * sizeof(int32_t);

    memcpy(trieStagingBuffer.mapPointer, skeletonMessagePack[0].mParentIdData.data(), jointPrefixBufferSize);
    memcpy(trieStagingBuffer.mapPointer + jointPrefixBufferSize, skeletonMessagePack[1].mParentIdData.data(), jointMergeBufferSize);
    memcpy(trieStagingBuffer.mapPointer + jointPrefixBufferSize + jointMergeBufferSize, skeletonParentPack.data(), jointParentBufferSize);
    memcpy(trieStagingBuffer.mapPointer + jointPrefixBufferSize + jointMergeBufferSize + jointParentBufferSize, skeletonParentPrefixPack2.data(), jointPrefix2BufferSize);

    g_pd3dCommandList->CopyBufferRegion(jointPrefixMessage.mBufferResource.Get(), 0, trieStagingBuffer.mBufferResource.Get(), 0, jointPrefixBufferSize);
    g_pd3dCommandList->CopyBufferRegion(jointMergeInput.mBufferResource.Get(), 0, trieStagingBuffer.mBufferResource.Get(), jointPrefixBufferSize, jointMergeBufferSize);
    g_pd3dCommandList->CopyBufferRegion(jointSamuelAlgorithmMessage.mBufferResource.Get(), 0, trieStagingBuffer.mBufferResource.Get(), jointPrefixBufferSize + jointMergeBufferSize, jointParentBufferSize);
    g_pd3dCommandList->CopyBufferRegion(jointPrefixMessage2.mBufferResource.Get(), 0, trieStagingBuffer.mBufferResource.Get(), jointPrefixBufferSize + jointMergeBufferSize + jointParentBufferSize, jointPrefix2BufferSize);

    D3D12_RESOURCE_BARRIER meshBufferBarrier[4];
    meshBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(jointPrefixMessage.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(jointMergeInput.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[2] = CD3DX12_RESOURCE_BARRIER::Transition(jointSamuelAlgorithmMessage.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[3] = CD3DX12_RESOURCE_BARRIER::Transition(jointPrefixMessage2.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    g_pd3dCommandList->ResourceBarrier(4, meshBufferBarrier);
    int a = 0;
}

void GpuSkeletonTreeLocalToWorld::OnUpdate(std::vector<SkeletalMeshRenderBatch>& meshValueList, uint32_t currentFrameIndex, float delta_time)
{
    dispathcCount.clear();
    std::vector<DirectX::XMUINT4> prefixOffsetPack;
    int32_t globelBlockOffset = 0;
    for (int32_t meshIndex = 0; meshIndex < meshValueList.size(); ++meshIndex)
    {
        meshValueList[meshIndex].CollectLocalToWorldUniformMessage(0,prefixOffsetPack, globelBlockOffset);
    }
    dispathcCount.push_back(prefixOffsetPack.size());
    globelBlockOffset = 0;
    std::vector<DirectX::XMUINT4> mergeOffsetPack;
    for (int32_t meshIndex = 0; meshIndex < meshValueList.size(); ++meshIndex)
    {
        meshValueList[meshIndex].CollectLocalToWorldUniformMessage(1, mergeOffsetPack, globelBlockOffset);
    }
    dispathcCount.push_back(mergeOffsetPack.size());
    if (prefixUniformCpu[0].mapPointer == nullptr)
    {
        for (int32_t i = 0; i < 3; ++i)
        {
            prefixUniformCpu[i].Create((prefixOffsetPack.size() + 100) * sizeof(DirectX::XMUINT4));
            mergeUniformCpu[i].Create((mergeOffsetPack.size() + 100) * sizeof(DirectX::XMUINT4));
        }
        prefixUniformGpu.Create((prefixOffsetPack.size() + 100) * sizeof(DirectX::XMUINT4), sizeof(DirectX::XMUINT4));
        mergeUniformGpu.Create((mergeOffsetPack.size() + 100) * sizeof(DirectX::XMUINT4), sizeof(DirectX::XMUINT4));
    }
    memcpy(prefixUniformCpu[currentFrameIndex].mapPointer, prefixOffsetPack.data(), prefixOffsetPack.size() * sizeof(DirectX::XMUINT4));
    memcpy(mergeUniformCpu[currentFrameIndex].mapPointer, mergeOffsetPack.data(), mergeOffsetPack.size() * sizeof(DirectX::XMUINT4));
    size_t prefixCopySize = SizeAligned2Pow(prefixOffsetPack.size() * sizeof(DirectX::XMFLOAT4), 255);
    size_t mergeCopySize = SizeAligned2Pow(mergeOffsetPack.size() * sizeof(DirectX::XMFLOAT4), 255);
    g_pd3dCommandList->CopyBufferRegion(prefixUniformGpu.mBufferResource.Get(), 0, prefixUniformCpu[currentFrameIndex].mBufferResource.Get(), 0, prefixCopySize);
    g_pd3dCommandList->CopyBufferRegion(mergeUniformGpu.mBufferResource.Get(), 0, mergeUniformCpu[currentFrameIndex].mBufferResource.Get(), 0, mergeCopySize);
    
    D3D12_RESOURCE_BARRIER meshBufferBarrier[2];
    meshBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(prefixUniformGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(mergeUniformGpu.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    g_pd3dCommandList->ResourceBarrier(2, meshBufferBarrier);
}

void GpuSkeletonTreeLocalToWorld::OnDispatch(
    SimpleReadWriteBuffer animationResultOutput,
    GpuResourceUtil::GlobelPipelineManager& allPepelines,
    GpuSimulationType curGpuType
)
{
    if (curGpuType == GpuSimulationType::SimulationOur)
    {
        //prefix Pass
        D3D12_RESOURCE_BARRIER gpuAnimationLocalToWorldPrefixBarrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(animationResultOutput.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap0.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        g_pd3dCommandList->ResourceBarrier(2, gpuAnimationLocalToWorldPrefixBarrier);
        g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get());
        GpuResourceUtil::BindDescriptorToPipelineCS(0, animationResultOutput.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(1, jointPrefixMessage.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(2, prefixUniformGpu.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(3, worldSpaceSkeletonResultMap0.mDescriptorOffsetUAV);
        g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationLocalToWorldPrefixPipeline.Get());
        g_pd3dCommandList->Dispatch(dispathcCount[0], 1, 1);
        //merge Pass
        D3D12_RESOURCE_BARRIER gpuAnimationLocalToWorldMergeBarrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap0.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap1.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        g_pd3dCommandList->ResourceBarrier(2, gpuAnimationLocalToWorldMergeBarrier);
        g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationLocalToWorldMergeRootParam.Get());
        GpuResourceUtil::BindDescriptorToPipelineCS(0, worldSpaceSkeletonResultMap0.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(1, jointMergeInput.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(2, mergeUniformGpu.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(3, worldSpaceSkeletonResultMap1.mDescriptorOffsetUAV);
        g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationLocalToWorldMergePipeline.Get());
        g_pd3dCommandList->Dispatch(dispathcCount[1], 1, 1);
    }
    else if (curGpuType == GpuSimulationType::SimulationSamuel)
    {
        D3D12_RESOURCE_BARRIER gpuAnimationLocalToWorldPrefixBarrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(animationResultOutput.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap1.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        g_pd3dCommandList->ResourceBarrier(2, gpuAnimationLocalToWorldPrefixBarrier);
        g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get());
        GpuResourceUtil::BindDescriptorToPipelineCS(0, animationResultOutput.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(1, jointSamuelAlgorithmMessage.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(2, prefixUniformGpu.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(3, worldSpaceSkeletonResultMap1.mDescriptorOffsetUAV);
        g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationLocalToWorldSamuelPipeline.Get());
        g_pd3dCommandList->Dispatch(dispathcCount[0], 1, 1);
    }
    else if (curGpuType == GpuSimulationType::SimulationOur2)
    {
        //prefix Pass
        D3D12_RESOURCE_BARRIER gpuAnimationLocalToWorldPrefixBarrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(animationResultOutput.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap0.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        g_pd3dCommandList->ResourceBarrier(2, gpuAnimationLocalToWorldPrefixBarrier);
        g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get());
        GpuResourceUtil::BindDescriptorToPipelineCS(0, animationResultOutput.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(1, jointPrefixMessage2.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(2, prefixUniformGpu.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(3, worldSpaceSkeletonResultMap0.mDescriptorOffsetUAV);
        g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationLocalToWorldPrefix2Pipeline.Get());
        g_pd3dCommandList->Dispatch(dispathcCount[0], 1, 1);
        //merge Pass
        D3D12_RESOURCE_BARRIER gpuAnimationLocalToWorldMergeBarrier[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap0.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(worldSpaceSkeletonResultMap1.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        };
        g_pd3dCommandList->ResourceBarrier(2, gpuAnimationLocalToWorldMergeBarrier);
        g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationLocalToWorldMergeRootParam.Get());
        GpuResourceUtil::BindDescriptorToPipelineCS(0, worldSpaceSkeletonResultMap0.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(1, jointMergeInput.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(2, mergeUniformGpu.mDescriptorOffsetSRV);
        GpuResourceUtil::BindDescriptorToPipelineCS(3, worldSpaceSkeletonResultMap1.mDescriptorOffsetUAV);
        g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationLocalToWorldMergePipeline.Get());
        g_pd3dCommandList->Dispatch(dispathcCount[1], 1, 1);
    }
}

void GpuAnimPoseGen::CreateOnCmdListOpen(std::vector<SkeletalMeshRenderBatch>& meshValueList)
{
    allSkeletalBlockCount = 0;
    std::vector<DirectX::XMFLOAT4X4> BindPosePack;
    std::vector<uint32_t> bindReflectId;
    for (int32_t meshIndex = 0; meshIndex < meshValueList.size(); ++meshIndex)
    {
        meshValueList[meshIndex].CollectMeshBindMessage(BindPosePack, bindReflectId);
        allSkeletalBlockCount += meshValueList[meshIndex].ComputeAlignedSkeletalBlockCount();
    }
    MeshBindMatrix.Create((BindPosePack.size() + 1024) * sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT4X4));
    BindBoneToSkeletonBoneId.Create((bindReflectId.size() + 1024) * sizeof(uint32_t), sizeof(uint32_t));
    MeshBindMessageStagingBuffer.Create((BindPosePack.size() + bindReflectId.size() + 1024) * sizeof(DirectX::XMFLOAT4X4));

    size_t MeshBindMatrixSize = BindPosePack.size() * sizeof(DirectX::XMFLOAT4X4);
    size_t BindBoneToSkeletonBoneIdSize = bindReflectId.size() * sizeof(uint32_t);
    memcpy(MeshBindMessageStagingBuffer.mapPointer, BindPosePack.data(), MeshBindMatrixSize);
    memcpy(MeshBindMessageStagingBuffer.mapPointer + MeshBindMatrixSize, bindReflectId.data(), BindBoneToSkeletonBoneIdSize);
    g_pd3dCommandList->CopyBufferRegion(MeshBindMatrix.mBufferResource.Get(), 0, MeshBindMessageStagingBuffer.mBufferResource.Get(), 0, MeshBindMatrixSize);
    g_pd3dCommandList->CopyBufferRegion(BindBoneToSkeletonBoneId.mBufferResource.Get(), 0, MeshBindMessageStagingBuffer.mBufferResource.Get(), MeshBindMatrixSize, BindBoneToSkeletonBoneIdSize);

    D3D12_RESOURCE_BARRIER meshBufferBarrier[2];
    meshBufferBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(MeshBindMatrix.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    meshBufferBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(BindBoneToSkeletonBoneId.mBufferResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    g_pd3dCommandList->ResourceBarrier(2, meshBufferBarrier);
}

void GpuAnimPoseGen::OnDispatch(
    SimpleReadOnlyBuffer& uniformBuffer,
    SimpleReadWriteBuffer& animationResultBuffer,
    SimpleReadWriteBuffer& SkinPoseBuffer,
    GpuResourceUtil::GlobelPipelineManager& allPepelines
)
{
    //prefix Pass
    D3D12_RESOURCE_BARRIER gpuAnimationPoseGenBarrier[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(animationResultBuffer.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(SkinPoseBuffer.mBufferResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    };
    g_pd3dCommandList->ResourceBarrier(2, gpuAnimationPoseGenBarrier);
    g_pd3dCommandList->SetComputeRootSignature(GpuResourceUtil::globelGpuAnimationPoseGenRootParam.Get());
    GpuResourceUtil::BindDescriptorToPipelineCS(0, MeshBindMatrix.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(1, BindBoneToSkeletonBoneId.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(2, animationResultBuffer.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(3, uniformBuffer.mDescriptorOffsetSRV);
    GpuResourceUtil::BindDescriptorToPipelineCS(4, SkinPoseBuffer.mDescriptorOffsetUAV);
    g_pd3dCommandList->SetPipelineState(allPepelines.GpuAnimationPoseGenPipeline.Get());
    g_pd3dCommandList->Dispatch(allSkeletalBlockCount, 1, 1);
    D3D12_RESOURCE_BARRIER gpuAnimationPoseGenBarrierReturn[] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(SkinPoseBuffer.mBufferResource.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
    };
    g_pd3dCommandList->ResourceBarrier(1, gpuAnimationPoseGenBarrierReturn);
}
#define BufferSize16M 16777216
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
    //size_t allSkeletalBufferSize = 0;
    size_t allAlignedSkeletalBufferSize = 0;
    for (int32_t meshIndex = 0; meshIndex < allMeshCount; ++meshIndex)
    {
        json curMesh = allMeshDataList[meshIndex];
        std::string curMeshName = curMesh["MeshFileName"];
        std::string curSkeletonName = curMesh["SkeletonFileName"];
        json curAnimationList = curMesh["AnimationFileList"];
        std::vector<std::string> animationNameList;
        int allAnimationCount = curAnimationList.size();
        for (int32_t animIndex = 0; animIndex < allAnimationCount; ++animIndex)
        {
            std::string curAnimationName = curAnimationList[animIndex];
            animationNameList.push_back(curAnimationName);
        }
        meshValueList[meshIndex].CreateOnCmdListOpen(meshIndex,curMeshName, curSkeletonName, animationNameList, "demo_asset_data/skeletal_mesh_demo/lion3/lion3.material");
        allMeshVertexBufferSize += meshValueList[meshIndex].ComputeSkinResultBufferCount();
        //allSkeletalBufferSize += meshValueList[meshIndex].ComputeSkeletalMatrixBufferCount();
        allAlignedSkeletalBufferSize += meshValueList[meshIndex].ComputeAlignedSkeletalBlockCount() * 64 * sizeof(DirectX::XMFLOAT4X4);
    }

    gpuAnimationSimulationPass.CreateOnCmdListOpen(meshValueList);
    gpuLocalToWorldPass.CreateOnCmdListOpen(meshValueList);
    gpuPoseGenPass.CreateOnCmdListOpen(meshValueList);

    SkeletonAnimationResultGpu.Create(allAlignedSkeletalBufferSize, sizeof(DirectX::XMFLOAT4X4));

    skinVertexResult.Create(allMeshVertexBufferSize, sizeof(DirectX::XMFLOAT4));
    //skin pass arg buffer
    skinIndirectArgBufferGpu.Create(BufferSize16M, sizeof(DirectX::XMFLOAT4X4));

    //world & view
    viewBufferGpu.Create(65535);
    viewBufferGpu.mBufferResource->SetName(L"ViewGpuBuffer");
    for (int32_t i = 0; i < 3; ++i)
    {
        worldMatrixBufferCpu[i].Create(BufferSize16M);
        viewBufferCpu[i].Create(65535);
        SkeletonAnimationResultCpu[i].Create(allAlignedSkeletalBufferSize);
        skinIndirectArgBufferCpu[i].Create(BufferSize16M);
    }
    worldMatrixBufferGpu.Create(BufferSize16M,sizeof(DirectX::XMFLOAT4X4));
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
    curType = SimulationType::SimulationGpu;
    curGpuType = GpuSimulationType::SimulationOur;
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
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuSkinInputRootParam.Get(),mAllPipelines.GpuSkinDispatchPipeline, L"demo_asset_data/shader/animation_simulation/skin_simulation.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationSumulationInputRootParam.Get(),mAllPipelines.GpuAnimationSimulationPipeline,L"demo_asset_data/shader/animation_simulation/animation_simulation.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get(), mAllPipelines.GpuAnimationLocalToWorldPrefixPipeline, L"demo_asset_data/shader/animation_simulation/local_to_world_prefix.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get(), mAllPipelines.GpuAnimationLocalToWorldPrefix2Pipeline, L"demo_asset_data/shader/animation_simulation/local_to_world_prefix2.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationLocalToWorldPrefixRootParam.Get(), mAllPipelines.GpuAnimationLocalToWorldSamuelPipeline, L"demo_asset_data/shader/animation_simulation/local_to_world_samuel.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationLocalToWorldMergeRootParam.Get(), mAllPipelines.GpuAnimationLocalToWorldMergePipeline, L"demo_asset_data/shader/animation_simulation/local_to_world_merge.hlsl");
    GpuResourceUtil::GenerateComputeShaderPipeline(GpuResourceUtil::globelGpuAnimationPoseGenRootParam.Get(), mAllPipelines.GpuAnimationPoseGenPipeline, L"demo_asset_data/shader/animation_simulation/generate_pose.hlsl");
    
}

struct ViewParam 
{
    DirectX::XMFLOAT4X4 cViewMatrix;
    DirectX::XMFLOAT4X4 cProjectionMatrix;
    DirectX::XMFLOAT4 cCamPos;
};

void AnimationSimulateDemo::UpdateSkeletalMeshBatch(std::vector<DirectX::XMFLOAT4X4> &worldMatrixArray, uint32_t& indirectSimulationCount, uint32_t currentFrameIndex,float delta_time)
{
    if (curType == SimulationType::SimulationGpu)
    {
        size_t globelSkinVertNum = 0;
        size_t globelSkinMatrixNum = 0;
        size_t globelIndirectArgNum = 0;
        for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
        {
            meshValueList[skelMeshIndex].UpdateSkinValueGpu(globelSkinVertNum, globelSkinMatrixNum, globelIndirectArgNum, skinIndirectArgBufferCpu[currentFrameIndex]);
        } 
        size_t indirectArgCopySize = SizeAligned2Pow(globelIndirectArgNum * sizeof(GpuResourceUtil::computePassIndirectCommand), 255);
        g_pd3dCommandList->CopyBufferRegion(skinIndirectArgBufferGpu.mBufferResource.Get(), 0, skinIndirectArgBufferCpu[currentFrameIndex].mBufferResource.Get(), 0, indirectArgCopySize);
        indirectSimulationCount = globelIndirectArgNum;
        gpuAnimationSimulationPass.OnUpdate(meshValueList, currentFrameIndex, delta_time);
        gpuLocalToWorldPass.OnUpdate(meshValueList, currentFrameIndex, delta_time);
      
    }
    else
    {
        UpdateResultBufferCpu(currentFrameIndex, delta_time, indirectSimulationCount);
    }
    //UpdateResultBufferGpu(currentFrameIndex, delta_time, indirectSimulationCount);
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        size_t globelSkinVertNum = 0;
        meshValueList[skelMeshIndex].UpdateInstanceOffset(worldMatrixArray);
    }
}

void AnimationSimulateDemo::UpdateResultBufferCpu(uint32_t currentFrameIndex,float delta_time, uint32_t& indirectSimulationCount)
{
    size_t globelSkinVertNum = 0;
    size_t globelSkinMatrixNum = 0;
    size_t globelIndirectArgNum = 0;
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].UpdateSkinValue(globelSkinVertNum,globelSkinMatrixNum, globelIndirectArgNum, SkeletonAnimationResultCpu[currentFrameIndex], skinIndirectArgBufferCpu[currentFrameIndex], delta_time);
    }
    //skin matrix
    size_t copySize = SizeAligned2Pow(globelSkinMatrixNum * sizeof(DirectX::XMFLOAT4X4), 255);
    g_pd3dCommandList->CopyBufferRegion(SkeletonAnimationResultGpu.mBufferResource.Get(), 0, SkeletonAnimationResultCpu[currentFrameIndex].mBufferResource.Get(), 0, copySize);

    //test arg buffer
    size_t indirectArgCopySize = SizeAligned2Pow(globelIndirectArgNum * sizeof(GpuResourceUtil::computePassIndirectCommand), 255);
    g_pd3dCommandList->CopyBufferRegion(skinIndirectArgBufferGpu.mBufferResource.Get(), 0, skinIndirectArgBufferCpu[currentFrameIndex].mBufferResource.Get(), 0, indirectArgCopySize);
    indirectSimulationCount = globelIndirectArgNum;
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

    uint32_t indirectSimulationCount = 0;
    UpdateSkeletalMeshBatch(worldMatrixArray, indirectSimulationCount,currentFrameIndex, delta_time);

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
    //gpu animation
    if (curType == SimulationType::SimulationGpu)
    {
        gpuAnimationSimulationPass.OnDispatch(mAllPipelines);
        gpuLocalToWorldPass.OnDispatch(gpuAnimationSimulationPass.animationResultOutput,mAllPipelines,curGpuType);
        gpuPoseGenPass.OnDispatch(gpuLocalToWorldPass.prefixUniformGpu, gpuLocalToWorldPass.worldSpaceSkeletonResultMap1, SkeletonAnimationResultGpu, mAllPipelines);
    }
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
        indirectSimulationCount,
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
    floorMeshRenderer.Draw(viewBindPoint, mAllPipelines.floorDrawPipeline.Get(),0,0,1);
    for (int32_t skelMeshIndex = 0; skelMeshIndex < meshValueList.size(); ++skelMeshIndex)
    {
        meshValueList[skelMeshIndex].Draw(viewBindPoint, mAllPipelines.skinDrawPipeline.Get());
    }
  
    //std::unordered_map<size_t, size_t> skyBindPoint = viewBindPoint;
    //skyBindPoint.insert({ 3,skySpecularDescriptorOffset });



}