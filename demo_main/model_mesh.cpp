#include"model_mesh.h"
SimpleBufferStaging g_gpuStageBuffer;
SimpleReadOnlyBuffer g_gpuSceneStaticVertexBuffer;
SimpleReadOnlyBuffer g_gpuSceneSkinVertexBuffer;
SimpleBuffer g_gpuSceneIndexBuffer;
size_t stageOffset = 0;
size_t staticVbOffset = 0;
size_t ibOffset = 0;
size_t skinVbOffset = 0;
void InitGpuSceneMeshBuffer()
{
    g_gpuStageBuffer.Create(16 * 1024 * 1024);
    g_gpuSceneStaticVertexBuffer.Create(16 * 1024 * 1024, 4 * sizeof(float));
    g_gpuSceneSkinVertexBuffer.Create(16 * 1024 * 1024,4 * sizeof(uint32_t));
    g_gpuSceneIndexBuffer.Create(16 * 1024 * 1024);
}

void SimpleStaticMesh::ReadVertexData(size_t idx, const byte*& ptr)
{
    SimpleSubMesh& subMeshData = mSubMesh[idx];
    memcpy(subMeshData.mVertexData.data(), ptr, subMeshData.mVertexData.size() * sizeof(BaseVertex));
    ptr += subMeshData.mVertexData.size() * sizeof(BaseVertex);
    memcpy(subMeshData.mIndexData.data(), ptr, subMeshData.mIndexData.size() * sizeof(uint32_t));
    ptr += subMeshData.mIndexData.size() * sizeof(uint32_t);
}

void SimpleStaticMesh::GenerateSubmesh(SimpleSubMesh& curSubmesh, size_t submeshVertexSize, size_t submeshIndexSize)
{
    curSubmesh.mVertexData.resize(submeshVertexSize);
    curSubmesh.mIndexData.resize(submeshIndexSize);
}

void SimpleStaticMesh::Create(const std::string& meshFile)
{
    mVbOffset = staticVbOffset / (sizeof(DirectX::XMFLOAT4) * 3);
    mIbOffset = ibOffset / sizeof(uint32_t);
    std::vector<char> buffer;
    LoadFileToVectorBinary(meshFile, buffer);
    const byte* data_header = (const byte*)buffer.data();
    size_t offset = 0;
    const byte* ptr = data_header;
    size_t submeshSize;
    memcpy(&submeshSize, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    for (size_t id = 0; id < submeshSize; ++id)
    {
        size_t submeshVertexSize;
        size_t submeshIndexSize;
        memcpy(&submeshVertexSize, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        memcpy(&submeshIndexSize, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        SimpleSubMesh sub_mesh;
        GenerateSubmesh(sub_mesh, submeshVertexSize, submeshIndexSize);
        mSubMesh.push_back(sub_mesh);
    }
    for (size_t idx = 0; idx < mSubMesh.size(); ++idx)
    {
        ReadVertexData(idx, ptr);
    }
    std::vector<DirectX::XMFLOAT4> vertexBufferValue;
    for (size_t vi = 0; vi < mSubMesh[0].mVertexData.size(); ++vi)
    {
        DirectX::XMFLOAT4 vec1, vec2, vec3;
        vec1.x = mSubMesh[0].mVertexData[vi].pos.x;
        vec1.y = mSubMesh[0].mVertexData[vi].pos.y;
        vec1.z = mSubMesh[0].mVertexData[vi].pos.z;
        vec1.w = mSubMesh[0].mVertexData[vi].normal.x;
        vec2.x = mSubMesh[0].mVertexData[vi].normal.y;
        vec2.y = mSubMesh[0].mVertexData[vi].normal.z;
        vec2.z = mSubMesh[0].mVertexData[vi].tangent.x;
        vec2.w = mSubMesh[0].mVertexData[vi].tangent.y;
        vec3.x = mSubMesh[0].mVertexData[vi].tangent.z;
        vec3.y = mSubMesh[0].mVertexData[vi].tangent.w;
        vec3.z = mSubMesh[0].mVertexData[vi].uv[0].x;
        vec3.w = mSubMesh[0].mVertexData[vi].uv[0].y;
        vertexBufferValue.push_back(vec1);
        vertexBufferValue.push_back(vec2);
        vertexBufferValue.push_back(vec3);
    }
    uint32_t bufferSize = mSubMesh[0].mVertexData.size() * sizeof(DirectX::XMFLOAT4) * 3;
    uint32_t indexSize = mSubMesh[0].mIndexData.size() * sizeof(uint32_t);
    memcpy(g_gpuStageBuffer.mapPointer + stageOffset, vertexBufferValue.data(), bufferSize);
    g_pd3dCommandList->CopyBufferRegion(g_gpuSceneStaticVertexBuffer.mBufferResource.Get(), staticVbOffset, g_gpuStageBuffer.mBufferResource.Get(), stageOffset, bufferSize);
    memcpy(g_gpuStageBuffer.mapPointer + stageOffset + bufferSize, mSubMesh[0].mIndexData.data(), indexSize);
    g_pd3dCommandList->CopyBufferRegion(g_gpuSceneIndexBuffer.mBufferResource.Get(), ibOffset, g_gpuStageBuffer.mBufferResource.Get(), stageOffset + bufferSize, indexSize);

    vbView.BufferLocation = g_gpuSceneStaticVertexBuffer.mBufferResource.Get()->GetGPUVirtualAddress() + staticVbOffset;
    vbView.SizeInBytes = bufferSize;
    vbView.StrideInBytes = sizeof(DirectX::XMFLOAT4) * 3;
    ibView.BufferLocation = g_gpuSceneIndexBuffer.mBufferResource.Get()->GetGPUVirtualAddress() + ibOffset;
    ibView.Format = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
    ibView.SizeInBytes = indexSize;

    stageOffset += bufferSize + indexSize;
    staticVbOffset += bufferSize;
    ibOffset += indexSize;
}

void SimpleSkeletalMeshData::Create(
    const std::string& meshFile,
    const std::string& skeletonFile,
    const std::vector<std::string>& animationFileList
) 
{
    SimpleStaticMesh::Create(meshFile);
    for (size_t id = 0; id < mSubMesh.size(); ++id)
    {
        mSubMesh[id].mSkinData.resize(mSubMesh[id].mVertexData.size());
    }
    mSkeleton.Create(skeletonFile);
    mAnimationList.resize(animationFileList.size());
    for (int32_t i = 0; i < animationFileList.size(); ++i)
    {
        mAnimationList[i].Create(animationFileList[i]);
    }
}

void SimpleSkeletalMeshData::GenerateSubmesh(SimpleSubMesh& curSubmesh, size_t submeshVertexSize, size_t submeshIndexSize)
{
    SimpleStaticMesh::GenerateSubmesh(curSubmesh, submeshVertexSize, submeshIndexSize);
    curSubmesh.mSkinData.resize(submeshVertexSize);
}

void SimpleSkeletalMeshData::ReadVertexData(size_t idx, const byte*& ptr)
{
    SimpleSubMesh& subMeshData = mSubMesh[idx];
    SimpleStaticMesh::ReadVertexData(idx, ptr);
    memcpy(subMeshData.mSkinData.data(), ptr, subMeshData.mSkinData.size() * sizeof(SkinVertex));
    ptr += subMeshData.mSkinData.size() * sizeof(SkinVertex);
    size_t refBoneNumber = 0;
    memcpy(&refBoneNumber, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    for (int32_t boneIndex = 0; boneIndex < refBoneNumber; ++boneIndex)
    {
        size_t nameLength = 0;
        memcpy(&nameLength, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        char* namePtr = new char[nameLength + 1];
        namePtr[nameLength] = '\0';
        memcpy(namePtr, ptr, nameLength * sizeof(char));
        subMeshData.mRefBoneName.push_back(namePtr);
        delete[] namePtr;
        ptr += nameLength * sizeof(char);
    }
    subMeshData.mRefBonePose.resize(refBoneNumber);
    memcpy(subMeshData.mRefBonePose.data(), ptr, refBoneNumber * sizeof(DirectX::XMFLOAT4X4));
    ptr += refBoneNumber * sizeof(DirectX::XMFLOAT4X4);

    std::vector<DirectX::XMUINT4> skinBufferValue;
    for (size_t vi = 0; vi < mSubMesh[0].mSkinData.size(); ++vi)
    {
        DirectX::XMUINT4 vec1, vec2;
        vec1.x = mSubMesh[0].mSkinData[vi].mRefBone[0];
        vec1.y = mSubMesh[0].mSkinData[vi].mRefBone[1];
        vec1.z = mSubMesh[0].mSkinData[vi].mRefBone[2];
        vec1.w = mSubMesh[0].mSkinData[vi].mRefBone[3];

        vec2.x = mSubMesh[0].mSkinData[vi].mWeight[0];
        vec2.y = mSubMesh[0].mSkinData[vi].mWeight[1];
        vec2.z = mSubMesh[0].mSkinData[vi].mWeight[2];
        vec2.w = mSubMesh[0].mSkinData[vi].mWeight[3];
        skinBufferValue.push_back(vec1);
        skinBufferValue.push_back(vec2);
    }
    uint32_t bufferSize = skinBufferValue.size() * sizeof(DirectX::XMUINT4);
    memcpy(g_gpuStageBuffer.mapPointer + stageOffset, skinBufferValue.data(), bufferSize);
    g_pd3dCommandList->CopyBufferRegion(g_gpuSceneSkinVertexBuffer.mBufferResource.Get(), skinVbOffset, g_gpuStageBuffer.mBufferResource.Get(), stageOffset, bufferSize);
    stageOffset += bufferSize;
    skinVbOffset += bufferSize;
}



